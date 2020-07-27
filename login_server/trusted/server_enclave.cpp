/**
 * Author: Alexander S.
 * Trusted part of the login server. I.e., the part that will run inside an enclave.
 * Since the trusted part must be assumed to be very simple, we do not use any external libraries here.
 * This allows for an easy adaptation for real TEEs, such as Intel SGX.
 */


#include "server_enclave.h"
#include "enclave_t_substitute.h"
#include "tee_functions.h"
#include "cryptlib.h"
#include "../../include/shared_functions.h"
#include "../include/login_server_config.h"

namespace c1::login_server {

void LoginServerEnclave::init() {
  // Additional code could go here if necessary
  ocall_print_string("LoginServerEnclave initialized!\n");
}

void LoginServerEnclave::received_msg_from_peer(const char *ptr, size_t len) {
  std::vector<uint8_t> working_vec(ptr, ptr + len);
  size_t cur = 0;
  auto msg = MessageSerializer::deserialize_message(working_vec, cur);
  if (std::holds_alternative<JoinMessage>(msg)) {
    const JoinMessage &received_message = std::get<JoinMessage>(msg);
    ocall_print_string("LoginServerEnclave received join message from: \n");
    ocall_print_string(received_message.to_string().c_str());

    if (peers_.size() < numRequiredPeers_) {
      peers_.emplace_back(received_message.sender);
      ocall_print_string(
          ("Current number of registered peers: " + std::to_string(peers_.size()) + "\n").c_str());
      if (peers_.size() >= numRequiredPeers_) {
        initialize_system();
      }
    }
  }
}

int LoginServerEnclave::main_loop() {
  return !initialized_;
}

void LoginServerEnclave::send_msg_to_peer(const std::string& peer_uri, char *msg_raw, int msg_len) const {
  ocall_send_msg_to_peer(peer_uri.c_str(), msg_raw, msg_len);
}

void LoginServerEnclave::initialize_system() {
  ocall_print_string("Ready to initialize the system.\n");

  //Generate keys
  std::array<uint8_t, kTee_aesgcm_key_size> sk_pseud = c1::cryptlib::keygen();
  std::array<uint8_t, kTee_aesgcm_key_size> sk_enc = c1::cryptlib::keygen();
  std::array<uint8_t, kTee_cmac_key_size> sk_routing = c1::cryptlib::gen_routing_key();

  //Initialize overlay schemes
  for (int i = 0; i < peers_.size(); ++i) {
    peers_[i].id = i;
  }

  std::vector<std::vector<PeerInformation>> associated_quorums; // stores, for each quorum, the associated nodes
  std::vector<std::vector<PeerInformation>>
      emulated_quorums(kNumQuorumNodes); // stores, for each quorum, the nodes emulating this quorum
  std::vector<uint64_t> peers_associated_quorums
      (numRequiredPeers_); // stores redundant data, used for efficiency, maps each peer to its associated quorum
  std::vector<uint64_t> peers_emulated_quorums
      (numRequiredPeers_); // stores redundant data, used for efficiency, maps each peer to the quorum it emulates

  // determine a mapping for peers to associated quorums
  for (int i = 0; i < kNumQuorumNodes; ++i) {
    associated_quorums.emplace_back();

    for (int j = i * kNumNodesPerQuorum;
         j < (i + 1) * kNumNodesPerQuorum || (i == kNumQuorumNodes - 1 && j < numRequiredPeers_); ++j) {
      associated_quorums.at(i).push_back(peers_.at(j));
      peers_associated_quorums[j] = i;
    }
  }
  // note: in practice, the login server would not need to be seeded - here we just use a fixed value for now
  TeeFunctions::seed(4711);

  // determine the mapping of peers to quorums they emulate
  for (int i = 0; i < numRequiredPeers_; ++i) {
    uint64_t random_number;
    TeeFunctions::tee_read_rand(&random_number);
    uint64_t random_quorum = random_number % kNumQuorumNodes;
    emulated_quorums.at(random_quorum).push_back(peers_.at(i));
    peers_emulated_quorums[i] = random_quorum;
  }

  // output, for all quorums, which peers they are emulated by
  for (int i = 0; i < kNumQuorumNodes; ++i) {
    assert(!emulated_quorums.at(i).empty());
    ocall_print_string((std::string("Quorum ") + std::to_string(i) + " is emulated by: ").c_str());
    for (const auto &client : emulated_quorums.at(i)) {
      ocall_print_string((std::to_string(client.id) + ", ").c_str());
    }
    ocall_print_string("\n");
  }

  // output, for all quorums, which peers are associated to them
  for (int i = 0; i < kNumQuorumNodes; ++i) {
    assert(!associated_quorums.at(i).empty());
    ocall_print_string((std::string("Nodes associated with quorum ") + std::to_string(i) + ": ").c_str());
    for (const auto &client : associated_quorums.at(i)) {
      ocall_print_string((std::to_string(client.id) + ", ").c_str());
    }
    ocall_print_string("\n");
  }

//  ocall_print_string(("gamma_send quorum of node 0: " + std::to_string(peers_associated_quorums[0]) + '\n').c_str());
//  ocall_print_string(("gamma_receive quorum of node 0: " + std::to_string(peers_emulated_quorums[0]) + '\n').c_str());
//  ocall_print_string("gamma_route neighbor quorums of node 0: ");
//  for_all_neighbors(peers_emulated_quorums[0],
//                    kDimension,
//                    [&emulated_quorums](uint64_t neighbor_quorum) {
//                      ocall_print_string((std::to_string(neighbor_quorum) + ", ").c_str());
//                    });
//  ocall_print_string("\n");

  // send the init message to every peer:
  for (int i = 0; i < numRequiredPeers_; ++i) {

    // gamma_send : all nodes that emulate the quorum node that i is associated with
    std::vector<PeerInformation> gamma_send = emulated_quorums.at(peers_associated_quorums[i]);
    // gamma_receive: all nodes that are associated with the quorum node that i emulates
    std::vector<PeerInformation> gamma_receive = associated_quorums.at(peers_emulated_quorums[i]);
    std::map<uint64_t, std::vector<PeerInformation>> gamma_route;

    for_all_neighbors(peers_emulated_quorums[i],
                      kDimension,
                      [&gamma_route, &emulated_quorums](uint64_t neighbor_quorum) {
                        gamma_route[neighbor_quorum] = emulated_quorums.at(neighbor_quorum);
                      });

    InitMessage init_message(peers_[i].id, numRequiredPeers_, kDimension,
                             peers_associated_quorums[i], peers_emulated_quorums[i],
                             gamma_send, gamma_receive, gamma_route, sk_pseud, sk_enc, sk_routing);

    std::vector<uint8_t> init_message_serialized;
    init_message_serialized.reserve(MessageSerializer::estimate_size(init_message));
    MessageSerializer::serialize_message(init_message, init_message_serialized);
    send_msg_to_peer(peers_[i].uri,
                     reinterpret_cast<char *>(init_message_serialized.data()),
                     init_message_serialized.size());
  }
  initialized_ = true;
}

void LoginServerEnclave::set_num_required_peers(int num) {
  numRequiredPeers_ = num;
}

} // ~namespace
