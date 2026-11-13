/*
 * mbrsa.c - RSA の公開鍵の計算 (署名の検証)。BearSSL 版で br_rsa_i31_pkcs1_vrfy の代わりに使う
 *
 * 計算の時間が値によって変わる (最後の引き算や繰り上がりの回数)。入る値がすべて公開の値の署名の検証だけに使い、
 * 秘密の値を暗号化する RSA の鍵の交換 (br_ssl_client_set_rsapub) には使わない (BearSSL の時間が変わらない i31 のまま)
 *
 * BearSSL の i31 は 31 ビットずつで計算し、秘密の値にも使えるように指数の全ビットで同じ計算をする。
 * 公開鍵の計算では値も指数も秘密ではないので、次のようにして速くした (P100 で RSA-4096 の 1 回が約 400 ms だった):
 *  - 32 ビットずつで計算し、足し込み付きの掛け算 (MbrAddMul) は x86 の MUL と ADC で書く
 *  - 指数 (65537 など) の 0 のビットでは掛け算をしない
 *  - Montgomery の形にするための R^2 mod m を鍵ごとに覚えて使い回す (ルート証明書・中間証明書の鍵は毎回同じ)
 * 結果は BearSSL と同じ (timing\bench\bhs.exe -check で br_rsa_i31_public と比べる)。
 */
#include <windows.h>
#include <string.h>
#include "bearssl.h"
#include "mbrsa.h"

#define MBR_BITS  4096                          /* 鍵の大きさの上限 (BearSSL の BR_MAX_RSA_SIZE と同じ) */
#define MBR_MAXW  (MBR_BITS / 32)               /* 語の数の上限 (128 語) */
#define MBR_CACHE 8                             /* R^2 mod m を覚える鍵の数 */

typedef struct {
    int           nlen;                         /* 0 = 空き */
    unsigned char n[MBR_BITS / 8];
    uint32_t      r2[MBR_MAXW];
    DWORD         used;
} MbrEnt;

static MbrEnt           g_mbr[MBR_CACHE];
static CRITICAL_SECTION g_mbrCs;
static int              g_mbrReady;

uint32_t br_rsa_pkcs1_sig_unpad(const unsigned char *sig, size_t sig_len,
    const unsigned char *hash_oid, size_t hash_len, unsigned char *hash_out);

/* 起動時に 1 回 (TLS を使う前に) */
void MbRsaInit(void)
{
    InitializeCriticalSection(&g_mbrCs);
    g_mbrReady = 1;
}

/* rp[0..n-1] += up[0..n-1] * v。戻り値は最上位からの繰り上がり。n は 1 以上 */
static uint32_t MbrAddMul(uint32_t *rp, const uint32_t *up, int n, uint32_t v)
{
    uint32_t c = 0;                             /* _asm の中で入れる (入れないと警告になる) */

    _asm {
        mov     esi, up
        mov     edi, rp
        mov     ecx, n
        mov     ebx, v
        push    ebp
        xor     ebp, ebp
    addmul_loop:
        mov     eax, [esi]
        mul     ebx
        add     eax, ebp
        adc     edx, 0
        add     eax, [edi]
        adc     edx, 0
        mov     [edi], eax
        mov     ebp, edx
        add     esi, 4
        add     edi, 4
        dec     ecx
        jnz     addmul_loop
        mov     eax, ebp
        pop     ebp
        mov     c, eax
    }
    return c;
}

/* a と b (n 語) の大小 (a >= b なら 1) */
static int MbrGe(const uint32_t *a, const uint32_t *b, int n)
{
    while (--n >= 0) {
        if (a[n] != b[n]) {
            return a[n] > b[n];
        }
    }
    return 1;
}

/* a -= b (n 語)。戻り値は借り */
static uint32_t MbrSub(uint32_t *a, const uint32_t *b, int n)
{
    uint32_t br = 0;
    int      i;
    for (i = 0; i < n; i++) {
        uint32_t x = a[i], y = b[i] + br;
        br   = (y < br) | (x < y);
        a[i] = x - y;
    }
    return br;
}

/* t[0..2n-1] += a[i]^2 を t[2i..2i+1] に (i = 0..n-1)。繰り上がりは次の語へ送る (全体は 2n 語に収まる) */
static void MbrAddSquares(uint32_t *t, const uint32_t *a, int n)
{
    _asm {
        mov     esi, a
        mov     edi, t
        mov     ecx, n
        push    ebp
        xor     ebp, ebp
    addsq_loop:
        mov     eax, [esi]
        mul     eax
        add     eax, ebp
        adc     edx, 0
        add     [edi], eax
        adc     [edi+4], edx
        mov     ebp, 0
        adc     ebp, 0
        add     esi, 4
        add     edi, 8
        dec     ecx
        jnz     addsq_loop
        pop     ebp
    }
}

static void MbrRedc(uint32_t *d, uint32_t *t, const uint32_t *m, int n, uint32_t m0i);

/* d = a * a / R mod m (Montgomery の 2 乗)。a[i]a[j] (i < j) を 1 回ずつ計算して 2 倍し、a[i]^2 を足す。d は a と同じでもよい */
static void MbrMontSqr(uint32_t *d, const uint32_t *a, const uint32_t *m, int n, uint32_t m0i)
{
    uint32_t t[2 * MBR_MAXW + 2];
    int      i;

    memset(t, 0, (2 * n + 2) * sizeof(uint32_t));
    for (i = 0; i < n - 1; i++) {
        t[i + n] = MbrAddMul(t + 2 * i + 1, a + i + 1, n - 1 - i, a[i]);
    }
    for (i = 2 * n - 1; i > 0; i--) {
        t[i] = (t[i] << 1) | (t[i - 1] >> 31);
    }
    t[0] <<= 1;
    MbrAddSquares(t, a, n);
    MbrRedc(d, t, m, n, m0i);
}

/* d = a * b / R mod m (Montgomery の掛け算。R = 2^(32n)、m0i = -1/m mod 2^32)。d は a・b と同じでもよい */
static void MbrMontMul(uint32_t *d, const uint32_t *a, const uint32_t *b, const uint32_t *m, int n, uint32_t m0i)
{
    uint32_t t[2 * MBR_MAXW + 2];
    int      i;

    memset(t, 0, (2 * n + 2) * sizeof(uint32_t));
    for (i = 0; i < n; i++) {
        t[i + n] = MbrAddMul(t + i, b, n, a[i]);
    }
    MbrRedc(d, t, m, n, m0i);
}

/* d = t / R mod m (t は 2n 語の積。t[2n] と t[2n+1] は 0。t を書き換える) */
static void MbrRedc(uint32_t *d, uint32_t *t, const uint32_t *m, int n, uint32_t m0i)
{
    int i, k;

    for (i = 0; i < n; i++) {
        uint32_t c = MbrAddMul(t + i, m, n, t[i] * m0i);
        for (k = i + n; c != 0; k++) {
            t[k] += c;
            c = t[k] < c;
        }
    }
    /* 結果は t[n..2n] (2m より小さい)。m 以上なら m を引く */
    if (t[2 * n] != 0 || MbrGe(t + n, m, n)) {
        MbrSub(t + n, m, n);
    }
    memcpy(d, t + n, n * sizeof(uint32_t));
}

/* a = 2a mod m (a < m) */
static void MbrDouble(uint32_t *a, const uint32_t *m, int n)
{
    uint32_t top = a[n - 1] >> 31;
    int      i;
    for (i = n - 1; i > 0; i--) {
        a[i] = (a[i] << 1) | (a[i - 1] >> 31);
    }
    a[0] <<= 1;
    if (top || MbrGe(a, m, n)) {
        MbrSub(a, m, n);
    }
}

/* r2 = R^2 mod m。2^(m のビット数 - 1) から倍にして R mod m を作り、
   「2^e R」の形の値の 2 乗 (e が倍) と倍 (e + 1) で e = 32n にする */
static void MbrR2(uint32_t *r2, const uint32_t *m, int n, uint32_t m0i)
{
    int      bits = 32 * n, i, top;
    uint32_t w = m[n - 1];

    while ((w & 0x80000000UL) == 0) {
        w <<= 1;
        bits--;
    }
    memset(r2, 0, n * sizeof(uint32_t));
    r2[(bits - 1) >> 5] = (uint32_t)1 << ((bits - 1) & 31);    /* 2^(ビット数 - 1) < m */
    for (i = bits - 1; i < 32 * n; i++) {
        MbrDouble(r2, m, n);                                    /* R mod m (e = 0) */
    }
    MbrDouble(r2, m, n);                                        /* e = 1 */
    for (top = 30; top >= 0 && ((32 * n) >> top) == 0; top--) {
    }
    for (top--; top >= 0; top--) {
        MbrMontSqr(r2, r2, m, n, m0i);                          /* e を倍に */
        if (((32 * n) >> top) & 1) {
            MbrDouble(r2, m, n);                                /* e + 1 */
        }
    }
}

/* key (nlen バイト) の R^2 mod m を覚えていれば r2 に写して 1 */
static int MbrCacheGet(const unsigned char *key, int nlen, uint32_t *r2, int n)
{
    int i, hit = 0;
    if (!g_mbrReady) {
        return 0;
    }
    EnterCriticalSection(&g_mbrCs);
    for (i = 0; i < MBR_CACHE; i++) {
        if (g_mbr[i].nlen == nlen && memcmp(g_mbr[i].n, key, nlen) == 0) {
            memcpy(r2, g_mbr[i].r2, n * sizeof(uint32_t));
            g_mbr[i].used = GetTickCount();
            hit = 1;
            break;
        }
    }
    LeaveCriticalSection(&g_mbrCs);
    return hit;
}

static void MbrCachePut(const unsigned char *key, int nlen, const uint32_t *r2, int n)
{
    int i, k = 0;
    if (!g_mbrReady) {
        return;
    }
    EnterCriticalSection(&g_mbrCs);
    for (i = 0; i < MBR_CACHE; i++) {
        if (g_mbr[i].nlen == 0) {
            k = i;
            break;
        }
        if ((long)(g_mbr[i].used - g_mbr[k].used) < 0) {
            k = i;                              /* 一番長く使っていないもの */
        }
    }
    g_mbr[k].nlen = nlen;
    memcpy(g_mbr[k].n, key, nlen);
    memcpy(g_mbr[k].r2, r2, n * sizeof(uint32_t));
    g_mbr[k].used = GetTickCount();
    LeaveCriticalSection(&g_mbrCs);
}

/* 大きい方からのバイト列を語の並び (小さい方から) にする */
static void MbrDecode(uint32_t *w, int n, const unsigned char *p, int len)
{
    int i;
    memset(w, 0, n * sizeof(uint32_t));
    for (i = 0; i < len; i++) {
        w[i >> 2] |= (uint32_t)p[len - 1 - i] << ((i & 3) * 8);
    }
}

/* br_rsa_public と同じ: x (xlen バイト) を x^e mod n にする。成功なら 1 (x が n 以上などなら 0) */
uint32_t MbRsaPublic(unsigned char *x, size_t xlen, const br_rsa_public_key *pk)
{
    uint32_t             m[MBR_MAXW], a[MBR_MAXW], xr[MBR_MAXW], acc[MBR_MAXW], r2[MBR_MAXW], one[MBR_MAXW];
    uint32_t             m0i, y;
    const unsigned char *np = pk->n, *ep = pk->e;
    size_t               nlen = pk->nlen, elen = pk->elen, i;
    int                  n, bit, started = 0;

    while (nlen > 0 && *np == 0) {
        np++;
        nlen--;
    }
    while (elen > 0 && *ep == 0) {
        ep++;
        elen--;
    }
    if (nlen == 0 || nlen > MBR_BITS / 8 || xlen != nlen || elen == 0 || (np[nlen - 1] & 1) == 0) {
        return 0;
    }
    n = (int)((nlen + 3) / 4);
    MbrDecode(m, n, np, (int)nlen);
    MbrDecode(a, n, x, (int)xlen);
    if (MbrGe(a, m, n)) {
        return 0;                               /* x >= n */
    }
    y = m[0];                                   /* 1/m mod 2^32 (ニュートン法。奇数なら最初から 3 ビット合っている) */
    y *= 2 - m[0] * y;
    y *= 2 - m[0] * y;
    y *= 2 - m[0] * y;
    y *= 2 - m[0] * y;
    m0i = 0 - y;
    if (!MbrCacheGet(np, (int)nlen, r2, n)) {
        MbrR2(r2, m, n, m0i);
        MbrCachePut(np, (int)nlen, r2, n);
    }
    MbrMontMul(xr, a, r2, m, n, m0i);           /* x R mod m */
    for (i = 0; i < elen; i++) {
        for (bit = 7; bit >= 0; bit--) {
            int set = (ep[i] >> bit) & 1;
            if (!started) {
                if (set) {
                    memcpy(acc, xr, n * sizeof(uint32_t));
                    started = 1;
                }
                continue;
            }
            MbrMontSqr(acc, acc, m, n, m0i);
            if (set) {
                MbrMontMul(acc, acc, xr, m, n, m0i);
            }
        }
    }
    memset(one, 0, n * sizeof(uint32_t));
    one[0] = 1;
    MbrMontMul(acc, acc, one, m, n, m0i);       /* Montgomery の形から戻す */
    for (i = 0; i < xlen; i++) {
        x[xlen - 1 - i] = (unsigned char)(acc[i >> 2] >> ((i & 3) * 8));
    }
    return 1;
}

/* br_rsa_pkcs1_vrfy と同じ (PKCS#1 v1.5 の署名の検証) */
uint32_t MbRsaPkcs1Vrfy(const unsigned char *x, size_t xlen, const unsigned char *hash_oid, size_t hash_len,
                        const br_rsa_public_key *pk, unsigned char *hash_out)
{
    unsigned char sig[MBR_BITS / 8];

    if (xlen > sizeof(sig)) {
        return 0;
    }
    memcpy(sig, x, xlen);
    if (!MbRsaPublic(sig, xlen, pk)) {
        return 0;
    }
    return br_rsa_pkcs1_sig_unpad(sig, xlen, hash_oid, hash_len, hash_out);
}
