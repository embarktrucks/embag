#include <iostream>
#include <sstream>

#include "ros_value.h"

namespace Embag {

const RosValue &RosValue::operator()(const std::string &key) const {
  return get(key);
}

const RosValue &RosValue::operator[](const std::string &key) const {
  return get(key);
}

const RosValue &RosValue::operator[](const size_t idx) const {
  return at(idx);
}

const RosValue &RosValue::get(const std::string &key) const {
  if (type != Type::object) {
    throw std::runtime_error("Value is not an object");
  }
  return objects.at(key).get();
}


const RosValue &RosValue::at(const size_t idx) const {
  if (type != Type::array) {
    throw std::runtime_error("Value is not an array");
  }
  return values.at(idx).get();
}

std::string RosValue::toString(const std::string &path) const {
  switch (type) {
    case Type::ros_bool: {
      return path + " -> " + (as<bool>() ? "true" : "false");
    }
    // FIXME: Should have some mapping from type to as function some how
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
      // FIXME: actually print the string value
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
      for (const auto &object : objects) {
        if (path.empty()) {
          output << object.second.get().toString(object.first);
        } else {
          output << object.second.get().toString(path + "." + object.first);
        }

        // No need for a newline if our child is an object or array
        const auto &object_type = object.second.get().getType();
        if (!(object_type == Type::object || object_type == Type::array)) {
          output << std::endl;
        }
      }
      return output.str();
    }
    case Type::array: {
      std::ostringstream output;
      size_t i = 0;
      for (const auto &item : values) {
        const std::string array_path = path + "[" + std::to_string(i++) + "]";
        output << item.get().toString(array_path) << std::endl;
      }

      return output.str();
    }
    default: {
      return path + " -> unknown type";
    }
  }
}
}
