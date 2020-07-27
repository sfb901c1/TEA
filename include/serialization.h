/**
 * Author: Alexander S.
 *
 * Since we assume the TEE environment to be very simple and have no access to higher-level functions (which was in fact
 * the case when we implemented an earlier version of the prototype on Intel SGX), we need to implement our own
 * serialization mechanism:
 * Every serializable class should have Serializable as a base class. This class declares the (pure virtual) serialize()
 * and estimate_size() methods that need to be implemented by each serializable class (the latter is used to reduce
 * memory copies since it allows to estimate the size of the resulting vector storing the serialized bytes in advance).
 * For the deserialization, we assume the application to have the serialized data as a vector of uint8_t's.
 * Each deserialization function takes as parameters a reference to the vector and the current position in the vector
 * we're at.
 * All deserialization functions used in this project (e.g. to deserialize a number or a vector of objects of a common
 * type) are also provided in this file.
 */

#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <vector>
#include <cstdint>
#include <cstring>
#include <map>
#include <unordered_map>

namespace c1 {

/**
 * Virtual class to enable classes to subclass from this to make sure they have a serializable() function.
 */
class Serializable {
 public:
  virtual void serialize(std::vector<uint8_t> &working_vec) const = 0;
  virtual size_t estimate_size() const = 0;
  virtual ~Serializable() = default;
};

/**
 * Serialize a number into working_vec.
 * @tparam T type of the number
 * @param working_vec the serialization vector
 * @param number the number to be serialized
 */
template<typename T>
void serialize_number(std::vector<uint8_t> &working_vec, T number) {
  //this assumes that all platforms are equal regarding their endianness (sufficient for our purposes)
  auto *ptr = reinterpret_cast<uint8_t *>(&number);
  working_vec.insert(working_vec.end(), ptr, ptr + sizeof(T));
}

/**
 * Deserialize a number from working_vec at current position cur and update cur appropriately.
 * @tparam T type of the number
 * @param working_vec
 * @param cur index of the first byte of the number in working_vec
 * @return the deserialized number
 */
template<typename T>
T deserialize_number(const std::vector<uint8_t> &working_vec, size_t &cur) {
  // result.length_ = ((working_vec[cur++]<<24)|(working_vec[cur++]<<16)|(working_vec[cur++]<<8)|(working_vec[cur++]));
  /*
  T result = 0;
  for (int i = 0; i < sizeof(T); ++i) {
    result |= (working_vec[cur + i] << 8 * (sizeof(T) - i - 1));
  }
  cur += sizeof(T);
  return result;
   */
  T result = 0;
  auto ptr = reinterpret_cast<uint8_t *>(&result);
  // keep in mind that this is not robust against different endian-ness
  memcpy(ptr, &working_vec[cur], sizeof(T));
  cur += sizeof(T);
  return result;
}

/**
 * Serialize a vector of Serializable objects.
 * @tparam T type of the elements in the vector
 * @param working_vec byte vector which the serialized data is added to
 * @param vec the vector to be serialized
 */
template<typename T, typename std::enable_if<std::is_base_of<Serializable, T>::value>::type * = nullptr>
void
serialize_vec(std::vector<uint8_t> &working_vec, const std::vector<T> &vec) {
  //std::vector<uint8_t> result;
  serialize_number(working_vec, vec.size());
  for (const auto &elem : vec) {
    elem.serialize(working_vec);
  }
}

/**
 * Deserialize a vector of Serializable objects that have a deserialize(working_vec, cur) function.
 * @tparam T type of the elements in the vector
 * @param working_vec vector holding the serialized data
 * @param cur index of the first byte of the serialized vector in working_vec
 * @return the deserialized vector
 */
template<typename T, typename std::enable_if<std::is_base_of<Serializable, T>::value>::type * = nullptr>
std::vector<T> deserialize_vec(const std::vector<uint8_t> &working_vec, size_t &cur) {
  //std::vector<uint8_t> result;
  std::vector<T> result;
  auto size = deserialize_number<typename std::vector<T>::size_type>(working_vec, cur);
  //ocall_print_string(("Size is: " + std::to_string(size)).c_str());
  result.reserve(size);
  for (int i = 0; i < size; ++i) {
    result.emplace_back(T::deserialize(working_vec, cur));
  }
  return result;
}

/**
 * Serialize a map whose keys are Serializable objects or integer numbers and whose values are vectors of Serializable objects.
 * @tparam T1 key type
 * @tparam T2 type of the elements in the vector
 * @param working_vec byte vector which the serialized data is added to
 * @param map the map to be serialized
 */
template<typename T1, typename T2>
void serialize_map_of_vecs(std::vector<uint8_t> &working_vec, const std::map<T1, std::vector<T2>> &map) {
  static_assert(std::is_base_of<Serializable, T1>::value || std::is_integral<T1>::value,
                "First template parameter must be number or Serializable.");
  static_assert(std::is_base_of<Serializable, T2>::value,
                "Second template parameter must be Serializable.");
  serialize_number(working_vec, map.size());
  for (const auto &elem : map) {
    if constexpr (std::is_base_of<Serializable, T1>::value) {
      elem.first.serialize(working_vec);
    } else if constexpr (std::is_integral<T1>::value) {
      serialize_number(working_vec, elem.first);
    }
    serialize_vec(working_vec, elem.second);
  }
}

/**
 * Deserialize a map whose keys are Serializable objects or integers and whose values are vectors of Serializable objects.
 * @tparam T1 key type
 * @tparam T2 type of the elements in the vector
 * @param working_vec vector holding the serialized data
 * @param cur index of the first byte of the serialized vector in working_vec
 * @return the deserialized map
 */
template<typename T1, typename T2>
std::map<T1, std::vector<T2>> deserialize_map_of_vecs(const std::vector<uint8_t> &working_vec, size_t &cur) {
  static_assert(std::is_base_of<Serializable, T1>::value || std::is_integral<T1>::value,
                "First template parameter must be number or Serializable.");
  static_assert(std::is_base_of<Serializable, T2>::value,
                "Second template parameter must be Serializable.");

  std::map<T1, std::vector<T2>> result;
  auto size = deserialize_number<typename std::map<T1, std::vector<T2>>::size_type>(working_vec, cur);
  //result.reserve(size);
  for (int i = 0; i < size; ++i) {
    if constexpr (std::is_base_of<Serializable, T1>::value) {
      auto key = T1::deserialize(working_vec, cur);
      result[key] = deserialize_vec<T2>(working_vec, cur);
    } else if constexpr(std::is_integral<T1>::value) {
      auto key = deserialize_number<T1>(working_vec, cur);
      result[key] = deserialize_vec<T2>(working_vec, cur);
    }
  }
  return result;
}

/**
 * Serialize an unordered_map whose keys are Serializable objects and whose values are vectors of Serializable objects.
 * @tparam T1 key type
 * @tparam T2 type of the elements in the vector
 * @param working_vec byte vector which the serialized data is added to
 * @param map the unordered_map to be serialized
 */
template<typename T1,
    typename T2>
void
serialize_unordered_map_of_vecs(std::vector<uint8_t> &working_vec, const std::unordered_map<T1, std::vector<T2>> &map) {
  static_assert(std::is_base_of<Serializable, T1>::value,
                "First template parameter must be Serializable.");
  static_assert(std::is_base_of<Serializable, T2>::value,
                "Second template parameter must be Serializable.");

  serialize_number(working_vec, map.size());
  for (const auto &[key, value] : map) {
    key.serialize(working_vec);
    serialize_vec(working_vec, value);
  }
}

/**
 * Deserialize an unordered_map whose keys are Serializable objects and whose values are vectors of Serializable objects.
 * @tparam T1 key type
 * @tparam T2 type of the elements in the vector
 * @param working_vec vector holding the serialized data
 * @param cur index of the first byte of the serialized vector in working_vec
 * @return the deserialized unordered_map
 */
template<typename T1,
    typename T2>
std::unordered_map<T1, std::vector<T2>> deserialize_unordered_map_of_vecs(const std::vector<uint8_t> &working_vec,
                                                                          size_t &cur) {
  static_assert(std::is_base_of<Serializable, T1>::value,
                "First template parameter must be Serializable.");
  static_assert(std::is_base_of<Serializable, T2>::value,
                "Second template parameter must be Serializable.");
  std::unordered_map<T1, std::vector<T2>> result;
  auto size = deserialize_number<typename std::unordered_map<T1, std::vector<T2>>::size_type>(working_vec, cur);
  //result.reserve(size);
  for (int i = 0; i < size; ++i) {
    auto key = T1::deserialize(working_vec, cur);
    result[key] = deserialize_vec<T2>(working_vec, cur);
  }
  return result;
}

/**
 * Calculate the size of the serialization of vec.
 * @tparam T type of the elements in vec
 * @param vec
 * @return
 */
template<typename T, typename std::enable_if<std::is_base_of<Serializable, T>::value>::type * = nullptr>
size_t
estimate_vec_size(const std::vector<T> &vec) {
  //std::vector<uint8_t> result;
  auto result = sizeof(size_t);
  for (const auto &elem : vec) {
    result += elem.estimate_size();
  }
  return result;
}

/**
 * Calculate the size of the serialization of map.
 * @tparam T1 key type
 * @tparam T2 value type
 * @param map
 * @return
 */
template<typename T1, typename T2>
size_t estimate_map_of_vecs_size(const std::map<T1, std::vector<T2>> &map) {
  static_assert(std::is_base_of<Serializable, T1>::value || std::is_integral<T1>::value,
                "First template parameter must be number or Serializable.");
  static_assert(std::is_base_of<Serializable, T2>::value,
                "Second template parameter must be Serializable.");
  //std::vector<uint8_t> result;
  auto result = sizeof(size_t);
  for (const auto &[key, value] : map) {
    if constexpr (std::is_base_of<Serializable, T1>::value) {
      result += key.estimate_size();
    } else if constexpr (std::is_integral<T1>::value) {
      result += sizeof(key);
    }
    result += estimate_vec_size(value);
  }
  return result;
}

/**
 * Calculate the size of the serialization of map.
 * @tparam T1 key type
 * @tparam T2 value type
 * @param map
 * @return
 */
template<typename T1, typename T2>
size_t estimate_unordered_map_of_vecs_size(const std::unordered_map<T1, std::vector<T2>> &map) {
  static_assert(std::is_base_of<Serializable, T1>::value,
                "First template parameter must be Serializable.");
  static_assert(std::is_base_of<Serializable, T2>::value,
                "Second template parameter must be Serializable.");

  auto result = sizeof(size_t);
  for (const auto &[key, value] : map) {
    result += key.estimate_size();
    result += estimate_vec_size(value);
  }
  return result;
}

}

#endif //SERIALIZATION_H
