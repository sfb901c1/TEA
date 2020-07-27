/**
 * Cryptlib used by both the login_server and the peers.
 */

#ifndef TEE_CRYPT_LIB_H
#define TEE_CRYPT_LIB_H

#include <cstdint>
#include <vector>
#include "../include/misc.h"
#include "tee_crypto_functions.h"

namespace c1::cryptlib {

std::vector<uint8_t> encrypt(const tee_aes_gcm_128bit_key_t &sk_enc,
                             const std::vector<uint8_t> &p,
                             const std::vector<uint8_t> &aad);

std::pair<std::vector<uint8_t>, std::vector<uint8_t>> decrypt(const tee_aes_gcm_128bit_key_t &sk_enc,
                                                              const std::vector<uint8_t> &c);

// key generation for sk_pseud or sk_end
std::array<uint8_t, kTee_aesgcm_key_size> keygen();

std::array<uint8_t, kTee_aesgcm_key_size> gen_routing_key();

onid_t get_intermediate_target(const tee_cmac_128bit_key_t &sk_routing,
                               c1::peer::Pseudonym bucket_dst,
                               round_t ell_dst,
                               dim_t overlay_dimension);

} // !namespace

#endif //TEE_CRYPT_LIB_H
