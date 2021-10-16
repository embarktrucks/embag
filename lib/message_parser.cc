#include <iostream>
#include <memory>
#include <string>

#include <boost/optional.hpp>

#include "message_parser.h"
#include "util.h"

namespace Embag {

std::unordered_map<std::string, size_t> MessageParser::primitive_size_map_ = {
  {"bool", sizeof(bool)},
  {"int8", sizeof(int8_t)},
  {"uint8", sizeof(uint8_t)},
  {"int16", sizeof(int16_t)},
  {"uint16", sizeof(uint16_t)},
  {"int32", sizeof(int32_t)},
  {"uint32", sizeof(uint32_t)},
  {"int64", sizeof(int64_t)},
  {"uint64", sizeof(uint64_t)},
  {"float32", sizeof(float)},
  {"float64", sizeof(double)},
  {"time", sizeof(RosValue::ros_time_t)},
  {"duration", sizeof(RosValue::ros_duration_t)},

  // Deprecated types
  {"byte", sizeof(int8_t)},
  {"char", sizeof(uint8_t)},
};

std::shared_ptr<RosValue> MessageParser::parse() {
  return createObject(msg_def_, scope_);
}

std::shared_ptr<RosValue> MessageParser::createObject(RosMsgTypes::ros_msg_def_base &object_definition, const std::string &scope) {
  auto object = std::make_shared<RosValue>(RosValue::_object_identifier());
  // TODO: create all the RosValues and pre-allocate the map at once?
  for (const auto &member : object_definition.members) {
    if (member.which() == 0) {
      auto field = boost::get<RosMsgTypes::ros_msg_field>(member);
      object->objects[field.field_name] = parseField(scope, field);
    }
  }

  return object;
}

std::shared_ptr<RosValue> MessageParser::createPrimitiveArray(RosMsgTypes::ros_msg_field &field, const size_t array_len) {
  auto array = std::make_shared<RosValue>(RosValue::_array_identifier());
  array->values.reserve(array_len);
  for (size_t i = 0; i < array_len; i++) {
    array->values.push_back(createPrimitive(field));
  }

  return array;
}

std::shared_ptr<RosValue> MessageParser::createObjectArray(RosMsgTypes::ros_embedded_msg_def &object_definition, const size_t array_len) {
  auto array = std::make_shared<RosValue>(RosValue::_array_identifier());
  array->values.reserve(array_len);
  const std::string scope = object_definition.getScope();
  for (size_t i = 0; i < array_len; i++) {
    array->values.push_back(createObject(object_definition, scope));
  }

  return array;
}

std::shared_ptr<RosValue> MessageParser::createPrimitive(RosMsgTypes::ros_msg_field &field) {
  auto primitive = std::make_shared<RosValue>(RosMsgTypes::ros_msg_field::primitive_type_map_[field.type_name]);
  primitive->primitive_info.message_buffer = message_buffer_;
  primitive->primitive_info.offset = offset_;

  if (primitive->type == RosValue::Type::string) {
    offset_ += *reinterpret_cast<const uint32_t* const>(primitive->getPrimitivePointer()) + sizeof(uint32_t);
  } else {
    offset_ += primitive_size_map_[field.type_name];
  }

  return primitive;
}

std::shared_ptr<RosValue> MessageParser::parseField(const std::string &scope, RosMsgTypes::ros_msg_field &field) {
  auto &primitive_type_map = RosMsgTypes::ros_msg_field::primitive_type_map_;

  switch (field.array_size) {
    // Undefined number of array objects
    case -1: {
      const uint32_t* const array_len = reinterpret_cast<uint32_t*>(&message_buffer_->at(offset_));
      offset_ += sizeof(uint32_t);

      if (primitive_type_map.count(field.type_name)) {
        return createPrimitiveArray(field, *array_len);
      } else {
        auto embedded_type = msg_def_.getEmbeddedType(scope, field);
        return createObjectArray(embedded_type, *array_len);
      }
    }
    // Not an array
    case 0: {
      if (primitive_type_map.count(field.type_name)) {
        return createPrimitive(field);
      } else {
        auto embedded_type = msg_def_.getEmbeddedType(scope, field);
        return createObject(embedded_type, embedded_type.getScope());
      }
    }
    // Array with fixed size
    default: {
      if (primitive_type_map.count(field.type_name)) {
        return createPrimitiveArray(field, field.array_size);
      } else {
        auto embedded_type = msg_def_.getEmbeddedType(scope, field);
        return createObjectArray(embedded_type, field.array_size);
      }
    }
  }
}
}
