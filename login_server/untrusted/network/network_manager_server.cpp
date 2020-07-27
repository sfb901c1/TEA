/**
 * Author: Alexander S.
 * The NetworkManagerLoginServer handles all network-related tasks for the login server.
 */

#include "network_manager_server.h"
#include "../enclave_u_substitute.h"
#include <iostream>

namespace c1::login_server {

NetworkManagerLoginServer::NetworkManagerLoginServer()
    : context_(1), socket_in_(context_, ZMQ_ROUTER), peers_{}, global_sgx_eid_(0),
      pollitems_{static_cast<void *>(socket_in_), 0, ZMQ_POLLIN, 0} {

}

bool NetworkManagerLoginServer::main_loop() {
  if (!has_started_) {
    socket_in_.bind("tcp://*:" + std::to_string(port_));

    has_started_ = true;
  }
  //static int run{0};

  zmq::poll(&pollitems_[0], 1, 0);

  if (pollitems_[0].revents & ZMQ_POLLIN) {
    // receive identity of peer
    zmq::message_t msg_client_identity;
    socket_in_.recv(&msg_client_identity);
    assert(msg_client_identity.more());

    // receive actual message content
    zmq::message_t msg_content;
    socket_in_.recv(&msg_content);
    assert(!msg_content.more());

    ecall_received_msg_from_client(global_sgx_eid_, reinterpret_cast<char *>(msg_content.data()), msg_content.size());
  }

  return true;
}

void NetworkManagerLoginServer::set_global_tee_eid(tee_enclave_id_t global_sgx_eid) {
  NetworkManagerLoginServer::global_sgx_eid_ = global_sgx_eid;
}

void NetworkManagerLoginServer::set_port(int port) {
  port_ = port;
}

void NetworkManagerLoginServer::send_msg_to_client(const std::string &recipient, const char *ptr, size_t len) {
  // if connection to recipient does not yet exist, establish it
  if (peers_.count(recipient) < 1) {
    peers_.emplace(recipient, Peer{zmq::socket_t(context_, ZMQ_DEALER)});
    peers_.at(recipient).socket.connect("tcp://" + recipient);
  }

  // send message to recipient
  zmq::message_t message(len);
  memcpy(message.data(), ptr, len);
  peers_.at(recipient).socket.send(message);
}

} // ~namespace
