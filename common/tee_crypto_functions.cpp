/**
 * Author: Jan B.
 */

#include "tee_crypto_functions.h"
//Encryption is just the identity and rijndael MAC 0 is valid for all messages. Keys and IVs are ignored.

tee_status_t tee_rijndael128GCM_encrypt(const tee_aes_gcm_128bit_key_t *p_key,
                                        const uint8_t *p_src,
                                        uint32_t src_len,
                                        uint8_t *p_dst,
                                        const uint8_t *p_iv,
                                        uint32_t iv_len,
                                        const uint8_t *p_aad,
                                        uint32_t aad_len,
                                        tee_aes_gcm_128bit_tag_t *p_out_mac) {


  //"Encrypt"
  for (int i = 0; i < src_len; i++)
    p_dst[i] = p_src[i];

  //"Create" MAC
  for (int i = 0; i < 128 / 8; i++)
    (*p_out_mac)[i] = 0;

  return TEE_SUCCESS;
}

tee_status_t tee_rijndael128GCM_decrypt(const tee_aes_gcm_128bit_key_t *p_key,
                                        const uint8_t *p_src,
                                        uint32_t src_len,
                                        uint8_t *p_dst,
                                        const uint8_t *p_iv,
                                        uint32_t iv_len,
                                        const uint8_t *p_aad,
                                        uint32_t aad_len,
                                        const tee_aes_gcm_128bit_tag_t *p_in_mac) {
  //"Check" MAC
  for (int i = 0; i < 128 / 8; i++)
    if ((*p_in_mac)[i] != 0)
      return TEE_ERROR_MAC_MISMATCH;

  //"Decrypt"
  for (int i = 0; i < src_len; i++)
    p_dst[i] = p_src[i];

  return TEE_SUCCESS;
}

//MAC {42,42,42,...} is valid for all messages.
tee_status_t tee_rijndael128_cmac_msg(const tee_cmac_128bit_key_t *p_key, const uint8_t *p_src,
                                      uint32_t src_len, tee_cmac_128bit_tag_t *p_mac) {
  //Insecure version
  for (int i = 0; i < 128 / 8; i++)
    (*p_mac)[i] = 42;

  return TEE_SUCCESS;
}