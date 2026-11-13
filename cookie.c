/*
 * cookie.c - MiniBrow の Cookie の保管 (RFC 6265 の簡易版)
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "cookie.h"

#define MAX_COOKIES     1000
#define MAX_PER_DOMAIN  60
#define MAX_COOKIE_LEN  4096            /* 名前と値の長さの合計 */
#define MAX_HDR_LEN     8192            /* Set-Cookie 1 行の長さ */
#define TIME_MAX        0xFFFFFFF0UL    /* 2106 年まで。それより先は切り詰める */

typedef struct {
    char          *name, *value, *domain, *path;   /* name の領域にまとめて確保 */
    unsigned long  expires;             /* 1970 年からの秒 (UTC)。0 = 終了で消える */
    unsigned long  seq;                 /* 作った順 (同じパスの長さなら古い方から送る) */
    BYTE           hostOnly, secure, httpOnly;
    BYTE           sameSite;                    /* 0 = 指定なし・None、1 = Lax、2 = Strict */
} Cookie;

static Cookie          *g_ck;           /* 作った順に並べる */
static int              g_n, g_cap, g_inited, g_dirty;
static unsigned long    g_seq;
static char             g_file[MAX_PATH];
static CRITICAL_SECTION g_cs;

/* 複数の MiniBrow (window.open で開いたウィンドウなど) の間での共有:
   2 つ以上動いている間は、変えるたびに全部を一時フォルダの minibrow.ck に書き出して版を上げる。
   ほかのプロセスは版が変わっていれば読み直す。版と動いている数は名前付きの共有メモリに置き、
   名前付きのミューテックスで順番に使う */
typedef struct {
    long ver;                           /* minibrow.ck を書き出した回数 */
    long procs;                         /* 動いている MiniBrow の数 */
} Share;

static HANDLE           g_mutex, g_map;
static Share           *g_share;
static long             g_ver;          /* 最後に読んだ (書いた) 版 */
static int              g_unshared;     /* minibrow.ck にまだ書いていない変更がある */
static char             g_shFile[MAX_PATH];

/* ---------------------------------------------------------------- 時刻 */

/* 1970-01-01 からの日数 (グレゴリオ暦) */
static long DaysFrom1970(long y, int m, int d)
{
    long era, yoe, doy, doe;

    y  -= m <= 2;
    era = (y >= 0 ? y : y - 399) / 400;
    yoe = y - era * 400;
    doy = (153L * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097L + doe - 719468L;
}

static unsigned long Now(void)
{
    SYSTEMTIME t;
    GetSystemTime(&t);
    return (unsigned long)DaysFrom1970(t.wYear, t.wMonth, t.wDay) * 86400UL
           + t.wHour * 3600UL + t.wMinute * 60UL + t.wSecond;
}

/* Expires の日付 ("Wed, 21 Oct 2015 07:28:00 GMT" や "21-Oct-15" など) を読む */
static int ParseDate(const char *s, unsigned long *out)
{
    static const char mons[] = "janfebmaraprmayjunjulaugsepoctnovdec";
    long        day = -1, mon = -1, year = -1, hh = 0, mi = 0, ss = 0, v;
    int         n, digits, a, b, c, haveTime = 0;
    char        tok[32], m3[4];
    const char *q;

    while (*s) {
        while (*s && !isalnum((BYTE)*s) && *s != ':') {
            s++;
        }
        for (n = 0; *s && (isalnum((BYTE)*s) || *s == ':'); s++) {
            if (n < (int)sizeof(tok) - 1) {
                tok[n++] = *s;
            }
        }
        tok[n] = '\0';
        if (n == 0) {
            break;
        }
        digits = (int)strspn(tok, "0123456789");
        if (!haveTime && digits > 0 && strchr(tok, ':') != NULL) {
            if (sscanf(tok, "%d:%d:%d", &a, &b, &c) == 3) {
                hh = a;
                mi = b;
                ss = c;
                haveTime = 1;
            }
        } else if (digits > 0) {
            v = atol(tok);
            if (day < 0 && digits <= 2) {
                day = v;
            } else if (year < 0 && digits >= 2 && digits <= 4) {
                year = v;
            }
        } else if (mon < 0 && n >= 3) {
            m3[0] = (char)tolower((BYTE)tok[0]);
            m3[1] = (char)tolower((BYTE)tok[1]);
            m3[2] = (char)tolower((BYTE)tok[2]);
            m3[3] = '\0';
            q = strstr(mons, m3);
            if (q != NULL && (q - mons) % 3 == 0) {
                mon = (long)(q - mons) / 3 + 1;
            }
        }
    }
    if (day < 1 || day > 31 || mon < 0 || year < 0 || hh > 23 || mi > 59 || ss > 60) {
        return 0;
    }
    if (year < 70) {
        year += 2000;
    } else if (year < 100) {
        year += 1900;
    }
    if (year < 1970) {
        *out = 1;                               /* 過去 = 削除の指示 */
    } else if (year > 2105) {
        *out = TIME_MAX;
    } else {
        *out = (unsigned long)DaysFrom1970(year, (int)mon, (int)day) * 86400UL
               + hh * 3600UL + mi * 60UL + ss;
        if (*out == 0) {
            *out = 1;                           /* 0 は「期限なし」と区別できないので */
        }
    }
    return 1;
}

/* ---------------------------------------------------------------- ドメインとパス */

static void LowerCopy(char *dst, const char *src, int size)
{
    int i;
    for (i = 0; src[i] && i < size - 1; i++) {
        dst[i] = (char)tolower((BYTE)src[i]);
    }
    dst[i] = '\0';
}

static int IsIp(const char *h)
{
    return strspn(h, "0123456789.") == strlen(h) || strchr(h, ':') != NULL;
}

/* ---- Public Suffix List (minibrow.c が作る psl.bin。読み取り専用で割り当てるので、OS がプロセスの間で共有する) */

static void Lock(void);
static void Unlock(void);

#define PSL_HEAD 8                              /* psl.bin の先頭の印の大きさ */

static char              g_pslPath[MAX_PATH];
static const char *volatile g_psl;              /* 割り当てた psl.bin (NULL = まだ・無い) */
static DWORD             g_pslSize;

void CookiePslFile(const char *path)
{
    lstrcpyn(g_pslPath, path, sizeof(g_pslPath));
}

/* psl.bin を割り当てる。同じ内容なら同じ割り当てを共有するよう、大きさと更新時刻を名前にする */
static void PslOpen(void)
{
    HANDLE   h = CreateFile(g_pslPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE   m;
    FILETIME ft;
    DWORD    size;
    char     name[48];
    const char *v;

    if (h == INVALID_HANDLE_VALUE) {
        return;
    }
    size = GetFileSize(h, NULL);
    if (size < PSL_HEAD + 2 || size > 8UL * 1024 * 1024 || !GetFileTime(h, NULL, NULL, &ft)) {
        CloseHandle(h);
        return;
    }
    sprintf(name, "MiniBrowPsl%lu_%lu", (unsigned long)size, (unsigned long)ft.dwLowDateTime);
    m = CreateFileMapping(h, NULL, PAGE_READONLY, 0, 0, name);
    v = m != NULL ? MapViewOfFile(m, FILE_MAP_READ, 0, 0, 0) : NULL;
    if (v == NULL || memcmp(v, "MBPSL1\0\0", PSL_HEAD) != 0 || v[size - 1] != '\0') {
        if (v != NULL) {
            UnmapViewOfFile((void *)v);
        }
        if (m != NULL) {
            CloseHandle(m);
        }
        CloseHandle(h);
        return;
    }
    g_pslSize = size;                           /* h と m は終了まで持つ (使用中は psl.bin を差し替えられない) */
    g_psl     = v;
}

/* 整列した規則の中に key があるか (壊れたファイルでも範囲の外は読まない) */
static int PslHas(const char *key)
{
    DWORD lo = PSL_HEAD, hi = g_pslSize;

    while (lo < hi) {
        DWORD s = lo + (hi - lo) / 2, e;
        int   c;
        while (s > lo && g_psl[s - 1] != '\0') {
            s--;
        }
        for (e = s; g_psl[e] != '\0'; e++) {
        }
        c = strcmp(key, g_psl + s);
        if (c == 0) {
            return 1;
        }
        if (c < 0) {
            hi = s;
        } else {
            lo = e + 1;
        }
    }
    return 0;
}

/* 一覧で d が公開サフィックスか (例外 !x → 完全一致 → *.親 → 最後の 1 段だけの決まり) */
static int PslIsPublic(const char *d)
{
    const char *dot = strchr(d, '.');
    char        key[260];

    if (strlen(d) >= 256) {
        return 0;
    }
    key[0] = '!';
    strcpy(key + 1, d);
    if (PslHas(key)) {
        return 0;
    }
    if (PslHas(d)) {
        return 1;
    }
    if (dot != NULL) {
        key[0] = '*';
        strcpy(key + 1, dot);
        if (PslHas(key)) {
            return 1;
        }
    }
    return dot == NULL;
}

/* 誰でも登録できるドメイン (com や co.jp、github.io など) なら真。一覧 (psl.bin) が無ければ簡易判定 */
static int IsPublicSuffix(const char *d)
{
    static const char *const second[] = {
        "ac", "co", "com", "ed", "edu", "go", "gob", "gov", "gr", "lg", "ltd",
        "mil", "ne", "net", "nom", "or", "org", "plc", "sch"
    };
    const char *dot = strrchr(d, '.'), *p;
    int         i, len;

    if (g_psl == NULL && g_pslPath[0]) {
        Lock();
        if (g_psl == NULL) {
            PslOpen();
        }
        Unlock();
    }
    if (g_psl != NULL) {
        return PslIsPublic(d);
    }
    if (dot == NULL) {
        return 1;
    }
    if (strlen(dot + 1) == 2) {
        /* 国別ドメインは co.jp のような 2 段目も持ち主ではないことが多い */
        for (p = dot; p > d && p[-1] != '.'; p--) {
        }
        if (p == d) {
            len = (int)(dot - p);
            for (i = 0; i < (int)(sizeof(second) / sizeof(second[0])); i++) {
                if ((int)strlen(second[i]) == len && strncmp(p, second[i], len) == 0) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* host の登録可能なドメイン (公開サフィックスの 1 つ下) の位置を返す */
static const char *RegDomain(const char *host)
{
    const char *cur = host, *dot;

    if (IsIp(host)) {
        return host;
    }
    while ((dot = strchr(cur, '.')) != NULL) {
        if (IsPublicSuffix(dot + 1)) {
            return cur;
        }
        cur = dot + 1;
    }
    return cur;
}

int CookieSameSite(const char *a, const char *b)
{
    char la[256], lb[256];

    if (!g_inited) {
        return 1;
    }
    LowerCopy(la, a, sizeof(la));
    LowerCopy(lb, b, sizeof(lb));
    return strcmp(RegDomain(la), RegDomain(lb)) == 0;
}

/* host が domain かその下のホストか */
static int DomainMatch(const char *host, const char *domain)
{
    size_t hl = strlen(host), dl = strlen(domain);

    if (strcmp(host, domain) == 0) {
        return 1;
    }
    return hl > dl && strcmp(host + hl - dl, domain) == 0 && host[hl - dl - 1] == '.' && !IsIp(host);
}

/* 要求パス ("?" 以降は見ない) が Cookie のパスの下か */
static int PathMatch(const char *req, const char *cpath)
{
    size_t rl = strcspn(req, "?#"), cl = strlen(cpath);

    if (cl > rl || strncmp(req, cpath, cl) != 0) {
        return 0;
    }
    return cl == rl || cpath[cl - 1] == '/' || req[cl] == '/';
}

/* Path 属性が無いときのパス = 要求パスの最後の "/" の手前まで */
static void DefaultPath(const char *req, char *out, int size)
{
    int n = (int)strcspn(req, "?#"), last = -1, i;

    for (i = 0; i < n; i++) {
        if (req[i] == '/') {
            last = i;
        }
    }
    if (req[0] != '/' || last <= 0) {
        strcpy(out, "/");
        return;
    }
    if (last > size - 1) {
        last = size - 1;
    }
    memcpy(out, req, last);
    out[last] = '\0';
}

/* ---------------------------------------------------------------- 保管 */

static void Remove(int i)
{
    free(g_ck[i].name);
    memmove(&g_ck[i], &g_ck[i + 1], (size_t)(g_n - i - 1) * sizeof(Cookie));
    g_n--;
}

static int Find(const char *name, const char *domain, const char *path)
{
    int i;
    for (i = 0; i < g_n; i++) {
        if (strcmp(g_ck[i].name, name) == 0 && strcmp(g_ck[i].domain, domain) == 0
            && strcmp(g_ck[i].path, path) == 0) {
            return i;
        }
    }
    return -1;
}

/* https の接続でないときに、触ってはいけない Secure の Cookie があるか
   (同じ名前で、ドメインがどちらかの下にあり、新しいパスが既存のパスの下) */
static int SecureProtected(const char *name, const char *domain, const char *path)
{
    int i;

    for (i = 0; i < g_n; i++) {
        if (g_ck[i].secure && strcmp(g_ck[i].name, name) == 0
            && (DomainMatch(domain, g_ck[i].domain) || DomainMatch(g_ck[i].domain, domain))
            && PathMatch(path, g_ck[i].path)) {
            return 1;
        }
    }
    return 0;
}
static void PurgeExpired(unsigned long now)
{
    int i;
    for (i = g_n - 1; i >= 0; i--) {
        if (g_ck[i].expires != 0 && g_ck[i].expires <= now) {
            Remove(i);
        }
    }
}

/* 数が多すぎるときは古いものから捨てる */
static void MakeRoom(const char *domain)
{
    int i, inDom, first;

    for (;;) {
        inDom = 0;
        first = -1;
        for (i = 0; i < g_n; i++) {
            if (strcmp(g_ck[i].domain, domain) == 0) {
                if (first < 0) {
                    first = i;
                }
                inDom++;
            }
        }
        if (inDom >= MAX_PER_DOMAIN) {
            Remove(first);
        } else if (g_n >= MAX_COOKIES) {
            Remove(0);
        } else {
            return;
        }
    }
}

static void Store(const char *name, const char *value, const char *domain, const char *path,
                  unsigned long expires, int hostOnly, int secure, int httpOnly, int sameSite)
{
    size_t        ln = strlen(name) + 1, lv = strlen(value) + 1, ld = strlen(domain) + 1;
    size_t        lp = strlen(path) + 1;
    unsigned long seq;
    int           i = Find(name, domain, path), at;
    Cookie       *c, *grown;
    char         *b;

    if (i >= 0) {
        /* 同じ名前・ドメイン・パスのものは置き換える (作った順は元のまま) */
        seq = g_ck[i].seq;
        Remove(i);
        at = i;
    } else {
        MakeRoom(domain);
        seq = ++g_seq;
        at = g_n;
    }
    if (g_n >= g_cap) {
        int cap = g_cap ? g_cap * 2 : 64;
        grown = realloc(g_ck, (size_t)cap * sizeof(Cookie));
        if (grown == NULL) {
            return;
        }
        g_ck  = grown;
        g_cap = cap;
    }
    b = malloc(ln + lv + ld + lp);
    if (b == NULL) {
        return;
    }
    memmove(&g_ck[at + 1], &g_ck[at], (size_t)(g_n - at) * sizeof(Cookie));
    g_n++;
    c = &g_ck[at];
    c->name   = b;
    c->value  = b + ln;
    c->domain = c->value + lv;
    c->path   = c->domain + ld;
    memcpy(c->name, name, ln);
    memcpy(c->value, value, lv);
    memcpy(c->domain, domain, ld);
    memcpy(c->path, path, lp);
    c->expires  = expires;
    c->seq      = seq;
    c->hostOnly = (BYTE)hostOnly;
    c->secure   = (BYTE)secure;
    c->httpOnly = (BYTE)httpOnly;
    c->sameSite = (BYTE)sameSite;
}

static char *Trim(char *s)
{
    char *e;
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t')) {
        *--e = '\0';
    }
    return s;
}

/* 制御文字 (保存形式を壊すタブや改行) を含むか */
static int HasCtl(const char *s)
{
    for (; *s; s++) {
        if ((BYTE)*s < 0x20 || *s == 0x7F) {
            return 1;
        }
    }
    return 0;
}

/* cookies.txt (Netscape 形式: ドメイン, 下位も, パス, Secure, 期限, 名前, 値) を読む。
   session = 期限 0 (終了で消える Cookie) も読む (minibrow.ck) */
static void Load(const char *file, int session)
{
    FILE         *fp = fopen(file, "r");
    char         *line, *p, *f[7], domain[256];
    int           n, len, httpOnly, sameSite;
    unsigned long now = Now(), expires;

    if (fp == NULL) {
        return;
    }
    line = malloc(MAX_HDR_LEN);
    while (line != NULL && fgets(line, MAX_HDR_LEN, fp) != NULL) {
        len = (int)strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        p = line;
        httpOnly = 0;
        sameSite = 0;
        if (strncmp(p, "#SameSite_Strict_", 17) == 0) {
            p += 17;                            /* MiniBrow の拡張 (ほかのソフトでは注釈の行になる) */
            sameSite = 2;
        } else if (strncmp(p, "#SameSite_Lax_", 14) == 0) {
            p += 14;
            sameSite = 1;
        }
        if (strncmp(p, "#HttpOnly_", 10) == 0) {
            p += 10;                            /* curl と同じ書き方 */
            httpOnly = 1;
        } else if (*p == '#' || *p == '\0') {
            continue;
        }
        n = 0;
        f[n++] = p;
        while (n < 7 && (p = strchr(p, '\t')) != NULL) {
            *p++ = '\0';
            f[n++] = p;
        }
        if (n < 7) {
            continue;
        }
        expires = strtoul(f[4], NULL, 10);
        if (expires == 0 ? !session : expires <= now) {
            continue;
        }
        LowerCopy(domain, f[0][0] == '.' ? f[0] + 1 : f[0], sizeof(domain));
        if (domain[0] == '\0' || f[2][0] != '/') {
            continue;
        }
        Store(f[5], f[6], domain, f[2], expires > TIME_MAX ? TIME_MAX : expires,
              stricmp(f[1], "TRUE") != 0, stricmp(f[3], "TRUE") == 0, httpOnly, sameSite);
    }
    free(line);
    fclose(fp);
}

/* 全部を file に書く。session = 期限 0 の Cookie も書く。書けたら 1 */
static int Write(const char *file, int session)
{
    char          tmp[MAX_PATH];
    FILE         *fp;
    unsigned long now = Now();
    int           i, ok;
    const Cookie *c;

    sprintf(tmp, "%s.new", file);
    fp = fopen(tmp, "w");
    if (fp == NULL) {
        return 0;
    }
    fputs("# Netscape HTTP Cookie File\n"
          "# MiniBrow が保存した Cookie。MiniBrow を閉じている間なら削除してよい\n\n", fp);
    for (i = 0; i < g_n; i++) {
        c = &g_ck[i];
        if (c->expires == 0 ? !session : c->expires <= now) {
            continue;
        }
        fprintf(fp, "%s%s%s%s\t%s\t%s\t%s\t%lu\t%s\t%s\n",
                c->sameSite == 2 ? "#SameSite_Strict_" : c->sameSite == 1 ? "#SameSite_Lax_" : "",
                c->httpOnly ? "#HttpOnly_" : "",
                c->hostOnly ? "" : ".", c->domain,
                c->hostOnly ? "FALSE" : "TRUE", c->path, c->secure ? "TRUE" : "FALSE",
                c->expires, c->name, c->value);
    }
    ok = !ferror(fp);
    ok = fclose(fp) == 0 && ok;
    if (ok) {
        /* Win95 には MoveFileEx が無いので消してから名前を変える */
        DeleteFile(file);
        ok = MoveFile(tmp, file);
    }
    if (!ok) {
        DeleteFile(tmp);
    }
    return ok;
}

/* ---------------------------------------------------------------- プロセス間の共有 */

static void ShareInit(void)
{
    char  dir[MAX_PATH];
    int   fresh;
    DWORD n;

    g_mutex = CreateMutex(NULL, FALSE, "MiniBrowCookieLock");
    g_map   = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(Share), "MiniBrowCookieShare");
    fresh   = GetLastError() != ERROR_ALREADY_EXISTS;
    n       = GetTempPath(sizeof(dir), dir);
    if (g_mutex == NULL || g_map == NULL || n == 0 || n > sizeof(dir) - 16
        || (g_share = (Share *)MapViewOfFile(g_map, FILE_MAP_WRITE, 0, 0, 0)) == NULL) {
        if (g_map != NULL) {
            CloseHandle(g_map);
        }
        if (g_mutex != NULL) {
            CloseHandle(g_mutex);
        }
        g_map   = NULL;
        g_mutex = NULL;
        return;                                 /* 共有しない */
    }
    sprintf(g_shFile, "%sminibrow.ck", dir);
    WaitForSingleObject(g_mutex, 5000);
    if (fresh) {
        DeleteFile(g_shFile);                   /* 前に異常終了したときの残り */
    }
    g_share->procs++;
    g_ver = 0;                                  /* ほかが書き出していれば最初に使うときに読む */
    ReleaseMutex(g_mutex);
}

static void Lock(void)
{
    if (g_mutex != NULL) {
        WaitForSingleObject(g_mutex, 5000);     /* 止まったプロセスが持ったままでも先へ進む */
    }
    EnterCriticalSection(&g_cs);
}

static void Unlock(void)
{
    LeaveCriticalSection(&g_cs);
    if (g_mutex != NULL) {
        ReleaseMutex(g_mutex);
    }
}

/* 変更をほかの MiniBrow に渡す (Lock の中で。1 つだけ動いている間は書かない) */
static void Publish(void)
{
    if (g_share == NULL || !g_unshared || g_share->procs < 2) {
        return;
    }
    if (Write(g_shFile, 1)) {
        g_ver      = ++g_share->ver;
        g_unshared = 0;
    }
}

/* ほかの MiniBrow が書き出していれば読み直す (Lock の中で)。
   まだ渡していない変更があれば (書き出しの依頼が間に合わなかった)、読んだものに重ねて渡す */
static void Sync(void)
{
    if (g_share == NULL || g_share->ver == g_ver) {
        return;
    }
    if (!g_unshared) {
        while (g_n > 0) {
            Remove(g_n - 1);
        }
    }
    Load(g_shFile, 1);
    g_ver = g_share->ver;
    Publish();
}

/* ---------------------------------------------------------------- 公開する関数 */

void CookieInit(const char *file)
{
    if (g_inited) {
        return;
    }
    InitializeCriticalSection(&g_cs);
    g_inited = 1;
    if (file != NULL && strlen(file) < sizeof(g_file) - 4) {
        strcpy(g_file, file);
        Load(g_file, 0);
    }
    g_dirty = 0;
    ShareInit();
}

int CookieProcs(void)
{
    return g_share != NULL ? (int)g_share->procs : 1;
}

void CookieShare(void)
{
    if (!g_inited) {
        return;
    }
    Lock();
    Sync();
    Publish();
    Unlock();
}

void CookieEnd(void)
{
    if (g_share == NULL) {
        return;
    }
    Lock();
    if (--g_share->procs <= 0) {
        DeleteFile(g_shFile);                   /* 最後の MiniBrow */
    }
    Unlock();
    UnmapViewOfFile(g_share);
    CloseHandle(g_map);
    CloseHandle(g_mutex);
    g_share = NULL;
    g_map   = NULL;
    g_mutex = NULL;
}

void CookieSave(void)
{
    if (!g_inited || g_file[0] == '\0') {
        return;
    }
    Lock();
    Sync();                                     /* ほかの MiniBrow が変えた分も含めて保存する */
    if (g_dirty && Write(g_file, 0)) {
        g_dirty = 0;
    }
    Unlock();
}

void CookieSet(const char *host, const char *reqPath, int secure, const char *hdr, int len, int fromScript, int ctx)
{
    char          *s, *p, *semi, *eq, *name, *value, *k, *v;
    char           lhost[256], domain[256], path[1024];
    unsigned long  now, expires = 0, t;
    int            hasMaxAge = 0, isSecure = 0, isHttpOnly = 0, hostOnly = 1, i, sameSite = 0;
    long           maxAge = 0;

    if (!g_inited || len <= 0 || len > MAX_HDR_LEN) {
        return;
    }
    s = malloc((size_t)len + 1);
    if (s == NULL) {
        return;
    }
    memcpy(s, hdr, len);
    s[len] = '\0';
    LowerCopy(lhost, host, sizeof(lhost));
    domain[0] = '\0';
    path[0]   = '\0';

    /* 名前=値 ("=" が無ければ名前の無い値) */
    semi = strchr(s, ';');
    if (semi != NULL) {
        *semi++ = '\0';
    }
    eq = strchr(s, '=');
    if (eq != NULL) {
        *eq   = '\0';
        name  = Trim(s);
        value = Trim(eq + 1);
    } else {
        name  = "";
        value = Trim(s);
    }
    if ((*name == '\0' && *value == '\0') || strlen(name) + strlen(value) > MAX_COOKIE_LEN
        || HasCtl(name) || HasCtl(value)) {
        goto done;
    }

    /* 属性 */
    for (p = semi; p != NULL; p = semi) {
        semi = strchr(p, ';');
        if (semi != NULL) {
            *semi++ = '\0';
        }
        eq = strchr(p, '=');
        if (eq != NULL) {
            *eq = '\0';
            v = Trim(eq + 1);
        } else {
            v = "";
        }
        k = Trim(p);
        if (stricmp(k, "expires") == 0) {
            if (!hasMaxAge && ParseDate(v, &t)) {
                expires = t;
            }
        } else if (stricmp(k, "max-age") == 0) {
            if (*v == '-' || isdigit((BYTE)*v)) {
                hasMaxAge = 1;
                maxAge    = atol(v);
            }
        } else if (stricmp(k, "domain") == 0) {
            if (*v == '.') {
                v++;
            }
            if (*v != '\0') {
                LowerCopy(domain, v, sizeof(domain));
            }
        } else if (stricmp(k, "path") == 0) {
            if (*v == '/' && strlen(v) < sizeof(path)) {
                strcpy(path, v);
            }
        } else if (stricmp(k, "secure") == 0) {
            isSecure = 1;
        } else if (stricmp(k, "httponly") == 0) {
            isHttpOnly = 1;
        } else if (stricmp(k, "samesite") == 0) {
            sameSite = stricmp(v, "strict") == 0 ? 2 : stricmp(v, "lax") == 0 ? 1 : 0;
        }
    }

    now = Now();
    if (hasMaxAge) {
        if (maxAge <= 0) {
            expires = 1;                        /* 削除の指示 */
        } else {
            expires = (unsigned long)maxAge > TIME_MAX - now ? TIME_MAX : now + (unsigned long)maxAge;
        }
    }
    if (domain[0] != '\0') {
        if (strcmp(domain, lhost) != 0) {
            /* 別のサイトや com などには付けさせない */
            if (IsPublicSuffix(domain) || !DomainMatch(lhost, domain)) {
                goto done;
            }
            hostOnly = 0;
        } else if (!IsPublicSuffix(domain) && !IsIp(domain)) {
            hostOnly = 0;
        }
    } else {
        strcpy(domain, lhost);
    }
    if (path[0] == '\0') {
        DefaultPath(reqPath, path, sizeof(path));
    }
    if (isSecure && !secure) {
        goto done;                              /* http からは Secure の Cookie を作れない */
    }
    if ((strncmp(name, "__Secure-", 9) == 0 && !isSecure)
        || (strncmp(name, "__Host-", 7) == 0 && (!isSecure || !hostOnly || strcmp(path, "/") != 0))) {
        goto done;
    }

    if (fromScript && isHttpOnly) {
        goto done;                              /* スクリプトからは HttpOnly の Cookie を作れない */
    }
    if (sameSite != 0 && ctx == COOKIE_CROSS) {
        goto done;                              /* 別のサイトからの要求では SameSite の Cookie を作らせない */
    }

    Lock();
    Sync();
    PurgeExpired(now);
    if (!secure && SecureProtected(name, domain, path)) {
        ;                                       /* http からは既存の Secure の Cookie を上書き・削除できない */
    } else if (fromScript && (i = Find(name, domain, path)) >= 0 && g_ck[i].httpOnly) {
        ;                                       /* HttpOnly の Cookie はスクリプトから変えられない */
    } else if (expires != 0 && expires <= now) {
        if ((i = Find(name, domain, path)) >= 0) {
            Remove(i);
        }
    } else {
        Store(name, value, domain, path, expires, hostOnly, isSecure, isHttpOnly, sameSite);
    }
    g_dirty    = 1;
    g_unshared = 1;
    Publish();
    Unlock();
done:
    free(s);
}

/* a を b より先に送るか (パスの長い順、同じなら古い順) */
static int Before(const Cookie *a, const Cookie *b)
{
    size_t la = strlen(a->path), lb = strlen(b->path);
    return la > lb || (la == lb && a->seq < b->seq);
}

char *CookieGet(const char *host, const char *reqPath, int secure, int forScript, int ctx)
{
    char          lhost[256], *out = NULL, *q;
    unsigned long now;
    int           i, j, m = 0, *idx;
    size_t        len = 0;
    const Cookie *c;

    if (!g_inited) {
        return NULL;
    }
    LowerCopy(lhost, host, sizeof(lhost));
    now = Now();
    Lock();
    Sync();
    idx = malloc((size_t)(g_n + 1) * sizeof(int));
    if (idx != NULL) {
        for (i = 0; i < g_n; i++) {
            c = &g_ck[i];
            if ((c->expires != 0 && c->expires <= now) || (c->secure && !secure) || (forScript && c->httpOnly)
                || (c->sameSite == 2 && ctx != COOKIE_SAME) || (c->sameSite == 1 && ctx == COOKIE_CROSS)
                || !(c->hostOnly ? strcmp(lhost, c->domain) == 0 : DomainMatch(lhost, c->domain))
                || !PathMatch(reqPath, c->path)) {
                continue;
            }
            for (j = m; j > 0 && Before(c, &g_ck[idx[j - 1]]); j--) {
                idx[j] = idx[j - 1];
            }
            idx[j] = i;
            m++;
            len += strlen(c->name) + strlen(c->value) + 3;
        }
        if (m > 0 && (out = malloc(len + 1)) != NULL) {
            q = out;
            for (j = 0; j < m; j++) {
                c = &g_ck[idx[j]];
                if (j > 0) {
                    *q++ = ';';
                    *q++ = ' ';
                }
                if (c->name[0] != '\0') {
                    strcpy(q, c->name);
                    q += strlen(q);
                    *q++ = '=';
                }
                strcpy(q, c->value);
                q += strlen(q);
            }
            *q = '\0';
        }
        free(idx);
    }
    Unlock();
    return out;
}
