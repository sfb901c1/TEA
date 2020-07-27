/**
 * Author: Alexander S.
 */

#include <regex>
#include "peer.h"
#include "../../include/CLI11.hpp"

int main(int argc, char *argv[]) {
  using namespace c1::peer;

  CLI::App app{"Peer"};
  int port_login_server = 5671;
  // the following are default values, to be overwritten by the command line arguments
  std::string ip_login_server = "127.0.0.1";
  std::string ip_self = "127.0.0.1";
  std::string ip_visualization;
  std::string id_visualization = "-1";
  std::string port_in = "*";
  std::string interface_port_in = "*";
  app.add_option("-p,--port-login-server", port_login_server, "Port of login server");
  app.add_option("-l,--ip-login-server", ip_login_server, "Ip of login server");
  app.add_option("-o,--ip-self", ip_self, "Own ip");
  app.add_option("-v,--ip-visualization", ip_visualization, "Visualization ip");
  app.add_option("-s,--vis-id", id_visualization, "Own self given id (for the visualization)");
  app.add_option("-i,--port-in", port_in, "In port (for login server and peers) that this peer is listening on");
  app.add_option("-c,--port-interface-in",
                 interface_port_in,
                 "In port (for the client interface) that this peer is listening on");
  CLI11_PARSE(app, argc, argv)

  std::regex pat{R"(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})"};
  if (!std::regex_match(ip_login_server, pat)) {
    std::cout << "Error: Incorrect Ip address" << std::endl;
    return 0;
  }

  if (!ip_visualization.empty()) {
    Client::instance().set_vis_ip(ip_visualization);
  }

  Client::instance().initialize_network_manager(port_login_server,
                                                ip_login_server,
                                                ip_self,
                                                id_visualization,
                                                port_in, interface_port_in);
  return Client::instance().run();
}