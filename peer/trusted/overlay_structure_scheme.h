/**
 * Author: Alexander S.
 * The following is a direct implementation of the scheme described in the paper. Therefore, we do not provide a
 * thorough documentation here. To understand the purpose and the realization of this class, please read the paper.
 */


#ifndef OVERLAY_STRUCTURE_SCHEME_H
#define OVERLAY_STRUCTURE_SCHEME_H

#include <cstdint>
#include "../../include/misc.h"
#include "../../include/message_structs.h"
#include "../shared/overlay_structure_scheme_message.h"
#include "../shared/overlay_return_tuple.h"

namespace c1::peer {

class OverlayStructureScheme {
  onid_t onid_assoc_;
  onid_t onid_emul_;
  std::vector<PeerInformation> gamma_agree_;
  std::vector<PeerInformation> gamma_send_;
  std::vector<PeerInformation> gamma_receive_;
  std::map<uint64_t, std::vector<PeerInformation>> gamma_route_;
  int64_t overlay_dimension_;
  round_t reconfiguration_time_;
  round_t agreement_time_;
  PeerInformation own_id_;

  onid_t onid_emul_prev_;
  onid_t onid_emul_new_;
  std::map<uint64_t, std::vector<PeerInformation>> gamma_agree_prev_;
  std::vector<PeerInformation> gamma_send_new_;
  std::map<uint64_t, std::vector<PeerInformation>> gamma_route_new_;
  std::map<uint64_t, std::vector<PeerInformation>> gamma_route_succ_;
  std::map<uint64_t, std::vector<PeerInformation>> gamma_route_prev_;
  std::vector<PeerInformation> gamma_receive_new_;

 public:
  void init(uint64_t onid_assoc,
            uint64_t onid_emul,
            const std::vector<PeerInformation> &gamma_send,
            const std::vector<PeerInformation> &gamma_receive,
            const std::map<uint64_t, std::vector<PeerInformation>> &gamma_route,
            int64_t overlay_dimension,
            round_t reconfiguration_time,
            round_t agreement_time,
            PeerInformation own_id);

  OverlayReturnTuple update(round_t round, const std::set<OverlayStructureSchemeMessage> &set_s);

};

} // !namespace

#endif //OVERLAY_STRUCTURE_SCHEME_H

