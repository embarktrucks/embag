#pragma once

#include <memory>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class RosValue {
 public:

  // TODO: convert this to boost::variant?

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

  std::unordered_map<std::string, std::unique_ptr<RosValue>> objects;
  std::vector<std::unique_ptr<RosValue>> values;

  Type type = object;

  RosValue () = default;
  explicit RosValue(Type type) : type(type) {}

  // Convenience accessors
  const std::unique_ptr<RosValue> & get(const std::string &key) {
    if (type != object) {
      throw std::runtime_error("Value is not an object");
    }
    return objects[key];
  }

  const std::unique_ptr<RosValue> & get(const size_t idx) {
    if (type != array) {
      throw std::runtime_error("Value is not an array");
    }
    return values[idx];
  }

  template<typename T>
  struct identity { typedef T type; };

  template<typename T>
  T & get() {
    return getValue(identity<T>());
  }

  void print(const std::string &path = "");

 private:
  // Primitive accessors
  bool & getValue(identity<bool>) {
    if (type != bool_value) {
      throw std::runtime_error("Value is not a bool");
    }
    return bool_value;
  }

  std::string & getValue(identity<std::string>) {
    if (type != string) {
      throw std::runtime_error("Value is not a string");
    }
    return string_value;
  }
};
