#include <boost/optional.hpp>

#include "message_parser.h"
#include "util.h"

std::unique_ptr<RosValue> MessageParser::parse() {
  auto parsed_message = make_unique<RosValue>(RosValue::Type::object);

  for (const auto &member : msg_def_->members) {
    if (member.which() == 0) {  // ros_msg_field
      const auto field = boost::get<Embag::ros_msg_field>(member);
      parsed_message->objects[field.field_name] = parseField(connection_data_.scope, field);
    }
  }

  return parsed_message;
}

std::unique_ptr<RosValue> MessageParser::parseField(const std::string &scope, const Embag::ros_msg_field &field) {
  auto parsed_field = make_unique<RosValue>();

  switch(field.array_size) {
    // Undefined number of array objects
    case -1: {
      uint32_t array_len;
      stream_.read(reinterpret_cast<char *>(&array_len), sizeof(array_len));

      parsed_field->type = RosValue::Type::array;

      if (array_len == 0) {
        return parsed_field;
      }

      if (primitive_type_map_.find(field.type_name) != primitive_type_map_.end()) {
        for (size_t i = 0; i < array_len; i++) {
          parsed_field->values.emplace_back(getPrimitiveField(field));
        }
      } else {
        auto embedded_type = getEmbeddedType(scope, field);
        parseArray(array_len, embedded_type, parsed_field);
      }
      break;
    }
    // Not an array
    case 0: {
      // Primitive type
      if (primitive_type_map_.find(field.type_name) != primitive_type_map_.end()) {
        parsed_field = getPrimitiveField(field);
      } else {
        // Embedded type
        parsed_field->type = RosValue::Type::object;
        auto embedded_type = getEmbeddedType(scope, field);
        for (const auto &member : embedded_type.members) {
          if (member.which() == 0) {  // ros_msg_field
            const auto embedded_field = boost::get<Embag::ros_msg_field>(member);
            parsed_field->objects[embedded_field.field_name] = parseField(embedded_type.getScope(), embedded_field);
          }
        }
      }
      break;
    }
    // Array with fixed size
    default: {
      parsed_field->type = RosValue::Type::array;
      if (primitive_type_map_.find(field.type_name) != primitive_type_map_.end()) {
        for (int32_t i = 0; i < field.array_size; i++) {
          getPrimitiveField(field);
        }
      } else {
        auto embedded_type = getEmbeddedType(scope, field);
        parseArray(field.array_size, embedded_type, parsed_field);
      }

      break;
    }
  }

  return parsed_field;
}

void MessageParser::parseArray(const size_t array_len, Embag::ros_embedded_msg_def &embedded_type, std::unique_ptr<RosValue> &value) {
  for (size_t i = 0; i < array_len; i++) {
    value->values.emplace_back(parseMembers(embedded_type));
  }
}

std::unique_ptr<RosValue> MessageParser::parseMembers(Embag::ros_embedded_msg_def &embedded_type) {
  auto values = make_unique<RosValue>(RosValue::Type::object);

  for (const auto &member : embedded_type.members) {
    // TODO: use strict_get() instead of this visitor thing
    if (member.which() == 0) {  // ros_msg_field
      const auto embedded_field = boost::get<Embag::ros_msg_field>(member);
      values->objects[embedded_field.field_name] = parseField(embedded_type.getScope(), embedded_field);
    }
  }

  return values;
}

Embag::ros_embedded_msg_def MessageParser::getEmbeddedType(const std::string &scope, const Embag::ros_msg_field &field) {
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

std::unique_ptr<RosValue> MessageParser::getPrimitiveField(const Embag::ros_msg_field& field) {
  RosValue::Type type = primitive_type_map_.at(field.type_name);
  auto value = make_unique<RosValue>(type);

  switch (type) {
    case RosValue::ros_bool: {
      stream_.read(reinterpret_cast<char *>(&value->bool_value), sizeof(value->bool_value));
      break;
    }
    case RosValue::int8: {
      stream_.read(reinterpret_cast<char *>(&value->int8_value), sizeof(value->int8_value));
      break;
    }
    case RosValue::uint8: {
      stream_.read(reinterpret_cast<char *>(&value->uint8_value), sizeof(value->uint8_value));
      break;
    }
    case RosValue::int16: {
      stream_.read(reinterpret_cast<char *>(&value->int16_value), sizeof(value->int16_value));
      break;
    }
    case RosValue::uint16: {
      stream_.read(reinterpret_cast<char *>(&value->uint16_value), sizeof(value->uint16_value));
      break;
    }
    case RosValue::int32: {
      stream_.read(reinterpret_cast<char *>(&value->int32_value), sizeof(value->int32_value));
      break;
    }
    case RosValue::uint32: {
      stream_.read(reinterpret_cast<char *>(&value->uint32_value), sizeof(value->uint32_value));
      break;
    }
    case RosValue::int64: {
      stream_.read(reinterpret_cast<char *>(&value->int64_value), sizeof(value->int64_value));
      break;
    }
    case RosValue::uint64: {
      stream_.read(reinterpret_cast<char *>(&value->uint64_value), sizeof(value->uint64_value));
      break;
    }
    case RosValue::float32: {
      stream_.read(reinterpret_cast<char *>(&value->float32_value), sizeof(value->float32_value));
      break;
    }
    case RosValue::float64: {
      stream_.read(reinterpret_cast<char *>(&value->float64_value), sizeof(value->float64_value));
      break;
    }
    case RosValue::string: {
      uint32_t string_len;
      stream_.read(reinterpret_cast<char *>(&string_len), sizeof(string_len));
      value->string_value = std::string(string_len, 0);
      stream_.read(&value->string_value[0], string_len);
      break;
    }
    case RosValue::ros_time: {
      stream_.read(reinterpret_cast<char *>(&value->time_value.secs), sizeof(value->time_value.secs));
      stream_.read(reinterpret_cast<char *>(&value->time_value.nsecs), sizeof(value->time_value.nsecs));
      break;
    }
    case RosValue::ros_duration: {
      stream_.read(reinterpret_cast<char *>(&value->duration_value.secs), sizeof(value->duration_value.secs));
      stream_.read(reinterpret_cast<char *>(&value->duration_value.nsecs), sizeof(value->duration_value.nsecs));
      break;
    }
    default: {
      throw std::runtime_error("Unable to read unknown type: " + field.type_name);
    };
  }

  return value;
}
