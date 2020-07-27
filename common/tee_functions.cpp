/**
 * Some functions provided by the trusted execution environment.
 * The interface is kept simple to provide similarity to existing TEEs (such as Intel SGX).
 * Therefore, no proper encapsulation takes place here.
 */

#include <chrono>
#include <iostream>

#include "tee_functions.h"

namespace c1 {

tee_time_t TeeFunctions::tee_get_trusted_time() {
  auto now = std::chrono::system_clock::now();
  auto now_s = std::chrono::time_point_cast<std::chrono::seconds>(now);
  auto value = now_s.time_since_epoch();
  return static_cast<tee_time_t>(value.count());
}

std::independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned char> TeeFunctions::rbe{};

}