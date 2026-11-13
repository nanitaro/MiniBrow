# wolfSSL 5.8.4 for Open Watcom (research build only: USE_WOLFSSL=1. wolfSSL is GPLv3 - never distribute the exe)
# The source is in wolfssl\ (not tracked), the settings in wolfcfg\user_settings.h.

# -oe=40: inline small functions (Watcom does not inline the WC_INLINE helpers such as rotlFixed without it)
WOLFFLAGS = -q -bt=nt $(MBOPT) -oe=40 -bm -w0 -za99 -DWOLFSSL_USER_SETTINGS -iwolfcfg -iwolfssl
# poly1305.c is about 20% faster without -oe (register pressure in the patched 32-bit block loop)
WOLFFLAGS_NOINL = -q -bt=nt $(MBOPT) -bm -w0 -za99 -DWOLFSSL_USER_SETTINGS -iwolfcfg -iwolfssl

WOLF_OBJS = &
    wobj\internal.obj &
    wobj\keys.obj &
    wobj\ssl.obj &
    wobj\tls.obj &
    wobj\tls13.obj &
    wobj\wolfio.obj &
    wobj\aes.obj &
    wobj\asn.obj &
    wobj\chacha.obj &
    wobj\chacha20_poly1305.obj &
    wobj\poly1305.obj &
    wobj\curve25519.obj &
    wobj\fe_operations.obj &
    wobj\ecc.obj &
    wobj\hash.obj &
    wobj\hmac.obj &
    wobj\kdf.obj &
    wobj\memory.obj &
    wobj\random.obj &
    wobj\rsa.obj &
    wobj\sha.obj &
    wobj\sha256.obj &
    wobj\sha512.obj &
    wobj\md5.obj &
    wobj\sp_int.obj &
    wobj\sp_c32.obj &
    wobj\wolftime.obj &
    wobj\wolfmath.obj &
    wobj\wc_port.obj &
    wobj\error.obj &
    wobj\logging.obj &
    wobj\coding.obj &
    wobj\signature.obj &
    wobj\wc_encrypt.obj

wolfssl.lib: $(WOLF_OBJS)
	wlib -q -n -b wolfssl.lib @wolfssl.lbc

wobj\internal.obj: wolfssl\src\internal.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\src\internal.c

wobj\keys.obj: wolfssl\src\keys.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\src\keys.c

wobj\ssl.obj: wolfssl\src\ssl.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\src\ssl.c

wobj\tls.obj: wolfssl\src\tls.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\src\tls.c

wobj\tls13.obj: wolfssl\src\tls13.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\src\tls13.c

wobj\wolfio.obj: wolfssl\src\wolfio.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\src\wolfio.c

wobj\aes.obj: wolfssl\wolfcrypt\src\aes.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\aes.c

wobj\asn.obj: wolfssl\wolfcrypt\src\asn.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\asn.c

wobj\chacha.obj: wolfssl\wolfcrypt\src\chacha.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\chacha.c

wobj\chacha20_poly1305.obj: wolfssl\wolfcrypt\src\chacha20_poly1305.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\chacha20_poly1305.c

wobj\poly1305.obj: wolfssl\wolfcrypt\src\poly1305.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS_NOINL) -fo=$@ wolfssl\wolfcrypt\src\poly1305.c

wobj\curve25519.obj: wolfssl\wolfcrypt\src\curve25519.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\curve25519.c

wobj\fe_operations.obj: wolfssl\wolfcrypt\src\fe_operations.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\fe_operations.c

wobj\ecc.obj: wolfssl\wolfcrypt\src\ecc.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\ecc.c

wobj\hash.obj: wolfssl\wolfcrypt\src\hash.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\hash.c

wobj\hmac.obj: wolfssl\wolfcrypt\src\hmac.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\hmac.c

wobj\kdf.obj: wolfssl\wolfcrypt\src\kdf.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\kdf.c

wobj\memory.obj: wolfssl\wolfcrypt\src\memory.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\memory.c

wobj\random.obj: wolfssl\wolfcrypt\src\random.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\random.c

wobj\rsa.obj: wolfssl\wolfcrypt\src\rsa.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\rsa.c

wobj\sha.obj: wolfssl\wolfcrypt\src\sha.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\sha.c

wobj\sha256.obj: wolfssl\wolfcrypt\src\sha256.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\sha256.c

wobj\sha512.obj: wolfssl\wolfcrypt\src\sha512.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\sha512.c

wobj\md5.obj: wolfssl\wolfcrypt\src\md5.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\md5.c

wobj\sp_int.obj: wolfssl\wolfcrypt\src\sp_int.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\sp_int.c

wobj\wolftime.obj: wolfcfg\wolftime.c
	@if not exist wobj mkdir wobj
	wcc386 -q -bt=nt $(MBOPT) -bm -w4 -za99 -fo=$@ wolfcfg\wolftime.c

wobj\sp_c32.obj: wolfssl\wolfcrypt\src\sp_c32.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\sp_c32.c

wobj\wolfmath.obj: wolfssl\wolfcrypt\src\wolfmath.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\wolfmath.c

wobj\wc_port.obj: wolfssl\wolfcrypt\src\wc_port.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\wc_port.c

wobj\error.obj: wolfssl\wolfcrypt\src\error.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\error.c

wobj\logging.obj: wolfssl\wolfcrypt\src\logging.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\logging.c

wobj\coding.obj: wolfssl\wolfcrypt\src\coding.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\coding.c

wobj\signature.obj: wolfssl\wolfcrypt\src\signature.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\signature.c

wobj\wc_encrypt.obj: wolfssl\wolfcrypt\src\wc_encrypt.c wolfcfg\user_settings.h
	@if not exist wobj mkdir wobj
	wcc386 $(WOLFFLAGS) -fo=$@ wolfssl\wolfcrypt\src\wc_encrypt.c

