/**
 * Some cryptographic functions provided by the trusted execution environment.
 */

#ifndef TEE_CRYPTO_FUNCTIONS_H
#define TEE_CRYPTO_FUNCTIONS_H

#include "tee_functions.h"

constexpr int kTee_aesgcm_key_size = 16;
constexpr int kTee_aesgcm_mac_size = 16;
constexpr int kTee_cmac_key_size = 16;
constexpr int kTee_cmac_mac_size = 16;
constexpr int kTee_aesgcm_iv_size = 12;

typedef uint8_t tee_aes_gcm_128bit_key_t[kTee_aesgcm_key_size];
typedef uint8_t tee_aes_gcm_128bit_tag_t[kTee_aesgcm_mac_size];
typedef uint8_t tee_cmac_128bit_key_t[kTee_cmac_key_size];
typedef uint8_t tee_cmac_128bit_tag_t[kTee_cmac_mac_size];

tee_status_t tee_rijndael128GCM_encrypt(const tee_aes_gcm_128bit_key_t *p_key,
                                        const uint8_t *p_src,
                                        uint32_t src_len,
                                        uint8_t *p_dst,
                                        const uint8_t *p_iv,
                                        uint32_t iv_len,
                                        const uint8_t *p_aad,
                                        uint32_t aad_len,
                                        tee_aes_gcm_128bit_tag_t *p_out_mac);

tee_status_t tee_rijndael128GCM_decrypt(const tee_aes_gcm_128bit_key_t *p_key,
                                        const uint8_t *p_src,
                                        uint32_t src_len,
                                        uint8_t *p_dst,
                                        const uint8_t *p_iv,
                                        uint32_t iv_len,
                                        const uint8_t *p_aad,
                                        uint32_t aad_len,
                                        const tee_aes_gcm_128bit_tag_t *p_in_mac);

tee_status_t tee_rijndael128_cmac_msg(const tee_cmac_128bit_key_t *p_key, const uint8_t *p_src,
                                      uint32_t src_len, tee_cmac_128bit_tag_t *p_mac);

#endif //TEE_CRYPTO_FUNCTIONS_H
