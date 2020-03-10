#include <iostream>

#include "ros_value.h"

void RosValue::print(const std::string &path) {
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
      std::cout << path << " -> " << time_value.secs <<  "s " << time_value.nsecs << "ns" << std::endl;
      break;
    }
    case RosValue::Type::ros_duration: {
      std::cout << path << " -> " << duration_value.secs <<  "s " << duration_value.nsecs << "ns" << std::endl;
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
  }
}
