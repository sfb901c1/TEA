/**
 * Author: Alexander S.
 * Main class of the untrusted part of the login server. I.e., the part that will run outside an enclave.
 */

#ifndef SERVER_H
#define SERVER_H

#include "network/network_manager_server.h"
#include <cstdio>
#include "../../common/tee_functions.h"

namespace c1::login_server {

/**
 * Main login server class.
 */
class LoginServer {
 public:
  /**
   * Get singleton of the login server.
   * @return
   */
  static LoginServer &instance() {
    static LoginServer INSTANCE;
    return INSTANCE;

  }

  /**
   * Run the login server.
   * @param kNumRequiredPeers number of peers that the login server waits for until it issues the system to start.
   * @param port port the login server will be listening on.
   * @return 0 on normal termination.
   */
  int run(int kNumRequiredPeers, int port);

  /**
   * Send a message to the peer with uri recipient.
   * @param recipient
   * @param msg
   * @param msg_len
   */
  void send_msg_to_client(const std::string &recipient, const char *msg, size_t msg_len);

 private:
  LoginServer();

  /** Global EID (enclave ID) shared by multiple threads */
  tee_enclave_id_t global_eid_ = 0;

  /** The network manager object for this login server. */
  NetworkManagerLoginServer network_manager_;
};

} // ~namespace


#if defined(__cplusplus)
extern "C" {
#endif

void ocall_print_string(const char *str);
void ocall_send_msg_to_peer(const char *client_uri, const char *msg, size_t msg_len);

#if defined(__cplusplus)
}
#endif

#endif //SERVER_H
