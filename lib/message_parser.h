#pragma once

#include <unordered_map>

#include "ros_value.h"
#include "ros_bag_types.h"
#include "ros_msg_types.h"
#include "span.hpp"
#include "util.h"

namespace Embag {
class MessageParser {
 public:
  MessageParser(
      const std::shared_ptr<std::vector<char>> message_buffer,
      size_t offset,
      const std::string &scope,
      RosMsgTypes::ros_msg_def& msg_def
  ) : message_buffer_(message_buffer), offset_(offset), scope_(scope), msg_def_(msg_def) {};

  std::shared_ptr<RosValue> parse();

 private:
  static std::unordered_map<std::string, size_t> primitive_size_map_;

  std::shared_ptr<RosValue> createObject(RosMsgTypes::ros_msg_def_base &object_definition, const std::string &scope);
  std::shared_ptr<RosValue> createObjectArray(RosMsgTypes::ros_embedded_msg_def &object_definition, const size_t array_len);
  std::shared_ptr<RosValue> createPrimitiveArray(RosMsgTypes::ros_msg_field &field, const size_t array_len);
  std::shared_ptr<RosValue> createPrimitive(RosMsgTypes::ros_msg_field &field);
  std::shared_ptr<RosValue> parseField(const std::string &scope, RosMsgTypes::ros_msg_field &field);

  const std::shared_ptr<std::vector<char>> message_buffer_;
  size_t offset_;
  const std::string& scope_;
  RosMsgTypes::ros_msg_def& msg_def_;
};
}
