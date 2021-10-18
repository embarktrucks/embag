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
  )
  : message_buffer_(message_buffer)
  , message_buffer_offset_(offset)
  , ros_values_(std::make_shared<std::vector<RosValue>>())
  , ros_values_offset_(0)
  , scope_(scope)
  , msg_def_(msg_def)
  {
  };

  const RosValue* parse();

 private:
  static std::unordered_map<std::string, size_t> primitive_size_map_;

  RosValue::ros_value_pointer_t createObject(RosMsgTypes::ros_msg_def_base &object_definition, const std::string &scope);
  RosValue::ros_value_pointer_t createObjectArray(RosMsgTypes::ros_embedded_msg_def &object_definition, const size_t array_len);
  RosValue::ros_value_pointer_t createPrimitiveArray(RosMsgTypes::ros_msg_field &field, const size_t array_len);
  RosValue::ros_value_pointer_t createPrimitive(RosMsgTypes::ros_msg_field &field);
  RosValue::ros_value_pointer_t parseField(const std::string &scope, RosMsgTypes::ros_msg_field &field);

  const std::shared_ptr<std::vector<char>> message_buffer_;
  size_t message_buffer_offset_;

  std::shared_ptr<std::vector<RosValue>> ros_values_;
  size_t ros_values_offset_;

  const std::string& scope_;
  RosMsgTypes::ros_msg_def& msg_def_;
};
}
