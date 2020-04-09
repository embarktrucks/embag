#pragma once

#include <string>

#include "ros_value.h"
#include "message_parser.h"
#include "ros_msg_types.h"
#include "util.h"

namespace Embag {
class RosMessage {
 public:
  std::string topic;
  RosValue::ros_time_t timestamp;
  std::string md5;
  char* raw_data = nullptr;
  uint32_t raw_data_len = 0;

  const std::unique_ptr<RosValue> &data(const std::string &key) {
    if (!parsed_) {
      hydrate();
    }

    return data_->get(key);
  }

  bool has(const std::string &key) {
    if (!parsed_) {
      hydrate();
    }

    return data_->has(key);
  }

  void print() {
    if (!parsed_) {
      hydrate();
    }

    data_->print();
  }

 private:
  bool parsed_ = false;
  std::unique_ptr<RosValue> data_;
  std::shared_ptr<RosMsgTypes::ros_msg_def> msg_def_;
  std::string scope_;

  void hydrate() {
    // FIXME: streaming this data means copying it into basic types.  It would be faster to just set pointers...
    message_stream stream{raw_data, raw_data_len};

    MessageParser msg{stream, scope_, msg_def_};

    data_ = msg.parse();

    parsed_ = true;
  }

  friend class View;
};
}