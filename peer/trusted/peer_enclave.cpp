/**
 * Author Alexander S.
 * Most of this implementation (in particular, the traffic_in/out(), send/receive_message() methods) probably cannot be
 * understood without reading the paper first and are not supposed to be.
 */


#include "peer_enclave.h"
#include "structs/aad_tuple.h"
#include "routing_scheme.h"
#include "../../common/cryptlib.h"
#include "../include/vis_data.h"
#include "../../include/receiver_blob_pair.h"

// the following assert is defined via old-style DEFINE means because we cannot assume std::string to be available
// inside the enclave (which for, e.g., Intel SGX is not the case)
#define ASSERT(x) \
  if (!(x)) { \
    ocall_print_string("assert " #x "; failed\n"); \
  } \
  assert (x);

const bool ABORT_ON_DELAYED_MESSAGE = false;

namespace c1::peer {

void ClientEnclave::init() {
  // Additional initialization stuff could be added here
}

void ClientEnclave::network_init(uint8_t ip1, uint8_t ip2, uint8_t ip3, uint8_t ip4, uint64_t port) {
  own_id_.uri = Uri{ip1, ip2, ip3, ip4, port};
  // in a practical realization, some encryption would be needed to be added here

  JoinMessage join_msg(own_id_);
  std::vector<uint8_t> join_msg_serialized;
  join_msg_serialized.reserve(MessageSerializer::estimate_size(join_msg));
  MessageSerializer::serialize_message(join_msg, join_msg_serialized);
  try_and_send_msg_to_server(reinterpret_cast<void *>(join_msg_serialized.data()), join_msg_serialized.size());

  //ocall_print_string("Peer sent join message!\n");
}

void ClientEnclave::received_msg_from_login_server(const uint8_t *msg_ptr, size_t msg_len) {
  std::vector<uint8_t> working_vec(msg_ptr, msg_ptr + msg_len);
  size_t cur = 0;
  auto msg = MessageSerializer::deserialize_message(working_vec, cur);

  if (std::holds_alternative<InitMessage>(msg)) {
    PRINT_CPP_STRING("Received init msg from login_server...\n");
    auto current_time = TeeFunctions::tee_get_trusted_time();
    init_time_ = current_time;

    auto &init_message = std::get<InitMessage>(msg);

    memcpy(sk_pseud_, init_message.get_sk_pseud_(), kTee_aesgcm_key_size);
    memcpy(sk_enc_, init_message.get_sk_enc_(), kTee_aesgcm_key_size);
    memcpy(sk_routing_, init_message.get_sk_routing_(), kTee_cmac_key_size);

    overlay_dimension_ = init_message.get_overlay_dimension_();
    onid_repr_ = init_message.get_onid_assoc_();

    own_id_.id = init_message.get_receiver_id_();

    TeeFunctions::seed(own_id_.id);

    overlay_structure_scheme_.init(init_message.get_onid_assoc_(),
                                   init_message.get_onid_emul_(),
                                   init_message.get_gamma_send_(),
                                   init_message.get_gamma_receive_(),
                                   init_message.get_gamma_route_(),
                                   overlay_dimension_,
                                   (overlay_dimension_ + 4) * 2,
                                   calculate_agreement_time(m_corrupt_),
                                   own_id_);

    max_quorum_size_ = calculate_max_quorum_size(init_message.get_num_total_nodes_());
    //PRINT_CPP_STRING("Calculated max_quorum_size is: " + std::to_string(max_quorum_size_) + '\n');

    max_routing_msg_out_ = calculate_max_routing_out(max_quorum_size_, overlay_dimension_);

    m_corrupt_ = calculate_m_corrupt(init_message.get_num_total_nodes_());

    initialized_ = true;

//    ocall_print_string("PeerEnclave initialized Overlay Structure Scheme. \n");
  }
}

void ClientEnclave::try_and_send_msg_to_server(void *msg_raw, size_t msg_len) const {
  // encryption would take place here

  ocall_send_msg_to_server(reinterpret_cast<uint8_t *>(msg_raw), msg_len);
}

bool ClientEnclave::generate_pseudonym(uint8_t *pseudonym) {
  memset(pseudonym, 0, kPseudonymSize);
  if (pseudonyms_.size() >= kAMax) { // too many pseudonyms already
    ocall_print_string("Maximum number of pseudonyms exceeded...\n");
    return false;
  }

  DecryptedPseudonym pseud{onid_repr_, own_id_, static_cast<uint8_t>(pseudonyms_.size())};

  std::vector<uint8_t> pseud_vec;
  pseud.serialize(pseud_vec);
//  ocall_print_string(("Pseudonym vector has size: " + std::to_string(pseud_vec.size()) + '\n').c_str());
  std::vector<uint8_t> aad_vec;
  auto encrypted_pseudonym = cryptlib::encrypt(sk_pseud_, pseud_vec, aad_vec);
//  ocall_print_string(("Encrypted pseudonym vector has size: " + std::to_string(encrypted_pseudonym.size())
//      + '\n').c_str());

  std::copy(encrypted_pseudonym.begin(),
            encrypted_pseudonym.begin() + std::min(kPseudonymSize, encrypted_pseudonym.size()),
            pseudonym);

  pseudonyms_.push_back(pseud);
  pseudonyms_enc_.push_back(encrypted_pseudonym);
  num_q_out_entries_for_round_for_pseudonym_.resize(num_q_out_entries_for_round_for_pseudonym_.size() + 1);
  q_in_for_pseudonyms_.resize(q_in_for_pseudonyms_.size() + 1);
  return true;
}

void ClientEnclave::send_message(uint8_t n_src[kPseudonymSize],
                                 uint8_t msg[kMessageSize],
                                 uint8_t n_dst[kPseudonymSize],
                                 int64_t t_dst) {
  if (!initialized_) {
    return;
  }

  ocall_print_string("Send_message called!\n");

  auto pseud_n_src_decr = decrypt_pseudonym(Pseudonym{n_src});
  auto pseud_n_src = Pseudonym{n_src};
  auto msg_msg = Message{msg};
  auto pseud_n_dst = Pseudonym{n_dst};

  if (std::find(pseudonyms_.begin(), pseudonyms_.end(), pseud_n_src_decr) == pseudonyms_.end()) {
    // this node does not have pseudonym n_src, abort
    ocall_print_string("Source pseudonym does not exist at this node!\n");
    return;
  }

  if (t_dst < get_t_dst_lower_bound()) {
//  if (get_time() > t_dst
//      - (calculate_agreement_time(m_corrupt_) + calculate_routing_time(overlay_dimension_) + 4) * 4 * kDelta) {
    // message is too late, abort
    ocall_print_string("Message is too late! Canceled ...\n");
    return;
  }

  ocall_print_string("Message is not too late. It's being processed!\n");

  auto l_dst = calculate_round_from_t(t_dst);
  auto &num_entries_for_round =
      num_q_out_entries_for_round_for_pseudonym_.at(pseud_n_src_decr.get_local_num());
  if (num_entries_for_round.count(static_cast<const unsigned long &>(l_dst)) != 0
      && num_entries_for_round[l_dst] >= kSend) {
    // too many message for that round already sent
    ocall_print_string("Message limit for that round was exceeded ...\n");
    return;
  }
  MessageTuple m{pseud_n_src, msg_msg, pseud_n_dst, t_dst};
  q_out_.push(m);
  if (num_entries_for_round.count(static_cast<const unsigned long &>(l_dst)) == 0) {
    num_entries_for_round[l_dst] = 1;
  } else {
    num_entries_for_round[l_dst] += 1;
  }
  ocall_print_string("Message successfully scheduled for injection!\n");
}

int ClientEnclave::receive_message(uint8_t *n_dst,
                                   uint8_t *msg,
                                   uint8_t *n_src,
                                   int64_t *t_dst) {
  if (!initialized_) {
    return false;
  }

  ocall_print_string("Trying to receive message...\n");

  // check whether the decrypted version of the pseudonym does exist...
  bool found = false;
  for (auto enc_pseud : pseudonyms_enc_) {
    found = true;
    for (size_t i = 0; i < kPseudonymSize; ++i) {
      if (i < enc_pseud.size()) {
        if (enc_pseud[i] != n_dst[i]) {
          found = false;
          break;
        }
      } else if (n_dst[i] != 0) {
        found = false;
        break;
      }
    }
    if (found) {
      break;
    }
  }
  if (!found) {
    ocall_print_string("This pseudonym does not exist!\n");
    return false;
  }

  auto pseud_n_dst = decrypt_pseudonym(Pseudonym{n_dst});
  if (std::find(pseudonyms_.begin(), pseudonyms_.end(), pseud_n_dst) == pseudonyms_.end()) {
    // this node does not have pseudonym n_dst, abort
    ocall_print_string("This pseudonym does not exist (double-check)!\n");
    return false;
  }

  if (q_in_for_pseudonyms_.at(pseud_n_dst.get_local_num()).empty()) {
    ocall_print_string("No message ready!\n");
    return false;
  }

  auto &message_tuple = q_in_for_pseudonyms_.at(pseud_n_dst.get_local_num()).top();
  if (message_tuple.t_dst > get_time()) {
    // even the message with lowest t_dst is not due yet, abort
    ocall_print_string("No message ready!\n");
    return false;
  }
  std::copy(message_tuple.m.get().data(), message_tuple.m.get().data() + kMessageSize, msg);
  std::copy(message_tuple.n_src.get().data(), message_tuple.n_src.get().data() + kPseudonymSize, n_src);
  *t_dst = message_tuple.t_dst;

  q_in_for_pseudonyms_.at(pseud_n_dst.get_local_num()).pop();
  return true;
}

int ClientEnclave::traffic_out() {
//  ocall_print_string("Beginning of traffic_out()\n");

//  PRINT_CPP_STRING("Invokation of traffic_out... time: " + std::to_string(get_time()) +"\n");

  if (!initialized_) {
    return false;
  }

  typedef std::map<PeerInformation, std::vector<MessageTuple>> default_out_t;
  // std::vector<PeerBlobPair> out_structure; // will be declared later
  std::map<PeerInformation, std::vector<AnnouncementTuple>> out_announce;
  std::map<PeerInformation, std::vector<AgreementTuple>> out_agreement;
  default_out_t out_inject;
  //std::map<PeerInformation, std::vector<RoutingSchemeTuple>> out_routing;
  std::unordered_map<PeerInformation, std::vector<RoutingSchemeTuple>> out_routing;
  default_out_t out_predeliver;
  default_out_t out_deliver;
  std::vector<RoutingSchemeTuple> s_routing;

  //ocall_print_string("test2\n");

  // establish a round model
  auto subround = calculate_subround_from_t(get_time());
  if (subround % 4 != 0) {
    return false;
  }

  if (subround / 4 == cur_round_) {
    return false; // we already had a call of traffic_out this round...
  }

  ocall_print_string("\n==========================================================================\n");
  ocall_print_string(std::string("\nsubround: " + std::to_string(subround) + "\n").c_str());
  ocall_print_string(std::string("this round is: " + std::to_string((cur_round_ + 1)) + "\n").c_str());

  ASSERT(cur_round_ == (subround / 4) - 1) // otherwise, I am corrupted - so quit
  cur_round_++;

  ocall_print_string(("traffic_out called ... in round " + std::to_string(cur_round_) + " (time "
      + std::to_string(get_time()) + "). " +
      "Message can be sent for t = " + std::to_string(get_t_dst_lower_bound()) + "\n\n").c_str());

  //run overlay maintenance
  auto overlay_result = overlay_structure_scheme_.update(cur_round_ + 1, in_structure_.at(0));
  gamma_agree_for_round_.push_front(overlay_result.gamma_agree);
  if (static_cast<round_t>(gamma_agree_for_round_.size()) > calculate_agreement_time(m_corrupt_)) {
    gamma_agree_for_round_.pop_back(); // do not store more than L_agreement entries
  }
  auto &onid_emul_l_now = overlay_result.onid_emul;
  auto &onid_emul_l_prev = onid_emul_;
  onid_emul_ = onid_emul_l_now;
  auto &out_structure = overlay_result.s_overlay_prime;
  auto &gamma_route = overlay_result.gamma_route;

//  ocall_print_string("test4\n");
  //announce outgoing messages
  while (!q_out_.empty() && q_out_.top().is_due(cur_round_, overlay_dimension_, m_corrupt_)) {
    ocall_print_string("Announcing a message!\n");
    for (auto &peer : overlay_result.gamma_send) {
      out_announce[peer].emplace_back(q_out_.top(), onid_repr_);
    }
    q_out_.pop();
  }

//  ocall_print_string("test5\n");
  // start agreement for (other nodes') outgoing messages
  for (auto &announce: in_announce_[0]) {
//    ocall_print_string("I received an announce message\n");
    agreement_tuples_.emplace_back(AgreementLocalTuple{announce.m, true, DistributedAgreementScheme(),
                                                       gamma_agree_for_round_.at(1), announce.onid_src, 0});
  }

//  ocall_print_string("test6\n");
  // update running agreement schemes
  for (auto &agreement: in_agreement_[0]) {
    if (agreement.l < calculate_agreement_time(m_corrupt_) - 1) {
      bool found = false;
      for (auto &elem: agreement_tuples_) {
        if (elem.message == agreement.m) {
          found = true;
          break;
        }
      }
      if (!found) {
        agreement_tuples_.emplace_back(AgreementLocalTuple{agreement.m, false, DistributedAgreementScheme(),
                                                           gamma_agree_for_round_.at(static_cast<unsigned long>(agreement.l)),
                                                           agreement.onid_src,
                                                           agreement.l - 1});
      }
    }
  }
  for (auto &agreement_tuple: agreement_tuples_) {
    // compute s_m
    std::vector<PeerInformation> s_m;
    for (auto &agreement_in: in_agreement_[0]) {
      if (agreement_tuple.message == agreement_in.m) {
        s_m.push_back(agreement_in.s);
      }
    }
    // call update
    auto new_agreement_messages = agreement_tuple.agreement_scheme.update(agreement_tuple.aware,
                                                                          agreement_tuple.participating_nodes,
                                                                          own_id_,
                                                                          s_m,
                                                                          agreement_tuple.round + 1);
    // update tuple
    agreement_tuple.round++;
    // reset state
    agreement_tuple.aware = false;
    // add messages to out
    for (auto &new_agreement_message : new_agreement_messages) {
      out_agreement[new_agreement_message.receiver].emplace_back(AgreementTuple{agreement_tuple.message,
                                                                                agreement_tuple.onid_src,
                                                                                new_agreement_message.aware_node,
                                                                                agreement_tuple.round + 1});
    }
  }

  // finalize finished agreement scheme runs
  for (auto &agreement_tuple : agreement_tuples_) {
    if (agreement_tuple.round == calculate_agreement_time(m_corrupt_) - 1) {
      // compute s_m
      std::vector<PeerInformation> s_m;
      for (auto &agreement_in: in_agreement_[0]) {
        if (agreement_tuple.message == agreement_in.m) {
          s_m.push_back(agreement_in.s);
        }
      }
      // call finalize
      bool v = agreement_tuple.agreement_scheme.finalize(own_id_,
                                                         s_m,
                                                         0); // TODO change to correct value after high performance test
      if (!v) {
        continue; // v = 0, do nothing
      }
      // deletion see below

      for (auto &id: gamma_route.at(agreement_tuple.onid_src)) {
        out_inject[id].emplace_back(agreement_tuple.message);
      }
    }
  }
  // remove the tuples of the current round (did not do this during the loop for performance reasons)
  auto it = std::remove_if(agreement_tuples_.begin(), agreement_tuples_.end(),
                           [this](const AgreementLocalTuple &agreement_tuple) {
                             return agreement_tuple.round == calculate_agreement_time(m_corrupt_) - 1;
                           });

  agreement_tuples_.erase(it, agreement_tuples_.end());


  // inject (other nodes') message into routing
  if (!in_inject_[0].empty()) {
    auto in_inject_filtered = obtain_elements_that_exceed_m_corrupt<MessageTuple>(in_inject_[0]);
//    PRINT_CPP_STRING("I have received that many inject messages: " + std::to_string(in_inject_filtered.size()) + '\n');

    for (const auto &inject_message : in_inject_filtered) {
//      PRINT_CPP_STRING("I have a valid injection message...\n");
      if (inject_message.is_dummy()) {
        continue; // ignore this message
      }
      PRINT_CPP_STRING("...which is not a dummy...\n");
      auto onid_dst = decrypt_pseudonym(inject_message.n_dst).get_onid_repr();
      auto onid_src = decrypt_pseudonym(inject_message.n_src).get_onid_repr();
      s_routing.emplace_back(RoutingSchemeTuple{inject_message,
                                                onid_dst,
                                                inject_message.n_dst,
                                                cur_round_ + calculate_routing_time(overlay_dimension_),
                                                onid_src
      });
    }
  }

//  ocall_print_string("test8\n");
  // route messages
//  PRINT_CPP_STRING("Currently waiting there are " + std::to_string(in_routing_[0].size()) + " routing messages.\n");
  auto s_primes =
      obtain_elements_that_exceed_m_corrupt<std::vector<RoutingSchemeTuple>>(
          in_routing_[0]);
//  PRINT_CPP_STRING("s_primes: " + std::to_string(s_primes.size()) + "\n");
  std::vector<RoutingSchemeTuple> v_set;
  for (const auto &s_uppercase : s_primes) {
    for (const auto &s_lowercase : s_uppercase) {
      v_set.push_back(s_lowercase);
//      PRINT_CPP_STRING("s.l_dst: " + std::to_string(s_lowercase.l_dst) + "\n" );
    }
  }
  for (const auto &v : v_set) {
    if (cur_round_ < v.l_dst) {
      s_routing.push_back(v);
    }
  }
  auto s_routing_prime = RoutingScheme::route(s_routing, cur_round_, overlay_dimension_, sk_routing_);
  for (const auto &[onid, peers] : gamma_route) {
    for (const auto &i : peers) {
      std::vector<RoutingSchemeTuple> s_prime_onid_current;
      std::copy_if(s_routing_prime.begin(), s_routing_prime.end(), std::back_inserter(s_prime_onid_current),
                   [&, onid = onid](const auto elem) { return elem.onid_current == onid; });
      ASSERT (out_routing.count(i) == 0)
      out_routing[i] = s_prime_onid_current;
    }
  }

//  ocall_print_string("test9\n");
  // run pre-delivery majority vote
  for (const auto &v : v_set) {
    if (v.l_dst == cur_round_) {
      ocall_print_string("We do send a non-dummy predeliver-message ... \n");
      for (const auto &i : gamma_route[onid_emul_l_prev])
        out_predeliver[i].emplace_back(v.m);
    }
  }

//  ocall_print_string("test10\n");
  // deliver messages to final destination
  auto set_of_predeliver_messages = obtain_elements_that_exceed_m_corrupt<MessageTuple>(
      in_predeliver_[0]);
  for (const auto &message : set_of_predeliver_messages) {
    if (!message.is_dummy()) {
      ocall_print_string("We have received a non-dummy predeliver-message ... \n");
      out_deliver[decrypt_pseudonym(message.n_dst).get_peer_information()].emplace_back(message);
    }
  }

  // add dummy messages
  for (const auto &i : overlay_result.gamma_send) { // announce type
    ASSERT (out_announce[i].size() <= kSend * kAMax)
    out_announce[i].reserve(kSend * kAMax);
    while (out_announce[i].size() < kSend * kAMax) {
      out_announce[i].emplace_back(MessageTuple::create_dummy(), onid_repr_);
    }
  }

  size_t max_temp_routing_out = 0;
  for (const auto&[onid, ids] : overlay_result.gamma_route) { // routing type
    for (const auto &i : ids) {
      ASSERT (out_routing[i].size() <= max_routing_msg_out_)
      if (out_routing[i].size() > max_temp_routing_out) {
        max_temp_routing_out = out_routing[i].size();
      }

      out_routing[i].reserve(max_routing_msg_out_);
      while (out_routing[i].size() < max_routing_msg_out_) {
        //ocall_print_string("creating dummy... \n");
        out_routing[i].emplace_back(RoutingSchemeTuple::create_dummy());
      }
    }
  }
  //PRINT_CPP_STRING("max out_routing size is: " + std::to_string(max_temp_routing_out) + '\n');

  for (const auto &i : overlay_result.gamma_route[onid_emul_l_prev]) { // predeliver type
    ASSERT (out_predeliver[i].size() <= kRecv * kAMax * overlay_result.gamma_receive.size())
    out_predeliver[i].reserve(kRecv * kAMax * overlay_result.gamma_receive.size());
    while (out_predeliver[i].size() < kRecv * kAMax * overlay_result.gamma_receive.size()) {
      out_predeliver[i].emplace_back(MessageTuple::create_dummy());
    }
  }

  for (const auto &i : overlay_result.gamma_receive) { // deliver type
    ASSERT (out_deliver[i].size() <= kRecv * kAMax)
    if (!out_deliver[i].empty()) {
      PRINT_CPP_STRING(
          "I have " + std::to_string(out_deliver[i].size()) + "deliver messages for " + std::to_string(i.id) + "\n");
    }

    out_deliver[i].reserve(kRecv * kAMax);
    while (out_deliver[i].size() < kRecv * kAMax) {
      out_deliver[i].emplace_back(MessageTuple::create_dummy());
    }
  }

  //PRINT_CPP_STRING("OUT_ROUTING_SIZE: " + std::to_string(out_routing.size()));

  // encrypt and authenticate outgoing data
  std::set<PeerInformation> all_i;
  for (const auto &[key, _] : out_announce) { all_i.insert(key); }
  for (const auto &[key, _] : out_agreement) { all_i.insert(key); }
  for (const auto &[key, _] : out_inject) { all_i.insert(key); }
  for (const auto &[key, _] : out_routing) { all_i.insert(key); }
  for (const auto &[key, _] : out_predeliver) { all_i.insert(key); }
  for (const auto &[key, _] : out_deliver) { all_i.insert(key); }
  for (const auto &[key, _] : out_structure) { all_i.insert(key); }

  std::vector<ReceiverBlobPair> i_c_pairs;
  i_c_pairs.reserve(all_i.size());
  for (auto &i : all_i) {

    // compute p_i
    std::vector<uint8_t> p_i_serialized;
    auto p_i_serialized_estimated = estimate_vec_size(out_announce[i])
        + estimate_vec_size(out_agreement[i])
        + estimate_vec_size(out_inject[i])
        + estimate_vec_size(out_routing[i])
        + estimate_vec_size(out_predeliver[i])
        + estimate_vec_size(out_deliver[i]);
    p_i_serialized.reserve(p_i_serialized_estimated);

//    if (out_announce.count(i) == 0) { ocall_print_string("Out_announce does not exist\n"); }
    serialize_vec(p_i_serialized, out_announce[i]);
//    if (out_agreement.count(i) == 0) { ocall_print_string("out_agreement does not exist\n"); }
    serialize_vec(p_i_serialized, out_agreement[i]);
//    if (out_inject.count(i) == 0) { ocall_print_string("out_inject does not exist\n"); }
    serialize_vec(p_i_serialized, out_inject[i]);
//    if (out_routing.count(i) == 0) { ocall_print_string("out_routing does not exist\n"); }
    serialize_vec(p_i_serialized, out_routing[i]);
//    if (out_predeliver.count(i) == 0) { ocall_print_string("out_predeliver does not exist\n"); }
    serialize_vec(p_i_serialized, out_predeliver[i]);
//    if (out_deliver.count(i) == 0) { ocall_print_string("out_deliver does not exist\n"); }
    serialize_vec(p_i_serialized, out_deliver[i]);

    //PRINT_CPP_STRING("estimated p_i_serialized size was: " + std::to_string(p_i_serialized_estimated) + " and new size is: "
    //  + std::to_string(p_i_serialized.size())+'\n');

//    ocall_print_string("serializing aad...\n");
    // compute aad_i
//    PRINT_CPP_STRING("Receiver is: " + std::string(i) + '\n');

    auto aad_i = AadTuple{own_id_, i, cur_round_ + 1, out_structure[i]};
    std::vector<uint8_t> aad_i_serialized;
    aad_i_serialized.reserve(aad_i.estimate_size());
    aad_i.serialize(aad_i_serialized);

    if constexpr (kDisplay_traffic_debug_messages) {
      if (std::tuple(
          out_announce[i].size(),
          out_agreement[i].size(),
          out_inject[i].size(),
          out_routing[i].size(),
          out_predeliver[i].size(),
          out_deliver[i].size())
          == std::tuple(
              0, 0, 0,
              0, 0, 0)) {
      } else {
        PRINT_CPP_STRING("sending msg to " + std::to_string(i.id));
        //    ocall_print_string(("Sending msg to " + std::to_string(i.id) + ", p_ser.size() = "
        //        + std::to_string(p_i_serialized.size()) + " and aad_i_ser.size() = "
        //        + std::to_string(aad_i_serialized.size()) + "\n").c_str());
        if (!out_announce[i].empty()) {
          ocall_print_string(("(announce:" + std::to_string(out_announce[i].size()) + ")").c_str());
        }
        if (!out_agreement[i].empty()) {
          ocall_print_string(("(agreement:" + std::to_string(out_agreement[i].size()) + ")").c_str());
        }
        if (!out_inject[i].empty()) {
          ocall_print_string(("(inject:" + std::to_string(out_inject[i].size()) + ")").c_str());
        }
        if (!out_routing[i].empty()) {
          ocall_print_string(("(routing:" + std::to_string(out_routing[i].size()) + ")").c_str());
        }
        if (!out_predeliver[i].empty()) {
          ocall_print_string(("(predeliver:" + std::to_string(out_predeliver[i].size()) + ")").c_str());
        }
        if (!out_deliver[i].empty()) {
          ocall_print_string(("(deliver:" + std::to_string(out_deliver[i].size()) + ")").c_str());
        }
        //}
        PRINT_CPP_STRING("\n");
      }
      ////    print_cppstring(", ");
    }

    // compute c_i
    i_c_pairs.emplace_back(ReceiverBlobPair{i, cryptlib::encrypt(sk_enc_, p_i_serialized, aad_i_serialized)});
  } //~all_i
//  ocall_print_string("\n");

//  ocall_print_string(("Size of i_c_pairs is: " + std::to_string(i_c_pairs.size()) + "\n").c_str());

  std::vector<uint8_t> output;
  output.reserve(estimate_vec_size(i_c_pairs));
  serialize_vec(output, i_c_pairs);

  // move all in[1] to in[0]
  in_structure_[0] = std::move(in_structure_[1]);
  in_structure_[1].clear();
  in_announce_[0] = std::move(in_announce_[1]);
  in_announce_[1].clear();
  in_agreement_[0] = std::move(in_agreement_[1]);
  in_agreement_[1].clear();
  in_inject_[0] = std::move(in_inject_[1]);
  in_inject_[1].clear();
  in_routing_[0] = std::move(in_routing_[1]);
  in_routing_[1].clear();
  in_predeliver_[0] = std::move(in_predeliver_[1]);
  in_predeliver_[1].clear();
  in_deliver_[0] = std::move(in_deliver_[1]);
  in_deliver_[1].clear();
  traffic_in_received_from_[0] = std::move(traffic_in_received_from_[1]);
  traffic_in_received_from_[1].clear();

  // actually return the output
  ocall_traffic_out_return(output.data(), output.size());


  //determine whether a message has to be sent:
  bool has_waiting_message = false;
  for (const auto &pseud: num_q_out_entries_for_round_for_pseudonym_) {
    for (const auto&[round, num] : pseud) {
      if (num > 0 && round >= calculate_round_from_t(get_t_dst_lower_bound())) {
        has_waiting_message = true;
      }
    }
  }

  bool has_delivered_unready_message = false;
  bool has_delivered_ready_message = false;
  for (const auto &q_in : q_in_for_pseudonyms_) {
    if (!q_in.empty()) {
      if (q_in.top().t_dst > get_time()) {
        has_delivered_unready_message = true;
      } else {
        has_delivered_ready_message = true;
      }
    }
  }

#ifdef BUILD_WITH_VISUALIZATION
  VisData vis_data(own_id_,
                   onid_repr_,
                   cur_round_,
                   overlay_result,
                   out_announce,
                   out_agreement,
                   out_inject,
                   out_routing,
                   out_predeliver,
                   out_deliver,
                   has_waiting_message,
                   has_delivered_unready_message,
                   has_delivered_ready_message);
  std::vector<uint8_t> vis_data_serialized;
  vis_data.serialize(vis_data_serialized);
  ocall_vis_data(vis_data_serialized.data(), vis_data_serialized.size());
#endif

  ocall_print_string("Finished TrafficOut()...\n");

  return true;
}

void ClientEnclave::traffic_in(const uint8_t *ptr, size_t len) {
//  ocall_print_string("Traffic_in called!\n");
  if (!initialized_) {
    return;
  }

  // decrypt data
  std::vector<uint8_t> data;
  data.insert(data.end(), ptr, ptr + len);
  auto[p_decrypted, aad_decrypted] = cryptlib::decrypt(sk_enc_, data);
  if (p_decrypted.empty() && aad_decrypted.empty()) {
    ocall_print_string("Decrypted message is empty!\n");
    return;
  }

  // deserialize aad
  size_t cur_aad = 0;
  auto aad = AadTuple::deserialize(aad_decrypted, cur_aad);

  // deserialize p
  size_t cur_p = 0;
  auto p_announce = deserialize_vec<AnnouncementTuple>(p_decrypted, cur_p);
  auto p_agreement = deserialize_vec<AgreementTuple>(p_decrypted, cur_p);
  auto p_inject = deserialize_vec<MessageTuple>(p_decrypted, cur_p);
  auto p_routing = deserialize_vec<RoutingSchemeTuple>(p_decrypted, cur_p);
  auto p_predeliver = deserialize_vec<MessageTuple>(p_decrypted, cur_p);
  auto p_deliver = deserialize_vec<MessageTuple>(p_decrypted, cur_p);

  if (aad.receiver != own_id_) {
    PRINT_CPP_STRING("Received a misguided message ... actual target is: " + std::string(aad.receiver) + '\n');
    PRINT_CPP_STRING("Sender is: " + std::string(aad.sender) + '\n');
    return; // message was misguided
  }

  if (aad.round <= cur_round_) {
    ocall_print_string("Received a message that was sent too late ...\n");
    if constexpr(ABORT_ON_DELAYED_MESSAGE) {
      assert(false);
    }
    return;
  }

  auto cur_or_next = aad.round - cur_round_ - 1; // compute whether in[0] or in[1] needs to be used
  ASSERT(cur_or_next < 2)

  if (traffic_in_received_from_[cur_or_next][aad.sender]) {
    ocall_print_string("Received a message a second time ...\n");
    // possible replay attack (message was already received)
    return;
  }

  traffic_in_received_from_[cur_or_next][aad.sender] = true;

  in_announce_[cur_or_next].insert(std::end(in_announce_[cur_or_next]), std::begin(p_announce), std::end(p_announce));
  in_agreement_[cur_or_next].insert(std::end(in_agreement_[cur_or_next]),
                                    std::begin(p_agreement),
                                    std::end(p_agreement));
  in_inject_[cur_or_next].insert(std::end(in_inject_[cur_or_next]), std::begin(p_inject), std::end(p_inject));
  in_routing_[cur_or_next].push_back(std::move(p_routing));
  in_predeliver_[cur_or_next].insert(std::end(in_predeliver_[cur_or_next]),
                                     std::begin(p_predeliver),
                                     std::end(p_predeliver));
  in_structure_[cur_or_next].insert(std::begin(aad.p_structure),
                                    std::end(aad.p_structure));
  in_deliver_[cur_or_next].insert(std::begin(p_deliver),
                                  std::end(p_deliver));

  {
    // remove dummies from routing
    auto it = std::remove_if(in_routing_[cur_or_next].begin(),
                             in_routing_[cur_or_next].end(),
                             [](const std::vector<RoutingSchemeTuple> &routing_vec) {
                               return routing_vec.empty();
                             });
    in_routing_[cur_or_next].erase(it, in_routing_[cur_or_next].end());
  }

  // deliver message
  if (traffic_in_received_from_[cur_or_next].size() > m_corrupt_) {
    for (auto &message : in_deliver_[cur_or_next]) {
      if (!message.is_dummy()) {
        auto &q_in = q_in_for_pseudonyms_.at(decrypt_pseudonym(message.n_dst).get_local_num());
        if (q_in.find(message) == q_in.last()) {
          //ocall_print_string("I actually received a message!!!\n");
          //PRINT_CPP_STRING("Message is: " + message.to_string() + '\n');
          q_in.push(message);
        }
      }
    }
  }
//  ocall_print_string("TrafficIn() finished!\n");
  //ocall_print_string("\n");
}

round_t ClientEnclave::get_time() const {
  if (!initialized_) {
    return 0;
  }

  auto current_time = TeeFunctions::tee_get_trusted_time();
  auto time = current_time - init_time_;
  ASSERT(time >= 0)
  return time;
}

round_t ClientEnclave::get_t_dst_lower_bound() const {
  if (!initialized_) {
    return 0;
  }

  return get_time() + (calculate_agreement_time(m_corrupt_)
      + calculate_routing_time(overlay_dimension_) + 4) * 4 * kDelta;

}

DecryptedPseudonym ClientEnclave::decrypt_pseudonym(const c1::peer::Pseudonym &pseudonym) const {
  std::vector<uint8_t> pseud_vec(pseudonym.get().data(), pseudonym.get().data() + pseudonym.get().size());
  auto pseud_decr = cryptlib::decrypt(sk_pseud_, pseud_vec);

  size_t cur = 0;
  return DecryptedPseudonym::deserialize(pseud_decr.first, cur);
}

template<typename T>
std::vector<T> ClientEnclave::obtain_elements_that_exceed_m_corrupt(const std::vector<T> &vec) const {
  std::map<T, size_t> elems_u_counter;
  for (const auto &elem_t : vec) {
    if (elems_u_counter.count(elem_t) == 0) {
      elems_u_counter[elem_t] = 0;
    }
    elems_u_counter[elem_t]++;
  }

  std::vector<T> result;

  for (const auto &[elem, count] : elems_u_counter) {
    if (count > m_corrupt_) {
      result.push_back(elem);
    }
  }

  return result;
}

} // ~namespace

#if defined(__cplusplus)
extern "C" {
#endif

void ecall_init() {
  c1::peer::ClientEnclave::instance().init();
}

void ecall_network_init(uint8_t ip1, uint8_t ip2, uint8_t ip3, uint8_t ip4, uint64_t port) {
  c1::peer::ClientEnclave::instance().network_init(ip1, ip2, ip3, ip4, port);
}

void ecall_received_msg_from_server(const uint8_t *ptr, size_t len) {
  c1::peer::ClientEnclave::instance().received_msg_from_login_server(ptr, len);
}

int ecall_generate_pseudonym(uint8_t pseudonym[kPseudonymSize]) {
  return c1::peer::ClientEnclave::instance().generate_pseudonym(pseudonym);
}

void ecall_send_message(uint8_t n_src[kPseudonymSize],
                        uint8_t msg[kMessageSize],
                        uint8_t n_dst[kPseudonymSize],
                        int64_t t_dst) {
  c1::peer::ClientEnclave::instance().send_message(n_src, msg, n_dst, t_dst);
}

int ecall_receive_message(uint8_t n_dst[kPseudonymSize],
                          uint8_t msg[kMessageSize],
                          uint8_t n_src[kPseudonymSize],
                          int64_t *t_dst) {
  return c1::peer::ClientEnclave::instance().receive_message(n_dst, msg, n_src, t_dst);
}

int ecall_traffic_out() {
  return c1::peer::ClientEnclave::instance().traffic_out();
}

void ecall_traffic_in(const uint8_t *ptr, size_t len) {
  c1::peer::ClientEnclave::instance().traffic_in(ptr, len);
}

int64_t ecall_get_time() {
  return c1::peer::ClientEnclave::instance().get_time();
}

int64_t ecall_get_t_dst_lower_bound() {
  return c1::peer::ClientEnclave::instance().get_t_dst_lower_bound();
}

#if defined(__cplusplus)
}
#endif
