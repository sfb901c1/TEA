/**
 * Author: Alexander S.
 * Main class for the untrusted part (i.e., running outside an enclave) of the peer.
 */

#include "peer.h"
#include <iostream>
#ifdef BUILD_WITH_VISUALIZATION
#include <json/json.h>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include "../include/vis_data.h"
#endif
#include "enclave_u_substitute.h"
#include "../../include/receiver_blob_pair.h"

namespace c1::peer {

Client::Client() : global_eid_(0), network_manager_() {}

void Client::set_vis_ip(const std::string &vis_ip) {
  vis_ip_ = vis_ip;
  visualization_on_ = true;
}

void Client::initialize_network_manager(int port,
                                        const std::string &ip_login_server,
                                        const std::string &ip_self,
                                        const std::string &id_visualization,
                                        const std::string &port_in,
                                        const std::string &interface_port_in) {
  network_manager_.initialize(port, ip_login_server, ip_self, id_visualization, port_in, interface_port_in);
}

int Client::run() {
  ecall_init(global_eid_);

  /* Inform the network manager of the global_eid_ */
  network_manager_.set_global_sgx_eid_and_network_init(global_eid_);

  //main loop
  for (auto start = std::chrono::system_clock::now(); true;) { // infinite (main!) loop
    if (!network_manager_.MainLoop()) {
      break;
    }
    if (!network_manager_.isInitialized()) {
      continue; // no response from the login_server yet, so don't call traffic_out yet
    }

    std::chrono::duration<double> time_diff = std::chrono::system_clock::now() - start;
    //   if (time_diff.count() >= static_cast<double>(kDelta) / 2.0) {
    start = std::chrono::system_clock::now();
    int ret_val;
    ecall_traffic_out(global_eid_, &ret_val);
    if (std::round(std::chrono::duration<double>(std::chrono::system_clock::now() - start).count()) > 0) {
      std::cout << "TrafficOut took time: "
                << std::round(std::chrono::duration<double>(std::chrono::system_clock::now() - start).count())
                << "\n";
    }
  }

  printf("Info: SampleEnclave successfully returned.\n");

//    printf("Enter a character before exit ...\n");
//    getchar();
  return 0;
}

void Client::send_msg_to_server(const void *ptr, size_t len) {
  network_manager_.send_msg_to_server(ptr, len);
}

void Client::traffic_out_return(const uint8_t *ptr, size_t len) {
  size_t cur = 0;
  std::vector<uint8_t> working_vec(ptr, ptr + len);

  auto i_c_pairs = deserialize_vec<ReceiverBlobPair>(working_vec, cur);
  for (auto &i_c : i_c_pairs) {
    network_manager_.send_msg_to_peer(i_c.i, i_c.payload.data(), i_c.payload.size());
  }

}

void Client::vis_data(const uint8_t *ptr, size_t len) {
#ifdef BUILD_WITH_VISUALIZATION
  if (!visualization_on_) {
    return;
  }

  size_t cur = 0;
  std::vector<uint8_t> working_vec(ptr, ptr + len);

  auto vis_data_deserialized = VisData::deserialize(working_vec, cur);
  auto vis_data_json = vis_data_deserialized.to_json();

  Json::StreamWriterBuilder builder;
  builder.settings_["indentation"] = "";
  std::string vis_data_json_string = Json::writeString(builder, vis_data_json);

  try {
    curlpp::Cleanup cleaner;
    curlpp::Easy request;

    std::string url = "http://" + vis_ip_ + ":8080/";
    //std::string url = "http://funda.cs.upb.de/test.php";

    request.setOpt(new curlpp::options::Url(url));
    request.setOpt(new curlpp::options::Timeout(2));
    //request.setOpt(new curlpp::options::Verbose(true));

    std::list<std::string> header;
    header.emplace_back("Content-Type: application/application/json");

    request.setOpt(new curlpp::options::HttpHeader(header));

    request.setOpt(new curlpp::options::PostFields(vis_data_json_string));
    request.setOpt(new curlpp::options::PostFieldSize(-1));

    request.perform();
  }
  catch (curlpp::LogicError &e) {
    std::cout << e.what() << std::endl;
  }
  catch (curlpp::RuntimeError &e) {
    std::cout << e.what() << std::endl;
  }

  std::cout << "\n";
#endif
}

} //~namespace



/* ocall function to print a string */
void ocall_print_string(const char *str) {
  /* Proxy/Bridge will check the length and null-terminate
   * the input string to prevent buffer overflow.
   */
  printf("%s", str);
}

/* ocall function to send a message to the login server */
void ocall_send_msg_to_server(const uint8_t *ptr, size_t len) {
  //std::cout << "Called send_msg_to_server (outside class)" << std::endl;
  c1::peer::Client::instance().send_msg_to_server(ptr, len);
}

/* ocall for a callback of traffic_out */
void ocall_traffic_out_return(const uint8_t *ptr, size_t len) {
  c1::peer::Client::instance().traffic_out_return(ptr, len);
}

/* ocall function to handle the visualization data */
void ocall_vis_data(const uint8_t *ptr, size_t len) {
  c1::peer::Client::instance().vis_data(ptr, len);
}
