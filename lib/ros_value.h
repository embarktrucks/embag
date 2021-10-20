#pragma once

#include <memory>
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "span.hpp"

namespace Embag {

class RosValue {
 public:
  struct ros_value_list_t {
    std::shared_ptr<std::vector<RosValue>> base;
    size_t offset;
    size_t length;

    const RosValue& at(size_t index) const {
      return base->at(offset + index);
    }
  };

  struct RosValuePointer {
    std::shared_ptr<std::vector<RosValue>> base;
    size_t index;

    RosValuePointer(std::shared_ptr<std::vector<RosValue>> base, size_t index)
    : base(base)
    , index(index)
    {
    }

    const RosValue *operator->() const {
      return &base->at(index);
    }
  };

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
  };

  struct ros_time_t {
    uint32_t secs = 0;
    uint32_t nsecs = 0;

    double to_sec() const {
      return double(secs) + double(nsecs) / 1e9;
    }

    long to_nsec() const {
      return long(secs) * long(1e9) + long(nsecs);
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

    double to_sec() const {
      return double(secs) + double(nsecs) / 1e9;
    }

    long to_nsec() const {
      return long(secs) * long(1e9) + long(nsecs);
    }

    ros_duration_t() {};
    ros_duration_t(const uint32_t secs, const uint32_t nsecs) : secs(secs), nsecs(nsecs) {}

    bool operator==(const ros_duration_t &other) const {
      return secs == other.secs && nsecs == other.nsecs;
    }
  };

  Type getType() const {
    return type;
  }

  // Constructors
  RosValue(const Type type)
    : type(type)
    // , primitive_info({})
  {
    if (type == Type::object || type == Type::array) {
      throw std::runtime_error("Cannot create an object or array with this constructor");
    }
  }
 private:
  struct _array_identifier {};
 public:
  RosValue(std::shared_ptr<std::unordered_map<std::string, size_t>> field_indexes)
    : type(Type::object)
    , field_indexes(field_indexes)
  {
  }
  RosValue(const _array_identifier &i)
    : type(Type::array)
  {
  }
  // RosValue(const RosValue &other): type(other.type) {
  //   if (type == Type::object) {
  //     new (&objects) auto(other.objects);
  //   } else if (type == Type::array) {
  //     new (&values) auto(other.values);
  //   } else {
  //     new (&primitive_info) auto(other.primitive_info);
  //   }
  // }
  // ~RosValue() {
  //   if (type == Type::object) {
  //     objects.~unordered_map();
  //   } else if (type == Type::array) {
  //     values.~vector();
  //   } else {
  //     primitive_info.~primitive_info_t();
  //   }
  // }


  // Convenience accessors
  const RosValue &operator()(const std::string &key) const;
  const RosValue &operator[](const std::string &key) const;
  const RosValue &operator[](const size_t idx) const;
  const RosValue &get(const std::string &key) const;
  const RosValue &at(size_t idx) const;
  const RosValue &at(const std::string &key) const;

  template<typename T>
  const T &getValue(const std::string &key) const {
    return get(key).as<T>();
  }

  template<typename T>
  const T as() const {
    if (type == Type::object || type == Type::array) {
      throw std::runtime_error("Value cannot be an object or array for as");
    }

    // TODO: Add check that the underlying type aligns with T
    return *reinterpret_cast<const T*>(getPrimitivePointer());
  }

  const std::string as() const {
    if (type != Type::string) {
      throw std::runtime_error("Cannot call as<std::string> for a non string");
    }

    const uint32_t string_length = *reinterpret_cast<const uint32_t* const>(getPrimitivePointer());
    const char* const string_loc = reinterpret_cast<const char* const>(getPrimitivePointer() + sizeof(uint32_t));
    return std::string(string_loc, string_loc + string_length);
  }

  bool has(const std::string &key) const {
    if (type != Type::object) {
      throw std::runtime_error("Value is not an object");
    }

    return field_indexes->count(key);
  }

  size_t size() const {
    if (type != Type::array or type != Type::object) {
      throw std::runtime_error("Value is not an array or an object");
    }

    return children.length;
  }

  std::unordered_map<std::string, RosValuePointer> getObjects() const {
    std::unordered_map<std::string, RosValuePointer> objects;
    objects.reserve(children.length);
    for (const auto& field : *field_indexes) {
      objects.emplace(std::make_pair(field.first, RosValuePointer(children.base, children.offset + field.second)));
    }
    return objects;
  }

  std::vector<RosValuePointer> getValues() const {
    std::vector<RosValuePointer> values;
    values.reserve(children.length);
    for (size_t i; i < children.length; ++i) {
      values.emplace_back(children.base, children.offset + i);
    }
    return values;
  }

  std::string toString(const std::string &path = "") const;
  void print(const std::string &path = "") const;

  // Used for accessor template resolution
  template<typename T>
  struct identity { typedef T type; };

 private:
  struct primitive_info_t {
    size_t offset;
    std::shared_ptr<std::vector<char>> message_buffer;

    primitive_info_t() : offset(0), message_buffer(nullptr) {}
  };

  Type type;
  // FIXME: Implement as a union
  // union {
  //   // If this is a primitive
    primitive_info_t primitive_info;

    // If this is an object or an array
    ros_value_list_t children;

    // If this is an object
    std::shared_ptr<std::unordered_map<std::string, size_t>> field_indexes;
  // };

  const char* const getPrimitivePointer() const {
    return &primitive_info.message_buffer->at(primitive_info.offset);
  }

  friend class MessageParser;
};
}
