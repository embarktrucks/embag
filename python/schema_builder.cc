#include "schema_builder.h"

py::object SchemaBuilder::generateSchema(const std::string &topic) {
  auto schema = ordered_dict_();

  if (!bag_->topicInBag(topic)) {
    throw std::runtime_error(topic + " not found in bag!");
  }

  msg_def_ = bag_->msgDefForTopic(topic);
  auto connections = bag_->connectionsForTopic(topic);

  for (const auto &member : msg_def_->members()) {
    if (member.which() == 0) {  // ros_msg_field
      auto field = boost::get<Embag::RosMsgTypes::FieldDef>(member);
      schema[field.name().c_str()] = schemaForField(field);
    }
  }

  return schema;
}

py::dict SchemaBuilder::schemaForField(const Embag::RosMsgTypes::FieldDef &field) const {
  auto &primitive_type_map = Embag::RosMsgTypes::FieldDef::primitive_type_map;
  auto field_def = py::dict{};

  // Not an array
  if (field.arraySize() == 0) {
    // Primitive type
    if (primitive_type_map.find(field.typeName()) != primitive_type_map.end()) {
      field_def["type"] = field.typeName();
    } else {
      // Embedded type
      auto children = ordered_dict_();
      for (const auto &member : field.typeDefinition().members()) {
        if (member.which() == 0) {  // ros_msg_field
          auto embedded_field = boost::get<Embag::RosMsgTypes::FieldDef>(member);
          children[embedded_field.name().c_str()] = schemaForField(embedded_field);
        }
      }

      field_def["type"] = "object";
      field_def["children"] = children;
    }
  } else {
    // Array type
    if (primitive_type_map.find(field.typeName()) != primitive_type_map.end()) {
      // Primitive type
      field_def["member_type"] = field.typeName();
    } else {
      // This is an array of embedded types
      auto children = ordered_dict_();
      for (const auto &member : field.typeDefinition().members()) {
        if (member.which() == 0) {  // ros_msg_field
          auto embedded_field = boost::get<Embag::RosMsgTypes::FieldDef>(member);
          children[embedded_field.name().c_str()] = schemaForField(embedded_field);
        }
      }

      field_def["children"] = children;
    }

    field_def["type"] = "array";
  }

  return field_def;
}
