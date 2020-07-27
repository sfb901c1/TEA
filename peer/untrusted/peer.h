/**
 * Author: Alexander S.
 * Main class for the untrusted part (i.e., running outside an enclave) of the peer.
 */

#ifndef PEER_H
#define PEER_H

#include "network/network_manager.h"
#include <cstdio>

namespace c1::peer {

/** Base peer class */
class Client {
 public:
  /**
   * Access the singleton.
   * @return
   */
  static Client &instance() {
    static Client INSTANCE;
    return INSTANCE;
  }

  void set_vis_ip(const std::string &vis_ip);

  void initialize_network_manager(int port,
                                  const std::string &ip_login_server,
                                  const std::string &ip_self,
                                  const std::string &id_visualization,
                                  const std::string &port_in,
                                  const std::string &interface_port_in);

  /** main loop (infinite) */
  int run();

  void send_msg_to_server(const void *ptr, size_t len);
  /**
   * Used to return the message pairs from traffic_out() (was necessary due to the enclave relationship)
   * @param ptr ptr to the return value
   * @param len its length
   */
  void traffic_out_return(const uint8_t *ptr, size_t len);

  /**
   * Used to send data to the visualization server. Does nothing if use_visualization_ is set to false.
   * @param ptr
   * @param len
   */
  void vis_data(const uint8_t *ptr, size_t len);

 private:
  Client();

  /* Global EID shared by multiple threads */
  tee_enclave_id_t global_eid_ = 0;
  network_manager network_manager_;
  std::string vis_ip_;
  bool visualization_on_ = false;
};

} // ~namespace

#if defined(__cplusplus)
extern "C" {
#endif

void ocall_print_string(const char *str);
void ocall_send_msg_to_server(const uint8_t *ptr, size_t len);
void ocall_traffic_out_return(const uint8_t *ptr, size_t len);
void ocall_vis_data(const uint8_t *ptr, size_t len);

#if defined(__cplusplus)
}
#endif

#endif //PEER_H
