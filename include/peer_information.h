/**
 * Author: Alexaxnder S.
 */

#ifndef PEER_INFORMATION_H
#define PEER_INFORMATION_H

#include "uri.h"
namespace c1 {

/**
 * Basic structure holding the id of a peer and the uri of its socket.
 */
struct PeerInformation : public Serializable {
  int64_t id;
  Uri uri;

  PeerInformation() : id(0), uri() {}
  PeerInformation(int64_t id, Uri uri) : id(id), uri(uri) {}

  inline bool operator==(const PeerInformation &o) const { return std::tie(id, uri) == std::tie(o.id, o.uri); }
  inline bool operator!=(const PeerInformation &o) const { return !operator==(o); }
  inline bool operator<(const PeerInformation &rhs) const { return std::tie(id, uri) < std::tie(rhs.id, rhs.uri); }
  inline bool operator>(const PeerInformation &rhs) const { return rhs < *this; }
  inline bool operator<=(const PeerInformation &rhs) const { return !(rhs < *this); }
  inline bool operator>=(const PeerInformation &rhs) const { return !(*this < rhs); }

  void serialize(std::vector <uint8_t> &working_vec) const override {
    serialize_number(working_vec, id);
    uri.serialize(working_vec);
  }

  size_t estimate_size() const override {
    return sizeof(id) + uri.estimate_size();
  }

  static PeerInformation deserialize(const std::vector <uint8_t> &working_vec, size_t &cur) {
    PeerInformation result;
    result.id = deserialize_number<decltype(id)>(working_vec, cur);
    result.uri = Uri::deserialize(working_vec, cur);
    return result;
  }

  inline operator std::string const() const {
    std::string result = "";
    result += "Id: " + std::to_string(id) + ", ";
    result += "Uri: " + std::string(uri);
    return result;
  }
};

}

namespace std {
template<>
struct hash<c1::PeerInformation> {
  std::size_t operator()(const c1::PeerInformation &k) const {
    using std::size_t;
    using std::hash;
    using std::string;

    return hash<decltype(k.id)>{}(k.id);
  }
};
}

#endif //PEER_INFORMATION_H
