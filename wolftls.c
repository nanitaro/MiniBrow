/*
 * wolftls.c - wolfSSL による TLS の接続 (USE_WOLFSSL=1 の研究用のビルドだけ。配布しない)
 *
 * wolfSSL は GPLv3 なので、これを組み込んだ exe は他人に渡さない。
 * ソケットは MiniBrow の WinSock 1.1 のものを I/O のコールバックで使う (winsock2 / ws2_32 は使わない)。
 */
#include <windows.h>
#include <winsock.h>
#include <stdlib.h>
#include <string.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/error-ssl.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/random.h>
#include "wolftls.h"

struct WtConn {
    WOLFSSL *ssl;
    SOCKET  *sock;
    int      hs;                        /* TLS の接続処理を終えた */
    int      err;                       /* 最後の wolfSSL のエラー番号 */
};

static WOLFSSL_CTX *g_ctx;

/* 希望する暗号方式の順番。AES の専用の命令の無い CPU (Pentium など) で速いものから並べる。
   P100 で測った受信の速さ: ChaCha20-Poly1305 336 KB/s、AES-GCM 約 90 KB/s (GHASH が遅い。AES-128 と 256 でほぼ同じ)、
   AES-CBC + HMAC-SHA1 約 380 KB/s。CBC は速いがパディングを突く攻撃の歴史があるので最後 (古いサーバ向け)。
   ChaCha20 の草案版 (-OLD) は入れない。サーバが自分の順番で選ぶ場合は、この順番は効かない */
#define WT_CIPHERS \
    "TLS13-CHACHA20-POLY1305-SHA256:TLS13-AES128-GCM-SHA256:TLS13-AES256-GCM-SHA384:" \
    "ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:" \
    "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:" \
    "ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:" \
    "ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA256:" \
    "ECDHE-ECDSA-AES256-SHA:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA384"

static int CbRecv(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    int n = recv(*(SOCKET *)ctx, buf, sz, 0);
    (void)ssl;
    if (n == 0) {
        return WOLFSSL_CBIO_ERR_CONN_CLOSE;
    }
    if (n < 0) {
        return WSAGetLastError() == WSAECONNRESET ? WOLFSSL_CBIO_ERR_CONN_RST : WOLFSSL_CBIO_ERR_GENERAL;
    }
    return n;
}

static int CbSend(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    int n = send(*(SOCKET *)ctx, buf, sz, 0);
    (void)ssl;
    return n > 0 ? n : WOLFSSL_CBIO_ERR_GENERAL;
}

/* VerifyCert=0: 発行元を信頼できないことだけを許す (有効期限とホスト名の検査は残す) */
static int Lenient(int ok, WOLFSSL_X509_STORE_CTX *st)
{
    if (ok) {
        return 1;
    }
    return st != NULL && (st->error == ASN_NO_SIGNER_E || st->error == ASN_SELF_SIGNED_E);
}

void WtInit(void)
{
    wolfSSL_Init();
}


/* 連結した DER の証明書を 1 枚ずつ c に入れ、入った数を返す (規格に合わない証明書は読み飛ばして残りを使う)。
   PEM の展開は minibrow.c の表引き (PemForEachCert) で済ませてある。1 枚の長さは先頭の SEQUENCE の長さの欄で知る */
static int LoadBlocks(WOLFSSL_CTX *c, const unsigned char *der, long len)
{
    const unsigned char *p = der, *end = der + len;
    int                  n = 0;

    while (end - p >= 2 && p[0] == 0x30) {
        long body, hdr;
        if (p[1] < 0x80) {
            hdr  = 2;
            body = p[1];
        } else if (p[1] >= 0x81 && p[1] <= 0x83 && end - p >= 2 + (p[1] & 0x7F)) {
            int k;
            hdr  = 2 + (p[1] & 0x7F);
            body = 0;
            for (k = 2; k < hdr; k++) {
                body = (body << 8) | p[k];
            }
        } else {
            break;
        }
        if (body > end - p - hdr) {
            break;
        }
        if (wolfSSL_CTX_load_verify_buffer(c, p, hdr + body, WOLFSSL_FILETYPE_ASN1) == WOLFSSL_SUCCESS) {
            n++;
        }
        p += hdr + body;
    }
    return n;
}

int WtLoadCa(const unsigned char *der, long len, int oldTls)
{
    WOLFSSL_CTX *c = wolfSSL_CTX_new(wolfSSLv23_client_method());
    int          n = 0;

    if (c == NULL) {
        return 0;
    }
    wolfSSL_CTX_SetMinVersion(c, oldTls ? WOLFSSL_TLSV1 : WOLFSSL_TLSV1_2);
    wolfSSL_CTX_set_cipher_list(c, WT_CIPHERS);
    if (der != NULL && len > 0) {
        n = LoadBlocks(c, der, len);
    }
    wolfSSL_CTX_SetIORecv(c, CbRecv);
    wolfSSL_CTX_SetIOSend(c, CbSend);
    if (g_ctx != NULL) {
        wolfSSL_CTX_free(g_ctx);        /* 使用中の接続が持つ参照は、その接続を閉じるまで残る */
    }
    g_ctx = c;
    return n;
}

int WtCountCa(const unsigned char *der, long len)
{
    WOLFSSL_CTX *c = wolfSSL_CTX_new(wolfSSLv23_client_method());
    int          n;

    if (c == NULL) {
        return 0;
    }
    n = der != NULL && len > 0 ? LoadBlocks(c, der, len) : 0;
    wolfSSL_CTX_free(c);
    return n;
}

void WtSha256(const void *data, unsigned long len, unsigned char *out)
{
    wc_Sha256 h;

    wc_InitSha256(&h);
    wc_Sha256Update(&h, (const byte *)data, (word32)len);
    wc_Sha256Final(&h, out);
    wc_Sha256Free(&h);
}

int WtRandom(unsigned char *out, unsigned long len, int reseed)
{
    static WC_RNG rng;
    static int    ready;

    if (reseed && ready) {
        wc_FreeRng(&rng);               /* 操作のタイミングがたまった: 種を取り直して作り直す */
        ready = 0;
    }
    if (!ready) {
        if (wc_InitRng(&rng) != 0) {
            return 0;
        }
        ready = 1;
    }
    return wc_RNG_GenerateBlock(&rng, out, (word32)len) == 0;
}

int WtReady(void)
{
    return g_ctx != NULL;
}

void WtReset(void)
{
    if (g_ctx != NULL) {
        wolfSSL_CTX_free(g_ctx);
        g_ctx = NULL;
    }
}

/* host が IP アドレスか (SNI には入れない) */
static int IsIpHost(const char *h)
{
    return strspn(h, "0123456789.") == strlen(h) || strchr(h, ':') != NULL;
}

WtConn *WtOpen(unsigned int *sock, const char *sni, int verify, void *sess)
{
    WtConn  *c;
    WOLFSSL *ssl;

    if (g_ctx == NULL || (ssl = wolfSSL_new(g_ctx)) == NULL) {
        return NULL;
    }
    c = (WtConn *)calloc(1, sizeof(*c));
    if (c == NULL) {
        wolfSSL_free(ssl);
        return NULL;
    }
    c->ssl  = ssl;
    c->sock = (SOCKET *)sock;
    wolfSSL_SetIOReadCtx(ssl, c->sock);
    wolfSSL_SetIOWriteCtx(ssl, c->sock);
    if (!IsIpHost(sni)) {
        wolfSSL_UseSNI(ssl, WOLFSSL_SNI_HOST_NAME, sni, (unsigned short)strlen(sni));
    }
    wolfSSL_check_domain_name(ssl, sni);
    wolfSSL_set_verify(ssl, WOLFSSL_VERIFY_PEER, verify ? NULL : Lenient);
    wolfSSL_UseSessionTicket(ssl);      /* TLS 1.2 でもチケットを求める (TLS 1.3 は常にチケット) */
    if (sess != NULL) {
        wolfSSL_set_session(ssl, (WOLFSSL_SESSION *)sess);     /* 期限切れなら wolfSSL が使わない */
    }
    return c;
}

void *WtGetSession(WtConn *c)
{
    return c != NULL && c->hs ? (void *)wolfSSL_get1_session(c->ssl) : NULL;
}

void WtFreeSession(void *sess)
{
    if (sess != NULL) {
        wolfSSL_SESSION_free((WOLFSSL_SESSION *)sess);
    }
}

int WtResumed(WtConn *c)
{
    return c != NULL && wolfSSL_session_reused(c->ssl);
}

int WtSend(WtConn *c, const char *p, int n)
{
    if (c == NULL) {
        return 0;
    }
    if (!c->hs) {
        int r = wolfSSL_connect(c->ssl);
        if (r != WOLFSSL_SUCCESS) {
            c->err = wolfSSL_get_error(c->ssl, r);
            return 0;
        }
        c->hs = 1;
    }
    while (n > 0) {
        int k = wolfSSL_write(c->ssl, p, n);
        if (k <= 0) {
            c->err = wolfSSL_get_error(c->ssl, k);
            return 0;
        }
        p += k;
        n -= k;
    }
    return 1;
}

int WtRecv(WtConn *c, char *buf, int len)
{
    int n, e;

    if (c == NULL) {
        return -1;
    }
    n = wolfSSL_read(c->ssl, buf, len);
    if (n > 0) {
        return n;
    }
    e = wolfSSL_get_error(c->ssl, n);
    /* close_notify を送らずに切断するサーバが多いので、切断も終端とみなす */
    if (e == WOLFSSL_ERROR_ZERO_RETURN || e == SOCKET_PEER_CLOSED_E || e == SOCKET_ERROR_E) {
        return 0;
    }
    c->err = e;
    return -1;
}

/* 受け取ったまま読んでいないデータがあるか (復号したものと、まだ処理していない受信バッファの中のもの) */
int WtPending(WtConn *c)
{
    return c != NULL && (wolfSSL_pending(c->ssl) > 0 || wolfSSL_has_pending(c->ssl));
}

int WtError(WtConn *c, int *raw)
{
    int e = c != NULL ? c->err : 0;

    *raw = e;
    switch (e) {
    case 0:
        return WT_NONE;
    case ASN_NO_SIGNER_E:
    case ASN_SELF_SIGNED_E:
        return WT_UNTRUSTED;
    case ASN_AFTER_DATE_E:
    case ASN_BEFORE_DATE_E:
        return WT_EXPIRED;
    case DOMAIN_NAME_MISMATCH:
    case IPADDR_MISMATCH:
        return WT_NAME;
    case VERSION_ERROR:
        return WT_VERSION;
    case FATAL_ERROR: {
        WOLFSSL_ALERT_HISTORY h;
        memset(&h, 0, sizeof(h));
        wolfSSL_get_alert_history(c->ssl, &h);
        *raw = h.last_rx.code;
        return WT_ALERT;
    }
    case SOCKET_ERROR_E:
    case SOCKET_PEER_CLOSED_E:
        return WT_IO;
    default:
        return WT_OTHER;
    }
}

void WtClose(WtConn *c)
{
    if (c != NULL) {
        wolfSSL_free(c->ssl);
        free(c);
    }
}
