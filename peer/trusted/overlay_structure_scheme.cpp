/**
 * Author: Alexander S.
 * The following is a direct implementation of the scheme described in the paper. Therefore, we do not provide a
 * thorough documentation here. To understand the purpose and the realization of this class, please read the paper.
 */

#include "overlay_structure_scheme.h"
#include "../../include/shared_functions.h"

namespace c1::peer {
void OverlayStructureScheme::init(uint64_t onid_assoc,
                                  uint64_t onid_emul,
                                  const std::vector<PeerInformation> &gamma_send,
                                  const std::vector<PeerInformation> &gamma_receive,
                                  const std::map<uint64_t, std::vector<PeerInformation>> &gamma_route,
                                  int64_t overlay_dimension,
                                  round_t reconfiguration_time,
                                  round_t agreement_time,
                                  PeerInformation own_id) {
  onid_assoc_ = onid_assoc;
  onid_emul_new_ = onid_emul;
  //gamma_agree_ = gamma_route.at(onid_emul);
  gamma_send_new_ = gamma_send;
  gamma_receive_ = gamma_receive;
  gamma_receive_new_ = gamma_receive;
  gamma_route_new_ = gamma_route;
  // gamma_route_succ_ is empty
  // gamma_route_prev_ is empty

  overlay_dimension_ = overlay_dimension;
  reconfiguration_time_ = reconfiguration_time;
  agreement_time_ = agreement_time;

  own_id_ = std::move(own_id);
}

OverlayReturnTuple OverlayStructureScheme::update(round_t round,
                                                  const std::set<OverlayStructureSchemeMessage> &set_s) {
//  std::cout << "updating overlayStructureScheme, with parameter round " << std::to_string(round) << "\n";

  // (1)
  std::map<PeerInformation, std::vector<OverlayStructureSchemeMessage>> s_prime;

  // (2)
  std::map<onid_t, std::vector<OverlayStructureSchemeMessage>> msg_out_q;

  // (3)
  if (round % (reconfiguration_time_ / 2) == 1) {
    // (a)
    onid_emul_prev_ = onid_emul_;

    // (b)
    onid_emul_ = onid_emul_new_;
    gamma_agree_.clear();
    for (auto &i_prime : gamma_route_new_[onid_emul_]) {
      gamma_agree_.push_back(i_prime);
    }
    gamma_send_ = std::move(gamma_send_new_);
    gamma_route_ = std::move(gamma_route_new_);

    // (c)
    uint64_t random_number = 0;
#ifndef TESTING
    // guard is to prevent this function from being required by the tests
    TeeFunctions::tee_read_rand<uint64_t>(&random_number);
#endif
    auto num_quorums = static_cast<uint64_t>(std::pow(2, overlay_dimension_));
    uint64_t random_quorum = random_number % num_quorums;
    onid_emul_new_ = random_quorum;

    // (d)
    gamma_send_new_.clear();
    gamma_route_new_.clear();

    // (e)
    gamma_agree_prev_.clear();
    for (auto &i_prime : gamma_route_succ_[onid_emul_prev_]) {
      gamma_agree_prev_[onid_emul_prev_].push_back(i_prime);
    }

    // (f)
    gamma_route_prev_.clear();
    for_all_neighbors(onid_emul_prev_, overlay_dimension_, [this](onid_t
                                                                  neighbor) {
      for (auto &i_prime : gamma_route_succ_[neighbor]) {
        gamma_route_prev_[neighbor].push_back(i_prime);
      }
    });

    // (g)
    gamma_route_succ_.clear();

    // (h)
    msg_out_q[onid_emul_new_].push_back(OverlayStructureSchemeMessage::createEmulateRequestMsg(onid_emul_new_,
                                                                                               own_id_));
  }

  // (4)
  if (round % (reconfiguration_time_ / 2) == 2) {
    gamma_receive_ = std::move(gamma_receive_new_);
    gamma_receive_new_.clear();
    gamma_route_prev_.clear();
    gamma_route_prev_ = gamma_route_;
  }

  // (5)
  if (round % (reconfiguration_time_ / 2) == agreement_time_ + 1) {
    gamma_agree_prev_.clear();
    for (auto &i_prime : gamma_route_[onid_emul_]) {
      gamma_agree_prev_[onid_emul_].push_back(i_prime);
    }
  }

  // (6)
  for (const auto &s : set_s) {
    // (a)
    if (s.t == OverlayStructureSchemeMessage::OverlayStructureSchemeMessageType::tEmulateRequestMsg) {
      // (i)
      if (s.onid != onid_emul_) {
        msg_out_q[s.onid].push_back(s);
      } else {
        // (ii)
        for_all_neighbors(onid_emul_, overlay_dimension_, [&msg_out_q,
            &s](onid_t
                neighbor) {
          msg_out_q[neighbor].emplace_back(OverlayStructureSchemeMessage::createEmulateRequestReceivedMsg(s.onid,
                                                                                                          s.peer_information));
        });
      }
    }

    // (b)
    if (s.t == OverlayStructureSchemeMessage::OverlayStructureSchemeMessageType::tEmulateRequestReceivedMsg) {
      gamma_route_succ_[s.onid].push_back(s.peer_information);
    }

    // (c)
    if (s.t == OverlayStructureSchemeMessage::OverlayStructureSchemeMessageType::tHandOverMsg) {
      for (auto&[onid, ids] : s.gamma_route) {
        for (auto &elem : ids) {
          if (std::find(gamma_route_new_[onid].begin(), gamma_route_new_[onid].end(), elem)
              == gamma_route_new_[onid].end()) { // do not add elements twice
            gamma_route_new_[onid].push_back(elem);
          }
        }
      }
      for (auto &elem : s.gamma_receive) {
        if (std::find(gamma_receive_new_.begin(), gamma_receive_new_.end(), elem)
            == gamma_receive_new_.end()) { // do not add elements twice
          gamma_receive_new_.push_back(elem);
        }
      }
    }

    // (d)
    if (s.t == OverlayStructureSchemeMessage::OverlayStructureSchemeMessageType::tSelfIntroduceMsg) {
      gamma_send_new_.push_back(s.peer_information);
    }
  }

  // (7)
  if (round % (reconfiguration_time_ / 2) == overlay_dimension_ + 2) {
    for (auto &i_prime: gamma_route_succ_[onid_emul_]) {
      s_prime[i_prime].push_back(OverlayStructureSchemeMessage::createHandOverMessage(gamma_route_succ_,
                                                                                      gamma_receive_));
    }
  }

  // (8)
  if (round % (reconfiguration_time_ / 2) == overlay_dimension_ + 3) {
    for (auto &i_prime: gamma_receive_new_) {
      s_prime[i_prime].push_back(OverlayStructureSchemeMessage::createSelfIntroduceMessage(own_id_));
    }
  }

  // (9)
  for (auto&[onid, msgs]: msg_out_q) {
    // determine onid'
    onid_t onid_prime = onid_emul_;
    for (int i = 0; i < overlay_dimension_; ++i) {
      unsigned long i_th_bit_of_own_onid = (onid_emul_ >> i) & 1U;
      unsigned long i_th_bit_of_onid = (onid >> i) & 1U;
      if (i_th_bit_of_onid != i_th_bit_of_own_onid) {
        onid_prime ^=
            (-i_th_bit_of_onid ^ onid_prime) & (1UL << i); // changes the i-th bit of onid_prime to i_th_bit_of_onid
        break;
      }
    }
    for (const auto &msg: msgs) {
      for (const auto &i_prime : gamma_route_[onid_prime]) {
        s_prime[i_prime].push_back(msg);
      }
    }
  }

  // (10)
  auto gamma_route_result = gamma_route_;
  for (const auto&[onid, ids] : gamma_agree_prev_) {
    for (auto &id: ids) {
      if (std::find(gamma_route_result[onid].begin(), gamma_route_result[onid].end(), id)
          == gamma_route_result[onid].end()) { // I know this is inefficient ...
        gamma_route_result[onid].push_back(id);
      }
    }
  }
  for (const auto&[onid, ids] : gamma_route_prev_) {
    for (auto &id: ids) {
      if (std::find(gamma_route_result[onid].begin(), gamma_route_result[onid].end(), id)
          == gamma_route_result[onid].end()) { // I know this is inefficient ...
        gamma_route_result[onid].push_back(id);
      }
    }
  }

  return OverlayReturnTuple{onid_emul_, gamma_agree_, gamma_send_, gamma_route_result, gamma_receive_, s_prime};
}

} // !namespace