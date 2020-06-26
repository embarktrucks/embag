#include <iostream>
#include <memory>
#include <string>

#include <boost/optional.hpp>

#include "message_parser.h"
#include "util.h"

namespace Embag {

std::unique_ptr<RosValue> MessageParser::parse() {
  auto parsed_message = make_unique<RosValue>(RosValue::Type::object);
  parsed_message->original_buffer_ = stream_;

  for (const auto &member : msg_def_->members) {
    if (member.which() == 0) {  // ros_msg_field
      auto field = boost::get<RosMsgTypes::ros_msg_field>(member);
      parsed_message->objects[field.field_name] = parseField(scope_, field);
    }
  }

  return parsed_message;
}

std::unique_ptr<RosValue> MessageParser::parseField(const std::string &scope, RosMsgTypes::ros_msg_field &field) {
  auto parsed_field = make_unique<RosValue>();
  auto &primitive_type_map = RosMsgTypes::ros_msg_field::primitive_type_map_;

  switch (field.array_size) {
    // Undefined number of array objects
    case -1: {
      uint32_t array_len;
      read_into(&array_len);

      parsed_field->type = RosValue::Type::array;

      if (array_len == 0) {
        return parsed_field;
      }

      if (primitive_type_map.find(field.type_name) != primitive_type_map.end()) {
        // This is a primitive type array
        parsed_field = getPrimitiveBlob(field, array_len);
      } else {
        // This is an array of embedded types
        auto embedded_type = getEmbeddedType(scope, field);
        parseArray(array_len, embedded_type, parsed_field);
      }
      break;
    }
    // Not an array
    case 0: {
      // Primitive type
      if (primitive_type_map.find(field.type_name) != primitive_type_map.end()) {
        parsed_field = getPrimitiveField(field);
      } else {
        // Embedded type
        parsed_field->type = RosValue::Type::object;
        parsed_field->original_buffer_ = stream_;
        auto embedded_type = getEmbeddedType(scope, field);
        for (const auto &member : embedded_type.members) {
          if (member.which() == 0) {  // ros_msg_field
            auto embedded_field = boost::get<RosMsgTypes::ros_msg_field>(member);
            parsed_field->objects[embedded_field.field_name] = parseField(embedded_type.getScope(), embedded_field);
          }
        }
      }
      break;
    }
    // Array with fixed size
    default: {
      parsed_field->type = RosValue::Type::array;
      if (primitive_type_map.find(field.type_name) != primitive_type_map.end()) {
        parsed_field = getPrimitiveBlob(field, field.array_size);
      } else {
        auto embedded_type = getEmbeddedType(scope, field);
        parseArray(field.array_size, embedded_type, parsed_field);
      }

      break;
    }
  }

  return parsed_field;
}

void MessageParser::parseArray(const size_t array_len,
                               RosMsgTypes::ros_embedded_msg_def &embedded_type,
                               std::unique_ptr<RosValue> &value) {
  value->values.reserve(array_len);
  for (size_t i = 0; i < array_len; i++) {
    value->values.emplace_back(parseMembers(embedded_type));
  }
}

std::unique_ptr<RosValue> MessageParser::parseMembers(RosMsgTypes::ros_embedded_msg_def &embedded_type) {
  auto values = make_unique<RosValue>(RosValue::Type::object);
  values->original_buffer_ = stream_;

  for (const auto &member : embedded_type.members) {
    if (member.which() == 0) {  // ros_msg_field
      auto embedded_field = boost::get<RosMsgTypes::ros_msg_field>(member);
      values->objects[embedded_field.field_name] = parseField(embedded_type.getScope(), embedded_field);
    }
  }

  return values;
}

RosMsgTypes::ros_embedded_msg_def MessageParser::getEmbeddedType(const std::string &scope,
                                                           const RosMsgTypes::ros_msg_field &field) {
  // TODO: optimize this with a map or something faster
  for (const auto &embedded_type : msg_def_->embedded_types) {
    if (embedded_type.type_name == field.type_name) {
      return embedded_type;
    }

    // ROS allows a type to lack its scope
    if (scope.length() >= embedded_type.type_name.length()) {
      continue;
    }

    const size_t scope_pos = embedded_type.type_name.find_first_of(scope);
    if (scope_pos != std::string::npos) {
      if (embedded_type.type_name.substr(scope.length() + 1) == field.type_name) {
        return embedded_type;
      }
    }
  }

  throw std::runtime_error("Unable to find embedded type: " + field.type_name);
}

std::unique_ptr<RosValue> MessageParser::getPrimitiveBlob(RosMsgTypes::ros_msg_field &field, uint32_t len) {
  auto value = make_unique<RosValue>(RosValue::Type::blob);
  const auto& type = field.get_ros_type();
  value->blob_storage.size = len;
  value->blob_storage.type = type;

  size_t bytes = 0;

  switch (type) {
    case RosValue::ros_bool:
    case RosValue::int8:
    case RosValue::uint8: {
      bytes = sizeof(uint8_t) * len;
      break;
    }
    case RosValue::int16:
    case RosValue::uint16: {
      bytes = sizeof(uint16_t) * len;
      break;
    }
    case RosValue::int32:
    case RosValue::uint32:
    case RosValue::float32: {
      bytes = sizeof(uint32_t) * len;
      break;
    }
    case RosValue::int64:
    case RosValue::uint64:
    case RosValue::float64: {
      bytes = sizeof(uint64_t) * len;
      break;
    }
    case RosValue::blob:
    default: {
      throw std::runtime_error("Unable to read unknown blob type: " + field.type_name);
    };
  }

  value->blob_storage.data.reserve(bytes);
  read_into(value->blob_storage.data, bytes);

  return value;
}

template <typename T>
void MessageParser::read_into(T* dest) {
  memcpy(dest, stream_.data(), sizeof(T));
  stream_ = stream_.subspan(sizeof(T));
}

void MessageParser::read_into(std::string& dest, size_t size) {
  dest = {stream_.data(), size};
  stream_ = stream_.subspan(size);
}

std::unique_ptr<RosValue> MessageParser::getPrimitiveField(RosMsgTypes::ros_msg_field &field) {
  RosValue::Type type = field.get_ros_type();
  auto value = make_unique<RosValue>(type);

  switch (type) {
    case RosValue::ros_bool: {
      read_into(&value->bool_value);
      break;
    }
    case RosValue::int8: {
      read_into(&value->int8_value);
      break;
    }
    case RosValue::uint8: {
      read_into(&value->uint8_value);
      break;
    }
    case RosValue::int16: {
      read_into(&value->int16_value);
      break;
    }
    case RosValue::uint16: {
      read_into(&value->uint16_value);
      break;
    }
    case RosValue::int32: {
      read_into(&value->int32_value);
      break;
    }
    case RosValue::uint32: {
      read_into(&value->uint32_value);
      break;
    }
    case RosValue::int64: {
      read_into(&value->int64_value);
      break;
    }
    case RosValue::uint64: {
      read_into(&value->uint64_value);
      break;
    }
    case RosValue::float32: {
      read_into(&value->float32_value);
      break;
    }
    case RosValue::float64: {
      read_into(&value->float64_value);
      break;
    }
    case RosValue::string: {
      uint32_t string_len;
      read_into(&string_len);
      value->string_value.reserve(string_len);
      read_into(value->string_value, string_len);
      break;
    }
    case RosValue::ros_time: {
      read_into(&value->time_value.secs);
      read_into(&value->time_value.nsecs);
      break;
    }
    case RosValue::ros_duration: {
      read_into(&value->duration_value.secs);
      read_into(&value->duration_value.nsecs);
      break;
    }
    default: {
      throw std::runtime_error("Unable to read unknown type: " + field.type_name);
    };
  }

  return value;
}
}
