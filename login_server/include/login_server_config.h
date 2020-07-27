/**
 * Configuration file for the login server.
 */

#ifndef LOGIN_SERVER_CONFIG_H
#define LOGIN_SERVER_CONFIG_H

namespace c1::login_server {

/** currently hardcodes the dimension of the overlay network */
constexpr int kDimension{3};
/** currently hardcodes the desired number of nodes associated to each quorum */
constexpr int kNumNodesPerQuorum{10};
/** the total number of nodes in the overlay network */
constexpr int kNumQuorumNodes = 1 << kDimension;

}

#endif //LOGIN_SERVER_CONFIG_H
