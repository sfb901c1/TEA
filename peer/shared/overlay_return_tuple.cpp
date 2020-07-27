/**
 * Author: Alexander S.
 * Defines the struct returned by the OverlayStructureScheme.
 */

#include "overlay_return_tuple.h"
#include <utility>

namespace c1::peer {

void OverlayReturnTuple::serialize(std::vector<uint8_t> &working_vec) const {
  // note: this whole function is not necessary for production (only for the visualization)
  c1::serialize_number(working_vec, onid_emul);
  c1::serialize_vec(working_vec, gamma_agree);
  c1::serialize_vec(working_vec, gamma_send);
  c1::serialize_map_of_vecs(working_vec, gamma_route);
  c1::serialize_vec(working_vec, gamma_receive);
  // s_overlay_prime:
  c1::serialize_number(working_vec, s_overlay_prime.size());
  for (const auto &elem : s_overlay_prime) {
    elem.first.serialize(working_vec);
    c1::serialize_vec(working_vec, elem.second);
  }
}
size_t OverlayReturnTuple::estimate_size() const {
  return sizeof(onid_emul) + c1::estimate_vec_size(gamma_agree)
      + c1::estimate_vec_size(gamma_send)
          //+ estimate_vec_size(gamma_route)
      + c1::estimate_vec_size(gamma_receive);
  //overlay_prime ...
  // note: s_overlay_prime not needed because not serialized
}
OverlayReturnTuple OverlayReturnTuple::deserialize(const std::vector<uint8_t> &working_vec, size_t &cur) {
  //OverlayReturnTuple result;
  auto onid_emul = c1::deserialize_number<onid_t>(working_vec, cur);
  auto gamma_agree = c1::deserialize_vec<c1::PeerInformation>(working_vec, cur);
  auto gamma_send = c1::deserialize_vec<c1::PeerInformation>(working_vec, cur);
  auto gamma_route = c1::deserialize_map_of_vecs<onid_t, c1::PeerInformation>(working_vec, cur);
  auto gamma_receive = c1::deserialize_vec<c1::PeerInformation>(working_vec, cur);

  //s_overlay_prime:
  std::map<c1::PeerInformation, std::vector<c1::peer::OverlayStructureSchemeMessage>> s_overlay_prime;
  auto size = c1::deserialize_number<typename std::map<uint64_t,
                                                       std::vector<c1::peer::OverlayStructureSchemeMessage>>::size_type>(
      working_vec, cur);
  for (int i = 0; i < size; ++i) {
    auto key = c1::PeerInformation::deserialize(working_vec, cur);
    s_overlay_prime[key] = c1::deserialize_vec<c1::peer::OverlayStructureSchemeMessage>(working_vec, cur);
  }

  return OverlayReturnTuple(onid_emul, gamma_agree, gamma_send, gamma_route, gamma_receive, s_overlay_prime);
}

OverlayReturnTuple::OverlayReturnTuple(onid_t onid_emul,
                                       std::vector<c1::PeerInformation> gamma_agree,
                                       std::vector<c1::PeerInformation> gamma_send,
                                       std::map<onid_t, std::vector<c1::PeerInformation>> gamma_route,
                                       std::vector<c1::PeerInformation> gamma_receive,
                                       std::map<c1::PeerInformation,
                                                std::vector<c1::peer::OverlayStructureSchemeMessage>> s_overlay_prime)
    : onid_emul(onid_emul),
      gamma_agree(std::move(gamma_agree)),
      gamma_send(std::move(gamma_send)),
      gamma_route(std::move(gamma_route)),
      gamma_receive(std::move(gamma_receive)),
      s_overlay_prime(std::move(s_overlay_prime)) {}

}