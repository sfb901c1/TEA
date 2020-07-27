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
void ecall_received_msg_from_client(const char *msg, size_t msg_len);
int ecall_main_loop();
void ecall_kNumRequiredClients(int k);

#ifdef __cplusplus
}
#endif /* __cplusplus */

tee_status_t ecall_init(tee_enclave_id_t eid) {
  ecall_init();
}

tee_status_t ecall_received_msg_from_client(tee_enclave_id_t eid, const char *msg, size_t msg_len) {
  ecall_received_msg_from_client(msg, msg_len);
}

tee_status_t ecall_main_loop(tee_enclave_id_t eid, int *retval) {
  *retval = ecall_main_loop();
}

tee_status_t ecall_kNumRequiredClients(tee_enclave_id_t eid, int k) {
  ecall_kNumRequiredClients(k);
}
