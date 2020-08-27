#pragma once

#include <memory>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "span.hpp"

namespace Embag {

class RosValue {
 public:

  enum class Type {
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

    double to_sec() const {
      return float(secs) + float(nsecs) / 1e9;
    }

    ros_time_t() {};
    ros_time_t(const uint32_t secs, const uint32_t nsecs) : secs(secs), nsecs(nsecs) {}

    bool operator==(const ros_time_t &other) const {
      return secs == other.secs && nsecs == other.nsecs;
    }
  };

  struct ros_duration_t {
    int32_t secs = 0;
    int32_t nsecs = 0;
  };

  struct blob_t {
    std::string data;
    size_t byte_size = 0;
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
  const RosValue &operator()(const std::string &key) const;
  const RosValue &operator[](const std::string &key) const;
  const RosValue &operator[](const size_t idx) const;
  const RosValue &get(const std::string &key) const;
  const RosValue &at(size_t idx) const;

  template<typename T>
  const T &getValue(const std::string &key) const {
    if (type != Type::object) {
      throw std::runtime_error("Value is not an object");
    }
    return objects.at(key)->getValueImpl(identity<T>());
  }

  template<typename T>
  const T &as() const {
    if (type == Type::object) {
      throw std::runtime_error("Value cannot be an object for as");
    }
    return getValueImpl(identity<T>());
  }

  bool has(const std::string &key) const {
    if (type != Type::object) {
      throw std::runtime_error("Value is not an object");
    }
    return objects.count(key) > 0;
  }

  size_t size() const {
    if (type != Type::array) {
      throw std::runtime_error("Value is not an array");
    }

    return values.size();
  }

  std::unordered_map<std::string, std::shared_ptr<RosValue>> getObjects() const {
    return objects;
  }

  std::vector<std::shared_ptr<RosValue>> getValues() const {
    return values;
  }

  blob_t getBlob() const {
    if (type != Type::blob) {
      throw std::runtime_error("Value is not blob");
    }

    return blob_storage;
  }

  std::string toString(const std::string &path = "") const;
  void print(const std::string &path = "") const;
  const nonstd::span<char> original_buffer() const {
    return original_buffer_;
  }

  // Used for accessor template resolution
  template<typename T>
  struct identity { typedef T type; };

 private:

  // TODO: use boost::variant?
  union {
    bool bool_value;
    int8_t int8_value;
    uint8_t uint8_value;
    int16_t int16_value;
    uint16_t uint16_value;
    int32_t int32_value;
    uint32_t uint32_value;
    int64_t int64_value;
    uint64_t uint64_value;
    float float32_value;
    double float64_value;
  };

  std::string string_value;
  ros_time_t time_value;
  ros_duration_t duration_value;

  blob_t blob_storage;

  // Only set when this is a message (type = object)
  nonstd::span<char> original_buffer_;

  std::unordered_map<std::string, std::shared_ptr<RosValue>> objects;
  std::vector<std::shared_ptr<RosValue>> values;

  // Default type
  Type type = Type::object;

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
