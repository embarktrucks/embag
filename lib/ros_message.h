#pragma once

#include <string>

#include "ros_value.h"

namespace Embag {
class RosMessage {
 public:
  std::string topic;
  RosValue::ros_time_t timestamp;
  std::unique_ptr<RosValue> data_;
  std::string md5;

  const std::unique_ptr<RosValue> &data(const std::string &key) {
    return data_->get(key);
  }

  void print() {
    data_->print();
  }
};
}