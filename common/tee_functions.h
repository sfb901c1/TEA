/**
 * Some functions (assumed to be) provided by the trusted execution environment.
 * The interface is kept simple to provide similarity to existing TEEs (such as Intel SGX).
 * Therefore, no proper encapsulation takes place here.
 */

#ifndef TEE_FUNCTIONS_H
#define TEE_FUNCTIONS_H

#include <stdlib.h> /* for size_t */
#include <stdint.h> /* for uint64_t */
#include <random>
#include <climits>
#include <algorithm>
#include <array>
#include <cstring>

typedef uint64_t tee_time_t;
typedef uint8_t tee_time_source_nonce_t[32];

typedef enum status_t {
  TEE_SUCCESS,
  TEE_ERROR_MAC_MISMATCH
} tee_status_t;

typedef uint64_t tee_enclave_id_t;

namespace c1 {

/**
 * Class providing some functions that the trusted execution environment would provide.
 */
class TeeFunctions {
 public:

  /**
   * Should be called before any function returning a random value.
   * @param seed
   */
  static void seed(int64_t seed) { rbe.seed(seed); };

/**
 * Obtain the current elapsed time.
 * @return trusted time stamp in seconds relative to a reference point
 */
  static tee_time_t tee_get_trusted_time();

/**
 * Get a random byte string of specified length (in bytes) as an array.
 * @return std::array containing all bytes.
 */
  template<int size>
  static std::array<uint8_t, size> tee_read_rand() {
    std::array<uint8_t, size> data;
    std::generate(begin(data), end(data), std::ref(TeeFunctions::rbe));
    return data;
  }

/**
 * Get a random byte string of the length of the parameter whose pointer is given (result will also be stored in that parameter).
 * @param random_number A number type whose bytes will be filled with the random byte string.
 */
  template<typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
  static void tee_read_rand(T *random_number) {
    auto rand = tee_read_rand<sizeof(T)>();
    memcpy(random_number, rand.data(), sizeof(T));
  }

 private:
  /* The random bits engine. */
  static std::independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned char> rbe;

};

}

#endif //TEE_FUNCTIONS_H
