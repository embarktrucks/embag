#include <iostream>
#include <sstream>

#include "ros_value.h"

namespace Embag {

const RosValue::RosValuePointer RosValue::operator()(const std::string &key) const {
  return get(key);
}

const RosValue::RosValuePointer RosValue::operator[](const std::string &key) const {
  return get(key);
}

const RosValue::RosValuePointer RosValue::operator[](const size_t idx) const {
  return at(idx);
}

const RosValue::RosValuePointer RosValue::at(const std::string &key) const {
  return get(key);
}

const RosValue::RosValuePointer RosValue::get(const std::string &key) const {
  if (type_ != Type::object) {
    throw std::runtime_error("Value is not an object");
  }

  return object_info_.children.at(object_info_.field_indexes->at(key));
}

template<>
const std::string RosValue::as<std::string>() const {
  if (type_ != Type::string) {
    throw std::runtime_error("Cannot call as<std::string> for a non string");
  }

  const uint32_t string_length = *reinterpret_cast<const uint32_t* const>(getPrimitivePointer());
  const char* const string_loc = reinterpret_cast<const char* const>(getPrimitivePointer() + sizeof(uint32_t));
  return std::string(string_loc, string_loc + string_length);
}

const RosValue::RosValuePointer RosValue::at(const size_t idx) const {
  if (type_ != Type::array) {
    throw std::runtime_error("Value is not an array");
  }

  return array_info_.children.at(idx);
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
const std::pair<const std::string&, const RosValue::RosValuePointer> RosValue::const_iterator<const std::pair<const std::string&, const RosValue::RosValuePointer>, std::unordered_map<std::string, size_t>::const_iterator>::operator*() const {
  return std::make_pair(index_->first, value_.object_info_.children.at(index_->second));
}

}
