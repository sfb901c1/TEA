/**
 * Author: Alexander S.
 * Trusted part of the login server. I.e., the part that will run inside an enclave.
 * Since the trusted part must be assumed to be very simple, we do not use any external libraries here.
 * This allows for an easy adaptation for real TEEs, such as Intel SGX.
 */

#ifndef _ENCLAVE_H_
#define _ENCLAVE_H_

#include <cstdlib>
#include <cassert>
#include <vector>
#include <string>
#include "../../include/message_structs.h"

namespace c1::login_server {

/**
 * The enclave of the login server.
 * Methods are very similar to those of the client_enclave.
 */
class LoginServerEnclave {
 private:
  LoginServerEnclave() = default;

 public:
  /**
   * Retrieve the login server singleton.
   * @return singleton reference
   */
  static LoginServerEnclave &instance() {
    static LoginServerEnclave INSTANCE;
    return INSTANCE;
  }

  /** Should be called on initialization of the enclave. */
  void init();

  /** Called when a message from a peer has been received. */
  void received_msg_from_peer(const char *ptr, size_t len);

  /**
   * Main loop function. Is supposed to be called regularly in the main while(true) loop.
   * @return true as long as the system has not yet started (after which the login server can stop running).
   */
  int main_loop();

  /**
   * Set the number of peers which the login server waits for until it tells every peer to start.
   * @param num
   */
  void set_num_required_peers(int num);

 private:
  void send_msg_to_peer(const std::string& peer_uri, char *msg_raw, int msg_len) const;
  /** Initiate the start of the system. Should be called when all peers have connected.  */
  void initialize_system();

  /** all peers yet connected */
  std::vector<PeerInformation> peers_; //very simple: each peer gets added with its uri and id
  /** whether the system is already initialized () */
  bool initialized_ = false;
  /** the number of peers required for the system to start running */
  int numRequiredPeers_{};


};

} //~namespace

#if defined(__cplusplus)
extern "C" {
#endif

void ecall_init() { c1::login_server::LoginServerEnclave::instance().init(); }
void ecall_received_msg_from_client(const char *ptr, size_t len) {
  c1::login_server::LoginServerEnclave::instance().received_msg_from_peer(ptr,
                                                                          len);}
int ecall_main_loop() { return c1::login_server::LoginServerEnclave::instance().main_loop(); }
void ecall_kNumRequiredClients(int k) { c1::login_server::LoginServerEnclave::instance().set_num_required_peers(k); }

#if defined(__cplusplus)
}
#endif

#endif /* !_ENCLAVE_H_ */
