#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <vector>

class RosValue {
 public:

  enum Type {
    ros_bool,
    int8,
    uint8,
    int16,
    uint16,
    int32,
    uint32,
    int64,
    uint64,
    float32,
    float64,
    string,
    ros_time,
    ros_duration,

    // Custom types
    object,
    array,
  };

  // Generic types
  struct ros_time_t {
    uint32_t secs = 0;
    uint32_t nsecs = 0;
  };

  struct ros_duration_t {
    int32_t secs = 0;
    int32_t nsecs = 0;
  };

  bool bool_value = false;
  int8_t int8_value = 0;
  uint8_t uint8_value = 0;
  int16_t int16_value = 0;
  uint16_t uint16_value = 0;
  int32_t int32_value = 0;
  uint32_t uint32_value = 0;
  int64_t int64_value = 0;
  uint64_t uint64_value = 0;
  float float32_value = 0;
  double float64_value = 0;
  std::string string_value;
  ros_time_t time_value;
  ros_duration_t duration_value;

  std::map<std::string, RosValue> objects;
  std::vector<RosValue> values;

  Type type;

  RosValue () = default;
  explicit RosValue(Type type) : type(type) {}

  // Convenience accessors
  RosValue operator [](const std::string &key) {
    return objects[key];
  }

  RosValue operator [](const size_t idx) {
    return values[idx];
  }
 private:
};
