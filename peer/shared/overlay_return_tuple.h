/**
 * Author: Alexander S.
 * Declares the struct returned by the OverlayStructureScheme.
 */

#ifndef OVERLAY_RETURN_TUPLE_H
#define OVERLAY_RETURN_TUPLE_H

#include "../../include/config.h"
#include "../../include/serialization.h"
#include "../../include/message_structs.h"
#include "overlay_structure_scheme_message.h"

namespace c1::peer {

struct OverlayReturnTuple : public c1::Serializable {
  onid_t onid_emul;
  std::vector<c1::PeerInformation> gamma_agree;
  std::vector<c1::PeerInformation> gamma_send;
  std::map<onid_t, std::vector<c1::PeerInformation>> gamma_route;
  std::vector<c1::PeerInformation> gamma_receive;
  std::map<c1::PeerInformation, std::vector<OverlayStructureSchemeMessage>> s_overlay_prime;

  OverlayReturnTuple(onid_t onid_emul,
                     std::vector<c1::PeerInformation> gamma_agree,
                     std::vector<c1::PeerInformation> gamma_send,
                     std::map<onid_t, std::vector<c1::PeerInformation>> gamma_route,
                     std::vector<c1::PeerInformation> gamma_receive,
                     std::map<c1::PeerInformation,
                              std::vector<OverlayStructureSchemeMessage>> s_overlay_prime);

  void serialize(std::vector<uint8_t> &working_vec) const override;
  [[nodiscard]] size_t estimate_size() const override;
  static OverlayReturnTuple deserialize(const std::vector<uint8_t> &working_vec, size_t &cur);
};
}

#endif //OVERLAY_RETURN_TUPLE_H
