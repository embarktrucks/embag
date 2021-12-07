#pragma once

#include <memory>
#include <cstdint>
#include <cstring>
#include <pybind11/buffer_info.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "span.hpp"
#include "util.h"

namespace Embag {

class RosValue {
 public:
  class RosValuePointer : public VectorItemPointer<RosValue> {
   public:
    RosValuePointer()
    {
    }

    RosValuePointer(const std::weak_ptr<std::vector<RosValue>>& base)
      : RosValuePointer(base, 0)
    {
    }

    RosValuePointer(const std::weak_ptr<std::vector<RosValue>>& base, size_t index)
      : VectorItemPointer<RosValue>(base.lock(), index)
    {
    }

    const RosValuePointer operator()(const std::string &key) const {
      return (**this)(key);
    }

    const RosValuePointer operator[](const std::string &key) const {
      return (**this)[key];
    }

    const RosValuePointer operator[](const size_t idx) const {
      return (**this)[idx];
    }
  };

  struct ros_value_list_t {
    std::weak_ptr<std::vector<RosValue>> base;
    size_t offset;
    size_t length;

    const RosValuePointer at(size_t index) const {
      return RosValuePointer(base, offset + index);
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
  static size_t primitiveTypeToSize(const Type type);
  static std::string primitiveTypeToFormat(const Type type);

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
    return type_;
  }

 private:
  template<class ReturnType, class IndexType, class ChildIteratorType>
  class const_iterator_base {
   public:
    bool operator==(const ChildIteratorType& other) const {
      return index_ == other.index_;
    }

    bool operator!=(const ChildIteratorType& other) const {
      return !(*this == other);
    }

    ChildIteratorType& operator++() {
      ++index_;
      return *((ChildIteratorType*) this);
    }

    ChildIteratorType operator++(int) {
      return {value_, index_++};
    }
   protected:
    const_iterator_base(const RosValue& value, size_t index)
      : value_(value)
      , index_(index)
    {
    }

    const RosValue& value_;
    IndexType index_;
  };

 public:
  // Iterator that implements an InputIterator over the children
  // of RosValues that are of type object or array
  template<class ReturnType, class IndexType>
  class const_iterator;

  template<class ReturnType>
  class const_iterator<ReturnType, size_t> : public const_iterator_base<ReturnType, size_t, const_iterator<ReturnType, size_t>> {
   public:
    const_iterator(const RosValue& value, size_t index)
      : const_iterator_base<ReturnType, size_t, const_iterator<ReturnType, size_t>>(value, index)
    {
      if (value.type_ != Type::object && value.type_ != Type::array) {
        throw std::runtime_error("Cannot iterate a RosValue that is not an object or array");
      }
    }

    const ReturnType operator*() const {
      return this->value_.getChildren().at(this->index_);
    }
  };

  template<class ReturnType>
  class const_iterator<ReturnType, std::unordered_map<std::string, size_t>::const_iterator> : public const_iterator_base<ReturnType, std::unordered_map<std::string, size_t>::const_iterator, const_iterator<ReturnType, std::unordered_map<std::string, size_t>::const_iterator>> {
   public:
    const_iterator(const RosValue& value, std::unordered_map<std::string, size_t>::const_iterator index)
      : const_iterator_base<ReturnType, size_t, std::unordered_map<std::string, size_t>::const_iterator>(value, index)
    {
      if (value.type_ != Type::object) {
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
    if (type_ != Type::object) {
      throw std::runtime_error("Cannot iterate over the items of a RosValue that is not an object");
    }

    return RosValue::const_iterator<IteratorReturnType, std::unordered_map<std::string, size_t>::const_iterator>(*this, object_info_.field_indexes->begin());
  }
  template<class IteratorReturnType>
  const_iterator<IteratorReturnType, std::unordered_map<std::string, size_t>::const_iterator> endItems() const {
    if (type_ != Type::object) {
      throw std::runtime_error("Cannot iterate over the items of a RosValue that is not an object");
    }

    return RosValue::const_iterator<IteratorReturnType, std::unordered_map<std::string, size_t>::const_iterator>(*this, object_info_.field_indexes->end());
  }

 private:
  struct _array_identifier {};
 public:
  RosValue(const Type type)
    : type_(type)
    , primitive_info_()
  {
    if (type_ == Type::object || type_ == Type::array) {
      throw std::runtime_error("Cannot create an object or array with this constructor");
    }
  }
  RosValue(const std::shared_ptr<std::unordered_map<std::string, const size_t>>& field_indexes)
    : type_(Type::object)
    , object_info_()
  {
    object_info_.field_indexes = field_indexes;
  }
  RosValue(const _array_identifier &i)
    : type_(Type::array)
    , array_info_()
  {
  }

  // Define custom copy constructor and destructor because of the union for the infos
  RosValue(const RosValue &other): type_(other.type_) {
    if (type_ == Type::object) {
      new (&object_info_) auto(other.object_info_);
    } else if (type_ == Type::array) {
      new (&array_info_) auto(other.array_info_);
    } else {
      new (&primitive_info_) auto(other.primitive_info_);
    }
  }
  ~RosValue() {
    if (type_ == Type::object) {
      object_info_.~object_info_t();
    } else if (type_ == Type::array) {
      array_info_.~array_info_t();
    } else {
      primitive_info_.~primitive_info_t();
    }
  }


  // Convenience accessors
  const RosValuePointer operator()(const std::string &key) const;
  const RosValuePointer operator[](const std::string &key) const;
  const RosValuePointer operator[](const size_t idx) const;
  const RosValuePointer get(const std::string &key) const;
  const RosValuePointer at(size_t idx) const;
  const RosValuePointer at(const std::string &key) const;

  template<typename T>
  const T &getValue(const std::string &key) const {
    return get(key)->as<T>();
  }

  template<typename T>
  const T as() const {
    if (type_ == Type::object || type_ == Type::array) {
      throw std::runtime_error("Value cannot be an object or array for as");
    }

    // TODO: Add check that the underlying type aligns with T
    return *getPrimitivePointer<T>();
  }

  bool has(const std::string &key) const {
    if (type_ != Type::object) {
      throw std::runtime_error("Value is not an object");
    }

    return object_info_.field_indexes->count(key);
  }

  size_t size() const {
    if (type_ != Type::array && type_ != Type::object) {
      throw std::runtime_error("Value is not an array or an object");
    }

    return getChildren().length;
  }

  std::unordered_map<std::string, RosValuePointer> getObjects() const {
    if (type_ != Type::object) {
      throw std::runtime_error("Cannot getObjects of a non-object RosValue");
    }

    std::unordered_map<std::string, RosValuePointer> objects;
    objects.reserve(object_info_.children.length);
    for (const auto& field : *object_info_.field_indexes) {
      objects.emplace(field.first, RosValuePointer(object_info_.children.base, object_info_.children.offset + field.second));
    }
    return objects;
  }

  std::vector<RosValuePointer> getValues() const {
    if (type_ != Type::object && type_ != Type::array) {
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

  // Used for python bindings
  pybind11::buffer_info getPrimitiveArrayBufferInfo() {
    if (getType() != Embag::RosValue::Type::array) {
      throw std::runtime_error("Only arrays can be represented as buffers!");
    }

    const Embag::RosValue::Type type_of_elements = at(0)->getType();
    if (type_of_elements == Embag::RosValue::Type::object || type_of_elements == Embag::RosValue::Type::string) {
      throw std::runtime_error("In order to be represented as a buffer, an array's elements must not be objects or strings!");
    }

    const size_t size_of_elements = Embag::RosValue::primitiveTypeToSize(type_of_elements);
    return pybind11::buffer_info(
      (void*) at(0)->getPrimitivePointer<void>(),
      size_of_elements,
      Embag::RosValue::primitiveTypeToFormat(type_of_elements),
      1,
      { size() },
      { size_of_elements },
      false
    );
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
    std::shared_ptr<std::unordered_map<std::string, const size_t>> field_indexes;
  };

  Type type_;
  union {
    primitive_info_t primitive_info_;
    array_info_t array_info_;
    object_info_t object_info_;
  };

  template<typename T>
  const T* const getPrimitivePointer() const {
    return reinterpret_cast<const T* const>(&primitive_info_.message_buffer->at(primitive_info_.offset));
  }

  const ros_value_list_t& getChildren() const {
    switch(type_) {
      case Type::object:
        return object_info_.children;
      case Type::array:
        return array_info_.children;
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
