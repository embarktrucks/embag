#include <iostream>
#include <memory>
#include <string>

#include <boost/optional.hpp>

#include "message_parser.h"
#include "util.h"

namespace Embag {

std::shared_ptr<RosValue> MessageParser::parse() {
  auto parsed_message = std::make_shared<RosValue>(RosValue::Type::object);
  parsed_message->original_buffer_ = stream_;

  for (const auto &member : msg_def_->members) {
    if (member.which() == 0) {  // ros_msg_field
      auto field = boost::get<RosMsgTypes::ros_msg_field>(member);
      parsed_message->objects[field.field_name] = parseField(scope_, field);
    }
  }

  return parsed_message;
}

std::shared_ptr<RosValue> MessageParser::parseField(const std::string &scope, RosMsgTypes::ros_msg_field &field) {
  auto parsed_field = std::make_shared<RosValue>();
  auto &primitive_type_map = RosMsgTypes::ros_msg_field::primitive_type_map_;

  switch (field.array_size) {
    // Undefined number of array objects
    case -1: {
      uint32_t array_len;
      read_into(&array_len);

      parsed_field->type = RosValue::Type::array;

      if (primitive_type_map.count(field.type_name)) {
        // This is a primitive type array
        parsed_field = getPrimitiveBlob(field, array_len);
      } else {
        // This is an array of embedded types
        auto embedded_type = msg_def_->getEmbeddedType(scope, field);
        parseArray(array_len, embedded_type, parsed_field);
      }
      break;
    }
    // Not an array
    case 0: {
      // Primitive type
      if (primitive_type_map.count(field.type_name)) {
        parsed_field = getPrimitiveField(field);
      } else {
        // Embedded type
        parsed_field->type = RosValue::Type::object;
        parsed_field->original_buffer_ = stream_;
        auto embedded_type = msg_def_->getEmbeddedType(scope, field);
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
      if (primitive_type_map.count(field.type_name)) {
        parsed_field = getPrimitiveBlob(field, field.array_size);
      } else {
        auto embedded_type = msg_def_->getEmbeddedType(scope, field);
        parseArray(field.array_size, embedded_type, parsed_field);
      }

      break;
    }
  }

  return parsed_field;
}

void MessageParser::parseArray(const size_t array_len,
                               RosMsgTypes::ros_embedded_msg_def &embedded_type,
                               std::shared_ptr<RosValue> &value) {
  value->values.reserve(array_len);
  for (size_t i = 0; i < array_len; i++) {
    value->values.emplace_back(parseMembers(embedded_type));
  }
}

std::shared_ptr<RosValue> MessageParser::parseMembers(RosMsgTypes::ros_embedded_msg_def &embedded_type) {
  auto values = std::make_shared<RosValue>(RosValue::Type::object);
  values->original_buffer_ = stream_;

  for (const auto &member : embedded_type.members) {
    if (member.which() == 0) {  // ros_msg_field
      auto embedded_field = boost::get<RosMsgTypes::ros_msg_field>(member);
      values->objects[embedded_field.field_name] = parseField(embedded_type.getScope(), embedded_field);
    }
  }

  return values;
}

std::shared_ptr<RosValue> MessageParser::getPrimitiveBlob(RosMsgTypes::ros_msg_field &field, uint32_t len) {
  auto value = std::make_shared<RosValue>(RosValue::Type::blob);
  const auto& type = field.get_ros_type();
  value->blob_storage.size = len;
  value->blob_storage.type = type;

  size_t bytes = 0;

  switch (type) {
    case RosValue::Type::ros_bool:
    case RosValue::Type::int8:
    case RosValue::Type::uint8: {
      bytes = sizeof(uint8_t) * len;
      break;
    }
    case RosValue::Type::int16:
    case RosValue::Type::uint16: {
      bytes = sizeof(uint16_t) * len;
      break;
    }
    case RosValue::Type::int32:
    case RosValue::Type::uint32:
    case RosValue::Type::float32: {
      bytes = sizeof(uint32_t) * len;
      break;
    }
    case RosValue::Type::int64:
    case RosValue::Type::uint64:
    case RosValue::Type::float64: {
      bytes = sizeof(uint64_t) * len;
      break;
    }
    case RosValue::Type::string: {
      // Arrays of strings are ok to unpack so we'll handle them differently
      value = std::make_shared<RosValue>(RosValue::Type::array);
      for (size_t i = 0; i < len; i++) {
        uint32_t string_len;
        read_into(&string_len);

        const auto string = std::make_shared<RosValue>(RosValue::Type::string);
        string->string_value.resize(string_len);
        read_into(string->string_value, string_len);
        value->values.emplace_back(string);
      }

      return value;
    }
    case RosValue::Type::blob:
    default: {
      throw std::runtime_error("Unable to read unknown blob type: " + field.type_name);
    };
  }

  value->blob_storage.byte_size = bytes;
  value->blob_storage.data.resize(bytes);
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

std::shared_ptr<RosValue> MessageParser::getPrimitiveField(RosMsgTypes::ros_msg_field &field) {
  RosValue::Type type = field.get_ros_type();
  auto value = std::make_shared<RosValue>(type);

  switch (type) {
    case RosValue::Type::ros_bool: {
      read_into(&value->bool_value);
      break;
    }
    case RosValue::Type::int8: {
      read_into(&value->int8_value);
      break;
    }
    case RosValue::Type::uint8: {
      read_into(&value->uint8_value);
      break;
    }
    case RosValue::Type::int16: {
      read_into(&value->int16_value);
      break;
    }
    case RosValue::Type::uint16: {
      read_into(&value->uint16_value);
      break;
    }
    case RosValue::Type::int32: {
      read_into(&value->int32_value);
      break;
    }
    case RosValue::Type::uint32: {
      read_into(&value->uint32_value);
      break;
    }
    case RosValue::Type::int64: {
      read_into(&value->int64_value);
      break;
    }
    case RosValue::Type::uint64: {
      read_into(&value->uint64_value);
      break;
    }
    case RosValue::Type::float32: {
      read_into(&value->float32_value);
      break;
    }
    case RosValue::Type::float64: {
      read_into(&value->float64_value);
      break;
    }
    case RosValue::Type::string: {
      uint32_t string_len;
      read_into(&string_len);
      value->string_value.resize(string_len);
      read_into(value->string_value, string_len);
      break;
    }
    case RosValue::Type::ros_time: {
      read_into(&value->time_value.secs);
      read_into(&value->time_value.nsecs);
      break;
    }
    case RosValue::Type::ros_duration: {
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
