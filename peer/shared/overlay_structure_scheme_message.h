/**
 * Author: Alexander S.
 * A message class used for the OverlayStructureScheme.
 */

#ifndef OVERLAY_STRUCTURE_SCHEME_MESSAGE_H
#define OVERLAY_STRUCTURE_SCHEME_MESSAGE_H

#include "../../include/serialization.h"
#include "../../include/message_structs.h"

namespace c1::peer {
// the following is not the nicest way of doing this, but it's keeping things simple for now
class OverlayStructureSchemeMessage : public Serializable {
 private:
  OverlayStructureSchemeMessage() {}

 public:
  enum class OverlayStructureSchemeMessageType : uint8_t {
    tEmulateRequestMsg,
    tEmulateRequestReceivedMsg,
    tHandOverMsg,
    tSelfIntroduceMsg
  } t;
  onid_t onid;
  PeerInformation peer_information;
  std::map<onid_t, std::vector<PeerInformation>> gamma_route;
  std::vector<PeerInformation> gamma_receive;

  OverlayStructureSchemeMessage(OverlayStructureSchemeMessageType t,
                                onid_t onid,
                                const PeerInformation &peer_information,
                                const std::map<onid_t, std::vector<PeerInformation>> &gamma_route,
                                const std::vector<PeerInformation> &gamma_receive_or_send);

  static OverlayStructureSchemeMessage createEmulateRequestMsg(onid_t onid_prime, const PeerInformation &i) {
    return OverlayStructureSchemeMessage{OverlayStructureSchemeMessageType::tEmulateRequestMsg, onid_prime, i,
                                         std::map<onid_t, std::vector<PeerInformation>>(),
                                         std::vector<PeerInformation>()};
  }
  static OverlayStructureSchemeMessage createEmulateRequestReceivedMsg(onid_t onid_prime, const PeerInformation &i) {
    return OverlayStructureSchemeMessage{OverlayStructureSchemeMessageType::tEmulateRequestReceivedMsg, onid_prime, i,
                                         std::map<onid_t, std::vector<PeerInformation>>(),
                                         std::vector<PeerInformation>()};
  }
  static OverlayStructureSchemeMessage createHandOverMessage(const std::map<onid_t,
                                                                            std::vector<PeerInformation>> &gamma_route,
                                                             const std::vector<PeerInformation> &gamma_receive) {
    return OverlayStructureSchemeMessage{OverlayStructureSchemeMessageType::tHandOverMsg, 0, PeerInformation(),
                                         gamma_route, gamma_receive};
  }
  static OverlayStructureSchemeMessage createSelfIntroduceMessage(const PeerInformation &i) {
    return OverlayStructureSchemeMessage{OverlayStructureSchemeMessageType::tSelfIntroduceMsg, 0, i,
                                         std::map<onid_t, std::vector<PeerInformation>>(),
                                         std::vector<PeerInformation>()};
  }

  void serialize(std::vector<uint8_t> &working_vec) const override;

  static OverlayStructureSchemeMessage deserialize(const std::vector<uint8_t> &working_vec, size_t &cur);

  bool operator==(const OverlayStructureSchemeMessage &rhs) const {
    return std::tie(t, onid, peer_information, gamma_route, gamma_receive)
        == std::tie(rhs.t,
                    rhs.onid,
                    rhs.peer_information,
                    rhs.gamma_route,
                    rhs.gamma_receive);
  }
  bool operator!=(const OverlayStructureSchemeMessage &rhs) const {
    return !(rhs == *this);
  }
  size_t estimate_size() const override;

  bool operator<(const OverlayStructureSchemeMessage &rhs) const;
  bool operator>(const OverlayStructureSchemeMessage &rhs) const;
  bool operator<=(const OverlayStructureSchemeMessage &rhs) const;
  bool operator>=(const OverlayStructureSchemeMessage &rhs) const;

};

} // ~namespace

#endif //OVERLAY_STRUCTURE_SCHEME_MESSAGE_H