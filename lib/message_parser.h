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
      const RosMsgTypes::MsgDef& msg_def
  )
  : message_buffer_(message_buffer)
  , message_buffer_offset_(offset)
  , ros_values_(std::make_shared<std::vector<RosValue>>())
  , ros_values_offset_(0)
  , msg_def_(msg_def)
  {
  };

  const RosValue::Pointer parse();

 private:
  static std::unordered_map<std::string, size_t> primitive_size_map_;

  void initObject(size_t object_offset, const RosMsgTypes::BaseMsgDef &object_definition);
  void initArray(size_t array_offset, const RosMsgTypes::FieldDef &field);
  void initPrimitive(size_t primitive_offset, const RosMsgTypes::FieldDef &field);
  void emplaceField(const RosMsgTypes::FieldDef &field);

  const std::shared_ptr<std::vector<char>> message_buffer_;
  size_t message_buffer_offset_;

  std::shared_ptr<std::vector<RosValue>> ros_values_;
  size_t ros_values_offset_;

  const RosMsgTypes::MsgDef& msg_def_;
};
}
