#pragma once

#include <unordered_map>

#include "embag.h"
#include "ros_value.h"
#include "ros_bag_types.h"
#include "util.h"

namespace Embag {

class MessageParser {
 public:
  MessageParser(
      message_stream &stream,
      const RosBagTypes::connection_data_t &connection_data,
      const std::shared_ptr<Bag::ros_msg_def> msg_def
  ) : stream_(stream), connection_data_(connection_data), msg_def_(msg_def) {};

  std::unique_ptr<RosValue> parse();

 private:

  std::unique_ptr<RosValue> parseField(const std::string &scope, Bag::ros_msg_field &field);
  void parseArray(size_t array_len, Bag::ros_embedded_msg_def &embedded_type, std::unique_ptr<RosValue> &value);
  std::unique_ptr<RosValue> parseMembers(Bag::ros_embedded_msg_def &embedded_type);
  Bag::ros_embedded_msg_def getEmbeddedType(const std::string &scope, const Bag::ros_msg_field &field);
  std::unique_ptr<RosValue> getPrimitiveBlob(Bag::ros_msg_field &field, uint32_t len);
  std::unique_ptr<RosValue> getPrimitiveField(Bag::ros_msg_field &field);

  message_stream &stream_;
  const RosBagTypes::connection_data_t &connection_data_;
  const std::shared_ptr<Bag::ros_msg_def> msg_def_;
};
}