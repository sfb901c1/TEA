/**
 * Author: Alexander S.
 * Some helper function used in the implementation of the procedure described by the paper.
 */

#ifndef HELPERS_H
#define HELPERS_H
#include <cstdint>
#include <queue>
#include <cmath>
#include "../../include/misc.h"
#include "enclave_t_substitute.h"

namespace c1::peer {

/**
 * Convenience function to print a string.
 * @param str the cpp string to be printed
*/
#define PRINT_CPP_STRING(str) \
  ocall_print_string(std::string(str).c_str())

/**
 * Calculates the maxmium quorum size based on the total number of nodes.
 * Typically, the result will be used in the calculate_max_routing_out function.
 * @param num_nodes
 * @return
 */
inline size_t calculate_max_quorum_size(int64_t num_nodes) {
  return static_cast<size_t>(std::ceil((1 + 1.0 / (kX * kX)) * std::pow(log2(num_nodes), 1.0 + kEpsilon))) + 2;
  // TODO TEMP the next line is the actual size, we use the upper line just to compensate for the fact that our n is too small
  return static_cast<size_t>(std::ceil((1 + 1.0 / (kX * kX)) * std::pow(log2(num_nodes), 1.0 + kEpsilon)));
}

/**
 * Calculcate the maximum size of a single routing_out element (messages for routing_out to be send to a single peer).
 * @param max_quorum_size the maximum quorum size
 * @param overlay_dimension the dimension of the hypercube
 * @return
 */
inline size_t calculate_max_routing_out(size_t max_quorum_size, dim_t overlay_dimension) {
  return 10;
  // TODO TEMP the above line is a temporary workaround against the huge size.. when running on different systems with
  // enough memory the following formula would have to be used:
  auto b_max = max_quorum_size * kAMax;
  return static_cast<size_t>(std::ceil(
      8 * std::pow(overlay_dimension, kEpsilon + 2) * kRecv * b_max));
}

/**
 * Calculate the maximum number of corrupted nodes m_corrupt (see paper) based on the total number of nodes.
 * @param num_nodes
 * @return
 */
inline size_t calculate_m_corrupt(int64_t num_nodes) {
  return static_cast<size_t>(std::ceil(0.5 * (1.0 - 1.0 / (kX * kX)) * std::pow(log2(num_nodes), 1.0 + kEpsilon))) - 4;
  // note: the upper calculation is to compensate for the fact that our n is too small and that for single-machine runs
  // the values must not get too big; an optimal choice would be the following (which should make a huge difference
  // for large n, though):
  // return static_cast<size_t>(std::floor(0.5 * (1.0 - 1.0 / (kX * kX)) * std::pow(log2(num_nodes), 1.0 + kEpsilon)));
}

/**
 * calculate L_agreement (see paper) based on maximum number of corrupted nodes
 * @param m_corrupt
 * @return
 */
inline round_t calculate_agreement_time(size_t m_corrupt) {
  return m_corrupt + 2;
}

/**
 * calculate L_routing (see paper) based on the dimension of the overlay network
 * @param dim
 * @return
 */
inline round_t calculate_routing_time(dim_t dim) {
  return 2 * dim;
}

/**
 * returns the l such that t in [4lDelta,(l+1)4Delta)
 * @param t
 * @return
 */
inline round_t calculate_round_from_t(round_t t) {
  return t / (4 * kDelta);
}

/**
 * returns the l such that t in [lDelta,(l+1)Delta)
 * @param t
 * @return
 */
inline round_t calculate_subround_from_t(round_t t) {
  return t / kDelta;
}

} //!namespace


#endif //HELPERS_H
