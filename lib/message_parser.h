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
      nonstd::span<char> stream,
      const std::string &scope,
      const std::shared_ptr<RosMsgTypes::ros_msg_def> msg_def
  ) : stream_(stream), scope_(scope), msg_def_(msg_def) {};

  std::unique_ptr<RosValue> parse();

 private:

  std::unique_ptr<RosValue> parseField(const std::string &scope, RosMsgTypes::ros_msg_field &field);
  void parseArray(size_t array_len, RosMsgTypes::ros_embedded_msg_def &embedded_type, std::unique_ptr<RosValue> &value);
  std::unique_ptr<RosValue> parseMembers(RosMsgTypes::ros_embedded_msg_def &embedded_type);
  RosMsgTypes::ros_embedded_msg_def getEmbeddedType(const std::string &scope, const RosMsgTypes::ros_msg_field &field);
  std::unique_ptr<RosValue> getPrimitiveBlob(RosMsgTypes::ros_msg_field &field, uint32_t len);
  std::unique_ptr<RosValue> getPrimitiveField(RosMsgTypes::ros_msg_field &field);

  template <typename T>
  void read_into(T* dest);
  void read_into(std::string& dest, size_t size);

  nonstd::span<char> stream_;
  const std::string scope_;
  const std::shared_ptr<RosMsgTypes::ros_msg_def> msg_def_;
};
}
