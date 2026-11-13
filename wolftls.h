/*
 * wolftls.h - wolfSSL による TLS の接続 (USE_WOLFSSL=1 の研究用のビルドだけ。配布しない)
 *
 *  - minibrow.c の Conn から呼ぶ。wolfSSL のヘッダは wolftls.c の中だけで読む (本体の C89 と型がぶつからないように)
 *  - WtLoadCa / WtReset / WtOpen は minibrow.c の g_csTA の中で呼ぶ (CTX の差し替えと参照を守る)
 */
#ifndef WOLFTLS_H
#define WOLFTLS_H

typedef struct WtConn WtConn;

/* 失敗の分類 (WtError) */
#define WT_NONE      0
#define WT_UNTRUSTED 1                  /* 発行元を信頼できない */
#define WT_EXPIRED   2                  /* 有効期限の外 */
#define WT_NAME      3                  /* ホスト名が一致しない */
#define WT_VERSION   4                  /* TLS のバージョンが合わない */
#define WT_ALERT     5                  /* サーバがアラートで拒否した (raw = アラートの番号) */
#define WT_IO        6                  /* 通信が切れた */
#define WT_OTHER     7                  /* そのほか (raw = wolfSSL のエラー番号) */

/* 起動時に 1 回 */
void    WtInit(void);
/* 信頼するルート証明書 (DER の証明書を連結したもの。PEM は minibrow.c の PemForEachCert で戻す) と
   最低のバージョン (oldTls = TLS 1.0 / 1.1 も許す) で CTX を作り直す。読めた証明書の数を返す */
int     WtLoadCa(const unsigned char *der, long len, int oldTls);
/* 連結した DER の証明書のうち、読める証明書の数 (cacert.pem を差し替える前の確かめ) */
int     WtCountCa(const unsigned char *der, long len);
/* SHA-256 (out は 32 バイト) */
void    WtSha256(const void *data, unsigned long len, unsigned char *out);
/* crypto.getRandomValues の乱数 (wolfCrypt の Hash_DRBG)。reseed なら種を取り直す。成功なら 1 */
int     WtRandom(unsigned char *out, unsigned long len, int reseed);
/* CTX があるか / 捨てる (次の WtOpen の前に作り直させる) */
int     WtReady(void);
void    WtReset(void);
/* *sock (接続済みのソケット) の上で sni のサーバと TLS を始める用意をする。verify = 0 なら発行元の検証だけ外す。
   sess (WtGetSession で取ったもの。NULL 可) があれば、それで再開を試みる */
WtConn *WtOpen(unsigned int *sock, const char *sni, int verify, void *sess);
/* 次の再開に使うセッション (WtFreeSession で解放)。接続処理を終えていなければ NULL */
void   *WtGetSession(WtConn *c);
void    WtFreeSession(void *sess);
/* この接続が覚えていたセッションで再開したか */
int     WtResumed(WtConn *c);
/* 送信 (最初の呼び出しで TLS の接続処理をする)。成功なら 1 */
int     WtSend(WtConn *c, const char *p, int n);
/* 受信。終端なら 0、エラーなら -1 */
int     WtRecv(WtConn *c, char *buf, int len);
/* まだ読んでいない受信データがあるか (パイプラインで次の応答を待つ前に確かめる) */
int     WtPending(WtConn *c);
/* 最後の失敗の分類 (WT_xxx)。raw にはアラートの番号か wolfSSL のエラー番号 */
int     WtError(WtConn *c, int *raw);
void    WtClose(WtConn *c);

/* wolfCrypt の乱数の種 (minibrow.c の GatherEntropy で作る。user_settings.h の CUSTOM_RAND_GENERATE_SEED) */
int     MbWolfSeed(unsigned char *out, unsigned int size);

#endif
