/*
 * cookie.h - MiniBrow の Cookie の保管
 *
 *  - Set-Cookie の Domain / Path / Expires / Max-Age / Secure を扱う
 *    HttpOnly の Cookie は document.cookie から読めず、作れず、変えられない
 *  - SameSite=Strict / Lax は要求元のサイトで送るかを決める (指定なしと None は今までどおり送る)
 *  - 期限付きの Cookie は exe と同じフォルダの cookies.txt (Netscape 形式) に保存する
 *  - 複数のスレッドから呼んでよい
 *  - 複数の MiniBrow のプロセスが動いている間は、一時フォルダの minibrow.ck を通して共有する
 */
#ifndef COOKIE_H
#define COOKIE_H

/* file = 保存先 (NULL なら保存しない)。呼ばなければ Cookie は使わない */
void  CookieInit(const char *file);
/* Cookie の Domain= の判定に使う一覧 (psl.bin) の場所。CookieInit の後に呼ぶ。ファイルが無ければ簡易判定 */
void  CookiePslFile(const char *path);
/* 変更があれば保存する */
void  CookieSave(void);
/* 動いている MiniBrow の数 (共有できなければ 1) */
int   CookieProcs(void);
/* まだほかの MiniBrow に渡していない変更を渡す (新しい MiniBrow が起動したとき) */
void  CookieShare(void);
/* 終了時 (CookieSave の後) */
void  CookieEnd(void);
/* 要求の文脈 (SameSite の判定に使う) */
#define COOKIE_SAME   0                 /* 同じサイトからの要求、利用者の操作 */
#define COOKIE_LAX    1                 /* 別のサイトからのページの移動 (GET など安全なメソッド) */
#define COOKIE_CROSS  2                 /* そのほかの別のサイトからの要求 */
/* ホスト a と b が同じサイト (登録可能なドメインが同じ) か */
int   CookieSameSite(const char *a, const char *b);
/* 応答の Set-Cookie: の値 (hdr から len バイト) を取り込む。secure = https で受け取った。
   fromScript = document.cookie への代入、ctx = COOKIE_xxx */
void  CookieSet(const char *host, const char *path, int secure, const char *hdr, int len, int fromScript, int ctx);
/* 送る Cookie: の値を返す (free で解放)。送るものが無ければ NULL。forScript = document.cookie 用、ctx = COOKIE_xxx */
char *CookieGet(const char *host, const char *path, int secure, int forScript, int ctx);

#endif
