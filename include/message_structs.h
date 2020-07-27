/**
 * Author: Alexander S.
 * This file contains structs used for messages between peer and login server and between peer and peer interface.
 */

#ifndef SHARED_STRUCTS_H
#define SHARED_STRUCTS_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstring>
#include <cassert>
#include <array>
#include <variant>
#include "../common/tee_crypto_functions.h"
#include "serialization.h"
#include "config.h"
#include "peer_information.h"

namespace c1 {

enum MsgTypes { kTypeJoinMessage, kTypeInitMessage };

/**
 * Returns the message type of a properly serialized message (where the type is encoded at the first four bytes).
 * @param ptr pointer to the message
 * @return
 */
inline MsgTypes get_message_type(const void *ptr) {
  return static_cast<MsgTypes>(reinterpret_cast<const uint64_t *>(ptr)[0]);
}

/**
 * Message used to register at the login server.
 */
struct JoinMessage : public Serializable {
  PeerInformation sender;

  explicit JoinMessage(const PeerInformation &sender)
      : sender(sender) {};

  static uint8_t get_type() { return kTypeJoinMessage; }

  inline std::string to_string() const {
    return "\t" + std::string(sender) + "\t (sender) \n";
  }

  void serialize(std::vector<uint8_t> &working_vec) const override {
    sender.serialize(working_vec);
  }

  size_t estimate_size() const override {
    return sender.estimate_size();
  }

  static JoinMessage deserialize(const std::vector<uint8_t> &working_vec, size_t &cur) {
    return JoinMessage{PeerInformation::deserialize(working_vec, cur)};
  }

  bool operator==(const JoinMessage &rhs) const {
    return sender == rhs.sender;
  }
  bool operator!=(const JoinMessage &rhs) const {
    return !(rhs == *this);
  }
};;;

/**
 * InitMessage sent from the login server to the peers to supply them with the init data.
 */
class InitMessage : public Serializable {
  int64_t receiver_id_;
  int64_t num_total_nodes_;
  int64_t overlay_dimension_;
  onid_t onid_assoc_;
  onid_t onid_emul_;
  std::vector<PeerInformation> gamma_send_{};
  std::vector<PeerInformation> gamma_receive_{};
  std::map<uint64_t, std::vector<PeerInformation>> gamma_route_{};
  std::array<uint8_t, kTee_aesgcm_key_size> sk_pseud_{};
  std::array<uint8_t, kTee_aesgcm_key_size> sk_enc_{};
  std::array<uint8_t, kTee_cmac_key_size> sk_routing_{};

  // fields containing keys in Intel SGX API form
  tee_aes_gcm_128bit_key_t sk_pseud_sgx_;
  tee_aes_gcm_128bit_key_t sk_enc_sgx_;
  tee_cmac_128bit_key_t sk_routing_sgx_;

 public:
  InitMessage(int64_t receiver_id_,
              int64_t num_total_nodes_,
              int64_t num_quorum_nodes_,
              onid_t onid_assoc_,
              onid_t onid_emul_,
              const std::vector<PeerInformation> &gamma_send_,
              const std::vector<PeerInformation> &gamma_receive_,
              const std::map<onid_t, std::vector<PeerInformation>> &gamma_route_,
              const std::array<uint8_t, kTee_aesgcm_key_size> sk_pseud_,
              const std::array<uint8_t, kTee_aesgcm_key_size> sk_enc_,
              const std::array<uint8_t, kTee_cmac_key_size> sk_routing_)
      : receiver_id_(receiver_id_), num_total_nodes_(num_total_nodes_),
        overlay_dimension_(num_quorum_nodes_),
        onid_assoc_(onid_assoc_), onid_emul_(onid_emul_),
        gamma_send_(gamma_send_),
        gamma_receive_(gamma_receive_),
        gamma_route_(gamma_route_),
        sk_pseud_(sk_pseud_), sk_enc_(sk_enc_), sk_routing_(sk_routing_) {
    copy_crypto_keys();
  }

  static uint8_t get_type() { return kTypeInitMessage; }

  int64_t get_receiver_id_() const {
    return receiver_id_;
  }
  int64_t get_num_total_nodes_() const {
    return num_total_nodes_;
  }

  int64_t get_overlay_dimension_() const {
    return overlay_dimension_;
  }

  onid_t get_onid_assoc_() const {
    return onid_assoc_;
  }

  onid_t get_onid_emul_() const {
    return onid_emul_;
  }

  const std::vector<PeerInformation> &get_gamma_send_() const {
    return gamma_send_;
  }

  const std::vector<PeerInformation> &get_gamma_receive_() const {
    return gamma_receive_;
  }

  const std::map<uint64_t, std::vector<PeerInformation>> &get_gamma_route_() const {
    return gamma_route_;
  }

  tee_aes_gcm_128bit_key_t &get_sk_pseud_() {
    return sk_pseud_sgx_;
  }

  tee_aes_gcm_128bit_key_t &get_sk_enc_() {
    return sk_enc_sgx_;
  }

  tee_cmac_128bit_key_t &get_sk_routing_() {
    return sk_routing_sgx_;
  }

  void serialize(std::vector<uint8_t> &working_vec) const override {
    serialize_number(working_vec, receiver_id_);
    serialize_number(working_vec, num_total_nodes_);
    serialize_number(working_vec, overlay_dimension_);
    serialize_number(working_vec, onid_assoc_);
    serialize_number(working_vec, onid_emul_);
    serialize_vec(working_vec, gamma_send_);
    serialize_vec(working_vec, gamma_receive_);
    serialize_map_of_vecs(working_vec, gamma_route_);
    working_vec.insert(working_vec.end(), sk_pseud_.begin(), sk_pseud_.end());
    working_vec.insert(working_vec.end(), sk_enc_.begin(), sk_enc_.end());
    working_vec.insert(working_vec.end(), sk_routing_.begin(), sk_routing_.end());
  }

  static InitMessage deserialize(const std::vector<uint8_t> &working_vec, size_t &cur) {
    //
    auto receiver_id = deserialize_number<decltype(InitMessage::receiver_id_)>(working_vec, cur);
    auto num_total_nodes = deserialize_number<decltype(InitMessage::num_total_nodes_)>(working_vec, cur);
    auto overlay_dimension = deserialize_number<decltype(InitMessage::overlay_dimension_)>(working_vec, cur);
    auto onid_assoc = deserialize_number<decltype(InitMessage::onid_assoc_)>(working_vec, cur);
    auto onid_emul = deserialize_number<decltype(InitMessage::onid_emul_)>(working_vec, cur);
    auto gamma_send = deserialize_vec<PeerInformation>(working_vec, cur);
    auto gamma_receive = deserialize_vec<PeerInformation>(working_vec, cur);
    auto gamma_route = deserialize_map_of_vecs<uint64_t, PeerInformation>(working_vec, cur);

    std::array<decltype(InitMessage::sk_pseud_)::value_type, kTee_aesgcm_key_size> sk_pseud;
    std::copy_n(std::make_move_iterator(working_vec.begin() + cur), sk_pseud.size(), sk_pseud.begin());
    cur += sk_pseud.size();

    std::array<decltype(InitMessage::sk_enc_)::value_type, kTee_aesgcm_key_size> sk_enc;
    std::copy_n(std::make_move_iterator(working_vec.begin() + cur), sk_enc.size(), sk_enc.begin());
    cur += sk_enc.size();

    std::array<decltype(InitMessage::sk_routing_)::value_type, kTee_aesgcm_key_size> sk_routing;
    std::copy_n(std::make_move_iterator(working_vec.begin() + cur), sk_routing.size(), sk_routing.begin());
    cur += sk_routing.size();

    return InitMessage{receiver_id, num_total_nodes, overlay_dimension, onid_assoc, onid_emul,
                       gamma_send, gamma_receive, gamma_route,
                       sk_pseud, sk_enc, sk_routing};
  };

  size_t estimate_size() const override {
    return sizeof(receiver_id_)
        + sizeof(num_total_nodes_)
        + sizeof(overlay_dimension_)
        + sizeof(onid_assoc_)
        + sizeof(onid_emul_)
        + estimate_vec_size(gamma_send_)
        + estimate_vec_size(gamma_receive_)
        + estimate_map_of_vecs_size(gamma_route_)
        + sizeof(decltype(sk_pseud_)::value_type) * sk_pseud_.size()
        + sizeof(decltype(sk_enc_)::value_type) * sk_enc_.size()
        + sizeof(decltype(sk_routing_)::value_type) * sk_routing_.size();
  }

 private:
  void copy_crypto_keys() {
    memcpy(sk_pseud_sgx_, sk_pseud_.data(), sk_pseud_.size());
    memcpy(sk_enc_sgx_, sk_enc_.data(), sk_enc_.size());
    memcpy(sk_routing_sgx_, sk_routing_.data(), sk_routing_.size());
  }
};

/**
 * Used by the peer interface to obtain the relevant information for a sendMessage.
 */
struct UserInterfaceMessageInjectionCommand {
  uint8_t n_src[kPseudonymSize];
  uint8_t msg[kMessageSize];
  uint8_t n_dst[kPseudonymSize];
  round_t t_dst;
};

class MessageSerializer {
 public:
  template<typename T>
  static void serialize_message(const T &msg, std::vector<uint8_t> &working_vec) {
    uint8_t type = msg.get_type();
    serialize_number<uint8_t>(working_vec, type);
    msg.serialize(working_vec);
  }

  static std::variant<JoinMessage, InitMessage> deserialize_message(const std::vector<uint8_t> &working_vec,
                                                                    size_t &cur) {
    auto type = deserialize_number<uint8_t>(working_vec, cur);
    if (type == kTypeJoinMessage) {
      return JoinMessage::deserialize(working_vec, cur);
    }
    if (type == kTypeInitMessage) {
      auto msg = InitMessage::deserialize(working_vec, cur);
      return msg;
    }
    assert(false);
  }

  template<typename T>
  static size_t estimate_size(const T &msg) {
    return sizeof(uint8_t) + msg.estimate_size();
  }
};

}

#endif //SHARED_STRUCTS_H
