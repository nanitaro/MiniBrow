/*
 * js.c - MiniBrow の JavaScript (Moddable XS エンジン)
 *
 *  - 1 ページに 1 つの実行環境 (XS のマシン) を作り、次のページで捨てる
 *  - window / document / location などは下の前処理部分 (JavaScript) で組み立てる。
 *    ページの情報は __h(op, ...) を通して minibrow.c の JsHostOp に問い合わせる
 *  - 1 回の実行 (スクリプト 1 つ、イベント 1 つ、タイマー 1 回) に時間の上限を設け、
 *    メモリは w95_xs.c で上限を設ける。超えたらそのページのスクリプトを止める
 *  - XS の文字列は UTF-8 (CESU-8)。ホストとはシステムのコードページ (ACP) で受け渡す。
 *    Win95 の API は UTF-8 を扱えないので変換は自前で行う
 */
#include "xsAll.h"
#include "xsScript.h"
#include "xs.h"
#include "js.h"

static txMachine *g_vm;                 /* このページの実行環境 (NULL = まだ無い) */
static long       g_memLimit = 4L * 1024 * 1024;
static long       g_timeLimit = 10000;
static unsigned long g_syncGen;         /* __sync を実行したときの g_domGen */
static int        g_syncOk;             /* g_syncGen が有効 (マシンを作り直すか文書を捨てたら 0) */
static DWORD      g_deadline;           /* 今の実行の打ち切り時刻 (GetTickCount) */
static int        g_busy;               /* 実行中 */
static int        g_dead;               /* このページのスクリプトを止めた */
static int        g_errors;             /* このページのエラーの数 */
static char       g_msg[512];

/* ---------------------------------------------------------------- 前処理部分 (window, document など) */

#include "prelude.h"

/* ---------------------------------------------------------------- 文字コード */

/* ACP の文字列を XS 用の UTF-8 (CESU-8) にする。malloc した文字列 */
static char *AcpToUtf8(const char *s, long len)
{
	int    n = len > 0 ? MultiByteToWideChar(CP_ACP, 0, s, (int)len, NULL, 0) : 0;
	WCHAR *w = malloc((n + 1) * sizeof(WCHAR));
	char  *o, *q;
	int    i;

	if (w == NULL)
		return NULL;
	if (n > 0)
		MultiByteToWideChar(CP_ACP, 0, s, (int)len, w, n);
	o = q = malloc((size_t)n * 3 + 1);
	if (o == NULL) {
		free(w);
		return NULL;
	}
	for (i = 0; i < n; i++) {
		unsigned c = w[i];
		if (c == 0x1F)
			c = 0xA0;		/* ホストの &nbsp; (0x1F) */
		if (c < 0x80) {
			*q++ = (char)c;
		} else if (c < 0x800) {
			*q++ = (char)(0xC0 | (c >> 6));
			*q++ = (char)(0x80 | (c & 0x3F));
		} else {
			/* サロゲートも 1 つずつ 3 バイトにする (CESU-8) */
			*q++ = (char)(0xE0 | (c >> 12));
			*q++ = (char)(0x80 | ((c >> 6) & 0x3F));
			*q++ = (char)(0x80 | (c & 0x3F));
		}
	}
	*q = '\0';
	free(w);
	return o;
}

/* XS の文字列 (UTF-8 / CESU-8) を ACP にする。malloc した文字列 */
static char *Utf8ToAcp(const char *s)
{
	const unsigned char *p = (const unsigned char *)s;
	size_t  len;
	WCHAR  *w;
	int     n = 0;
	char   *r;

	while (*p && *p < 0x80)
		p++;
	if (*p == '\0')
		return strdup(s);		/* ASCII だけ (番号など) は変換しない */
	p   = (const unsigned char *)s;
	len = strlen(s);
	w   = malloc((len + 1) * sizeof(WCHAR));
	if (w == NULL)
		return NULL;
	while (*p) {
		unsigned long c = *p++;
		if (c >= 0xC0 && (*p & 0xC0) == 0x80) {
			int more = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : 1;
			c &= more == 3 ? 0x07 : more == 2 ? 0x0F : 0x1F;
			while (more-- > 0 && (*p & 0xC0) == 0x80)
				c = (c << 6) | (*p++ & 0x3F);
		} else if (c >= 0x80) {
			c = 0xFFFD;
		}
		if (c == 0xA0)
			c = 0x1F;		/* &nbsp; はホストでは 0x1F */
		if (c >= 0x10000) {
			c -= 0x10000;
			w[n++] = (WCHAR)(0xD800 | (c >> 10));
			w[n++] = (WCHAR)(0xDC00 | (c & 0x3FF));
		} else {
			w[n++] = (WCHAR)c;
		}
	}
	{
		/* 片割れのサロゲートは捨てる (ホストの文字コードには表せない) */
		int i, k = 0;
		for (i = 0; i < n; i++) {
			if (w[i] >= 0xD800 && w[i] <= 0xDBFF && i + 1 < n && w[i + 1] >= 0xDC00 && w[i + 1] <= 0xDFFF) {
				w[k++] = w[i++];
				w[k++] = w[i];
			} else if (w[i] < 0xD800 || w[i] > 0xDFFF) {
				w[k++] = w[i];
			}
		}
		n = k;
	}
	r = JsHostWideToAcp((const unsigned short *)w, n);
	free(w);
	return r;
}

static void Put3(char **q, unsigned c)
{
	*(*q)++ = (char)(0xE0 | (c >> 12));
	*(*q)++ = (char)(0x80 | ((c >> 6) & 0x3F));
	*(*q)++ = (char)(0x80 | (c & 0x3F));
}

/* UTF-8 を XS 用の CESU-8 にする (4 バイトの文字はサロゲート 2 つに、壊れたバイトは U+FFFD に) */
static char *CesuFromUtf8(const char *s)
{
	const unsigned char *p = (const unsigned char *)s;
	char                *o = malloc(strlen(s) * 3 + 1), *q = o;

	if (o == NULL)
		return NULL;
	while (*p) {
		unsigned long c = *p;
		int           k, i;
		if (c < 0x80) {
			*q++ = (char)c;
			p++;
			continue;
		}
		k = (c & 0xE0) == 0xC0 ? 1 : (c & 0xF0) == 0xE0 ? 2 : (c & 0xF8) == 0xF0 ? 3 : 0;
		for (i = 1; i <= k && (p[i] & 0xC0) == 0x80; i++)
			;
		if (k == 0 || i <= k) {
			Put3(&q, 0xFFFD);
			p++;
			continue;
		}
		if (k < 3) {
			memcpy(q, p, k + 1);
			q += k + 1;
			p += k + 1;
			continue;
		}
		c = ((c & 7) << 18) | ((unsigned long)(p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
		p += 4;
		if (c < 0x10000 || c > 0x10FFFF) {
			Put3(&q, 0xFFFD);
			continue;
		}
		c -= 0x10000;
		Put3(&q, (unsigned)(0xD800 | (c >> 10)));
		Put3(&q, (unsigned)(0xDC00 | (c & 0x3FF)));
	}
	*q = '\0';
	return o;
}

/* ホストからの文字列を XS 用にする。先頭が \3 なら残りは UTF-8 (XHR の本文など)、ほかは ACP */
static char *HostToUtf8(const char *s)
{
	const unsigned char *p;

	if (s[0] == '\x03')
		return CesuFromUtf8(s + 1);
	for (p = (const unsigned char *)s; *p && *p < 0x80 && *p != 0x1F; p++)
		;
	if (*p == '\0')
		return strdup(s);		/* ASCII だけ (&nbsp; の 0x1F を除く) は変換しない */
	return AcpToUtf8(s, (long)strlen(s));
}

/* ---------------------------------------------------------------- ホスト関数 __h(op, ...) */

static void fx_host(xsMachine* the)
{
	char *argv[8], *r, *u;
	int   argc = xsToInteger(xsArgc), i;

	if (argc < 1)
		return;
	if (argc > 8)
		argc = 8;
	for (i = 0; i < argc; i++)
		argv[i] = Utf8ToAcp(xsToString(xsArg(i)));
	r = JsHostOp(argv[0] ? argv[0] : "", argc - 1, argv + 1);
	for (i = 0; i < argc; i++)
		free(argv[i]);
	if (r != NULL) {
		u = HostToUtf8(r);
		free(r);
		if (u != NULL) {
			xsResult = xsString(u);
			free(u);
		}
	}
}

/* バイト列を返すホスト関数 __hb(op, ...) → ArrayBuffer (無ければ undefined) */
static void fx_hostbin(xsMachine* the)
{
	char *argv[4], *r;
	int   argc = xsToInteger(xsArgc), i;
	long  len = 0;

	if (argc < 1)
		return;
	if (argc > 4)
		argc = 4;
	for (i = 0; i < argc; i++)
		argv[i] = Utf8ToAcp(xsToString(xsArg(i)));
	r = JsHostBin(argv[0] ? argv[0] : "", argc - 1, argv + 1, &len);
	for (i = 0; i < argc; i++)
		free(argv[i]);
	if (r != NULL) {
		xsResult = xsArrayBuffer(r, len);
		free(r);
	}
}

/* ---------------------------------------------------------------- 実行 */

static txBoolean Meter(txMachine* the, txU8 index)
{
	(void)the;
	(void)index;
	return (long)(GetTickCount() - g_deadline) < 0;
}

enum { CALL_INSTALL, CALL_SCRIPT, CALL_FUNC };

typedef struct {
	int         kind;
	const char *src;                    /* CALL_SCRIPT: UTF-8 のソース */
	long        len;
	const char *fn;                     /* CALL_FUNC: 呼ぶ大域関数の名前 */
	int         argc;
	char       *argv[4];                /* CALL_FUNC: UTF-8 の引数 */
	char       *result;                 /* CALL_FUNC: 戻り値 (ACP、malloc。文字列以外は NULL) */
	const void *bin;                    /* CALL_FUNC: あれば最後の引数に ArrayBuffer として足す */
	long        binLen;
} Call;

static void SetMsgUtf8(const char *prefix, const char *u)
{
	char *a = Utf8ToAcp(u);
	_snprintf(g_msg, sizeof(g_msg) - 1, "%s%s", prefix, a ? a : "");
	g_msg[sizeof(g_msg) - 1] = '\0';
	free(a);
}

/* XS のマシンに入って 1 つの呼び出しを行う。例外は g_msg に記録して 0 を返す */
/* 前処理部分のバイトコードの写しを作る (fxRunScript が名前を番号に書き換えて、終わったら解放する) */
static txScript *PreludeScript(void)
{
	txScript *s = c_malloc(sizeof(txScript));

	if (s == NULL)
		return NULL;
	memset(s, 0, sizeof(*s));
	s->symbolsBuffer = c_malloc(sizeof(g_preSym));
	s->codeBuffer    = c_malloc(sizeof(g_preCode));
	if (s->symbolsBuffer == NULL || s->codeBuffer == NULL) {
		fxDeleteScript(s);
		return NULL;
	}
	memcpy(s->symbolsBuffer, g_preSym, sizeof(g_preSym));
	memcpy(s->codeBuffer, g_preCode, sizeof(g_preCode));
	s->symbolsSize = sizeof(g_preSym);
	s->codeSize    = sizeof(g_preCode);
	memcpy(s->version, g_preVer, sizeof(s->version));
	return s;
}

static int Enter(Call *c)
{
	txMachine *machine = g_vm;
	int        ok = 0;

	xsBeginHost(machine);
	{
		xsVars(2);
		xsTry {
			if (c->kind == CALL_INSTALL) {
				xsSet(xsGlobal, xsID("__h"), xsNewHostFunction(fx_host, 1));
				xsSet(xsGlobal, xsID("__hb"), xsNewHostFunction(fx_hostbin, 1));
			}
			if (c->kind == CALL_INSTALL || c->kind == CALL_SCRIPT) {
				txSlot* realm = mxProgram.value.reference->next->value.module.realm;
				txStringCStream stream;
				txScript* script;
				stream.buffer = (char *)c->src;
				stream.offset = 0;
				stream.size = c->len;
				script = c->kind == CALL_INSTALL ? PreludeScript() : fxParseScript(the, &stream, fxStringCGetter, mxProgramFlag);
				if (script == NULL && c->kind == CALL_INSTALL)
					xsUnknownError("not enough memory");
				fxRunScript(the, script, mxRealmGlobal(realm), C_NULL,
				            mxRealmClosures(realm)->value.reference, C_NULL, mxProgram.value.reference);
				mxPop();
			} else if (c->bin != NULL) {
				/* XHR の結果: (番号, 文字列, ArrayBuffer) */
				xsVar(1) = xsArrayBuffer((void *)c->bin, c->binLen);
				xsVar(0) = xsCall3(xsGlobal, xsID((txString)c->fn), xsString(c->argv[0]), xsString(c->argv[1]), xsVar(1));
			} else {
				switch (c->argc) {
				case 0: xsVar(0) = xsCall0(xsGlobal, xsID((txString)c->fn)); break;
				case 1: xsVar(0) = xsCall1(xsGlobal, xsID((txString)c->fn), xsString(c->argv[0])); break;
				case 2: xsVar(0) = xsCall2(xsGlobal, xsID((txString)c->fn), xsString(c->argv[0]), xsString(c->argv[1])); break;
				case 3: xsVar(0) = xsCall3(xsGlobal, xsID((txString)c->fn), xsString(c->argv[0]), xsString(c->argv[1]),
				                           xsString(c->argv[2])); break;
				default: xsVar(0) = xsCall4(xsGlobal, xsID((txString)c->fn), xsString(c->argv[0]), xsString(c->argv[1]),
				                            xsString(c->argv[2]), xsString(c->argv[3])); break;
				}
				if (xsTypeOf(xsVar(0)) == xsStringType)
					c->result = Utf8ToAcp(xsToString(xsVar(0)));
			}
			while (the->promiseJobs) {
				the->promiseJobs = 0;
				fxRunPromiseJobs(the);
			}
			ok = 1;
		}
		xsCatch {
			SetMsgUtf8("", xsToString(xsException));
			g_errors++;
		}
	}
	xsEndHost(machine);
	return ok;
}

/* 時間の上限を付けて Enter する。止められたらこのページのスクリプトを終わりにする */
static int Invoke(Call *c)
{
	txMachine *machine = g_vm;
	int        ok = 0;

	if (machine == NULL || g_busy)
		return 0;
	g_msg[0] = '\0';                    /* 前回のエラーは報告済み */
	g_busy = 1;
	g_deadline = GetTickCount() + (DWORD)g_timeLimit;
	xsBeginMetering(machine, Meter, 10000);
	{
		ok = Enter(c);
	}
	xsEndMetering(machine);
	g_busy = 0;
	if (machine->exitStatus) {
		switch (machine->exitStatus) {
		case XS_TOO_MUCH_COMPUTATION_EXIT:
			_snprintf(g_msg, sizeof(g_msg) - 1, "スクリプトが %ld 秒以上動き続けたため止めました", g_timeLimit / 1000);
			break;
		case XS_NOT_ENOUGH_MEMORY_EXIT:
			_snprintf(g_msg, sizeof(g_msg) - 1, "スクリプトのメモリが上限 (%ld KB) を超えたため止めました",
			          g_memLimit / 1024);
			break;
		case XS_NATIVE_STACK_OVERFLOW_EXIT:
			_snprintf(g_msg, sizeof(g_msg) - 1, "スクリプトの呼び出しが深すぎるため止めました (C のスタック)");
			break;
		case XS_JAVASCRIPT_STACK_OVERFLOW_EXIT:
			_snprintf(g_msg, sizeof(g_msg) - 1, "スクリプトの呼び出しが深すぎるため止めました");
			break;
		default:
			_snprintf(g_msg, sizeof(g_msg) - 1, "スクリプトを止めました (%s)", fxAbortString(machine->exitStatus));
			break;
		}
		g_msg[sizeof(g_msg) - 1] = '\0';
		g_errors++;
		JsPageEnd();
		g_dead = 1;
		return 0;
	}
	return ok;
}

/* 大域関数 fn を ACP の引数で呼ぶ。戻り値の文字列は *result に (要 free) */
static int CallFunc(const char *fn, int argc, const char **argv, char **result)
{
	Call c;
	int  i, ok;

	memset(&c, 0, sizeof(c));
	c.kind = CALL_FUNC;
	c.fn   = fn;
	c.argc = argc;
	for (i = 0; i < argc; i++) {
		c.argv[i] = HostToUtf8(argv[i]);
		if (c.argv[i] == NULL) {
			c.argc = i;
			break;
		}
	}
	ok = Invoke(&c);
	for (i = 0; i < c.argc; i++)
		free(c.argv[i]);
	if (result != NULL)
		*result = c.result;
	else
		free(c.result);
	return ok;
}

/* document.フォーム名 などを DOM に合わせる。全部のフォームの部品と画像をたどるので、
   DOM が前回から変わっていなければ飛ばす (イベントのたびに動くと遅い) */
static void Sync(void)
{
	if (g_syncOk && g_syncGen == g_domGen)
		return;
	g_syncGen = g_domGen;
	CallFunc("__sync", 0, NULL, NULL);
	g_syncOk = g_vm != NULL;
}

/* ---------------------------------------------------------------- 公開する関数 */

void JsConfigure(long memLimit, long timeLimit)
{
	g_memLimit  = memLimit;
	g_timeLimit = timeLimit;
}

int JsPageBegin(void)
{
	static xsCreation creation = {
		64 * 1024,      /* initialChunkSize: 文字列・コードなど */
		256 * 1024,     /* incrementalChunkSize */
		4 * 1024,       /* initialHeapCount: オブジェクトの部品 (1 つ 16 バイト) */
		32 * 1024,      /* incrementalHeapCount (小さいと、たくさん作って捨てる処理で全体の回収が何度も走る) */
		32 * 1024,      /* stackCount: 呼び出しの深さ (1 つ 16 バイト。8K では難読化したスクリプトで足りない) */
		512,            /* initialKeyCount */
		256,            /* incrementalKeyCount */
		397,            /* nameModulo */
		61,             /* symbolModulo */
		64 * 1024,      /* parserBufferSize: 1 つの文字列リテラルの上限 */
		397,            /* parserTableModulo */
		0,              /* staticSize */
		0               /* nativeStackSize */
	};
	Call c;

	if (g_vm != NULL)
		return 1;
	if (g_dead)
		return 0;
	g_syncOk = 0;
	g_xsMemLimit = g_memLimit;
	g_xsMemUsed  = 0;
	g_vm = xsCreateMachine(&creation, "minibrow", NULL);
	if (g_vm == NULL) {
		_snprintf(g_msg, sizeof(g_msg) - 1, "スクリプトの実行環境を作れません (メモリ不足)");
		g_dead = 1;
		return 0;
	}
	memset(&c, 0, sizeof(c));
	c.kind = CALL_INSTALL;
	if (!Invoke(&c)) {
		JsPageEnd();
		g_dead = 1;
		return 0;
	}
	return 1;
}

void JsPageEnd(void)
{
	if (g_vm != NULL && !g_busy) {
		xsDeleteMachine(g_vm);
		g_vm = NULL;
	}
}

/* 新しいページ用に状態を戻す (JsPageEnd の後で呼ぶ) */
static void ResetPage(void)
{
	g_dead   = 0;
	g_errors = 0;
	g_msg[0] = '\0';
}

int JsPageActive(void)
{
	return g_vm != NULL;
}

int JsPageDead(void)
{
	return g_dead;
}

int JsBusy(void)
{
	return g_busy;
}

/* スクリプトを動かす。utf8 なら src は UTF-8 (外部スクリプト。ACP を通すと表せない文字が化けるので)、
   そうでなければ ACP */
static int RunScript(const char *src, long len, int utf8)
{
	char *buf, *u;
	long  i;
	Call  c;
	int   ok;

	if (!JsPageBegin())
		return 0;
	Sync();
	if (g_vm == NULL)
		return 0;
	/* 古いページの <!-- と --> (行頭のもの) は 1 行のコメントとして扱う (Annex B 相当) */
	buf = malloc(len + 1);
	if (buf == NULL)
		return 0;
	memcpy(buf, src, len);
	buf[len] = '\0';
	for (i = 0; i < len; i++) {
		long j = i;
		while (j < len && (buf[j] == ' ' || buf[j] == '\t' || buf[j] == '\r'))
			j++;
		if (j + 3 < len && memcmp(buf + j, "<!--", 4) == 0) {
			memcpy(buf + j, "//  ", 4);
		} else if (j + 2 < len && memcmp(buf + j, "-->", 3) == 0) {
			memcpy(buf + j, "// ", 3);
		}
		while (i < len && buf[i] != '\n')
			i++;
	}
	u = utf8 ? CesuFromUtf8(buf) : AcpToUtf8(buf, len);
	free(buf);
	if (u == NULL)
		return 0;
	memset(&c, 0, sizeof(c));
	c.kind = CALL_SCRIPT;
	c.src  = u;
	c.len  = (long)strlen(u);
	ok = Invoke(&c);
	free(u);
	return ok;
}

int JsRunScript(const char *src, long len)
{
	return RunScript(src, len, 0);
}

int JsRunScriptUtf8(const char *src, long len)
{
	return RunScript(src, len, 1);
}

int JsDispatch(int id, const char *type)
{
	const char *argv[2];
	char        num[16], *r = NULL;
	int         ok;

	if (!JsPageBegin())
		return 1;
	Sync();
	sprintf(num, "%d", id);
	argv[0] = num;
	argv[1] = type;
	ok = CallFunc("__event", 2, argv, &r);
	ok = !(ok && r != NULL && strcmp(r, "0") == 0);
	free(r);
	return ok;
}

/* イベントの種類 type (\x01 の前まで) を受け取るものが JS の側にあるか。分からなければ真 */
int JsHasListener(const char *type)
{
	char        name[32], *r = NULL;
	const char *argv[1];
	int         i, res;

	if (g_vm == NULL)
		return 1;
	for (i = 0; i < 31 && type[i] && type[i] != '\x01'; i++)
		name[i] = type[i];
	name[i] = '\0';
	argv[0] = name;
	if (!CallFunc("__hasev", 1, argv, &r)) {
		free(r);
		return 1;
	}
	res = r == NULL || r[0] != '0';
	free(r);
	return res;
}

void JsRunUrl(const char *code)
{
	const char *argv[1];
	if (!JsPageBegin())
		return;
	Sync();
	argv[0] = code;
	CallFunc("__evalUrl", 1, argv, NULL);
}

void JsFireReady(void)
{
	if (g_vm == NULL)
		return;
	Sync();
	CallFunc("__ready", 0, NULL, NULL);
}

void JsFireLoad(void)
{
	if (g_vm == NULL)
		return;
	Sync();
	CallFunc("__load", 0, NULL, NULL);
}

long JsTick(void)
{
	char *r = NULL;
	long  ms = -1;
	if (g_vm == NULL)
		return -1;
	if (CallFunc("__tick", 0, NULL, &r) && r != NULL)
		ms = atol(r);
	free(r);
	return g_vm != NULL ? ms : -1;
}

void JsXhrDone(int id, const char *result, const void *bin, long binLen)
{
	char num[16];
	Call c;

	if (g_vm == NULL)
		return;
	sprintf(num, "%d", id);
	memset(&c, 0, sizeof(c));
	c.kind    = CALL_FUNC;
	c.fn      = "__xhrdone";
	c.argc    = 2;
	c.argv[0] = HostToUtf8(num);
	c.argv[1] = HostToUtf8(result);
	c.bin     = bin != NULL ? bin : (binLen = 0, "");
	c.binLen  = binLen;
	if (c.argv[0] != NULL && c.argv[1] != NULL)
		Invoke(&c);
	free(c.argv[0]);
	free(c.argv[1]);
	free(c.result);
}

void JsXhrProgress(int id, long loaded, long total, int upload)
{
	const char *argv[3];
	char        a[16], b[16], t[16];

	if (g_vm == NULL)
		return;
	sprintf(a, "%d", id);
	sprintf(b, "%ld", loaded);
	sprintf(t, "%ld", total);
	argv[0] = a;
	argv[1] = b;
	argv[2] = t;
	CallFunc(upload ? "__xhrup" : "__xhrprog", 3, argv, NULL);
}

int JsCallFn(const char *fn, int argc, const char **argv)
{
	if (g_vm == NULL)
		return 0;
	return CallFunc(fn, argc, argv, NULL);
}

char *JsCallFnStr(const char *fn, int argc, const char **argv)
{
	char *r = NULL;
	if (g_vm == NULL)
		return NULL;
	CallFunc(fn, argc, argv, &r);
	return r;
}

void JsResetDoc(void)
{
	g_syncOk = 0;
	if (g_vm != NULL)
		CallFunc("__reset", 0, NULL, NULL);
}

void JsAddTime(unsigned long ms)
{
	g_deadline += (DWORD)ms;
}

/* ids (文書から外れている部分木ごとの要素の番号) を弱い参照にし、回収済みの要素の番号を返す (要 free) */
char *JsGc(const char *ids)
{
	txMachine  *machine = g_vm;
	const char *argv[1];
	char       *r = NULL;

	if (machine == NULL || g_busy)
		return NULL;
	xsBeginHost(machine);
	{
		xsCollectGarbage();
	}
	xsEndHost(machine);
	argv[0] = ids;
	CallFunc("__gc", 1, argv, &r);
	return r;
}

const char *JsMessage(void)
{
	return g_msg;
}

int JsErrorCount(void)
{
	return g_errors;
}

/* ページを移る: 実行環境を捨てて状態を戻す */
void JsNewPage(void)
{
	JsPageEnd();
	ResetPage();
}
