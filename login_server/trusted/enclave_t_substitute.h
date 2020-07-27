/**
 * Interface for the into-enclave calls (ecalls).
 * The use of a low-level API (and raw pointers) is motivated by the assumption that a TEE will provide only such
 * capabilities and is in fact inspired by the way Intel SGX implements the trusted / untrusted relationship.
 */


#ifndef ENCLAVE_T_SUBSTITUTE_H
#define ENCLAVE_T_SUBSTITUTE_H

#if defined(__cplusplus)
extern "C" {
#endif

void ecall_init();
void ecall_received_msg_from_client(const char* msg, size_t msg_len);
int ecall_main_loop();
void ecall_kNumRequiredClients(int k);

void ocall_print_string(const char* str);
void ocall_send_msg_to_peer(const char* client_uri, const char* msg, size_t msg_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif //ENCLAVE_T_SUBSTITUTE_H
