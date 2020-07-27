/**
 * Author: Jan B.
 */

#include "cryptlib.h"

#include <iostream>

#define ASSERT(x) \
  if (!(x)) { \
    ocall_print_string("assert x; failed"); \
  } \
  assert (x);

namespace c1 {

std::vector<uint8_t> cryptlib::encrypt(const tee_aes_gcm_128bit_key_t &sk_enc,
                                       const std::vector<uint8_t> &p,
                                       const std::vector<uint8_t> &aad) { //authenticated encryption done via randomized counter GCM

  std::vector<uint8_t>
      result; //will contain data in the format lct(sizeof(uint32_t)) || laad(sizeof(uint32_t)) || iv(kTee_aesgcm_iv_size) || aad(laad) || mac(kTee_aesgcm_mac_size)  || ciphertext(lct)
  uint32_t result_size =
      sizeof(uint32_t) + sizeof(uint32_t) + kTee_aesgcm_iv_size + aad.size() + kTee_aesgcm_mac_size + p.size();
  result.reserve(result_size);

  //Write length lct of the ciphertext to the result
  auto lct = static_cast<uint32_t>(p.size());
  serialize_number(result, lct);

  //Write length laad of the aad to the result
  auto laad = static_cast<uint32_t>(aad.size());
  serialize_number(result, laad);

  //Generate random IV and write it to result
  auto iv = TeeFunctions::tee_read_rand<kTee_aesgcm_iv_size>();
  result.insert(result.end(), &iv[0], &iv[kTee_aesgcm_iv_size]);

  //Copy aad to result
  result.insert(result.end(), aad.begin(), aad.end());

  //Encrypt p(lct) with additional authenticated data: lct(sizeof(uint32_t)) || laad(sizeof(uint32_t)) || iv(kTee_aesgcm_iv_size) || aad(laad)
  uint8_t ciphertext_out[lct];
  uint8_t mac_out[kTee_aesgcm_mac_size];
  auto status = tee_rijndael128GCM_encrypt(&sk_enc,
                                           p.data(),
                                           lct,
                                           ciphertext_out,
                                           iv.data(),
                                           kTee_aesgcm_iv_size, //note: iv length can be chosen more cleverly knowing how much data we encrypt at most with each IV
                                           result.data(),
                                           sizeof(uint32_t) + sizeof(uint32_t) + kTee_aesgcm_iv_size
                                               + laad, //aad for GCM
                                           &mac_out);
  assert(status == TEE_SUCCESS);

  //Append MAC and ciphertext to the result
  result.insert(result.end(), &mac_out[0], &mac_out[kTee_aesgcm_mac_size]);
  result.insert(result.end(), &ciphertext_out[0], &ciphertext_out[lct]);

  return result;
}

std::pair<std::vector<uint8_t>, std::vector<uint8_t>> cryptlib::decrypt(const tee_aes_gcm_128bit_key_t &sk_enc,
                                                                        const std::vector<uint8_t> &c) {
  size_t cur = 0;
  auto lct = deserialize_number<uint32_t>(c, cur);
  auto laad = deserialize_number<uint32_t>(c, cur);
  //std::cout << "lct: " << lct << '\n';
  //std::cout << "laad: " << laad << '\n';
  const uint8_t *iv = &c[sizeof(uint32_t) + sizeof(uint32_t)];
  const uint8_t *aad = &c[sizeof(uint32_t) + sizeof(uint32_t) + kTee_aesgcm_iv_size];
  const uint8_t *mac = &c[sizeof(uint32_t) + sizeof(uint32_t) + kTee_aesgcm_iv_size + laad];
  const uint8_t *ct = &c[sizeof(uint32_t) + sizeof(uint32_t) + kTee_aesgcm_iv_size + laad + kTee_aesgcm_mac_size];
  const uint8_t *gcmaad = &c[0];
  assert(c.size() == sizeof(uint32_t) + sizeof(uint32_t) + kTee_aesgcm_iv_size + laad + kTee_aesgcm_mac_size + lct);

  //Copy MAC to please tee_rijndael128GCM_decrypt's input requirements
  const uint8_t mac_tag[kTee_aesgcm_mac_size] = {mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7],
                                                 mac[8], mac[9], mac[10], mac[11], mac[12], mac[13], mac[14], mac[15]};

  //Decrypt
  uint8_t plaintext[lct];
  auto status = tee_rijndael128GCM_decrypt(&sk_enc,
                                           ct, lct,
                                           plaintext,
                                           iv, kTee_aesgcm_iv_size,
                                           gcmaad, sizeof(uint32_t) + sizeof(uint32_t) + kTee_aesgcm_iv_size + laad,
                                           &mac_tag);
  assert(status == TEE_SUCCESS);

  std::vector<uint8_t> resultPlaintext(plaintext, plaintext + lct);
  std::vector<uint8_t> resultAad(aad, aad + laad);

  return std::pair<std::vector<uint8_t>, std::vector<uint8_t>>(resultPlaintext, resultAad);
}

std::array<uint8_t, kTee_aesgcm_key_size> cryptlib::keygen() {
  return TeeFunctions::tee_read_rand<kTee_aesgcm_key_size>();
}

std::array<uint8_t, kTee_aesgcm_key_size> cryptlib::gen_routing_key() {
  return TeeFunctions::tee_read_rand<kTee_cmac_key_size>();
}

onid_t cryptlib::get_intermediate_target(const tee_cmac_128bit_key_t &sk_routing,
                                         c1::peer::Pseudonym bucket_dst,
                                         round_t ell_dst,
                                         dim_t overlay_dimension) {
  //Prepare input (bucket_dst, ell_dst) to the PRF
  std::vector<uint8_t> preimage;
  preimage.reserve(kPseudonymSize + sizeof(round_t));
  auto bucket_dst_bytes = bucket_dst.get();
  preimage.insert(preimage.end(), bucket_dst_bytes.begin(), bucket_dst_bytes.end());
  serialize_number(preimage, ell_dst);

  //Evaluate PRF
  tee_cmac_128bit_tag_t image;
  auto status = tee_rijndael128_cmac_msg(&sk_routing, preimage.data(), static_cast<uint32_t>(preimage.size()), &image);
  assert(status == TEE_SUCCESS);

  //Interpret PRF image as overlay node id
  onid_t result = (image[3] << 24) | (image[2] << 16) | (image[1] << 8) | (image[0]);
  result = result % (1UL << overlay_dimension); //reduce to desired dimension
  return result;
}

} // !namespace