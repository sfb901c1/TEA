/**
 * Author: Alexander S.
 */

#include "server.h"
#include "../../include/CLI11.hpp"

int main(int argc, char *argv[]) {
  using namespace c1::login_server;

  CLI::App app{"Login server"};
  int required_clients = 81;
  int port = 5671;
  app.add_option("-k", required_clients, "Number of the required clients");
  app.add_option("-p", port, "Port the login_server will be listening on");
  CLI11_PARSE(app, argc, argv);

  return LoginServer::instance().run(required_clients, port);
}