/* wolfSSL settings for the MiniBrow research build (USE_WOLFSSL=1, never distributed) (TLS 1.2 / 1.3 client only, C code only, Win95) */
#ifndef MINIBROW_WOLF_USER_SETTINGS_H
#define MINIBROW_WOLF_USER_SETTINGS_H

/* I/O: MiniBrow passes its own WinSock 1.1 socket through callbacks (no winsock2 / ws2_32) */
#define WOLFSSL_USER_IO
#define NO_WRITEV

/* client only */
#define NO_WOLFSSL_SERVER

/* protocols */
#define WOLFSSL_TLS13
#define HAVE_TLS_EXTENSIONS
#define HAVE_SUPPORTED_CURVES
#define HAVE_SNI
#define HAVE_EXTENDED_MASTER
#define HAVE_HKDF
#define WOLFSSL_ALT_CERT_CHAINS
/* TLS 1.0 / 1.1 are built in; MiniBrow allows them only with OldTls=1 (minimum version set at run time) */
#define WOLFSSL_ALLOW_TLSV10

/* algorithms */
#define HAVE_ECC
#define ECC_TIMING_RESISTANT
#define HAVE_CURVE25519
#define WC_RSA_PSS
#define WC_RSA_BLINDING
#define HAVE_AESGCM
#define GCM_TABLE_4BIT                  /* GHASH with a 4-bit table per key (without it: bit by bit, 128 steps per block) */
#define HAVE_CHACHA
#define HAVE_POLY1305
#define WOLFSSL_SHA384
#define WOLFSSL_SHA512

/* not used */
#define NO_DSA
#define NO_DH
#define NO_RC4
#define NO_MD4
#define NO_DES3
#define NO_PSK
#define NO_PWDBASED
#define NO_FILESYSTEM
#define NO_WOLFSSL_DIR
#define HAVE_SESSION_TICKET             /* TLS 1.2 のチケットと TLS 1.3 の再開 (PSK) */

/* portable C, small stack (fetch threads have 128 KB stacks) */
#define WOLFSSL_NO_ASM
#define TFM_NO_ASM
#define WOLFSSL_SMALL_STACK

/* big numbers up to 4096 bits (RSA 4096 root certificates such as ISRG Root X1) */
#define SP_INT_BITS 4096

/* fixed-size C code (sp_c32.c) for RSA 2048 / 3072 / 4096 and ECC P-256; other sizes use sp_int.c */
#define WOLFSSL_HAVE_SP_RSA
#define WOLFSSL_HAVE_SP_ECC
#define WOLFSSL_SP_4096

/* random seed comes from MiniBrow (CryptoAPI loaded dynamically, else timers; never a static advapi32 import) */
extern int MbWolfSeed(unsigned char *out, unsigned int size);
#define CUSTOM_RAND_GENERATE_SEED MbWolfSeed

/* Open Watcom: rotations as one ROL / ROR (misc.c uses _lrotl / _lrotr with #pragma intrinsic).
   wolfssl.mak compiles with -oe so that the small inline helpers are really inlined */
#if defined(__WATCOMC__)
#define INTEL_INTRINSICS
#endif

/* time: wolfSSL calls XTIME / XGMTIME in wolfcfg\wolftime.c instead of the C library.
   Open Watcom's time() / gmtime() run tzset() -> getenv("TZ") (a case-insensitive multi-byte scan of the
   environment) on every call; wolfSSL asks twice per certificate, and that was more than half of the
   time to load cacert.pem */
#define TIME_OVERRIDES
#define HAVE_TIME_T_TYPE
#define HAVE_TM_TYPE
#include <time.h>

#include <stdint.h>
#endif
