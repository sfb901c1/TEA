/**
 * Author: Alexander S.
 * The NetworkManager handles all network-related tasks for the peer.
 */

#include "network_manager.h"
#include <iostream>
#include <thread>
#include <regex>
#include "enclave_u_substitute.h"

namespace c1::peer {

static constexpr auto HEARTBEAT_INTERVAL = 500;

network_manager::network_manager() : context_{1}, server_socket_out_{context_, ZMQ_DEALER},
                                     server_and_peer_socket_in_{context_, ZMQ_DEALER},
                                     user_socket_{context_, ZMQ_REP},
                                     global_sgx_eid_{0},
                                     pollitems_{{static_cast<void *>(server_and_peer_socket_in_), 0, ZMQ_POLLIN, 0}},
                                     pollitems_user_{{static_cast<void *>(user_socket_), 0, ZMQ_POLLIN, 0}} {
}

int network_manager::get_port_from_uri(const std::string &uri_str) {
  auto colon_index = uri_str.find_last_of(':');
  auto host = uri_str.substr(0, colon_index);
  return stoi(uri_str.substr(colon_index + 1));
}

std::array<uint8_t, 4> network_manager::get_ipv4_from_uri(const std::string &uri_str) {
  std::smatch matches;
  std::regex pat{R"((\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3}):\d{1,5})"};
  if (std::regex_search(uri_str, matches, pat)) {
    return std::array<uint8_t, 4>{static_cast<uint8_t>(std::stoi(matches[1])),
                                  static_cast<uint8_t>(std::stoi(matches[2])),
                                  static_cast<uint8_t>(std::stoi(matches[3])),
                                  static_cast<uint8_t>(std::stoi(matches[4]))};
  }
  throw std::invalid_argument("No correctly formed IPv4 URI string.");
}

bool network_manager::MainLoop() {

  if (!initialized_url_) {
    initialize(5671, "localhost", "127.0.0.1", "0", "*", "*");
  }

  zmq::poll(&pollitems_[0], 1, HEARTBEAT_INTERVAL);

  // check in_socket
  if (pollitems_[0].revents & ZMQ_POLLIN) {
    //std::cout << "(UNTRUSTED) Received message..." << std::endl;
    // receive message
    zmq::message_t msg_content;
    bool rc = server_and_peer_socket_in_.recv(&msg_content);
    assert(rc);
    assert(!msg_content.more());

//    std::cout << "U: " << (rc ? "":"un") << "successfully received message of size " << msg_content.size() << " with first byte: " << std::to_string(*static_cast<uint8_t *>(msg_content.data())) << "...\n";

    if (!initialized_) { // message was sent from login_server
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      ecall_received_msg_from_server(global_sgx_eid_, static_cast<uint8_t *>(msg_content.data()), msg_content.size());
      initialized_ = true;
      /*std::cout << "Initialized everything..." << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      std::cout << "Waited long enough..." << std::endl;*/
    } else { // message was sent from other peer
      //std::cout << "calling traffic_in ..." << std::endl;
      //std::cout << "Received message of length " << msg_content.size() << std::endl;
      //std::cout << "\trc: " << rc << std::endl;
      ecall_traffic_in(global_sgx_eid_, static_cast<uint8_t *>(msg_content.data()), msg_content.size());
      //std::cout << "(untrusted) traffic_in call finished..." << std::endl;
    }
  }

  // check user socket
  zmq::poll(&pollitems_user_[0], 1, 0);
  if (pollitems_user_[0].revents & ZMQ_POLLIN) {
    // received message from user
    zmq::message_t msg_content;
    bool rc0 = user_socket_.recv(&msg_content);
    assert(rc0);
    assert(!msg_content.more());
    //assert(msg_content.size() == 1);
    char message_type = *static_cast<char *>(msg_content.data());
    switch (message_type) {
      case 0: { // generate pseudonym
        assert(msg_content.size() == 1);
        std::array<uint8_t, kPseudonymSize> pseud{};
        int pseudonym_generation_success;
        ecall_generate_pseudonym(global_sgx_eid_, &pseudonym_generation_success, pseud.data());
        pseudonyms.push_back(pseud);

        std::string gen_pseud;
        for (int i = 0; i < pseud.size(); ++i) {
          gen_pseud += std::to_string(pseud[i]) + (i == pseud.size() - 1 ? "" : " ");
        }

        std::cout << "Pseudonym is: " << gen_pseud << std::endl << std::endl;

        // send pseudonym to the user interface
        zmq::message_t message(gen_pseud.length());
        memcpy(message.data(), gen_pseud.c_str(), gen_pseud.length());
        bool rc = user_socket_.send(message);
        if (!rc) { return -1; }

        break;
      }
      case 1: { // send message
        auto injection =
            *reinterpret_cast<UserInterfaceMessageInjectionCommand *>(static_cast<char *>(msg_content.data()) + 1);
        ecall_send_message(global_sgx_eid_, injection.n_src, injection.msg, injection.n_dst, injection.t_dst);
        std::cout << "Msg text is: " << injection.msg << std::endl;
        zmq::message_t message(0);
        bool rc = user_socket_.send(message);
        if (!rc) { return -1; }

        break;
      }
      case 2: { // receive message
        auto injection =
            *reinterpret_cast<UserInterfaceMessageInjectionCommand *>(static_cast<char *>(msg_content.data()) + 1);
        int res;
        ecall_receive_message(global_sgx_eid_, &res, injection.n_dst, injection.msg, injection.n_src, &injection.t_dst);
        if (!res) {
          std::cout << "No message ready!\n";
          const char *msg = "There is no message ready yet!";
          std::copy(msg, msg + kMessageSize, injection.msg);
        } else {
          char msg[kMessageSize];
          std::copy(injection.msg, injection.msg + kMessageSize, msg);
          std::cout << "Received message: " << msg << '\n';
        }
        zmq::message_t message(sizeof(injection));
        memcpy(message.data(), reinterpret_cast<char *>(&injection), sizeof(injection));
        bool rc = user_socket_.send(message);
        if (!rc) { return -1; }
        break;
      }
      case 3: {  // get t_dst lower bound
        int64_t time[2];
        ecall_get_time(global_sgx_eid_, &time[0]);
        ecall_get_t_dst_lower_bound(global_sgx_eid_, &time[1]);
        //std::cout << "Time: " <<  time << '\n';

        zmq::message_t message(2 * sizeof(int64_t));
        memcpy(message.data(), &time[0], 2 * sizeof(int64_t));
        bool rc = user_socket_.send(message);
        if (!rc) { return -1; }
        break;
      }
      case 4: { // retrieve all pseudonyms
        int pseudonym_size = kPseudonymSize * sizeof(decltype(pseudonyms)::value_type::value_type);

        zmq::message_t message(pseudonyms.size() * pseudonym_size);
        auto msg_data_ptr = static_cast<char *>(message.data());
        for (int i = 0; i < pseudonyms.size(); ++i) {
          memcpy(msg_data_ptr + i * pseudonym_size, &pseudonyms[i], pseudonym_size);
        }

        bool rc = user_socket_.send(message);
        if (!rc) { return -1; }
        break;
      }
      case 5: { // retrieve last pseudonym

        std::string gen_pseud;
        for (int i = 0; i < kPseudonymSize; i++) {
          gen_pseud += std::to_string(pseudonyms[pseudonyms.size() - 1][i]) + " ";
        }
        std::cout << gen_pseud << std::endl;

        zmq::message_t message(gen_pseud.length());
        memcpy(message.data(), gen_pseud.c_str(), gen_pseud.length());
        bool rc = user_socket_.send(message);
        if (!rc) { return -1; }
        break;
      }
      default: {
        break;
      }
    }
  }


//server_socket_out_.close();

  return true;
}

void network_manager::set_global_sgx_eid_and_network_init(tee_enclave_id_t global_sgx_eid) {
  network_manager::global_sgx_eid_ = global_sgx_eid;
  ecall_network_init(global_sgx_eid,
                     ip_[0],
                     ip_[1],
                     ip_[2],
                     ip_[3],
                     static_cast<uint64_t>(in_port_));
}

void network_manager::send_msg_to_server(const void *ptr, size_t len) {
  //s_sendmore(server_socket_out_, client_id_);

  zmq::message_t message(len);
  memcpy(message.data(), ptr, len);

  bool rc = server_socket_out_.send(message);
  assert(rc);
  //return (rc);
  //std::cout << "(UNTRUSTED) Sent message to login_server " << rc << std::endl;
}

void network_manager::send_msg_to_peer(const PeerInformation &peer, const uint8_t *ptr, size_t len) {
  // if connection to recipient does not yet exist, establish it
  if (peers_.count(static_cast<const unsigned long &>(peer.id)) < 1) {
//    std::cout << "Establishing connection to peer " << std::string(peer.uri) << std::endl;
    peers_.emplace(peer.id, Peer{zmq::socket_t(context_, ZMQ_DEALER)});
    peers_.at(static_cast<const unsigned long &>(peer.id)).socket.connect("tcp://" + std::string(peer.uri));
  }
  // send message to recipient

  zmq::message_t message(len);
  memcpy(message.data(), ptr, len);

  bool rc = peers_.at(static_cast<const unsigned long &>(peer.id)).socket.send(message);
  assert(rc);

  /*if (peer.id == 0) {
    std::cout << "Sent message to peer 0 with length " << len << std::endl;
    //std::cout << "rc: " << rc << std::endl;
  }*/
  //return (rc);


}

void network_manager::initialize(int port,
                                 const std::string &ip_login_server,
                                 const std::string &ip_self,
                                 const std::string &id_visualization,
                                 const std::string &port_in,
                                 const std::string &interface_port_in) {
  // make sure that the in-ports are either * or a number
  if (port_in != "*") {
    assert(std::all_of(port_in.cbegin(), port_in.cend(), ::isdigit));
  }
  if (interface_port_in != "*") {
    assert(std::all_of(interface_port_in.cbegin(), interface_port_in.cend(), ::isdigit));
  }

  server_and_peer_socket_in_.setsockopt(ZMQ_LINGER, 0);
  user_socket_.setsockopt(ZMQ_LINGER, 0);

  try {
    server_and_peer_socket_in_.bind("tcp://" + ip_self + ":" + port_in);
  }
  catch (zmq::error_t &e) {
    std::cerr << "couldn't bind to server_and_peer_socket_in: " << e.what();
    abort();
  }

  char uri_chars[1024]; //make this sufficiently large. Otherwise an error will be thrown because of invalid argument
  size_t size = sizeof(uri_chars);
  server_and_peer_socket_in_.getsockopt(ZMQ_LAST_ENDPOINT, &uri_chars, &size);
  auto uri_str = std::string(uri_chars);

  in_port_ = get_port_from_uri(uri_str);
  ip_ = get_ipv4_from_uri(uri_str);

  // establish the user interface socket:
  try {
    user_socket_.bind("tcp://" + ip_self + ":" + interface_port_in);
  }
  catch (zmq::error_t &e) {
    std::cerr << "couldn't bind to socket (for user interface): " << e.what();
    abort();
  }
  user_socket_.getsockopt(ZMQ_LAST_ENDPOINT, &uri_chars, &size);
  user_port_ = get_port_from_uri(uri_chars);

  if (interface_port_in == "*") {
    // only in this case the user didn't manually specify the port in advance
    std::cout << "user socket is bound at port: " << user_port_ << std::endl;
  }

  std::string url = ip_login_server + ":" + std::to_string(port);
  server_socket_out_.connect("tcp://" + url);
  server_socket_out_.setsockopt(ZMQ_LINGER, 0);

  initialized_url_ = true;
}

network_manager::~network_manager() {
  server_socket_out_.close();
}
bool network_manager::isInitialized() const {
  return initialized_;
}

} // ~namespace
