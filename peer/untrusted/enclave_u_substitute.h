/**
 * Interface for the into-enclave calls (ecalls).
 * The use of a low-level API (and raw pointers) is motivated by the assumption that a TEE will provide only such
 * capabilities and is in fact inspired by the way Intel SGX implements the trusted / untrusted relationship.
 */


#ifndef PEER_ENCLAVE_U_SUBSTITUTE_H
#define PEER_ENCLAVE_U_SUBSTITUTE_H

#include "../../common/tee_functions.h"

tee_status_t ecall_init(tee_enclave_id_t eid);
tee_status_t ecall_network_init(tee_enclave_id_t eid,
                                uint8_t ip1,
                                uint8_t ip2,
                                uint8_t ip3,
                                uint8_t ip4,
                                uint64_t port);
tee_status_t ecall_received_msg_from_server(tee_enclave_id_t eid, const uint8_t *ptr, size_t len);
tee_status_t ecall_generate_pseudonym(tee_enclave_id_t eid, int *retval, uint8_t pseudonym[8]);
tee_status_t ecall_send_message(tee_enclave_id_t eid,
                                uint8_t n_src[8],
                                uint8_t msg[4],
                                uint8_t n_dst[8],
                                int64_t t_dst);
tee_status_t ecall_receive_message(tee_enclave_id_t eid,
                                   int *retval,
                                   uint8_t n_dst[8],
                                   uint8_t msg[4],
                                   uint8_t n_src[8],
                                   int64_t *t_dst);
tee_status_t ecall_traffic_out(tee_enclave_id_t eid, int *retval);
tee_status_t ecall_traffic_in(tee_enclave_id_t eid, const uint8_t *ptr, size_t len);
tee_status_t ecall_get_time(tee_enclave_id_t eid, int64_t *retval);
tee_status_t ecall_get_t_dst_lower_bound(tee_enclave_id_t eid, int64_t *retval);

#endif //PEER_ENCLAVE_U_SUBSTITUTE_H
