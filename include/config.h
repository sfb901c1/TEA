/**
 * Configuration file.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <cstddef>
#include <cstdint>

/** message size in bytes (refers to messages injected by users) */
constexpr std::size_t kMessageSize{128};
/** pseudonym size in bytes */
constexpr std::size_t kPseudonymSize{65};
//constexpr int kBlobSize{BLOB_SIZE};

/** variable x as in the paper (see definition of beta) */
constexpr int kX{2};
/** variable epsilon as in the paper */
constexpr double kEpsilon{.4};

/** variable Delta as in the paper */
constexpr int kDelta{4};
/** variable A_max as in the paper */
constexpr int kAMax{2};
/** variable k_send as in the paper */
constexpr int kSend{1};
/** variable k_recv as in the paper */
constexpr int kRecv{2};

typedef int64_t round_t;
typedef uint64_t onid_t;
typedef int64_t dim_t;

#endif //CONFIG_H
