# BearSSL 0.6 for Open Watcom (generated list - regenerate if bearssl\src changes)
# Optimization flags come from MBOPT in the main makefile.

BRFLAGS = -q -bt=nt $(MBOPT) -bm -w0 -za99 -DBR_USE_WIN32_RAND=0 -ibearssl\inc -ibearssl\src

BR_OBJS = &
    bobj\ccm.obj &
    bobj\eax.obj &
    bobj\gcm.obj &
    bobj\ccopy.obj &
    bobj\dec16be.obj &
    bobj\dec16le.obj &
    bobj\dec32be.obj &
    bobj\dec32le.obj &
    bobj\dec64be.obj &
    bobj\dec64le.obj &
    bobj\enc16be.obj &
    bobj\enc16le.obj &
    bobj\enc32be.obj &
    bobj\enc32le.obj &
    bobj\enc64be.obj &
    bobj\enc64le.obj &
    bobj\pemdec.obj &
    bobj\pemenc.obj &
    bobj\ec_all_m15.obj &
    bobj\ec_all_m31.obj &
    bobj\ec_c25519_i15.obj &
    bobj\ec_c25519_i31.obj &
    bobj\ec_c25519_m15.obj &
    bobj\ec_c25519_m31.obj &
    bobj\ec_curve25519.obj &
    bobj\ec_default.obj &
    bobj\ec_keygen.obj &
    bobj\ec_p256_m15.obj &
    bobj\ec_p256_m31.obj &
    bobj\ec_prime_i15.obj &
    bobj\ec_prime_i31.obj &
    bobj\ec_pubkey.obj &
    bobj\ec_secp256r1.obj &
    bobj\ec_secp384r1.obj &
    bobj\ec_secp521r1.obj &
    bobj\ecdsa_atr.obj &
    bobj\ecdsa_default_sign_asn1.obj &
    bobj\ecdsa_default_sign_raw.obj &
    bobj\ecdsa_default_vrfy_asn1.obj &
    bobj\ecdsa_default_vrfy_raw.obj &
    bobj\ecdsa_i15_bits.obj &
    bobj\ecdsa_i15_sign_asn1.obj &
    bobj\ecdsa_i15_sign_raw.obj &
    bobj\ecdsa_i15_vrfy_asn1.obj &
    bobj\ecdsa_i15_vrfy_raw.obj &
    bobj\ecdsa_i31_bits.obj &
    bobj\ecdsa_i31_sign_asn1.obj &
    bobj\ecdsa_i31_sign_raw.obj &
    bobj\ecdsa_i31_vrfy_asn1.obj &
    bobj\ecdsa_i31_vrfy_raw.obj &
    bobj\ecdsa_rta.obj &
    bobj\dig_oid.obj &
    bobj\dig_size.obj &
    bobj\ghash_ctmul.obj &
    bobj\ghash_ctmul32.obj &
    bobj\ghash_ctmul64.obj &
    bobj\ghash_pclmul.obj &
    bobj\ghash_pwr8.obj &
    bobj\md5.obj &
    bobj\md5sha1.obj &
    bobj\mgf1.obj &
    bobj\multihash.obj &
    bobj\sha1.obj &
    bobj\sha2big.obj &
    bobj\sha2small.obj &
    bobj\i15_add.obj &
    bobj\i15_bitlen.obj &
    bobj\i15_decmod.obj &
    bobj\i15_decode.obj &
    bobj\i15_decred.obj &
    bobj\i15_encode.obj &
    bobj\i15_fmont.obj &
    bobj\i15_iszero.obj &
    bobj\i15_moddiv.obj &
    bobj\i15_modpow.obj &
    bobj\i15_modpow2.obj &
    bobj\i15_montmul.obj &
    bobj\i15_mulacc.obj &
    bobj\i15_muladd.obj &
    bobj\i15_ninv15.obj &
    bobj\i15_reduce.obj &
    bobj\i15_rshift.obj &
    bobj\i15_sub.obj &
    bobj\i15_tmont.obj &
    bobj\i31_add.obj &
    bobj\i31_bitlen.obj &
    bobj\i31_decmod.obj &
    bobj\i31_decode.obj &
    bobj\i31_decred.obj &
    bobj\i31_encode.obj &
    bobj\i31_fmont.obj &
    bobj\i31_iszero.obj &
    bobj\i31_moddiv.obj &
    bobj\i31_modpow.obj &
    bobj\i31_modpow2.obj &
    bobj\i31_montmul.obj &
    bobj\i31_mulacc.obj &
    bobj\i31_muladd.obj &
    bobj\i31_ninv31.obj &
    bobj\i31_reduce.obj &
    bobj\i31_rshift.obj &
    bobj\i31_sub.obj &
    bobj\i31_tmont.obj &
    bobj\i32_add.obj &
    bobj\i32_bitlen.obj &
    bobj\i32_decmod.obj &
    bobj\i32_decode.obj &
    bobj\i32_decred.obj &
    bobj\i32_div32.obj &
    bobj\i32_encode.obj &
    bobj\i32_fmont.obj &
    bobj\i32_iszero.obj &
    bobj\i32_modpow.obj &
    bobj\i32_montmul.obj &
    bobj\i32_mulacc.obj &
    bobj\i32_muladd.obj &
    bobj\i32_ninv32.obj &
    bobj\i32_reduce.obj &
    bobj\i32_sub.obj &
    bobj\i32_tmont.obj &
    bobj\i62_modpow2.obj &
    bobj\hkdf.obj &
    bobj\hmac.obj &
    bobj\hmac_ct.obj &
    bobj\aesctr_drbg.obj &
    bobj\hmac_drbg.obj &
    bobj\sysrng.obj &
    bobj\rsa_default_keygen.obj &
    bobj\rsa_default_modulus.obj &
    bobj\rsa_default_oaep_decrypt.obj &
    bobj\rsa_default_oaep_encrypt.obj &
    bobj\rsa_default_pkcs1_sign.obj &
    bobj\rsa_default_pkcs1_vrfy.obj &
    bobj\rsa_default_priv.obj &
    bobj\rsa_default_privexp.obj &
    bobj\rsa_default_pub.obj &
    bobj\rsa_default_pubexp.obj &
    bobj\rsa_i15_keygen.obj &
    bobj\rsa_i15_modulus.obj &
    bobj\rsa_i15_oaep_decrypt.obj &
    bobj\rsa_i15_oaep_encrypt.obj &
    bobj\rsa_i15_pkcs1_sign.obj &
    bobj\rsa_i15_pkcs1_vrfy.obj &
    bobj\rsa_i15_priv.obj &
    bobj\rsa_i15_privexp.obj &
    bobj\rsa_i15_pub.obj &
    bobj\rsa_i15_pubexp.obj &
    bobj\rsa_i31_keygen.obj &
    bobj\rsa_i31_keygen_inner.obj &
    bobj\rsa_i31_modulus.obj &
    bobj\rsa_i31_oaep_decrypt.obj &
    bobj\rsa_i31_oaep_encrypt.obj &
    bobj\rsa_i31_pkcs1_sign.obj &
    bobj\rsa_i31_pkcs1_vrfy.obj &
    bobj\rsa_i31_priv.obj &
    bobj\rsa_i31_privexp.obj &
    bobj\rsa_i31_pub.obj &
    bobj\rsa_i31_pubexp.obj &
    bobj\rsa_i32_oaep_decrypt.obj &
    bobj\rsa_i32_oaep_encrypt.obj &
    bobj\rsa_i32_pkcs1_sign.obj &
    bobj\rsa_i32_pkcs1_vrfy.obj &
    bobj\rsa_i32_priv.obj &
    bobj\rsa_i32_pub.obj &
    bobj\rsa_i62_keygen.obj &
    bobj\rsa_i62_oaep_decrypt.obj &
    bobj\rsa_i62_oaep_encrypt.obj &
    bobj\rsa_i62_pkcs1_sign.obj &
    bobj\rsa_i62_pkcs1_vrfy.obj &
    bobj\rsa_i62_priv.obj &
    bobj\rsa_i62_pub.obj &
    bobj\rsa_oaep_pad.obj &
    bobj\rsa_oaep_unpad.obj &
    bobj\rsa_pkcs1_sig_pad.obj &
    bobj\rsa_pkcs1_sig_unpad.obj &
    bobj\rsa_ssl_decrypt.obj &
    bobj\settings.obj &
    bobj\prf.obj &
    bobj\prf_md5sha1.obj &
    bobj\prf_sha256.obj &
    bobj\prf_sha384.obj &
    bobj\ssl_ccert_single_ec.obj &
    bobj\ssl_ccert_single_rsa.obj &
    bobj\ssl_client.obj &
    bobj\ssl_client_default_rsapub.obj &
    bobj\ssl_client_full.obj &
    bobj\ssl_engine.obj &
    bobj\ssl_engine_default_aescbc.obj &
    bobj\ssl_engine_default_aesccm.obj &
    bobj\ssl_engine_default_aesgcm.obj &
    bobj\ssl_engine_default_chapol.obj &
    bobj\ssl_engine_default_descbc.obj &
    bobj\ssl_engine_default_ec.obj &
    bobj\ssl_engine_default_ecdsa.obj &
    bobj\ssl_engine_default_rsavrfy.obj &
    bobj\ssl_hashes.obj &
    bobj\ssl_hs_client.obj &
    bobj\ssl_hs_server.obj &
    bobj\ssl_io.obj &
    bobj\ssl_keyexport.obj &
    bobj\ssl_lru.obj &
    bobj\ssl_rec_cbc.obj &
    bobj\ssl_rec_ccm.obj &
    bobj\ssl_rec_chapol.obj &
    bobj\ssl_rec_gcm.obj &
    bobj\ssl_scert_single_ec.obj &
    bobj\ssl_scert_single_rsa.obj &
    bobj\ssl_server.obj &
    bobj\ssl_server_full_ec.obj &
    bobj\ssl_server_full_rsa.obj &
    bobj\ssl_server_mine2c.obj &
    bobj\ssl_server_mine2g.obj &
    bobj\ssl_server_minf2c.obj &
    bobj\ssl_server_minf2g.obj &
    bobj\ssl_server_minr2g.obj &
    bobj\ssl_server_minu2g.obj &
    bobj\ssl_server_minv2g.obj &
    bobj\aes_big_cbcdec.obj &
    bobj\aes_big_cbcenc.obj &
    bobj\aes_big_ctr.obj &
    bobj\aes_big_ctrcbc.obj &
    bobj\aes_big_dec.obj &
    bobj\aes_big_enc.obj &
    bobj\aes_common.obj &
    bobj\aes_ct.obj &
    bobj\aes_ct_cbcdec.obj &
    bobj\aes_ct_cbcenc.obj &
    bobj\aes_ct_ctr.obj &
    bobj\aes_ct_ctrcbc.obj &
    bobj\aes_ct_dec.obj &
    bobj\aes_ct_enc.obj &
    bobj\aes_ct64.obj &
    bobj\aes_ct64_cbcdec.obj &
    bobj\aes_ct64_cbcenc.obj &
    bobj\aes_ct64_ctr.obj &
    bobj\aes_ct64_ctrcbc.obj &
    bobj\aes_ct64_dec.obj &
    bobj\aes_ct64_enc.obj &
    bobj\aes_pwr8.obj &
    bobj\aes_pwr8_cbcdec.obj &
    bobj\aes_pwr8_cbcenc.obj &
    bobj\aes_pwr8_ctr.obj &
    bobj\aes_pwr8_ctrcbc.obj &
    bobj\aes_small_cbcdec.obj &
    bobj\aes_small_cbcenc.obj &
    bobj\aes_small_ctr.obj &
    bobj\aes_small_ctrcbc.obj &
    bobj\aes_small_dec.obj &
    bobj\aes_small_enc.obj &
    bobj\aes_x86ni.obj &
    bobj\aes_x86ni_cbcdec.obj &
    bobj\aes_x86ni_cbcenc.obj &
    bobj\aes_x86ni_ctr.obj &
    bobj\aes_x86ni_ctrcbc.obj &
    bobj\chacha20_ct.obj &
    bobj\chacha20_sse2.obj &
    bobj\des_ct.obj &
    bobj\des_ct_cbcdec.obj &
    bobj\des_ct_cbcenc.obj &
    bobj\des_support.obj &
    bobj\des_tab.obj &
    bobj\des_tab_cbcdec.obj &
    bobj\des_tab_cbcenc.obj &
    bobj\poly1305_ctmul.obj &
    bobj\poly1305_ctmul32.obj &
    bobj\poly1305_ctmulq.obj &
    bobj\poly1305_i15.obj &
    bobj\asn1enc.obj &
    bobj\encode_ec_pk8der.obj &
    bobj\encode_ec_rawder.obj &
    bobj\encode_rsa_pk8der.obj &
    bobj\encode_rsa_rawder.obj &
    bobj\skey_decoder.obj &
    bobj\x509_decoder.obj &
    bobj\x509_knownkey.obj &
    bobj\x509_minimal.obj &
    bobj\x509_minimal_full.obj

bearssl.lib: $(BR_OBJS)
	wlib -q -n -b bearssl.lib @bearssl.lbc

bobj\ccm.obj: bearssl\src\aead\ccm.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\aead\ccm.c

bobj\eax.obj: bearssl\src\aead\eax.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\aead\eax.c

bobj\gcm.obj: bearssl\src\aead\gcm.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\aead\gcm.c

bobj\ccopy.obj: bearssl\src\codec\ccopy.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\codec\ccopy.c

bobj\dec16be.obj: bearssl\src\codec\dec16be.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\codec\dec16be.c

bobj\dec16le.obj: bearssl\src\codec\dec16le.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\codec\dec16le.c

bobj\dec32be.obj: bearssl\src\codec\dec32be.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\codec\dec32be.c

bobj\dec32le.obj: bearssl\src\codec\dec32le.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\codec\dec32le.c

bobj\dec64be.obj: bearssl\src\codec\dec64be.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\codec\dec64be.c

bobj\dec64le.obj: bearssl\src\codec\dec64le.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\codec\dec64le.c

bobj\enc16be.obj: bearssl\src\codec\enc16be.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\codec\enc16be.c

bobj\enc16le.obj: bearssl\src\codec\enc16le.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\codec\enc16le.c

bobj\enc32be.obj: bearssl\src\codec\enc32be.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\codec\enc32be.c

bobj\enc32le.obj: bearssl\src\codec\enc32le.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\codec\enc32le.c

bobj\enc64be.obj: bearssl\src\codec\enc64be.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\codec\enc64be.c

bobj\enc64le.obj: bearssl\src\codec\enc64le.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\codec\enc64le.c

bobj\pemdec.obj: bearssl\src\codec\pemdec.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\codec\pemdec.c

bobj\pemenc.obj: bearssl\src\codec\pemenc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\codec\pemenc.c

bobj\ec_all_m15.obj: bearssl\src\ec\ec_all_m15.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ec_all_m15.c

bobj\ec_all_m31.obj: bearssl\src\ec\ec_all_m31.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ec_all_m31.c

bobj\ec_c25519_i15.obj: bearssl\src\ec\ec_c25519_i15.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ec_c25519_i15.c

bobj\ec_c25519_i31.obj: bearssl\src\ec\ec_c25519_i31.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ec_c25519_i31.c

bobj\ec_c25519_m15.obj: bearssl\src\ec\ec_c25519_m15.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ec_c25519_m15.c

bobj\ec_c25519_m31.obj: bearssl\src\ec\ec_c25519_m31.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ec_c25519_m31.c

bobj\ec_curve25519.obj: bearssl\src\ec\ec_curve25519.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ec_curve25519.c

bobj\ec_default.obj: bearssl\src\ec\ec_default.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ec_default.c

bobj\ec_keygen.obj: bearssl\src\ec\ec_keygen.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ec_keygen.c

bobj\ec_p256_m15.obj: bearssl\src\ec\ec_p256_m15.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ec_p256_m15.c

bobj\ec_p256_m31.obj: bearssl\src\ec\ec_p256_m31.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ec_p256_m31.c

bobj\ec_prime_i15.obj: bearssl\src\ec\ec_prime_i15.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ec_prime_i15.c

bobj\ec_prime_i31.obj: bearssl\src\ec\ec_prime_i31.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ec_prime_i31.c

bobj\ec_pubkey.obj: bearssl\src\ec\ec_pubkey.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ec_pubkey.c

bobj\ec_secp256r1.obj: bearssl\src\ec\ec_secp256r1.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ec_secp256r1.c

bobj\ec_secp384r1.obj: bearssl\src\ec\ec_secp384r1.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ec_secp384r1.c

bobj\ec_secp521r1.obj: bearssl\src\ec\ec_secp521r1.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ec_secp521r1.c

bobj\ecdsa_atr.obj: bearssl\src\ec\ecdsa_atr.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ecdsa_atr.c

bobj\ecdsa_default_sign_asn1.obj: bearssl\src\ec\ecdsa_default_sign_asn1.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ecdsa_default_sign_asn1.c

bobj\ecdsa_default_sign_raw.obj: bearssl\src\ec\ecdsa_default_sign_raw.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ecdsa_default_sign_raw.c

bobj\ecdsa_default_vrfy_asn1.obj: bearssl\src\ec\ecdsa_default_vrfy_asn1.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ecdsa_default_vrfy_asn1.c

bobj\ecdsa_default_vrfy_raw.obj: bearssl\src\ec\ecdsa_default_vrfy_raw.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ecdsa_default_vrfy_raw.c

bobj\ecdsa_i15_bits.obj: bearssl\src\ec\ecdsa_i15_bits.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ecdsa_i15_bits.c

bobj\ecdsa_i15_sign_asn1.obj: bearssl\src\ec\ecdsa_i15_sign_asn1.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ecdsa_i15_sign_asn1.c

bobj\ecdsa_i15_sign_raw.obj: bearssl\src\ec\ecdsa_i15_sign_raw.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ecdsa_i15_sign_raw.c

bobj\ecdsa_i15_vrfy_asn1.obj: bearssl\src\ec\ecdsa_i15_vrfy_asn1.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ecdsa_i15_vrfy_asn1.c

bobj\ecdsa_i15_vrfy_raw.obj: bearssl\src\ec\ecdsa_i15_vrfy_raw.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ecdsa_i15_vrfy_raw.c

bobj\ecdsa_i31_bits.obj: bearssl\src\ec\ecdsa_i31_bits.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ecdsa_i31_bits.c

bobj\ecdsa_i31_sign_asn1.obj: bearssl\src\ec\ecdsa_i31_sign_asn1.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ecdsa_i31_sign_asn1.c

bobj\ecdsa_i31_sign_raw.obj: bearssl\src\ec\ecdsa_i31_sign_raw.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ecdsa_i31_sign_raw.c

bobj\ecdsa_i31_vrfy_asn1.obj: bearssl\src\ec\ecdsa_i31_vrfy_asn1.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ecdsa_i31_vrfy_asn1.c

bobj\ecdsa_i31_vrfy_raw.obj: bearssl\src\ec\ecdsa_i31_vrfy_raw.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ecdsa_i31_vrfy_raw.c

bobj\ecdsa_rta.obj: bearssl\src\ec\ecdsa_rta.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ec\ecdsa_rta.c

bobj\dig_oid.obj: bearssl\src\hash\dig_oid.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\hash\dig_oid.c

bobj\dig_size.obj: bearssl\src\hash\dig_size.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\hash\dig_size.c

bobj\ghash_ctmul.obj: bearssl\src\hash\ghash_ctmul.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\hash\ghash_ctmul.c

bobj\ghash_ctmul32.obj: bearssl\src\hash\ghash_ctmul32.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\hash\ghash_ctmul32.c

bobj\ghash_ctmul64.obj: bearssl\src\hash\ghash_ctmul64.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\hash\ghash_ctmul64.c

bobj\ghash_pclmul.obj: bearssl\src\hash\ghash_pclmul.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\hash\ghash_pclmul.c

bobj\ghash_pwr8.obj: bearssl\src\hash\ghash_pwr8.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\hash\ghash_pwr8.c

bobj\md5.obj: bearssl\src\hash\md5.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\hash\md5.c

bobj\md5sha1.obj: bearssl\src\hash\md5sha1.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\hash\md5sha1.c

bobj\mgf1.obj: bearssl\src\hash\mgf1.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\hash\mgf1.c

bobj\multihash.obj: bearssl\src\hash\multihash.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\hash\multihash.c

bobj\sha1.obj: bearssl\src\hash\sha1.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\hash\sha1.c

bobj\sha2big.obj: bearssl\src\hash\sha2big.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\hash\sha2big.c

bobj\sha2small.obj: bearssl\src\hash\sha2small.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\hash\sha2small.c

bobj\i15_add.obj: bearssl\src\int\i15_add.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i15_add.c

bobj\i15_bitlen.obj: bearssl\src\int\i15_bitlen.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i15_bitlen.c

bobj\i15_decmod.obj: bearssl\src\int\i15_decmod.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i15_decmod.c

bobj\i15_decode.obj: bearssl\src\int\i15_decode.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i15_decode.c

bobj\i15_decred.obj: bearssl\src\int\i15_decred.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i15_decred.c

bobj\i15_encode.obj: bearssl\src\int\i15_encode.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i15_encode.c

bobj\i15_fmont.obj: bearssl\src\int\i15_fmont.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i15_fmont.c

bobj\i15_iszero.obj: bearssl\src\int\i15_iszero.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i15_iszero.c

bobj\i15_moddiv.obj: bearssl\src\int\i15_moddiv.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i15_moddiv.c

bobj\i15_modpow.obj: bearssl\src\int\i15_modpow.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i15_modpow.c

bobj\i15_modpow2.obj: bearssl\src\int\i15_modpow2.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i15_modpow2.c

bobj\i15_montmul.obj: bearssl\src\int\i15_montmul.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i15_montmul.c

bobj\i15_mulacc.obj: bearssl\src\int\i15_mulacc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i15_mulacc.c

bobj\i15_muladd.obj: bearssl\src\int\i15_muladd.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i15_muladd.c

bobj\i15_ninv15.obj: bearssl\src\int\i15_ninv15.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i15_ninv15.c

bobj\i15_reduce.obj: bearssl\src\int\i15_reduce.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i15_reduce.c

bobj\i15_rshift.obj: bearssl\src\int\i15_rshift.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i15_rshift.c

bobj\i15_sub.obj: bearssl\src\int\i15_sub.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i15_sub.c

bobj\i15_tmont.obj: bearssl\src\int\i15_tmont.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i15_tmont.c

bobj\i31_add.obj: bearssl\src\int\i31_add.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i31_add.c

bobj\i31_bitlen.obj: bearssl\src\int\i31_bitlen.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i31_bitlen.c

bobj\i31_decmod.obj: bearssl\src\int\i31_decmod.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i31_decmod.c

bobj\i31_decode.obj: bearssl\src\int\i31_decode.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i31_decode.c

bobj\i31_decred.obj: bearssl\src\int\i31_decred.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i31_decred.c

bobj\i31_encode.obj: bearssl\src\int\i31_encode.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i31_encode.c

bobj\i31_fmont.obj: bearssl\src\int\i31_fmont.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i31_fmont.c

bobj\i31_iszero.obj: bearssl\src\int\i31_iszero.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i31_iszero.c

bobj\i31_moddiv.obj: bearssl\src\int\i31_moddiv.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i31_moddiv.c

bobj\i31_modpow.obj: bearssl\src\int\i31_modpow.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i31_modpow.c

bobj\i31_modpow2.obj: bearssl\src\int\i31_modpow2.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i31_modpow2.c

bobj\i31_montmul.obj: bearssl\src\int\i31_montmul.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i31_montmul.c

bobj\i31_mulacc.obj: bearssl\src\int\i31_mulacc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i31_mulacc.c

bobj\i31_muladd.obj: bearssl\src\int\i31_muladd.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i31_muladd.c

bobj\i31_ninv31.obj: bearssl\src\int\i31_ninv31.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i31_ninv31.c

bobj\i31_reduce.obj: bearssl\src\int\i31_reduce.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i31_reduce.c

bobj\i31_rshift.obj: bearssl\src\int\i31_rshift.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i31_rshift.c

bobj\i31_sub.obj: bearssl\src\int\i31_sub.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i31_sub.c

bobj\i31_tmont.obj: bearssl\src\int\i31_tmont.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i31_tmont.c

bobj\i32_add.obj: bearssl\src\int\i32_add.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i32_add.c

bobj\i32_bitlen.obj: bearssl\src\int\i32_bitlen.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i32_bitlen.c

bobj\i32_decmod.obj: bearssl\src\int\i32_decmod.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i32_decmod.c

bobj\i32_decode.obj: bearssl\src\int\i32_decode.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i32_decode.c

bobj\i32_decred.obj: bearssl\src\int\i32_decred.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i32_decred.c

bobj\i32_div32.obj: bearssl\src\int\i32_div32.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i32_div32.c

bobj\i32_encode.obj: bearssl\src\int\i32_encode.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i32_encode.c

bobj\i32_fmont.obj: bearssl\src\int\i32_fmont.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i32_fmont.c

bobj\i32_iszero.obj: bearssl\src\int\i32_iszero.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i32_iszero.c

bobj\i32_modpow.obj: bearssl\src\int\i32_modpow.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i32_modpow.c

bobj\i32_montmul.obj: bearssl\src\int\i32_montmul.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i32_montmul.c

bobj\i32_mulacc.obj: bearssl\src\int\i32_mulacc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i32_mulacc.c

bobj\i32_muladd.obj: bearssl\src\int\i32_muladd.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i32_muladd.c

bobj\i32_ninv32.obj: bearssl\src\int\i32_ninv32.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i32_ninv32.c

bobj\i32_reduce.obj: bearssl\src\int\i32_reduce.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i32_reduce.c

bobj\i32_sub.obj: bearssl\src\int\i32_sub.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i32_sub.c

bobj\i32_tmont.obj: bearssl\src\int\i32_tmont.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i32_tmont.c

bobj\i62_modpow2.obj: bearssl\src\int\i62_modpow2.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\int\i62_modpow2.c

bobj\hkdf.obj: bearssl\src\kdf\hkdf.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\kdf\hkdf.c

bobj\hmac.obj: bearssl\src\mac\hmac.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\mac\hmac.c

bobj\hmac_ct.obj: bearssl\src\mac\hmac_ct.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\mac\hmac_ct.c

bobj\aesctr_drbg.obj: bearssl\src\rand\aesctr_drbg.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rand\aesctr_drbg.c

bobj\hmac_drbg.obj: bearssl\src\rand\hmac_drbg.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rand\hmac_drbg.c

bobj\sysrng.obj: bearssl\src\rand\sysrng.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rand\sysrng.c

bobj\rsa_default_keygen.obj: bearssl\src\rsa\rsa_default_keygen.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_default_keygen.c

bobj\rsa_default_modulus.obj: bearssl\src\rsa\rsa_default_modulus.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_default_modulus.c

bobj\rsa_default_oaep_decrypt.obj: bearssl\src\rsa\rsa_default_oaep_decrypt.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_default_oaep_decrypt.c

bobj\rsa_default_oaep_encrypt.obj: bearssl\src\rsa\rsa_default_oaep_encrypt.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_default_oaep_encrypt.c

bobj\rsa_default_pkcs1_sign.obj: bearssl\src\rsa\rsa_default_pkcs1_sign.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_default_pkcs1_sign.c

bobj\rsa_default_pkcs1_vrfy.obj: bearssl\src\rsa\rsa_default_pkcs1_vrfy.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_default_pkcs1_vrfy.c

bobj\rsa_default_priv.obj: bearssl\src\rsa\rsa_default_priv.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_default_priv.c

bobj\rsa_default_privexp.obj: bearssl\src\rsa\rsa_default_privexp.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_default_privexp.c

bobj\rsa_default_pub.obj: bearssl\src\rsa\rsa_default_pub.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_default_pub.c

bobj\rsa_default_pubexp.obj: bearssl\src\rsa\rsa_default_pubexp.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_default_pubexp.c

bobj\rsa_i15_keygen.obj: bearssl\src\rsa\rsa_i15_keygen.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i15_keygen.c

bobj\rsa_i15_modulus.obj: bearssl\src\rsa\rsa_i15_modulus.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i15_modulus.c

bobj\rsa_i15_oaep_decrypt.obj: bearssl\src\rsa\rsa_i15_oaep_decrypt.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i15_oaep_decrypt.c

bobj\rsa_i15_oaep_encrypt.obj: bearssl\src\rsa\rsa_i15_oaep_encrypt.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i15_oaep_encrypt.c

bobj\rsa_i15_pkcs1_sign.obj: bearssl\src\rsa\rsa_i15_pkcs1_sign.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i15_pkcs1_sign.c

bobj\rsa_i15_pkcs1_vrfy.obj: bearssl\src\rsa\rsa_i15_pkcs1_vrfy.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i15_pkcs1_vrfy.c

bobj\rsa_i15_priv.obj: bearssl\src\rsa\rsa_i15_priv.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i15_priv.c

bobj\rsa_i15_privexp.obj: bearssl\src\rsa\rsa_i15_privexp.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i15_privexp.c

bobj\rsa_i15_pub.obj: bearssl\src\rsa\rsa_i15_pub.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i15_pub.c

bobj\rsa_i15_pubexp.obj: bearssl\src\rsa\rsa_i15_pubexp.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i15_pubexp.c

bobj\rsa_i31_keygen.obj: bearssl\src\rsa\rsa_i31_keygen.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i31_keygen.c

bobj\rsa_i31_keygen_inner.obj: bearssl\src\rsa\rsa_i31_keygen_inner.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i31_keygen_inner.c

bobj\rsa_i31_modulus.obj: bearssl\src\rsa\rsa_i31_modulus.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i31_modulus.c

bobj\rsa_i31_oaep_decrypt.obj: bearssl\src\rsa\rsa_i31_oaep_decrypt.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i31_oaep_decrypt.c

bobj\rsa_i31_oaep_encrypt.obj: bearssl\src\rsa\rsa_i31_oaep_encrypt.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i31_oaep_encrypt.c

bobj\rsa_i31_pkcs1_sign.obj: bearssl\src\rsa\rsa_i31_pkcs1_sign.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i31_pkcs1_sign.c

bobj\rsa_i31_pkcs1_vrfy.obj: bearssl\src\rsa\rsa_i31_pkcs1_vrfy.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i31_pkcs1_vrfy.c

bobj\rsa_i31_priv.obj: bearssl\src\rsa\rsa_i31_priv.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i31_priv.c

bobj\rsa_i31_privexp.obj: bearssl\src\rsa\rsa_i31_privexp.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i31_privexp.c

bobj\rsa_i31_pub.obj: bearssl\src\rsa\rsa_i31_pub.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i31_pub.c

bobj\rsa_i31_pubexp.obj: bearssl\src\rsa\rsa_i31_pubexp.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i31_pubexp.c

bobj\rsa_i32_oaep_decrypt.obj: bearssl\src\rsa\rsa_i32_oaep_decrypt.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i32_oaep_decrypt.c

bobj\rsa_i32_oaep_encrypt.obj: bearssl\src\rsa\rsa_i32_oaep_encrypt.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i32_oaep_encrypt.c

bobj\rsa_i32_pkcs1_sign.obj: bearssl\src\rsa\rsa_i32_pkcs1_sign.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i32_pkcs1_sign.c

bobj\rsa_i32_pkcs1_vrfy.obj: bearssl\src\rsa\rsa_i32_pkcs1_vrfy.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i32_pkcs1_vrfy.c

bobj\rsa_i32_priv.obj: bearssl\src\rsa\rsa_i32_priv.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i32_priv.c

bobj\rsa_i32_pub.obj: bearssl\src\rsa\rsa_i32_pub.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i32_pub.c

bobj\rsa_i62_keygen.obj: bearssl\src\rsa\rsa_i62_keygen.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i62_keygen.c

bobj\rsa_i62_oaep_decrypt.obj: bearssl\src\rsa\rsa_i62_oaep_decrypt.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i62_oaep_decrypt.c

bobj\rsa_i62_oaep_encrypt.obj: bearssl\src\rsa\rsa_i62_oaep_encrypt.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i62_oaep_encrypt.c

bobj\rsa_i62_pkcs1_sign.obj: bearssl\src\rsa\rsa_i62_pkcs1_sign.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i62_pkcs1_sign.c

bobj\rsa_i62_pkcs1_vrfy.obj: bearssl\src\rsa\rsa_i62_pkcs1_vrfy.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i62_pkcs1_vrfy.c

bobj\rsa_i62_priv.obj: bearssl\src\rsa\rsa_i62_priv.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i62_priv.c

bobj\rsa_i62_pub.obj: bearssl\src\rsa\rsa_i62_pub.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_i62_pub.c

bobj\rsa_oaep_pad.obj: bearssl\src\rsa\rsa_oaep_pad.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_oaep_pad.c

bobj\rsa_oaep_unpad.obj: bearssl\src\rsa\rsa_oaep_unpad.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_oaep_unpad.c

bobj\rsa_pkcs1_sig_pad.obj: bearssl\src\rsa\rsa_pkcs1_sig_pad.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_pkcs1_sig_pad.c

bobj\rsa_pkcs1_sig_unpad.obj: bearssl\src\rsa\rsa_pkcs1_sig_unpad.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_pkcs1_sig_unpad.c

bobj\rsa_ssl_decrypt.obj: bearssl\src\rsa\rsa_ssl_decrypt.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\rsa\rsa_ssl_decrypt.c

bobj\settings.obj: bearssl\src\settings.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\settings.c

bobj\prf.obj: bearssl\src\ssl\prf.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\prf.c

bobj\prf_md5sha1.obj: bearssl\src\ssl\prf_md5sha1.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\prf_md5sha1.c

bobj\prf_sha256.obj: bearssl\src\ssl\prf_sha256.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\prf_sha256.c

bobj\prf_sha384.obj: bearssl\src\ssl\prf_sha384.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\prf_sha384.c

bobj\ssl_ccert_single_ec.obj: bearssl\src\ssl\ssl_ccert_single_ec.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_ccert_single_ec.c

bobj\ssl_ccert_single_rsa.obj: bearssl\src\ssl\ssl_ccert_single_rsa.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_ccert_single_rsa.c

bobj\ssl_client.obj: bearssl\src\ssl\ssl_client.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_client.c

bobj\ssl_client_default_rsapub.obj: bearssl\src\ssl\ssl_client_default_rsapub.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_client_default_rsapub.c

bobj\ssl_client_full.obj: bearssl\src\ssl\ssl_client_full.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_client_full.c

bobj\ssl_engine.obj: bearssl\src\ssl\ssl_engine.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_engine.c

bobj\ssl_engine_default_aescbc.obj: bearssl\src\ssl\ssl_engine_default_aescbc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_engine_default_aescbc.c

bobj\ssl_engine_default_aesccm.obj: bearssl\src\ssl\ssl_engine_default_aesccm.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_engine_default_aesccm.c

bobj\ssl_engine_default_aesgcm.obj: bearssl\src\ssl\ssl_engine_default_aesgcm.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_engine_default_aesgcm.c

bobj\ssl_engine_default_chapol.obj: bearssl\src\ssl\ssl_engine_default_chapol.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_engine_default_chapol.c

bobj\ssl_engine_default_descbc.obj: bearssl\src\ssl\ssl_engine_default_descbc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_engine_default_descbc.c

bobj\ssl_engine_default_ec.obj: bearssl\src\ssl\ssl_engine_default_ec.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_engine_default_ec.c

bobj\ssl_engine_default_ecdsa.obj: bearssl\src\ssl\ssl_engine_default_ecdsa.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_engine_default_ecdsa.c

bobj\ssl_engine_default_rsavrfy.obj: bearssl\src\ssl\ssl_engine_default_rsavrfy.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_engine_default_rsavrfy.c

bobj\ssl_hashes.obj: bearssl\src\ssl\ssl_hashes.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_hashes.c

bobj\ssl_hs_client.obj: bearssl\src\ssl\ssl_hs_client.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_hs_client.c

bobj\ssl_hs_server.obj: bearssl\src\ssl\ssl_hs_server.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_hs_server.c

bobj\ssl_io.obj: bearssl\src\ssl\ssl_io.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_io.c

bobj\ssl_keyexport.obj: bearssl\src\ssl\ssl_keyexport.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_keyexport.c

bobj\ssl_lru.obj: bearssl\src\ssl\ssl_lru.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_lru.c

bobj\ssl_rec_cbc.obj: bearssl\src\ssl\ssl_rec_cbc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_rec_cbc.c

bobj\ssl_rec_ccm.obj: bearssl\src\ssl\ssl_rec_ccm.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_rec_ccm.c

bobj\ssl_rec_chapol.obj: bearssl\src\ssl\ssl_rec_chapol.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_rec_chapol.c

bobj\ssl_rec_gcm.obj: bearssl\src\ssl\ssl_rec_gcm.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_rec_gcm.c

bobj\ssl_scert_single_ec.obj: bearssl\src\ssl\ssl_scert_single_ec.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_scert_single_ec.c

bobj\ssl_scert_single_rsa.obj: bearssl\src\ssl\ssl_scert_single_rsa.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_scert_single_rsa.c

bobj\ssl_server.obj: bearssl\src\ssl\ssl_server.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_server.c

bobj\ssl_server_full_ec.obj: bearssl\src\ssl\ssl_server_full_ec.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_server_full_ec.c

bobj\ssl_server_full_rsa.obj: bearssl\src\ssl\ssl_server_full_rsa.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_server_full_rsa.c

bobj\ssl_server_mine2c.obj: bearssl\src\ssl\ssl_server_mine2c.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_server_mine2c.c

bobj\ssl_server_mine2g.obj: bearssl\src\ssl\ssl_server_mine2g.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_server_mine2g.c

bobj\ssl_server_minf2c.obj: bearssl\src\ssl\ssl_server_minf2c.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_server_minf2c.c

bobj\ssl_server_minf2g.obj: bearssl\src\ssl\ssl_server_minf2g.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_server_minf2g.c

bobj\ssl_server_minr2g.obj: bearssl\src\ssl\ssl_server_minr2g.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_server_minr2g.c

bobj\ssl_server_minu2g.obj: bearssl\src\ssl\ssl_server_minu2g.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_server_minu2g.c

bobj\ssl_server_minv2g.obj: bearssl\src\ssl\ssl_server_minv2g.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\ssl\ssl_server_minv2g.c

bobj\aes_big_cbcdec.obj: bearssl\src\symcipher\aes_big_cbcdec.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_big_cbcdec.c

bobj\aes_big_cbcenc.obj: bearssl\src\symcipher\aes_big_cbcenc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_big_cbcenc.c

bobj\aes_big_ctr.obj: bearssl\src\symcipher\aes_big_ctr.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_big_ctr.c

bobj\aes_big_ctrcbc.obj: bearssl\src\symcipher\aes_big_ctrcbc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_big_ctrcbc.c

bobj\aes_big_dec.obj: bearssl\src\symcipher\aes_big_dec.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_big_dec.c

bobj\aes_big_enc.obj: bearssl\src\symcipher\aes_big_enc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_big_enc.c

bobj\aes_common.obj: bearssl\src\symcipher\aes_common.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_common.c

bobj\aes_ct.obj: bearssl\src\symcipher\aes_ct.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_ct.c

bobj\aes_ct_cbcdec.obj: bearssl\src\symcipher\aes_ct_cbcdec.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_ct_cbcdec.c

bobj\aes_ct_cbcenc.obj: bearssl\src\symcipher\aes_ct_cbcenc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_ct_cbcenc.c

bobj\aes_ct_ctr.obj: bearssl\src\symcipher\aes_ct_ctr.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_ct_ctr.c

bobj\aes_ct_ctrcbc.obj: bearssl\src\symcipher\aes_ct_ctrcbc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_ct_ctrcbc.c

bobj\aes_ct_dec.obj: bearssl\src\symcipher\aes_ct_dec.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_ct_dec.c

bobj\aes_ct_enc.obj: bearssl\src\symcipher\aes_ct_enc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_ct_enc.c

bobj\aes_ct64.obj: bearssl\src\symcipher\aes_ct64.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_ct64.c

bobj\aes_ct64_cbcdec.obj: bearssl\src\symcipher\aes_ct64_cbcdec.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_ct64_cbcdec.c

bobj\aes_ct64_cbcenc.obj: bearssl\src\symcipher\aes_ct64_cbcenc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_ct64_cbcenc.c

bobj\aes_ct64_ctr.obj: bearssl\src\symcipher\aes_ct64_ctr.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_ct64_ctr.c

bobj\aes_ct64_ctrcbc.obj: bearssl\src\symcipher\aes_ct64_ctrcbc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_ct64_ctrcbc.c

bobj\aes_ct64_dec.obj: bearssl\src\symcipher\aes_ct64_dec.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_ct64_dec.c

bobj\aes_ct64_enc.obj: bearssl\src\symcipher\aes_ct64_enc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_ct64_enc.c

bobj\aes_pwr8.obj: bearssl\src\symcipher\aes_pwr8.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_pwr8.c

bobj\aes_pwr8_cbcdec.obj: bearssl\src\symcipher\aes_pwr8_cbcdec.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_pwr8_cbcdec.c

bobj\aes_pwr8_cbcenc.obj: bearssl\src\symcipher\aes_pwr8_cbcenc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_pwr8_cbcenc.c

bobj\aes_pwr8_ctr.obj: bearssl\src\symcipher\aes_pwr8_ctr.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_pwr8_ctr.c

bobj\aes_pwr8_ctrcbc.obj: bearssl\src\symcipher\aes_pwr8_ctrcbc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_pwr8_ctrcbc.c

bobj\aes_small_cbcdec.obj: bearssl\src\symcipher\aes_small_cbcdec.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_small_cbcdec.c

bobj\aes_small_cbcenc.obj: bearssl\src\symcipher\aes_small_cbcenc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_small_cbcenc.c

bobj\aes_small_ctr.obj: bearssl\src\symcipher\aes_small_ctr.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_small_ctr.c

bobj\aes_small_ctrcbc.obj: bearssl\src\symcipher\aes_small_ctrcbc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_small_ctrcbc.c

bobj\aes_small_dec.obj: bearssl\src\symcipher\aes_small_dec.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_small_dec.c

bobj\aes_small_enc.obj: bearssl\src\symcipher\aes_small_enc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_small_enc.c

bobj\aes_x86ni.obj: bearssl\src\symcipher\aes_x86ni.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_x86ni.c

bobj\aes_x86ni_cbcdec.obj: bearssl\src\symcipher\aes_x86ni_cbcdec.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_x86ni_cbcdec.c

bobj\aes_x86ni_cbcenc.obj: bearssl\src\symcipher\aes_x86ni_cbcenc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_x86ni_cbcenc.c

bobj\aes_x86ni_ctr.obj: bearssl\src\symcipher\aes_x86ni_ctr.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_x86ni_ctr.c

bobj\aes_x86ni_ctrcbc.obj: bearssl\src\symcipher\aes_x86ni_ctrcbc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\aes_x86ni_ctrcbc.c

bobj\chacha20_ct.obj: bearssl\src\symcipher\chacha20_ct.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\chacha20_ct.c

bobj\chacha20_sse2.obj: bearssl\src\symcipher\chacha20_sse2.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\chacha20_sse2.c

bobj\des_ct.obj: bearssl\src\symcipher\des_ct.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\des_ct.c

bobj\des_ct_cbcdec.obj: bearssl\src\symcipher\des_ct_cbcdec.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\des_ct_cbcdec.c

bobj\des_ct_cbcenc.obj: bearssl\src\symcipher\des_ct_cbcenc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\des_ct_cbcenc.c

bobj\des_support.obj: bearssl\src\symcipher\des_support.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\des_support.c

bobj\des_tab.obj: bearssl\src\symcipher\des_tab.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\des_tab.c

bobj\des_tab_cbcdec.obj: bearssl\src\symcipher\des_tab_cbcdec.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\des_tab_cbcdec.c

bobj\des_tab_cbcenc.obj: bearssl\src\symcipher\des_tab_cbcenc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\des_tab_cbcenc.c

bobj\poly1305_ctmul.obj: bearssl\src\symcipher\poly1305_ctmul.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\poly1305_ctmul.c

bobj\poly1305_ctmul32.obj: bearssl\src\symcipher\poly1305_ctmul32.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\poly1305_ctmul32.c

bobj\poly1305_ctmulq.obj: bearssl\src\symcipher\poly1305_ctmulq.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\poly1305_ctmulq.c

bobj\poly1305_i15.obj: bearssl\src\symcipher\poly1305_i15.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\symcipher\poly1305_i15.c

bobj\asn1enc.obj: bearssl\src\x509\asn1enc.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\x509\asn1enc.c

bobj\encode_ec_pk8der.obj: bearssl\src\x509\encode_ec_pk8der.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\x509\encode_ec_pk8der.c

bobj\encode_ec_rawder.obj: bearssl\src\x509\encode_ec_rawder.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\x509\encode_ec_rawder.c

bobj\encode_rsa_pk8der.obj: bearssl\src\x509\encode_rsa_pk8der.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\x509\encode_rsa_pk8der.c

bobj\encode_rsa_rawder.obj: bearssl\src\x509\encode_rsa_rawder.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\x509\encode_rsa_rawder.c

bobj\skey_decoder.obj: bearssl\src\x509\skey_decoder.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\x509\skey_decoder.c

bobj\x509_decoder.obj: bearssl\src\x509\x509_decoder.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\x509\x509_decoder.c

bobj\x509_knownkey.obj: bearssl\src\x509\x509_knownkey.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\x509\x509_knownkey.c

bobj\x509_minimal.obj: bearssl\src\x509\x509_minimal.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\x509\x509_minimal.c

bobj\x509_minimal_full.obj: bearssl\src\x509\x509_minimal_full.c
	@if not exist bobj mkdir bobj
	wcc386 $(BRFLAGS) -fo=$@ bearssl\src\x509\x509_minimal_full.c

