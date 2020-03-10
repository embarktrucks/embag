#pragma once

#include <unordered_map>

#include "embag.h"
#include "ros_value.h"
#include "ros_bag_types.h"
#include "util.h"

class MessageParser {
 public:
  MessageParser(
      message_stream &stream,
      const RosBagTypes::connection_data_t &connection_data,
      const std::shared_ptr<Embag::ros_msg_def> msg_def
      ) : stream_(stream), connection_data_(connection_data), msg_def_(msg_def) {};

  std::unique_ptr<RosValue> parse();

 private:

  // TODO: it would be nice to not have to look up this mapping but establish it at parse time
  std::unordered_map<std::string, RosValue::Type> primitive_type_map_ = {
      {"bool", RosValue::ros_bool},
      {"int8", RosValue::int8},
      {"uint8", RosValue::uint8},
      {"int16", RosValue::int16},
      {"uint16", RosValue::uint16},
      {"uint8", RosValue::uint8},
      {"int32", RosValue::int32},
      {"uint32", RosValue::uint32},
      {"int64", RosValue::int64},
      {"uint64", RosValue::uint64},
      {"float32", RosValue::float32},
      {"float64", RosValue::float64},
      {"string", RosValue::string},
      {"time", RosValue::ros_time},
      {"duration", RosValue::ros_duration},

      // Deprecated types
      {"byte", RosValue::int8},
      {"char", RosValue::uint8},
  };

  std::unique_ptr<RosValue> parseField(const std::string &scope, const Embag::ros_msg_field &field);
  void parseArray(size_t array_len, Embag::ros_embedded_msg_def &embedded_type, std::unique_ptr<RosValue> &value);
  std::unique_ptr<RosValue> parseMembers(Embag::ros_embedded_msg_def &embedded_type);
  Embag::ros_embedded_msg_def getEmbeddedType(const std::string &scope, const Embag::ros_msg_field &field);
  std::unique_ptr<RosValue> getPrimitiveField(const Embag::ros_msg_field& field);

  message_stream &stream_;
  const RosBagTypes::connection_data_t &connection_data_;
  const std::shared_ptr<Embag::ros_msg_def> msg_def_;
};