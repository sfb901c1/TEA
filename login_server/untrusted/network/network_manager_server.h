/**
 * Author: Alexander S.
 * The NetworkManagerLoginServer handles all network-related tasks for the login server.
 */

#ifndef NETWORK_MANAGER_SERVER_H
#define NETWORK_MANAGER_SERVER_H

#include <zmq.h>
#include <zmq.hpp>
#include <unordered_map>
#include "../../../common/tee_functions.h"

namespace c1::login_server {

/**
 * Struct encapsulating the socket of a peer.
 */
struct Peer {
  zmq::socket_t socket;
  explicit Peer(zmq::socket_t &&socket) : socket(std::move(socket)) {}
};

/**
 * Class to manage all network stuff.
 */
class NetworkManagerLoginServer {
 public:
  /** Constructor. */
  NetworkManagerLoginServer();

  /**
 * Called once the global_sgx_eid is available.
 * @param global_sgx_eid_
 */
  void set_global_tee_eid(tee_enclave_id_t global_sgx_eid);

  /**
   * Set the port of the server. Must be called before the first call of main_loop() to have any effect.
   * @param port
   */
  void set_port(int port);

  /**
 * Called regularly.
 * @return true
 */
  bool main_loop();

  /**
   * Send message at ptr of length len to the peer with uri recipient.
   * @param recipient
   * @param ptr
   * @param len
   */
  void send_msg_to_client(const std::string &recipient, const char *ptr, size_t len);

 private:
  /** zeromq context */
  zmq::context_t context_;
  /** the incoming router socket */
  zmq::socket_t socket_in_;
  /** maps from URI to clients */
  std::unordered_map<std::string, Peer> peers_;
  /** global sgx eid, to be able to make ecalls */
  tee_enclave_id_t global_sgx_eid_;
  /** port of the login_server */
  int port_ = 5671;
  /** poller for the incoming socket */
  zmq::pollitem_t pollitems_[1];
  /** whether main_loop() has already been called or not */
  bool has_started_ = false;
};

} // ~namespace

#endif //NETWORK_MANAGER_SERVER_H
