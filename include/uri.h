/**
 * Author: Alexander S.
 */

#ifndef INCLUDE_URI_H
#define INCLUDE_URI_H

namespace c1 {

/**
 * An URI consisting of an IPv4 address (to be accessed byte-wise via ip1, ..., ip4) and a port number.
 */
struct Uri : public Serializable {
  uint8_t ip1{};
  uint8_t ip2{};
  uint8_t ip3{};
  uint8_t ip4{};
  uint64_t port{};

  Uri() = default;

  Uri(uint8_t ip1, uint8_t ip2, uint8_t ip3, uint8_t ip4, uint64_t port)
      : ip1(ip1), ip2(ip2), ip3(ip3), ip4(ip4), port(port) {}

  bool operator==(const Uri &rhs) const {
    return std::tie(port, ip1, ip2, ip3, ip4) == std::tie(rhs.port, rhs.ip1, rhs.ip2, rhs.ip3, rhs.ip4);
  }
  bool operator!=(const Uri &rhs) const {
    return !(rhs == *this);
  }

  bool operator<(const Uri &rhs) const {
    return std::tie(port, ip1, ip2, ip3, ip4)
        < std::tie(rhs.port, rhs.ip1, rhs.ip2, rhs.ip3, rhs.ip4);
  }
  bool operator>(const Uri &rhs) const {
    return rhs < *this;
  }
  bool operator<=(const Uri &rhs) const {
    return !(rhs < *this);
  }
  bool operator>=(const Uri &rhs) const {
    return !(*this < rhs);
  }

  void serialize(std::vector<uint8_t> &working_vec) const override {
    serialize_number(working_vec, ip1);
    serialize_number(working_vec, ip2);
    serialize_number(working_vec, ip3);
    serialize_number(working_vec, ip4);
    serialize_number(working_vec, port);
  }

  size_t estimate_size() const override {
    return sizeof(ip1) + sizeof(ip2) + sizeof(ip3) + sizeof(ip4) + sizeof(port);
  }

  static Uri deserialize(const std::vector<uint8_t> &working_vec, size_t &cur) {
    Uri result;
    result.ip1 = deserialize_number<uint8_t>(working_vec, cur);
    result.ip2 = deserialize_number<uint8_t>(working_vec, cur);
    result.ip3 = deserialize_number<uint8_t>(working_vec, cur);
    result.ip4 = deserialize_number<uint8_t>(working_vec, cur);
    result.port = deserialize_number<uint64_t>(working_vec, cur);
    return result;
  }

  inline operator std::string const() const {
    std::string result;
    result += std::to_string(ip1) + "." + std::to_string(ip2) + "." + std::to_string(ip3) + "." + std::to_string(ip4);
    result += ":" + std::to_string(port);
    return result;
  }
};

}

#endif //INCLUDE_URI_H
