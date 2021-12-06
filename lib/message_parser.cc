#include <iostream>
#include <memory>
#include <string>

#include <boost/optional.hpp>

#include "message_parser.h"
#include "util.h"

namespace Embag {

const RosValue::RosValuePointer MessageParser::parse() {
  // The lowest number of RosValues occurs when we have a message with only doubles in a single type.
  // The number of RosValues in this case is the number of doubles that can fit in our buffer,
  // plus one for the RosValue object that will point to all the doubles.
  ros_values_->reserve(message_buffer_->size() / sizeof(double) + 1);
  ros_values_->emplace_back(msg_def_.fieldIndexes());
  ros_values_offset_ = 1;
  initObject(0, msg_def_);
  return RosValue::RosValuePointer(ros_values_);
}

void MessageParser::initObject(size_t object_offset, const RosMsgTypes::BaseMsgDef &object_definition) {
  const size_t children_offset = ros_values_offset_;
  ros_values_->at(object_offset).object_info_.children.base = ros_values_;
  ros_values_->at(object_offset).object_info_.children.offset = children_offset;
  ros_values_->at(object_offset).object_info_.children.length = 0;
  for (auto &member: object_definition.members()) {
    if (member.which() == 0) {
      auto& field = boost::get<RosMsgTypes::FieldDef>(member);
      emplaceField(field);
    }
  }

  for (auto &member: object_definition.members()) {
    if (member.which() == 0) {
      auto& field = boost::get<RosMsgTypes::FieldDef>(member);
      const size_t child_offset = children_offset + ros_values_->at(object_offset).object_info_.children.length++;
      switch (ros_values_->at(child_offset).type_) {
        case RosValue::Type::object: {
          auto& embedded_type = field.typeDefinition();
          initObject(child_offset, embedded_type);
          break;
        }
        case RosValue::Type::array: {
          initArray(child_offset, field);
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

void MessageParser::emplaceField(const RosMsgTypes::FieldDef &field) {
  if (field.arraySize() == 0) {
    if (field.type() != RosValue::Type::object) {
      ros_values_->emplace_back(field.type());
    } else {
      auto& object_definition = field.typeDefinition();
      ros_values_->emplace_back(object_definition.fieldIndexes());
    }
  } else {
    ros_values_->emplace_back(RosValue::_array_identifier());
  }

  ++ros_values_offset_;
}

void MessageParser::initArray(size_t array_offset, const RosMsgTypes::FieldDef &field) {
  size_t array_length;
  if (field.arraySize() == -1) {
    array_length = *reinterpret_cast<uint32_t*>(&message_buffer_->at(message_buffer_offset_));
    message_buffer_offset_ += sizeof(uint32_t);
  } else {
    array_length = static_cast<uint32_t>(field.arraySize());
  }
  const size_t children_offset = ros_values_offset_;
  ros_values_offset_ += array_length;

  ros_values_->at(array_offset).array_info_.children.length = array_length;
  ros_values_->at(array_offset).array_info_.children.base = ros_values_;
  ros_values_->at(array_offset).array_info_.children.offset = children_offset;

  if (field.type() != RosValue::Type::object) {
    for (size_t i = 0; i < array_length; ++i) {
      ros_values_->emplace_back(field.type());
    }

    for (size_t i = 0; i < array_length; ++i) {
      initPrimitive(children_offset + i, field);
    }
  } else {
    auto& object_definition = field.typeDefinition();
    for (size_t i = 0; i < array_length; ++i) {
      ros_values_->emplace_back(object_definition.fieldIndexes());
    }

    for (size_t i = 0; i < array_length; ++i) {
      initObject(children_offset + i, object_definition);
    }
  }
}

void MessageParser::initPrimitive(size_t primitive_offset, const RosMsgTypes::FieldDef &field) {
  RosValue& primitive = ros_values_->at(primitive_offset);
  primitive.primitive_info_.message_buffer = message_buffer_;
  primitive.primitive_info_.offset = message_buffer_offset_;

  if (field.type() == RosValue::Type::string) {
    message_buffer_offset_ += *reinterpret_cast<const uint32_t* const>(primitive.getPrimitivePointer()) + sizeof(uint32_t);
  } else {
    message_buffer_offset_ += field.typeSize();
  }
}
}
