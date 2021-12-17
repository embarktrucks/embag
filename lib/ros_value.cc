#include <iostream>
#include <sstream>

#include "ros_value.h"

namespace Embag {

const RosValue::Pointer RosValue::ros_value_list_t::at(size_t index) const {
  if (index >= length) {
    throw std::out_of_range("Provided index is out of range!");
  }

  return RosValue::Pointer(base, offset + index);
}

const RosValue::Pointer RosValue::operator()(const std::string &key) const {
  return get(key);
}

const RosValue::Pointer RosValue::operator[](const std::string &key) const {
  return get(key);
}

const RosValue::Pointer RosValue::operator[](const size_t idx) const {
  return at(idx);
}

const RosValue::Pointer RosValue::at(const std::string &key) const {
  return get(key);
}

template<typename T>
const T &RosValue::getValue(const std::string &key) const {
  return get(key)->as<T>();
}

const RosValue::Pointer RosValue::get(const std::string &key) const {
  if (type_ != Type::object) {
    throw std::runtime_error("Value is not an object");
  }

  return at(object_info_.field_indexes->at(key));
}

template<>
const std::string RosValue::as<std::string>() const {
  if (type_ != Type::string) {
    throw std::runtime_error("Cannot call as<std::string> for a non string");
  }

  const uint32_t string_length = getPrimitive<uint32_t>();
  const char* const string_loc = &getPrimitive<char>() + sizeof(uint32_t);
  return std::string(string_loc, string_loc + string_length);
}

const RosValue::Pointer RosValue::at(const size_t idx) const {
  if (type_ == Type::object) {
    return object_info_.children.at(idx);
  } else if (type_ == Type::array) {
    return array_info_.children.at(idx);
  } else if (type_ == Type::primitive_array) {
    return RosValue::Pointer(
      primitive_array_info_.element_type,
      primitive_array_info_.message_buffer,
      primitive_array_info_.offset + idx
    );
  } else {
    throw std::runtime_error("Value is not an array or object");
  }
}

std::unordered_map<std::string, RosValue::Pointer> RosValue::getObjects() const {
  if (type_ != Type::object) {
    throw std::runtime_error("Cannot getObjects of a non-object RosValue");
  }

  std::unordered_map<std::string, RosValue::Pointer> objects;
  objects.reserve(object_info_.children.length);
  for (const auto& field : *object_info_.field_indexes) {
    objects.emplace(field.first, at(field.second));
  }
  return objects;
}

std::vector<RosValue::Pointer> RosValue::getValues() const {
  if (type_ != Type::object && type_ != Type::array && type_ != Type::primitive_array) {
    throw std::runtime_error("Cannot getValues of a non object or array RosValue");
  }

  std::vector<RosValue::Pointer> ros_value_pointers;
  const size_t size = this->size();
  ros_value_pointers.reserve(size);
  for (size_t i = 0; i < size; i++) {
    ros_value_pointers.push_back(at(i));
  }

  return ros_value_pointers;
}

pybind11::buffer_info RosValue::getPrimitiveArrayBufferInfo() {
  if (type_ != Embag::RosValue::Type::primitive_array) {
    throw std::runtime_error("Only primitive arrays can be represented as buffers!");
  }

  if (primitive_array_info_.element_type == Embag::RosValue::Type::string) {
    throw std::runtime_error("In order to be represented as a buffer, an array's elements must not be strings!");
  }

  const size_t size_of_elements = Embag::RosValue::primitiveTypeToSize(primitive_array_info_.element_type);
  return pybind11::buffer_info(
    (void*) &at(0)->getPrimitive<uint8_t>(),
    size_of_elements,
    Embag::RosValue::primitiveTypeToFormat(primitive_array_info_.element_type),
    1,
    { size() },
    { size_of_elements },
    true
  );
}

std::string RosValue::toString(const std::string &path) const {
  switch (type_) {
    case Type::ros_bool: {
      return path + " -> " + (as<bool>() ? "true" : "false");
    }
    // TODO: Could have some mapping from type to as function some how
    case Type::int8: {
      return path + " -> " + std::to_string(as<int8_t>());
    }
    case Type::uint8: {
      return path + " -> " + std::to_string(as<uint8_t>());
    }
    case Type::int16: {
      return path + " -> " + std::to_string(as<int16_t>());
    }
    case Type::uint16: {
      return path + " -> " + std::to_string(as<uint16_t>());
    }
    case Type::int32: {
      return path + " -> " + std::to_string(as<int32_t>());
    }
    case Type::uint32: {
      return path + " -> " + std::to_string(as<uint32_t>());
    }
    case Type::int64: {
      return path + " -> " + std::to_string(as<int64_t>());
    }
    case Type::uint64: {
      return path + " -> " + std::to_string(as<uint64_t>());
    }
    case Type::float32: {
      return path + " -> " + std::to_string(as<float>());
    }
    case Type::float64: {
      return path + " -> " + std::to_string(as<double>());
    }
    case Type::string: {
      return path + " -> " + as<std::string>();
    }
    case Type::ros_time: {
      const ros_time_t value = as<ros_time_t>();
      return path + " -> " + std::to_string(value.secs) + "s " + std::to_string(value.nsecs) + "ns";
    }
    case Type::ros_duration: {
      const ros_duration_t value = as<ros_duration_t>();
      return path + " -> " + std::to_string(value.secs) + "s " + std::to_string(value.nsecs) + "ns";
    }
    case Type::object: {
      std::ostringstream output;
      for (const auto& field : *object_info_.field_indexes) {
        if (path.empty()) {
          output << object_info_.children.at(field.second)->toString(field.first);
        } else {
          output << object_info_.children.at(field.second)->toString(path + "." + field.first);
        }

        // No need for a newline if our child is an object or array
        const auto &object_type = object_info_.children.at(field.second)->getType();
        if (!(object_type == Type::object || object_type == Type::array)) {
          output << std::endl;
        }
      }
      return output.str();
    }
    case Type::array: {
      std::ostringstream output;
      for (size_t i = 0; i < array_info_.children.length; ++i) {
        const std::string array_path = path + "[" + std::to_string(i) + "]";
        output << array_info_.children.at(i)->toString(array_path) << std::endl;
      }
      return output.str();
    }
    default: {
      return path + " -> unknown type";
    }
  }
}

void RosValue::print(const std::string &path) const {
  std::cout << toString(path);
}

size_t RosValue::primitiveTypeToSize(const Type type) {
  switch (type) {
    case (Type::ros_bool):
      return sizeof(bool);
    case (Type::int8):
      return sizeof(int8_t);
    case (Type::uint8):
      return sizeof(uint8_t);
    case (Type::int16):
      return sizeof(int16_t);
    case (Type::uint16):
      return sizeof(uint16_t);
    case (Type::int32):
      return sizeof(int32_t);
    case (Type::uint32):
      return sizeof(uint32_t);
    case (Type::int64):
      return sizeof(int64_t);
    case (Type::uint64):
      return sizeof(uint64_t);
    case (Type::float32):
      return sizeof(float);
    case (Type::float64):
      return sizeof(double);
    case (Type::ros_time):
      return sizeof(ros_time_t);
    case (Type::ros_duration):
      return sizeof(ros_duration_t);
    case (Type::string):
    case (Type::array):
    case (Type::object):
    default:
      throw std::runtime_error("Provided type is a string or a non-primitive!");
  }
}

std::string RosValue::primitiveTypeToFormat(const Type type) {
  switch (type) {
    case (Type::ros_bool):
      return "?";
    case (Type::int8):
      return "b";
    case (Type::uint8):
      return "B";
    case (Type::int16):
      return "h";
    case (Type::uint16):
      return "H";
    case (Type::int32):
      return "i";
    case (Type::uint32):
      return "I";
    case (Type::int64):
      return "q";
    case (Type::uint64):
      return "Q";
    case (Type::float32):
      return "f";
    case (Type::float64):
      return "d";
    case (Type::ros_time):
      return "II";
    case (Type::ros_duration):
      return "II";
    case (Type::string):
      throw std::runtime_error("Strings do not have a struct format!");
    case (Type::array):
    case (Type::object):
    default:
      throw std::runtime_error("Provided type is not a primitive!");
  }
}

/*
--------------
ITERATOR SETUP
--------------
*/
template<>
const std::string& RosValue::const_iterator<const std::string&, std::unordered_map<std::string, size_t>::const_iterator>::operator*() const {
  return index_->first;
}

template<>
const std::pair<const std::string&, const RosValue::Pointer> RosValue::const_iterator<const std::pair<const std::string&, const RosValue::Pointer>, std::unordered_map<std::string, size_t>::const_iterator>::operator*() const {
  return std::make_pair(index_->first, value_.object_info_.children.at(index_->second));
}

}
