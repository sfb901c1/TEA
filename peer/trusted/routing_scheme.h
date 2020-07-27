/**
 * Author: Alexander S.
 * The following is a direct implementation of the scheme described in the paper. Therefore, we do not provide a
 * thorough documentation here. To understand the purpose and the realization of this class, please read the paper.
 */

#ifndef ROUTING_SCHEME_H
#define ROUTING_SCHEME_H

#include "../../common/tee_crypto_functions.h"
#include "../../include/misc.h"

namespace c1::peer {

/**
 * The routing scheme (see paper).
 */
class RoutingScheme {
 public:
  /**
   * see paper
   * @param set_s
   * @param cur_round
   * @param overlay_dimension
   * @param sk_routing
   * @return
   */
  static std::vector<RoutingSchemeTuple> route(std::vector<RoutingSchemeTuple> &set_s,
                                               round_t cur_round,
                                               dim_t overlay_dimension,
                                               const tee_cmac_128bit_key_t &sk_routing
  );

 private:
  /** determine all messages that have to be cancelled in the current call of route()
   *  (messages that exceed k_receive for one target bucket are dropped and replaced by M_cancel, see paper)
   */
  static std::set<RoutingSchemeTuple> determine_elements_to_be_cancelled(std::vector<RoutingSchemeTuple> &set_s);
};

} // !namespace


#endif //ROUTING_SCHEME_H
