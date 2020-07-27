/**
 * Author: Alexander S.
 */

#define BOOST_TEST_MODULE StructureTest
#include <boost/test/included/unit_test.hpp>
#include "../peer/trusted/structs/aad_tuple.h"

using namespace boost::unit_test;

BOOST_AUTO_TEST_SUITE(peer_test_suite)

BOOST_AUTO_TEST_CASE(number_serialization_test) {
  long x{230985098};
  std::vector<uint8_t> working_vec;
  c1::serialize_number(working_vec, x);

  size_t cur = 0;
  auto y = c1::deserialize_number<decltype(x)>(working_vec, cur);
  BOOST_ASSERT(x == y);
}


BOOST_AUTO_TEST_CASE(aad_serialization_test) {
  c1::peer::AadTuple a1{c1::PeerInformation{12, c1::Uri(127, 0, 0, 1, 9999)},
                          c1::PeerInformation{72, c1::Uri(127, 0, 0, 1, 11111)},
                          12,
                          std::vector<c1::peer::OverlayStructureSchemeMessage>()
  };
  std::vector<uint8_t> vec;
  a1.serialize(vec);
  size_t cur = 0;
  auto a2 = c1::peer::AadTuple::deserialize(vec, cur);
  BOOST_ASSERT(a1 == a2);
}

BOOST_AUTO_TEST_SUITE_END();