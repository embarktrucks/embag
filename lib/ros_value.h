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

    const RosValue& operator*() const {
      return base->at(index);
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

 private:
  template<class ReturnType, class IndexType, class ChildIteratorType>
  struct _const_iterator {
   public:
    bool operator==(const ChildIteratorType& other) const {
      return index == other.index;
    }

    bool operator!=(const ChildIteratorType& other) const {
      return !(*this == other);
    }

    ChildIteratorType& operator++() {
      ++index;
      return *((ChildIteratorType*) this);
    }

    ChildIteratorType operator++(int) {
      return {value, index++};
    }
   protected:
    _const_iterator(const RosValue& value, size_t index)
      : value(value)
      , index(index)
    {
    }

    const RosValue& value;
    IndexType index;
  };

 public:
  template<class ReturnType, class IndexType>
  struct const_iterator;

  template<class ReturnType>
  struct const_iterator<ReturnType, size_t> : public _const_iterator<ReturnType, size_t, const_iterator<ReturnType, size_t>> {
   public:
    const_iterator(const RosValue& value, size_t index)
      : _const_iterator<ReturnType, size_t, const_iterator<ReturnType, size_t>>(value, index)
    {
      if (value.type != Type::object && value.type != Type::array) {
        throw std::runtime_error("Cannot iterate a RosValue that is not an object or array");
      }
    }

    const ReturnType operator*() const {
      return this->value.children.at(index);
    }
  };

  template<class ReturnType>
  struct const_iterator<ReturnType, std::unordered_map<std::string, size_t>::const_iterator> : public _const_iterator<ReturnType, std::unordered_map<std::string, size_t>::const_iterator, const_iterator<ReturnType, std::unordered_map<std::string, size_t>::const_iterator>> {
   public:
    const_iterator(const RosValue& value, std::unordered_map<std::string, size_t>::const_iterator index)
      : _const_iterator<ReturnType, size_t, std::unordered_map<std::string, size_t>::const_iterator>(value, index)
    {
      if (value.type != Type::object) {
        throw std::runtime_error("Cannot only iterate the keys or key/value pairs of an object");
      }
    }

    const ReturnType operator*() const;
  };

  template<class IteratorReturnType>
  const_iterator<IteratorReturnType, size_t> beginValues() const {
    return RosValue::const_iterator<IteratorReturnType, size_t>(*this, 0);
  }
  template<class IteratorReturnType>
  const_iterator<IteratorReturnType, size_t> endValues() const {
    return RosValue::const_iterator<IteratorReturnType, size_t>(*this, children.length);
  }
  template<class IteratorReturnType>
  const_iterator<IteratorReturnType, std::unordered_map<std::string, size_t>::const_iterator> beginItems() const {
    return RosValue::const_iterator<IteratorReturnType, std::unordered_map<std::string, size_t>::const_iterator>(*this, field_indexes->begin());
  }
  template<class IteratorReturnType>
  const_iterator<IteratorReturnType, std::unordered_map<std::string, size_t>::const_iterator> endItems() const {
    return RosValue::const_iterator<IteratorReturnType, std::unordered_map<std::string, size_t>::const_iterator>(*this, field_indexes->end());
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

  bool has(const std::string &key) const {
    if (type != Type::object) {
      throw std::runtime_error("Value is not an object");
    }

    return field_indexes->count(key);
  }

  size_t size() const {
    if (type != Type::array && type != Type::object) {
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
    for (size_t i = 0; i < children.length; ++i) {
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
