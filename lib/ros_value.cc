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
  return *objects.at(key);
}


const RosValue &RosValue::at(const size_t idx) const {
  if (type != Type::array) {
    throw std::runtime_error("Value is not an array");
  }
  return *values.at(idx);
}

const bool &RosValue::getValueImpl(identity<bool>) const {
  if (type != Type::ros_bool) {
    throw std::runtime_error("Value is not a bool");
  }
  return bool_value;
}

const int8_t &RosValue::getValueImpl(identity<int8_t>) const {
  if (type != Type::int8) {
    throw std::runtime_error("Value is not an int8");
  }
  return int8_value;
}

const uint8_t &RosValue::getValueImpl(identity<uint8_t>) const {
  if (type != Type::uint8) {
    throw std::runtime_error("Value is not a uint8");
  }
  return uint8_value;
}

const int16_t &RosValue::getValueImpl(identity<int16_t>) const {
  if (type != Type::int16) {
    throw std::runtime_error("Value is not an int16");
  }
  return int16_value;
}

const uint16_t &RosValue::getValueImpl(identity<uint16_t>) const {
  if (type != Type::uint16) {
    throw std::runtime_error("Value is not a uint16");
  }
  return uint16_value;
}

const int32_t &RosValue::getValueImpl(identity<int32_t>) const {
  if (type != Type::int32) {
    throw std::runtime_error("Value is not an int32");
  }
  return int32_value;
}

const uint32_t &RosValue::getValueImpl(identity<uint32_t>) const {
  if (type != Type::uint32) {
    throw std::runtime_error("Value is not a uint32");
  }
  return uint32_value;
}

const int64_t &RosValue::getValueImpl(identity<int64_t>) const {
  if (type != Type::int64) {
    throw std::runtime_error("Value is not an int64");
  }
  return int64_value;
}

const uint64_t &RosValue::getValueImpl(identity<uint64_t>) const {
  if (type != Type::uint64) {
    throw std::runtime_error("Value is not a uint64");
  }
  return uint64_value;
}

const float &RosValue::getValueImpl(identity<float>) const {
  if (type != Type::float32) {
    throw std::runtime_error("Value is not a float");
  }
  return float32_value;
}

const double &RosValue::getValueImpl(identity<double>) const {
  if (type != Type::float64) {
    throw std::runtime_error("Value is not a double");
  }
  return float64_value;
}

const std::string &RosValue::getValueImpl(identity<std::string>) const {
  if (type != Type::string) {
    throw std::runtime_error("Value is not a string");
  }
  return string_value;
}

const RosValue::ros_time_t &RosValue::getValueImpl(identity<RosValue::ros_time_t>) const {
  if (type != Type::ros_time) {
    throw std::runtime_error("Value is not a time type");
  }
  return time_value;
}

const RosValue::ros_duration_t &RosValue::getValueImpl(identity<RosValue::ros_duration_t>) const {
  if (type != Type::ros_duration) {
    throw std::runtime_error("Value is not a duration type");
  }
  return duration_value;
}

const RosValue::blob_t &RosValue::getValueImpl(identity<RosValue::blob_t>) const {
  if (type != Type::blob) {
    throw std::runtime_error("Value is not a blob type");
  }

  return blob_storage;
}

std::string RosValue::toString(const std::string &path) const {
  switch (type) {
    case Type::ros_bool: {
      return path + " -> " + (bool_value ? "true" : "false");
    }
    case Type::int8: {
      return path + " -> " + std::to_string(int8_value);
    }
    case Type::uint8: {
      return path + " -> " + std::to_string(uint8_value);
    }
    case Type::int16: {
      return path + " -> " + std::to_string(int16_value);
    }
    case Type::uint16: {
      return path + " -> " + std::to_string(uint16_value);
    }
    case Type::int32: {
      return path + " -> " + std::to_string(int32_value);
    }
    case Type::uint32: {
      return path + " -> " + std::to_string(uint32_value);
    }
    case Type::int64: {
      return path + " -> " + std::to_string(int64_value);
    }
    case Type::uint64: {
      return path + " -> " + std::to_string(uint64_value);
    }
    case Type::float32: {
      return path + " -> " + std::to_string(float32_value);
    }
    case Type::float64: {
      return path + " -> " + std::to_string(float64_value);
    }
    case Type::string: {
      return path + " -> " + string_value;
    }
    case Type::ros_time: {
      return path + " -> " + std::to_string(time_value.secs) + "s " + std::to_string(time_value.nsecs) + "ns";
    }
    case Type::ros_duration: {
      return path + " -> " + std::to_string(duration_value.secs) + "s " + std::to_string(duration_value.nsecs) + "ns";
    }
    case Type::object: {
      std::ostringstream output;
      for (const auto &object : objects) {
        if (path.empty()) {
          output << object.second->toString(object.first);
        } else {
          output << object.second->toString(path + "." + object.first);
        }

        // No need for a newline if our child is an object or array
        const auto &object_type = object.second->getType();
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
        output << item->toString(array_path) << std::endl;
      }

      return output.str();
    }
    case Type::blob: {
      return path + " -> blob";
    }
    default: {
      return path + " -> unknown type";
    }
  }
}

void RosValue::print(const std::string &path) const {
  std::cout << toString(path);
}
}
