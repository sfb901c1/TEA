/**
 * Author: Alexander S.
 * A message class used for the OverlayStructureScheme.
 */

#include "overlay_structure_scheme_message.h"

namespace c1::peer {

void OverlayStructureSchemeMessage::serialize(std::vector<uint8_t> &working_vec) const {
  serialize_number(working_vec, static_cast<uint8_t>(t));
  serialize_number(working_vec, onid);
  peer_information.serialize(working_vec);
  serialize_number(working_vec, gamma_route.size());
  for (auto&[onid, peer_information_vec] : gamma_route) {
    serialize_number(working_vec, onid);
    serialize_vec(working_vec, peer_information_vec);
  }
  serialize_vec(working_vec, gamma_receive);
}

size_t OverlayStructureSchemeMessage::estimate_size() const {
  auto result = sizeof(uint8_t) + sizeof(onid) + peer_information.estimate_size() + sizeof(size_t);
  for (auto&[onid, peer_information_vec] : gamma_route) {
    result += sizeof(onid);
    result += estimate_vec_size(peer_information_vec);
  }
  result += estimate_vec_size(gamma_receive);
  return result;
}

OverlayStructureSchemeMessage OverlayStructureSchemeMessage::deserialize(const std::vector<uint8_t> &working_vec,
                                                                         size_t &cur) {
  OverlayStructureSchemeMessage result;
  result.t = static_cast<OverlayStructureSchemeMessageType>(deserialize_number<uint8_t>(working_vec, cur));
  result.onid = deserialize_number<decltype(result.onid)>(working_vec, cur);
  result.peer_information = PeerInformation::deserialize(working_vec, cur);
  auto gamma_route_size = deserialize_number<size_t>(working_vec, cur);
  for (int i = 0; i < gamma_route_size; ++i) {
    auto onid = deserialize_number<onid_t>(working_vec, cur);
    result.gamma_route[onid] = deserialize_vec<PeerInformation>(working_vec, cur);
  }
  result.gamma_receive = deserialize_vec<PeerInformation>(working_vec, cur);

  return result;
}

OverlayStructureSchemeMessage::OverlayStructureSchemeMessage(
    OverlayStructureSchemeMessage::OverlayStructureSchemeMessageType t,
    onid_t onid,
    const PeerInformation &peer_information,
    const std::map<onid_t,
                   std::vector<PeerInformation>> &gamma_route,
    const std::vector<PeerInformation> &gamma_receive_or_send)
    : t(t),
      onid(onid),
      peer_information(peer_information),
      gamma_route(gamma_route),
      gamma_receive(gamma_receive_or_send) {}

bool OverlayStructureSchemeMessage::operator<(const OverlayStructureSchemeMessage &rhs) const {
  return std::tie(t, onid, peer_information, gamma_route, gamma_receive)
      < std::tie(rhs.t,
                 rhs.onid,
                 rhs.peer_information,
                 rhs.gamma_route,
                 rhs.gamma_receive);
}

bool OverlayStructureSchemeMessage::operator>(const OverlayStructureSchemeMessage &rhs) const {
  return rhs < *this;
}

bool OverlayStructureSchemeMessage::operator<=(const OverlayStructureSchemeMessage &rhs) const {
  return !(rhs > *this);
}

bool OverlayStructureSchemeMessage::operator>=(const OverlayStructureSchemeMessage &rhs) const {
  return !(*this < rhs);
}

} // ~namespace c1