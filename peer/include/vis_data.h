/**
 * Author: Alexander S.
 * Class for storing, serializing and deserializing the data that needs to be sent to the visualization.
 */

#ifndef VIS_DATA_H
#define VIS_DATA_H

#include "../../include/serialization.h"
#include "../trusted/overlay_structure_scheme.h"
#include "../shared/overlay_return_tuple.h"
#include <json/value.h>
#include <json/json.h>

namespace c1::peer {

/**
 * Contains all necessary data for the visualization.
 */
class VisData : public Serializable {
 public:
  PeerInformation own_id;
  onid_t onid_repr;
  onid_t cur_round;
  OverlayReturnTuple overlay_result;
  std::map<PeerInformation, std::vector<AnnouncementTuple>> out_announce;
  std::map<PeerInformation, std::vector<AgreementTuple>> out_agreement;
  std::map<PeerInformation, std::vector<MessageTuple>> out_inject;
  std::unordered_map<PeerInformation, std::vector<RoutingSchemeTuple>> out_routing;
  std::map<PeerInformation, std::vector<MessageTuple>> out_predeliver;
  std::map<PeerInformation, std::vector<MessageTuple>> out_deliver;
  bool has_waiting_message;
  bool has_delivered_unready_message;
  bool has_delivered_ready_message;

  /**
   * Constructor.
   * @param own_id
   * @param onid_repr
   * @param cur_round
   * @param overlay_result
   * @param out_announce
   * @param out_agreement
   * @param out_inject
   * @param out_routing
   * @param out_predeliver
   * @param out_deliver
   * @param has_waiting_message
   * @param has_delivered_unready_message
   * @param has_delivered_ready_message
   */
  VisData(const PeerInformation &own_id,
          onid_t onid_repr,
          onid_t cur_round,
          const OverlayReturnTuple &overlay_result,
          const std::map<PeerInformation, std::vector<AnnouncementTuple>> &out_announce,
          const std::map<PeerInformation, std::vector<AgreementTuple>> &out_agreement,
          const std::map<PeerInformation, std::vector<MessageTuple>> &out_inject,
          const std::unordered_map<PeerInformation, std::vector<RoutingSchemeTuple>> &out_routing,
          const std::map<PeerInformation, std::vector<MessageTuple>> &out_predeliver,
          const std::map<PeerInformation, std::vector<MessageTuple>> &out_deliver,
          bool has_waiting_message,
          bool has_delivered_unready_message,
          bool has_delivered_ready_message)
      : own_id(own_id),
        onid_repr(onid_repr),
        cur_round(cur_round),
        overlay_result(overlay_result),
        out_announce(out_announce),
        out_agreement(out_agreement),
        out_inject(out_inject),
        out_routing(out_routing),
        out_predeliver(out_predeliver),
        out_deliver(out_deliver),
        has_waiting_message(has_waiting_message),
        has_delivered_unready_message(has_delivered_unready_message),
        has_delivered_ready_message(has_delivered_ready_message) {}


  /**
   * Serializes this object.
   * @param working_vec
   */
  void serialize(std::vector<uint8_t> &working_vec) const override {
    own_id.serialize(working_vec);
    serialize_number(working_vec, onid_repr);
    serialize_number(working_vec, cur_round);
    overlay_result.serialize(working_vec);
    serialize_map_of_vecs(working_vec, out_announce);
    serialize_map_of_vecs(working_vec, out_agreement);
    serialize_map_of_vecs(working_vec, out_inject);
    serialize_unordered_map_of_vecs(working_vec, out_routing);
    serialize_map_of_vecs(working_vec, out_predeliver);
    serialize_map_of_vecs(working_vec, out_deliver);
    serialize_number(working_vec, has_waiting_message);
    serialize_number(working_vec, has_delivered_unready_message);
    serialize_number(working_vec, has_delivered_ready_message);
  }

  /**
   * estimate_size() function according to Serializable
   * @return
   */
  size_t estimate_size() const override {
    return own_id.estimate_size() + sizeof(onid_repr) + sizeof(cur_round) + overlay_result.estimate_size()
      + estimate_map_of_vecs_size(out_announce)
      + estimate_map_of_vecs_size(out_agreement)
      + estimate_map_of_vecs_size(out_inject)
      + estimate_unordered_map_of_vecs_size(out_routing)
      + estimate_map_of_vecs_size(out_predeliver)
      + estimate_map_of_vecs_size(out_deliver)
      + sizeof (has_waiting_message)
      + sizeof (has_delivered_unready_message)
      + sizeof (has_delivered_ready_message);
  }

  /**
   * deserialization function (according to Serializable)
   * @param working_vec
   * @param cur
   * @return
   */
  static VisData deserialize(const std::vector<uint8_t> &working_vec, size_t &cur) {
    auto own_id = PeerInformation::deserialize(working_vec, cur);
    auto onid_repr = deserialize_number<decltype(VisData::onid_repr)>(working_vec, cur);
    auto cur_round = deserialize_number<decltype(VisData::cur_round)>(working_vec, cur);
    auto overlay_result = OverlayReturnTuple::deserialize(working_vec, cur);
    auto out_announce = deserialize_map_of_vecs<PeerInformation, AnnouncementTuple>(working_vec, cur);
    auto out_agreement = deserialize_map_of_vecs<PeerInformation, AgreementTuple>(working_vec, cur);
    auto out_inject = deserialize_map_of_vecs<PeerInformation, MessageTuple>(working_vec, cur);
    auto out_routing = deserialize_unordered_map_of_vecs<PeerInformation, RoutingSchemeTuple>(working_vec, cur);
    auto out_predeliver = deserialize_map_of_vecs<PeerInformation, MessageTuple>(working_vec, cur);
    auto out_deliver = deserialize_map_of_vecs<PeerInformation, MessageTuple>(working_vec, cur);
    auto has_waiting_message = deserialize_number<decltype(VisData::has_waiting_message)>(working_vec, cur);
    auto has_delivered_unready_message = deserialize_number<decltype(VisData::has_delivered_unready_message)>(working_vec, cur);
    auto has_delivered_ready_message = deserialize_number<decltype(VisData::has_delivered_ready_message)>(working_vec, cur);

    return VisData(own_id, onid_repr, cur_round, overlay_result,
                   out_announce, out_agreement,
                   out_inject, out_routing,
                   out_predeliver, out_deliver,
                   has_waiting_message,
                   has_delivered_unready_message,
                   has_delivered_ready_message);
  }

  /**
   * converts returns this object as json
   * @return
   */
  Json::Value to_json() {
    Json::Value root;
    static_assert(std::is_same<decltype(own_id.id), int64_t>::value); // manual check since implicit conversion to Json:Value is not possible
    root["own_id"] = Json::Value::Int64(own_id.id);
    static_assert(std::is_same<decltype(onid_repr), uint64_t>::value); // manual check since implicit conversion to Json:Value is not possible
    root["onid_repr"] = Json::Value::UInt64(onid_repr);
    static_assert(std::is_same<decltype(onid_repr), uint64_t>::value); // manual check since implicit conversion to Json:Value is not possible
    root["cur_round"] = Json::Value::UInt64(cur_round);
    static_assert(std::is_same<decltype(overlay_result.onid_emul), uint64_t>::value); // manual check since implicit conversion to Json:Value is not possible
    root["onid_emul"] =Json::Value::UInt64(overlay_result.onid_emul);
    for (const auto& elem : overlay_result.gamma_agree) {
      root["gamma_agree"].append(std::to_string(elem.id));
    }
    for (const auto& elem : overlay_result.gamma_send) {
      root["gamma_send"].append(std::to_string(elem.id));
    }
    for (const auto&[first, second] : overlay_result.gamma_route) {
      for (const auto &elem2 : second) {
        root["gamma_route"][std::to_string(first)].append(std::to_string(elem2.id));
      }
    }
    for (const auto& elem : overlay_result.gamma_receive) {
      root["gamma_receive"].append(std::to_string(elem.id));
    }
    for (const auto&[first, second] : out_announce) {
      for (const auto &elem2 : second) {
        root["out_announce"][std::to_string(first.id)].append(elem2.to_json_string());
      }
    }
    for (const auto&[first, second] : out_agreement) {
      for (const auto &elem2 : second) {
        root["out_agreement"][std::to_string(first.id)].append(elem2.to_json_string());
      }
    }
    for (const auto&[first, second] : out_inject) {
      for (const auto &elem2 : second) {
        root["out_inject"][std::to_string(first.id)].append(elem2.to_json_string());
      }
    }
    for (const auto&[first, second] : out_routing) {
      for (const auto &elem2 : second) {
        root["out_routing"][std::to_string(first.id)].append(elem2.to_json_string());
      }
    }
    for (const auto&[first, second] : out_predeliver) {
      for (const auto &elem2 : second) {
        root["out_predeliver"][std::to_string(first.id)].append(elem2.to_json_string());
      }
    }
    for (const auto&[first, second] : out_deliver) {
      for (const auto &elem2 : second) {
        root["out_deliver"][std::to_string(first.id)].append(elem2.to_json_string());
      }
    }
    root["has_waiting_message"] = has_waiting_message;
    root["has_delivered_unready_message"] = has_delivered_unready_message;
    root["has_delivered_ready_message"] = has_delivered_ready_message;


    static_assert(std::is_same<decltype(overlay_result.s_overlay_prime.size()), size_t>::value); // manual check since implicit conversion to Json:Value is not possible
    root["s_overlay_prime.size"] = Json::Value::UInt64(overlay_result.s_overlay_prime.size());
    for (const auto& [key, vec] : overlay_result.s_overlay_prime) {
      for (const auto& elem: vec) {
        Json::Value my_elem;
        my_elem["t"].append(std::to_string(static_cast<uint8_t>(elem.t)));
        my_elem["onid"].append(std::to_string(elem.onid));
        my_elem["id"].append(std::to_string(elem.peer_information.id));
        for (const auto& [key2, vec2]: elem.gamma_route) {
          for (const auto& elem3: vec2) {
            my_elem["gamma_route"][std::to_string(key2)].append(std::to_string(elem3.id));
          }
        }
        for (const auto& elem2 : elem.gamma_receive) {
          my_elem["gamma_receive"].append(std::to_string(elem2.id));
        }
        root["s_overlay_prime"][std::to_string(key.id)].append(my_elem);
      }
    }

    return root;
  }

};

} // namespace

#endif //VIS_DATA_H
