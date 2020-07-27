/**
 * Interface for the into-enclave calls (ecalls).
 * The use of a low-level API (and raw pointers) is motivated by the assumption that a TEE will provide only such
 * capabilities and is in fact inspired by the way Intel SGX implements the trusted / untrusted relationship.
 */


#ifndef LOGIN_SERVER_ENCLAVE_U_SUBSTITUTE_H
#define LOGIN_SERVER_ENCLAVE_U_SUBSTITUTE_H

#include "../../common/tee_functions.h"


tee_status_t ecall_init(tee_enclave_id_t eid);
tee_status_t ecall_received_msg_from_client(tee_enclave_id_t eid, const char* msg, size_t msg_len);
tee_status_t ecall_main_loop(tee_enclave_id_t eid, int* retval);
tee_status_t ecall_kNumRequiredClients(tee_enclave_id_t eid, int k);


#endif //LOGIN_SERVER_ENCLAVE_U_SUBSTITUTE_H
