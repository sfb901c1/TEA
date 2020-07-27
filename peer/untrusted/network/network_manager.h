/**
 * Author: Alexander S.
 * The NetworkManager handles all network-related tasks for the peer.
 */

#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <zmq.h>
#include <zmq.hpp>
#include <unordered_map>
#include "../../../include/message_structs.h"

namespace c1::peer {

/**
 * Struct encapsulating the socket of a peer.
 */
struct Peer {
  zmq::socket_t socket;
  Peer(zmq::socket_t &&socket) : socket(std::move(socket)) {}
};

/**
 * Class to manage all network-related aspects.
 */
class network_manager {
 public:
  /**
   * Constructor.
   */
  network_manager();
  /**
   * Destructor.
   */
  virtual ~network_manager();

  /**
   * Called once the global_sgx_eid is available.
   * @param global_sgx_eid_
   */
  void set_global_sgx_eid_and_network_init(tee_enclave_id_t global_sgx_eid);

  /**
   * Called regularly.
   * @return true
   */
  bool MainLoop();

  /**
   * Send message at ptr of length len to the login_server.
   * @param ptr
   * @param len
   */
  void send_msg_to_server(const void *ptr, size_t len);
  /**
   * Send message at ptr of length len to the peer given by peer.
   * @param peer
   * @param ptr
   * @param len
   */
  void send_msg_to_peer(const PeerInformation &peer, const uint8_t *ptr, size_t len);

  void initialize(int port,
                  const std::string &ip_login_server,
                  const std::string &ip_self,
                  const std::string &id_visualization,
                  const std::string &port_in,
                  const std::string &interface_port_in);

 private:
  /** zeromq context */
  zmq::context_t context_;
  /** outgoing socket to the login server */
  zmq::socket_t server_socket_out_;
  /** the incoming socket for all messages from login_server (prior to initialization) and other peers (after initialization) */
  zmq::socket_t server_and_peer_socket_in_;
  /** the incoming socket for messages from the peer interface */
  zmq::socket_t user_socket_;
  /** global sgx eid, to be able to make ecalls */
  tee_enclave_id_t global_sgx_eid_;
  /** poller for the login_server and peer in-socket */
  zmq::pollitem_t pollitems_[1];
  /** poller for the peer interface in-socket */
  zmq::pollitem_t pollitems_user_[1];
  /** port of server_and_peer_socket_in_ */
  int in_port_{};
  /** port of user_socket_ */
  int user_port_{};
  /** the id used for the visualization */
  std::string id_visualization_;
  /** the ip stored as an array (because the enclave needs it that way) */
  std::array<uint8_t, 4> ip_{};
  /** maps PeerInformation to peers */
  std::unordered_map<uint64_t, Peer> peers_;
  /** whether the system has already been initialized (login server's work is done, all peers have joined the system) */
  bool initialized_ = false;
  /** tells whether initialize() has been called */
  bool initialized_url_ = false;

  std::vector<std::array<uint8_t, kPseudonymSize>> pseudonyms;

 public:
  /**
   *  returns whether the system has already been initialized (login_server's work is done, all peers have joined the system)
   * @return
   */
  bool isInitialized() const;
 private:

  /**
   * Get only the port from an URI string
   * @param uri_str
   * @return
   */
  static int get_port_from_uri(const std::string &uri_str);

  /**
   * Get an IPv4 address from an URI string (if the address is IPv4)
   * @param uri_str
   * @return the IPv4 address as an array of four ints
   */
  static std::array<uint8_t, 4> get_ipv4_from_uri(const std::string &uri_str);
};

} //!namespace


#endif //NETWORK_MANAGER_H
