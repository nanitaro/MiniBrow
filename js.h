/*
 * js.h - MiniBrow の JavaScript (XS エンジン) の窓口
 *
 * Netscape 3 程度の機能 (document.write、フォーム、画像の差し替え、location、
 * タイマー、イベント属性) を、js.c に埋め込んだ JavaScript の前処理部分で組み立てる。
 * ページの情報の読み書きは JsHostOp を通して minibrow.c が行う。
 * 文字列はすべてシステムのコードページ (ACP) で受け渡す。
 * 画面のスレッドだけから呼ぶこと。
 */
#ifndef JS_H
#define JS_H

/* 起動時に 1 回。memLimit = 1 ページのメモリ上限 (バイト)、timeLimit = 1 回の実行の上限 (ミリ秒) */
void        JsConfigure(long memLimit, long timeLimit);
/* DOM の世代の番号 (minibrow.c が木か属性を変えるたびに増やす)。変わっていなければ __sync を飛ばす */
extern unsigned long g_domGen;

/* ページの実行環境を用意する (用意済みなら何もしない)。使えなければ 0 */
int         JsPageBegin(void);
/* ページの実行環境を捨てる */
void        JsPageEnd(void);
/* 新しいページを読む前に呼ぶ (実行環境を捨て、エラーの記録なども消す) */
void        JsNewPage(void);
/* このページの実行環境がある */
int         JsPageActive(void);
/* このページでスクリプトが止められた (時間切れ・メモリ不足) */
int         JsPageDead(void);
/* スクリプトを実行中 (入れ子で呼ばないこと) */
int         JsBusy(void);

/* スクリプトを実行する。成功なら 1 */
int         JsRunScript(const char *src, long len);
/* UTF-8 のスクリプト (外部スクリプト) を動かす */
int         JsRunScriptUtf8(const char *src, long len);
/* イベントを要素に送る (属性・プロパティ・addEventListener のものを親へ伝えながら呼ぶ)。
   id = DOM の要素の番号 (0 = document, -1 = window)。preventDefault されたら 0 */
int         JsDispatch(int id, const char *type);
/* javascript: の URL を実行する */
/* イベントの種類を受け取るものが JS の側にあるか (無ければ送らなくてよい) */
int         JsHasListener(const char *type);
void        JsRunUrl(const char *code);
/* ページの解析が終わった (DOMContentLoaded) */
void        JsFireReady(void);
/* ページの読み込み完了。画像も読み終えた後に呼ぶ (window.onload / <body onload>) */
void        JsFireLoad(void);
/* 期限の来たタイマーを実行する。次のタイマーまでのミリ秒 (無ければ -1) */
long        JsTick(void);
/* 文書から外れている部分木ごとの要素の番号 ("1,5;9,12") を渡し、スクリプトから参照されなくなった
   (回収された) 要素の番号を返す (malloc した文字列、NULL 可) */
char       *JsGc(const char *ids);
/* XMLHttpRequest の結果を渡す (result の形は minibrow.c の XhrResult。先頭が \3 なら UTF-8)。
   bin は本文のバイト列 (ArrayBuffer として渡す。NULL なら空) */
void        JsXhrDone(int id, const char *result, const void *bin, long binLen);
/* XMLHttpRequest の進み具合 (upload なら送信側。total = 全体の大きさ、分からなければ 0) */
void        JsXhrProgress(int id, long loaded, long total, int upload);
/* 前処理部分の大域関数 fn を呼ぶ (実行環境が無ければ何もしない)。成功なら 1 */
int         JsCallFn(const char *fn, int argc, const char **argv);
/* 同じく呼んで、戻り値の文字列を返す (ACP、要 free。文字列でなければ NULL) */
char       *JsCallFnStr(const char *fn, int argc, const char **argv);
/* document.open: 前の文書の要素・イベントの登録を捨てる (変数・タイマーは残す) */
void        JsResetDoc(void);
/* 今の実行の時間の上限を ms 延ばす (同期の XHR で待った分) */
void        JsAddTime(unsigned long ms);

/* 最後のエラーの説明 (ACP)。無ければ "" */
const char *JsMessage(void);
/* このページで起きたエラーの数 */
int         JsErrorCount(void);

/* ---- minibrow.c が用意する関数 */

/* 前処理部分の __h(op, ...) から呼ばれる。戻り値は malloc した文字列 (NULL = undefined) */
char       *JsHostOp(const char *op, int argc, char **argv);
/* 前処理部分の __hb(op, ...) から呼ばれる。バイト列 (malloc、長さは *len) を返す (NULL = undefined) */
char       *JsHostBin(const char *op, int argc, char **argv, long *len);
/* UTF-16 を ACP に変換する (malloc した文字列) */
char       *JsHostWideToAcp(const unsigned short *w, int n);

#endif
