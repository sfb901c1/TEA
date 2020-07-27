/**
 * Author: Alexander S.
 * Main class of the untrusted part of the login server. I.e., the part that will run outside an enclave.
 */

#include "server.h"
#include "enclave_u_substitute.h"
#include <pwd.h>
#include <iostream>
#include <thread>

namespace c1::login_server {

LoginServer::LoginServer() : global_eid_(0), network_manager_() {}

int LoginServer::run(int kNumRequiredPeers, int port) {
  ecall_kNumRequiredClients(global_eid_, kNumRequiredPeers);
  ecall_init(global_eid_);

  // Inform the network manager of the global_eid_
  network_manager_.set_global_tee_eid(global_eid_);
  // Set the port to be used by the network manager
  network_manager_.set_port(port);

  std::cout << "Successfully initialized login_server!" << std::endl;

  //main loop
  while (true) {
    if (!network_manager_.main_loop()) {
      break;
    }

    int return_value;
    ecall_main_loop(global_eid_, &return_value);
    if (!return_value) {
      break;
    }

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(0.01s);
  }
  return 0;
}

void LoginServer::send_msg_to_client(const std::string &recipient, const char *msg, size_t msg_len) {
  network_manager_.send_msg_to_client(recipient, msg, msg_len);
}

} // ~namespace

/* OCall functions */
void ocall_print_string(const char *str) {
  /* Proxy/Bridge will check the length and null-terminate the input string to prevent buffer overflow.
   */
  printf("%s", str);
}

void ocall_send_msg_to_peer(const char *client_uri, const char *msg, size_t msg_len) {
  c1::login_server::LoginServer::instance().send_msg_to_client(client_uri, msg, msg_len);
}
