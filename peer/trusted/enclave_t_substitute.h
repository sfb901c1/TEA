/**
 * Interface for the into-enclave calls (ecalls) and out-of-enclave calls (ocalls).
 * The use of a low-level API (and raw pointers) is motivated by the assumption that a TEE will provide only such
 * capabilities and is in fact inspired by the way Intel SGX implements the trusted / untrusted relationship.
 */

#ifndef PEER_ENCLAVE_T_SUBSTITUTE_H
#define PEER_ENCLAVE_T_SUBSTITUTE_H

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

void ocall_print_string(const char *str);
void ocall_send_msg_to_server(const uint8_t *ptr, size_t len);
void ocall_traffic_out_return(const uint8_t *ptr, size_t len);
void ocall_vis_data(const uint8_t *ptr, size_t len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //PEER_ENCLAVE_T_SUBSTITUTE_H
