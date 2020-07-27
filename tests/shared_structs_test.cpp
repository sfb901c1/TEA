/**
 * Author: Alexander S.
 */

#define BOOST_TEST_MODULE SharedStructsTest
#include <boost/test/included/unit_test.hpp>
#include "../include/message_structs.h"

using namespace boost::unit_test;

BOOST_AUTO_TEST_SUITE(shared_structs_test_suite)

BOOST_AUTO_TEST_CASE(join_message_test) {
  c1::JoinMessage jm(c1::PeerInformation{13, c1::Uri(192,168,178,10, 4711)});

  std::vector<uint8_t> jm_serialized;
  jm.serialize(jm_serialized);

  size_t cur = 0;
  auto jm_deserialized = c1::JoinMessage::deserialize(jm_serialized, cur);

  BOOST_ASSERT(jm_deserialized.sender.id == 13);
  BOOST_ASSERT(jm_deserialized == jm);
}

BOOST_AUTO_TEST_CASE(join_message_via_message_serializer_test) {
  c1::JoinMessage jm(c1::PeerInformation{13, c1::Uri(192,168,178,10, 4711)});

  std::vector<uint8_t> jm_serialized;
  c1::MessageSerializer::serialize_message(jm, jm_serialized);

  size_t cur = 0;
  auto jm_deserialized = c1::MessageSerializer::deserialize_message(jm_serialized, cur);
  BOOST_ASSERT(std::holds_alternative<c1::JoinMessage>(jm_deserialized));

  BOOST_ASSERT(std::get<c1::JoinMessage>(jm_deserialized).sender.id == 13);
  BOOST_ASSERT(std::get<c1::JoinMessage>(jm_deserialized) == jm);
}



BOOST_AUTO_TEST_CASE(init_message_test) {
  std::vector<c1::PeerInformation> gamma_send;
  std::vector<c1::PeerInformation> gamma_receive;
  std::map<uint64_t, std::vector<c1::PeerInformation>> gamma_route;

  gamma_send.push_back(c1::PeerInformation{12, c1::Uri(127, 0, 0, 1, 9999)});
  gamma_route[13] = std::vector<c1::PeerInformation>();
  c1::PeerInformation pi1(c1::PeerInformation{72, c1::Uri(127, 0, 0, 1, 11111)});
  gamma_route[13].push_back(pi1);
  gamma_route[19] = std::vector<c1::PeerInformation>();
  gamma_route[19].push_back(c1::PeerInformation{3, c1::Uri(127, 0, 0, 1, 9876)});

  //Test keys
  std::array<uint8_t, kTee_aesgcm_key_size> sk_pseud;
  std::array<uint8_t, kTee_aesgcm_key_size> sk_enc;
  std::array<uint8_t, kTee_cmac_key_size> sk_routing;
  for (int i=1; i<kTee_aesgcm_key_size; i++) {
    sk_pseud[i] = 0;
    sk_enc[i] = 0;
  }
  for (int i=1; i<kTee_cmac_key_size; i++) {
    sk_routing[i] = 0;
  }
  sk_pseud[0] = 1;
  sk_enc[0] = 2;
  sk_routing[0] = 3;


  c1::InitMessage im(1, 20, 5, 1, 3, gamma_send, gamma_receive,
      gamma_route,
      sk_pseud, sk_enc, sk_routing);

  BOOST_ASSERT(im.get_receiver_id_() == 1);
  BOOST_ASSERT(im.get_num_total_nodes_() == 20);
  BOOST_ASSERT(im.get_overlay_dimension_() == 5);
  BOOST_ASSERT(im.get_onid_assoc_() == 1);
  BOOST_ASSERT(im.get_onid_emul_() == 3);
  BOOST_ASSERT(im.get_gamma_send_().size() == 1);
  BOOST_ASSERT(im.get_gamma_send_().at(0).id == 12);
  BOOST_ASSERT(im.get_gamma_send_().at(0).uri == c1::Uri(127, 0, 0, 1, 9999));
  BOOST_ASSERT(im.get_gamma_route_().size() == 2);
  BOOST_ASSERT(im.get_gamma_route_().at(13).size() == 1);
  BOOST_ASSERT(im.get_gamma_route_().at(19).size() == 1);
  BOOST_ASSERT(im.get_sk_pseud_()[0] == 1);
  BOOST_ASSERT(im.get_sk_enc_()[0] == 2);
  BOOST_ASSERT(im.get_sk_routing_()[0] == 3);

  //auto im_serialized = im.serialize().first;
  std::vector<uint8_t> im_serialized;
  c1::MessageSerializer::serialize_message(im, im_serialized);

  size_t cur = 0;
  auto im_deserialized_var = c1::MessageSerializer::deserialize_message(im_serialized, cur);
  BOOST_ASSERT(std::holds_alternative<c1::InitMessage>(im_deserialized_var));
  auto& im_deserialized = std::get<c1::InitMessage>(im_deserialized_var);


  //c1::InitMessage im_deserialized(im_serialized.get());
  BOOST_ASSERT(im_deserialized.get_receiver_id_() == 1);
  BOOST_ASSERT(im_deserialized.get_num_total_nodes_() == 20);
  BOOST_ASSERT(im_deserialized.get_overlay_dimension_() == 5);
  BOOST_ASSERT(im_deserialized.get_onid_assoc_() == 1);
  BOOST_ASSERT(im_deserialized.get_onid_emul_() == 3);
  BOOST_ASSERT(im_deserialized.get_gamma_send_().size() == 1);
  BOOST_ASSERT(im_deserialized.get_gamma_send_().at(0).id == 12);
  BOOST_ASSERT(im_deserialized.get_gamma_send_().at(0).uri == c1::Uri(127, 0, 0, 1, 9999));
  BOOST_ASSERT(im_deserialized.get_gamma_receive_().size() == 0);
  BOOST_ASSERT(im_deserialized.get_gamma_route_().size() == 2);
  BOOST_ASSERT(im_deserialized.get_gamma_route_().at(13).size() == 1);
  BOOST_ASSERT(im_deserialized.get_gamma_route_().at(13).at(0) == pi1);
  BOOST_ASSERT(im_deserialized.get_gamma_route_().at(19).size() == 1);
  BOOST_ASSERT(im_deserialized.get_sk_pseud_()[0] == 1);
  BOOST_ASSERT(im_deserialized.get_sk_enc_()[0] == 2);
  BOOST_ASSERT(im_deserialized.get_sk_routing_()[0] == 3);

}

BOOST_AUTO_TEST_CASE(peer_information_serialization_test) {
  c1::PeerInformation pi{12, c1::Uri(127, 0, 0, 1, 9999)};
  std::vector<uint8_t> vec;
  pi.serialize(vec);
  size_t cur = 0;
  auto pi2 = c1::PeerInformation::deserialize(vec, cur);
  BOOST_ASSERT(pi == pi2);
}

BOOST_AUTO_TEST_CASE(vector_serialization_test) {
  std::vector<c1::PeerInformation> pi_vec
      {c1::PeerInformation{12, c1::Uri(127, 0, 0, 1, 9999)}, c1::PeerInformation{72, c1::Uri(127, 0, 0, 1, 333)},
       c1::PeerInformation{3, c1::Uri(127, 0, 0, 1, 30)}};
  std::vector<uint8_t> working_vec;
  c1::serialize_vec(working_vec, pi_vec);
  size_t cur = 0;
  auto pi_vec2 = c1::deserialize_vec<c1::PeerInformation>(working_vec, cur);
  BOOST_ASSERT(pi_vec.size() == pi_vec2.size());
  for (int i = 0; i < pi_vec.size(); ++i) {
    BOOST_ASSERT(pi_vec[i] == pi_vec2[i]);
  }

}


BOOST_AUTO_TEST_CASE(peer_information_operators_and_set_test) {
  std::set<c1::PeerInformation> test_set;
  c1::PeerInformation pi(12, c1::Uri(127, 0, 0, 1, 39475));
  test_set.insert(pi);
  c1::PeerInformation pi2(12, c1::Uri(127, 0, 0, 1, 39475));
  test_set.insert(pi2);
  BOOST_ASSERT(test_set.count(pi) == 1);
  int i = 0;
  for (auto &elem : test_set) {
    i++;
  }
  BOOST_ASSERT(i == 1);
}

BOOST_AUTO_TEST_SUITE_END();

