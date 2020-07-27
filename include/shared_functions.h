/**
 * Author: Alexander S.
 * One helper function used in both the server and the client.
 */

#ifndef SHARED_FUNCTIONS_H
#define SHARED_FUNCTIONS_H

#include <cstdint>
#include <functional>
#include "config.h"

namespace c1 {

/**
 * Perform func on all onids that are an id of a neighbor of node_id in the overlay network
 * @param node_id
 * @param dimension the dimension of the overlay network
 * @param func the function to be called on onid' where onid' is a neighbor of node_id in the overlay network
 */
void for_all_neighbors(onid_t node_id, dim_t dimension, std::function<void(onid_t)> func) {
  func(node_id);
  for (dim_t i = 0; i < dimension; ++i) {
    onid_t node_id_i_th_bit_flipped = node_id ^1UL << i;
    func(node_id_i_th_bit_flipped);
  }
}

} // !namespace

#endif //SHARED_FUNCTIONS
