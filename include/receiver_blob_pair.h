/**
 * Author: Alexander S.
 */

#ifndef RECEIVER_BLOB_PAIR_H
#define RECEIVER_BLOB_PAIR_H

namespace c1 {

/**
 * Contains a PeerInformation and a blob of data (used to encode the (i,c)-Pairs of trafficOut()).
 */
class ReceiverBlobPair : public Serializable {
 public:

  PeerInformation i;
  std::vector<uint8_t> payload;

  ReceiverBlobPair(PeerInformation i, std::vector<uint8_t> payload) : i(std::move(i)), payload(std::move(payload)) {}

  void serialize(std::vector<uint8_t> &working_vec) const override {
    i.serialize(working_vec);

    serialize_number(working_vec, payload.size());
    working_vec.insert(std::end(working_vec), std::begin(payload), std::end(payload));
  }

  size_t estimate_size() const override {
    return i.estimate_size() + sizeof(size_t) + payload.size();
  }

  static ReceiverBlobPair deserialize(const std::vector<uint8_t> &working_vec, size_t &cur) {
    auto i = PeerInformation::deserialize(working_vec, cur);
    auto count = deserialize_number<size_t>(working_vec, cur);

    std::vector<uint8_t> payload(&working_vec[cur], &working_vec[cur] + count);
    cur += count;

    return ReceiverBlobPair(i, payload);
  }
};

}

#endif //RECEIVER_BLOB_PAIR_H
