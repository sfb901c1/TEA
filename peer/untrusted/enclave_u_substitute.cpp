/**
 * Interface for the into-enclave calls (ecalls).
 * The use of a low-level API (and raw pointers) is motivated by the assumption that a TEE will provide only such
 * capabilities and is in fact inspired by the way Intel SGX implements the trusted / untrusted relationship.
 */


#include "enclave_u_substitute.h"

#if defined(__cplusplus)
extern "C" {
#endif

void ecall_init();
void ecall_network_init(uint8_t ip1, uint8_t ip2, uint8_t ip3, uint8_t ip4, uint64_t port);
void ecall_received_msg_from_server(const uint8_t *ptr, size_t len);
int ecall_generate_pseudonym(uint8_t pseudonym[8]);
void ecall_send_message(uint8_t n_src[8], uint8_t msg[4], uint8_t n_dst[8], int64_t t_dst);
int ecall_receive_message(uint8_t n_dst[8], uint8_t msg[4], uint8_t n_src[8], int64_t *t_dst);
int ecall_traffic_out();
void ecall_traffic_in(const uint8_t *ptr, size_t len);
int64_t ecall_get_time();
int64_t ecall_get_t_dst_lower_bound();

#ifdef __cplusplus
}
#endif /* __cplusplus */

tee_status_t ecall_init(tee_enclave_id_t eid) {
  ecall_init();
}

tee_status_t ecall_network_init(tee_enclave_id_t eid,
                                uint8_t ip1,
                                uint8_t ip2,
                                uint8_t ip3,
                                uint8_t ip4,
                                uint64_t port) {
  ecall_network_init(ip1, ip2, ip3, ip4, port);
}

tee_status_t ecall_received_msg_from_server(tee_enclave_id_t eid, const uint8_t *ptr, size_t len) {
  ecall_received_msg_from_server(ptr, len);
}

tee_status_t ecall_generate_pseudonym(tee_enclave_id_t eid, int *retval, uint8_t pseudonym[8]) {
  *retval = ecall_generate_pseudonym(pseudonym);
}

tee_status_t ecall_send_message(tee_enclave_id_t eid,
                                uint8_t n_src[8],
                                uint8_t msg[4],
                                uint8_t n_dst[8],
                                int64_t t_dst) {
  ecall_send_message(n_src, msg, n_dst, t_dst);
}

tee_status_t ecall_receive_message(tee_enclave_id_t eid,
                                   int *retval,
                                   uint8_t n_dst[8],
                                   uint8_t msg[4],
                                   uint8_t n_src[8],
                                   int64_t *t_dst) {
  *retval = ecall_receive_message(n_dst, msg, n_src, t_dst);
}

tee_status_t ecall_traffic_out(tee_enclave_id_t eid, int *retval) {
  *retval = ecall_traffic_out();
}

tee_status_t ecall_traffic_in(tee_enclave_id_t eid, const uint8_t *ptr, size_t len) {
  ecall_traffic_in(ptr, len);
}

tee_status_t ecall_get_time(tee_enclave_id_t eid, int64_t *retval) {
  *retval = ecall_get_time();
}

tee_status_t ecall_get_t_dst_lower_bound(tee_enclave_id_t eid, int64_t *retval) {
  *retval = ecall_get_t_dst_lower_bound();
}
