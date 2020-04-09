#include <iostream>

#include "ros_value.h"

namespace Embag {

const std::unique_ptr<RosValue> &RosValue::operator()(const std::string &key) const {
  return get(key);
}

const std::unique_ptr<RosValue> &RosValue::get(const std::string &key) const {
  if (type != object) {
    throw std::runtime_error("Value is not an object");
  }
  return objects.at(key);
}

const std::unique_ptr<RosValue> &RosValue::at(const size_t idx) const {
  if (type != array) {
    throw std::runtime_error("Value is not an array");
  }
  return values.at(idx);
}

const bool &RosValue::getValueImpl(identity<bool>) const {
  if (type != ros_bool) {
    throw std::runtime_error("Value is not a bool");
  }
  return bool_value;
}

const int8_t &RosValue::getValueImpl(identity<int8_t>) const {
  if (type != int8) {
    throw std::runtime_error("Value is not an int8");
  }
  return int8_value;
}

const uint8_t &RosValue::getValueImpl(identity<uint8_t>) const {
  if (type != uint8) {
    throw std::runtime_error("Value is not a uint8");
  }
  return uint8_value;
}

const int16_t &RosValue::getValueImpl(identity<int16_t>) const {
  if (type != int16) {
    throw std::runtime_error("Value is not an int16");
  }
  return int16_value;
}

const uint16_t &RosValue::getValueImpl(identity<uint16_t>) const {
  if (type != uint16) {
    throw std::runtime_error("Value is not a uint16");
  }
  return uint16_value;
}

const int32_t &RosValue::getValueImpl(identity<int32_t>) const {
  if (type != int32) {
    throw std::runtime_error("Value is not an int32");
  }
  return int32_value;
}

const uint32_t &RosValue::getValueImpl(identity<uint32_t>) const {
  if (type != uint32) {
    throw std::runtime_error("Value is not a uint32");
  }
  return uint32_value;
}

const int64_t &RosValue::getValueImpl(identity<int64_t>) const {
  if (type != int64) {
    throw std::runtime_error("Value is not an int64");
  }
  return int64_value;
}

const uint64_t &RosValue::getValueImpl(identity<uint64_t>) const {
  if (type != uint64) {
    throw std::runtime_error("Value is not a uint64");
  }
  return uint64_value;
}

const float &RosValue::getValueImpl(identity<float>) const {
  if (type != uint64) {
    throw std::runtime_error("Value is not a float");
  }
  return float32_value;
}

const double &RosValue::getValueImpl(identity<double>) const {
  if (type != uint64) {
    throw std::runtime_error("Value is not a double");
  }
  return float64_value;
}

const std::string &RosValue::getValueImpl(identity<std::string>) const {
  if (type != string) {
    throw std::runtime_error("Value is not a string");
  }
  return string_value;
}

const RosValue::ros_time_t &RosValue::getValueImpl(identity<RosValue::ros_time_t>) const {
  if (type != ros_time) {
    throw std::runtime_error("Value is not a time type");
  }
  return time_value;
}

const RosValue::ros_duration_t &RosValue::getValueImpl(identity<RosValue::ros_duration_t>) const {
  if (type != ros_duration) {
    throw std::runtime_error("Value is not a duration type");
  }
  return duration_value;
}

const RosValue::blob_t &RosValue::getValueImpl(identity<RosValue::blob_t>) const {
  if (type != blob) {
    throw std::runtime_error("Value is not a blob type");
  }

  return blob_storage;
}

void RosValue::print(const std::string &path) const {
  switch (type) {
    case Type::ros_bool: {
      std::cout << path << " -> " << (bool_value ? "true" : "false") << std::endl;
      break;
    }
    case RosValue::Type::int8: {
      std::cout << path << " -> " << +int8_value << std::endl;
      break;
    }
    case RosValue::Type::uint8: {
      std::cout << path << " -> " << +uint8_value << std::endl;
      break;
    }
    case RosValue::Type::int16: {
      std::cout << path << " -> " << +int16_value << std::endl;
      break;
    }
    case RosValue::Type::uint16: {
      std::cout << path << " -> " << +uint16_value << std::endl;
      break;
    }
    case RosValue::Type::int32: {
      std::cout << path << " -> " << +int32_value << std::endl;
      break;
    }
    case RosValue::Type::uint32: {
      std::cout << path << " -> " << +uint32_value << std::endl;
      break;
    }
    case RosValue::Type::int64: {
      std::cout << path << " -> " << +int64_value << std::endl;
      break;
    }
    case RosValue::Type::uint64: {
      std::cout << path << " -> " << +uint64_value << std::endl;
      break;
    }
    case RosValue::Type::float32: {
      std::cout << path << " -> " << +float32_value << std::endl;
      break;
    }
    case RosValue::Type::float64: {
      std::cout << path << " -> " << +float64_value << std::endl;
      break;
    }
    case RosValue::Type::string: {
      std::cout << path << " -> " << string_value << std::endl;
      break;
    }
    case RosValue::Type::ros_time: {
      std::cout << path << " -> " << time_value.secs << "s " << time_value.nsecs << "ns" << std::endl;
      break;
    }
    case RosValue::Type::ros_duration: {
      std::cout << path << " -> " << duration_value.secs << "s " << duration_value.nsecs << "ns" << std::endl;
      break;
    }
    case RosValue::Type::object: {
      for (const auto &object : objects) {
        if (path.empty()) {
          object.second->print(object.first);
        } else {
          object.second->print(path + "." + object.first);
        }
      }
      break;
    }
    case RosValue::Type::array: {
      size_t i = 0;
      for (const auto &item : values) {
        const std::string array_path = path + "[" + std::to_string(i++) + "]";
        item->print(array_path);
      }
      break;
    }
    case RosValue::Type::blob: {
      std::cout << path << " -> blob" << std::endl;
      break;
    }
    default: {
      std::cout << path << " -> unknown type" << std::endl;
      break;
    }
  }
}
}
