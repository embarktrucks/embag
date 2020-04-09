#pragma once

#include <memory>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Embag {

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
    blob,
  };

  struct ros_time_t {
    uint32_t secs = 0;
    uint32_t nsecs = 0;
  };

  struct ros_duration_t {
    int32_t secs = 0;
    int32_t nsecs = 0;
  };

  struct blob_t {
    std::string data;
    size_t size = 0;
    Type type = Type::uint8;
  };

  Type getType() const {
    return type;
  }

  // Constructors
  RosValue() = default;
  explicit RosValue(const Type type) : type(type) {}

  // Convenience accessors
  const std::unique_ptr<RosValue> &operator()(const std::string &key) const;
  const std::unique_ptr<RosValue> &get(const std::string &key) const;
  const std::unique_ptr<RosValue> &at(size_t idx) const;

  template<typename T>
  const T &getValue(const std::string &key) const {
    if (type != object) {
      throw std::runtime_error("Value is not an object");
    }
    return objects.at(key)->getValueImpl(identity<T>());
  }

  template<typename T>
  const T &as() const {
    if (type == object) {
      throw std::runtime_error("Value cannot be an object for as");
    }
    return getValueImpl(identity<T>());
  }

  bool has(const std::string &key) const {
    if (type != object) {
      throw std::runtime_error("Value is not an object");
    }
    return objects.count(key) > 0;
  }

  void print(const std::string &path = "") const;

 private:

  // TODO: union these so we use less memory
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

  blob_t blob_storage;

  std::unordered_map<std::string, std::unique_ptr<RosValue>> objects;
  std::vector<std::unique_ptr<RosValue>> values;

  // Default type
  Type type = object;

  // Used for accessor template resolution
  template<typename T>
  struct identity { typedef T type; };

  // Primitive accessors
  const bool &getValueImpl(identity<bool>) const;
  const int8_t &getValueImpl(identity<int8_t>) const;
  const uint8_t &getValueImpl(identity<uint8_t>) const;
  const int16_t &getValueImpl(identity<int16_t>) const;
  const uint16_t &getValueImpl(identity<uint16_t>) const;
  const int32_t &getValueImpl(identity<int32_t>) const;
  const uint32_t &getValueImpl(identity<uint32_t>) const;
  const int64_t &getValueImpl(identity<int64_t>) const;
  const uint64_t &getValueImpl(identity<uint64_t>) const;
  const float &getValueImpl(identity<float>) const;
  const double &getValueImpl(identity<double>) const;
  const std::string &getValueImpl(identity<std::string>) const;
  const ros_time_t &getValueImpl(identity<ros_time_t>) const;
  const ros_duration_t &getValueImpl(identity<ros_duration_t>) const;
  const blob_t &getValueImpl(identity<blob_t>) const;

  friend class MessageParser;
};
}
