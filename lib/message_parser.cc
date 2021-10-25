#include <iostream>
#include <memory>
#include <string>

#include <boost/optional.hpp>

#include "message_parser.h"
#include "util.h"

namespace Embag {

const RosValue* MessageParser::parse() {
  // The lowest number of RosValues occurs when we have a message with only doubles in a single type.
  // The number of RosValues in this case is the number of doubles that can fit in our buffer,
  // plus one for the RosValue object that will point to all the doubles.
  ros_values_->reserve(message_buffer_->size() / sizeof(double) + 1);
  ros_values_->emplace_back(msg_def_.getFieldIndexes());
  ros_values_offset_ = 1;
  initObject(0, msg_def_, scope_);
  return &ros_values_->front();
}

void MessageParser::initObject(size_t object_offset, RosMsgTypes::ros_msg_def_base &object_definition, const std::string &scope) {
  const size_t children_offset = ros_values_offset_;
  ros_values_->at(object_offset).object_info.children.base = ros_values_;
  ros_values_->at(object_offset).object_info.children.offset = children_offset;
  ros_values_->at(object_offset).object_info.children.length = 0;
  for (auto &member: object_definition.members) {
    if (member.which() == 0) {
      auto& field = boost::get<RosMsgTypes::ros_msg_field>(member);
      emplaceField(scope, field);
    }
  }

  for (auto &member: object_definition.members) {
    if (member.which() == 0) {
      auto& field = boost::get<RosMsgTypes::ros_msg_field>(member);
      const size_t child_offset = children_offset + ros_values_->at(object_offset).object_info.children.length++;
      switch (ros_values_->at(child_offset).type) {
        case RosValue::Type::object: {
          auto& embedded_type = msg_def_.getEmbeddedType(scope, field);
          initObject(child_offset, embedded_type, embedded_type.getScope());
          break;
        }
        case RosValue::Type::array: {
          initArray(child_offset, scope, field);
          break;
        }
        default: {
          // Primitive
          initPrimitive(child_offset, field);
        }
      }
    }
  }
}

void MessageParser::emplaceField(const std::string &scope, RosMsgTypes::ros_msg_field &field) {
  if (field.array_size == 0) {
    const std::pair<RosValue::Type, size_t>& field_type_info = field.getTypeInfo();
    if (field_type_info.first != RosValue::Type::object) {
      ros_values_->emplace_back(field_type_info.first);
    } else {
      auto& object_definition = msg_def_.getEmbeddedType(scope, field);
      ros_values_->emplace_back(object_definition.getFieldIndexes());
    }
  } else {
    ros_values_->emplace_back(RosValue::_array_identifier());
  }

  ++ros_values_offset_;
}

void MessageParser::initArray(size_t array_offset, const std::string &scope, RosMsgTypes::ros_msg_field &field) {
  size_t array_length;
  if (field.array_size == -1) {
    array_length = *reinterpret_cast<uint32_t*>(&message_buffer_->at(message_buffer_offset_));
    message_buffer_offset_ += sizeof(uint32_t);
  } else {
    array_length = static_cast<uint32_t>(field.array_size);
  }
  const size_t children_offset = ros_values_offset_;
  ros_values_offset_ += array_length;

  ros_values_->at(array_offset).array_info.children.length = array_length;
  ros_values_->at(array_offset).array_info.children.base = ros_values_;
  ros_values_->at(array_offset).array_info.children.offset = children_offset;

  const std::pair<RosValue::Type, size_t>& field_type_info = field.getTypeInfo();
  if (field_type_info.first != RosValue::Type::object) {
    for (size_t i = 0; i < array_length; ++i) {
      ros_values_->emplace_back(field_type_info.first);
    }

    for (size_t i = 0; i < array_length; ++i) {
      initPrimitive(children_offset + i, field);
    }
  } else {
    auto& object_definition = msg_def_.getEmbeddedType(scope, field);
    for (size_t i = 0; i < array_length; ++i) {
      ros_values_->emplace_back(object_definition.getFieldIndexes());
    }

    const std::string children_scope = object_definition.getScope();
    for (size_t i = 0; i < array_length; ++i) {
      initObject(children_offset + i, object_definition, children_scope);
    }
  }
}

void MessageParser::initPrimitive(size_t primitive_offset, RosMsgTypes::ros_msg_field &field) {
  RosValue& primitive = ros_values_->at(primitive_offset);
  primitive.primitive_info.message_buffer = message_buffer_;
  primitive.primitive_info.offset = message_buffer_offset_;

  const std::pair<RosValue::Type, size_t>& field_type_info = field.getTypeInfo();
  if (field_type_info.first == RosValue::Type::string) {
    message_buffer_offset_ += *reinterpret_cast<const uint32_t* const>(primitive.getPrimitivePointer()) + sizeof(uint32_t);
  } else {
    message_buffer_offset_ += field_type_info.second;
  }
}
}
