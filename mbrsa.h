/*
 * mbrsa.h - RSA の公開鍵の計算 (BearSSL 版で br_rsa_i31_public / br_rsa_i31_pkcs1_vrfy の代わりに使う)
 */
#ifndef MBRSA_H
#define MBRSA_H

/* 起動時に 1 回 (鍵ごとに覚える前計算の排他の準備) */
void     MbRsaInit(void);
/* br_rsa_public と同じ形。計算の時間が値で変わるので、秘密の値には使わない (RSA の鍵の交換には渡さない) */
uint32_t MbRsaPublic(unsigned char *x, size_t xlen, const br_rsa_public_key *pk);
/* br_rsa_pkcs1_vrfy と同じ形 (br_ssl_engine_set_rsavrfy と br_x509_minimal_set_rsa に渡す) */
uint32_t MbRsaPkcs1Vrfy(const unsigned char *x, size_t xlen, const unsigned char *hash_oid, size_t hash_len,
                        const br_rsa_public_key *pk, unsigned char *hash_out);

#endif
