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
      return this->value.getChildren().at(index);
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
    return RosValue::const_iterator<IteratorReturnType, size_t>(*this, getChildren().length);
  }
  template<class IteratorReturnType>
  const_iterator<IteratorReturnType, std::unordered_map<std::string, size_t>::const_iterator> beginItems() const {
    if (type != Type::object) {
      throw std::runtime_error("Cannot iterate over the items of a RosValue that is not an object");
    }

    return RosValue::const_iterator<IteratorReturnType, std::unordered_map<std::string, size_t>::const_iterator>(*this, object_info.field_indexes->begin());
  }
  template<class IteratorReturnType>
  const_iterator<IteratorReturnType, std::unordered_map<std::string, size_t>::const_iterator> endItems() const {
    if (type != Type::object) {
      throw std::runtime_error("Cannot iterate over the items of a RosValue that is not an object");
    }

    return RosValue::const_iterator<IteratorReturnType, std::unordered_map<std::string, size_t>::const_iterator>(*this, object_info.field_indexes->end());
  }

 private:
  struct _array_identifier {};
 public:
  RosValue(const Type type)
    : type(type)
    , primitive_info()
  {
    if (type == Type::object || type == Type::array) {
      throw std::runtime_error("Cannot create an object or array with this constructor");
    }
  }
  RosValue(std::shared_ptr<std::unordered_map<std::string, size_t>> field_indexes)
    : type(Type::object)
    , object_info()
  {
    object_info.field_indexes = field_indexes;
  }
  RosValue(const _array_identifier &i)
    : type(Type::array)
    , array_info()
  {
  }
  RosValue(const RosValue &other): type(other.type) {
    if (type == Type::object) {
      new (&object_info) auto(other.object_info);
    } else if (type == Type::array) {
      new (&array_info) auto(other.array_info);
    } else {
      new (&primitive_info) auto(other.primitive_info);
    }
  }
  ~RosValue() {
    if (type == Type::object) {
      object_info.~object_info_t();
    } else if (type == Type::array) {
      array_info.~array_info_t();
    } else {
      primitive_info.~primitive_info_t();
    }
  }


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

    return object_info.field_indexes->count(key);
  }

  size_t size() const {
    if (type != Type::array && type != Type::object) {
      throw std::runtime_error("Value is not an array or an object");
    }

    return getChildren().length;
  }

  std::unordered_map<std::string, RosValuePointer> getObjects() const {
    if (type != Type::object) {
      throw std::runtime_error("Cannot getObjects of a non-object RosValue");
    }

    std::unordered_map<std::string, RosValuePointer> objects;
    objects.reserve(object_info.children.length);
    for (const auto& field : *object_info.field_indexes) {
      objects.emplace(std::make_pair(field.first, RosValuePointer(object_info.children.base, object_info.children.offset + field.second)));
    }
    return objects;
  }

  std::vector<RosValuePointer> getValues() const {
    if (type != Type::object && type != Type::array) {
      throw std::runtime_error("Cannot getValues of a non object or array RosValue");
    }

    const ros_value_list_t& children = getChildren();
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
    size_t offset = 0;
    std::shared_ptr<std::vector<char>> message_buffer = nullptr;
  };

  struct array_info_t {
    ros_value_list_t children;
  };

  struct object_info_t {
    ros_value_list_t children;
    std::shared_ptr<std::unordered_map<std::string, size_t>> field_indexes;
  };

  Type type;
  union {
    primitive_info_t primitive_info;
    array_info_t array_info;
    object_info_t object_info;
  };

  const char* const getPrimitivePointer() const {
    return &primitive_info.message_buffer->at(primitive_info.offset);
  }

  const ros_value_list_t& getChildren() const {
    switch(type) {
      case Type::object:
        return object_info.children;
      case Type::array:
        return array_info.children;
      default:
        throw std::runtime_error("Cannot getChildren of a RosValue that is not an object or array");
    }
  }

  friend class MessageParser;
};

template<>
const std::string RosValue::as<std::string>() const;

template<>
const std::string& RosValue::const_iterator<const std::string&, std::unordered_map<std::string, size_t>::const_iterator>::operator*() const;

template<>
const std::pair<const std::string&, const RosValue&> RosValue::const_iterator<const std::pair<const std::string&, const RosValue&>, std::unordered_map<std::string, size_t>::const_iterator>::operator*() const;

}
