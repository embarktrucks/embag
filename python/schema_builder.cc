#include "schema_builder.h"

py::object SchemaBuilder::generateSchema(const std::string &topic) {
  auto schema = ordered_dict_();

  if (!bag_->topicInBag(topic)) {
    throw std::runtime_error(topic + " not found in bag!");
  }

  msg_def_ = bag_->msgDefForTopic(topic);
  auto connections = bag_->connectionsForTopic(topic);

  for (const auto &member : msg_def_->members_) {
    if (member.which() == 0) {  // ros_msg_field
      auto field = boost::get<Embag::RosMsgTypes::ros_msg_field>(member);
      schema[field.field_name_.c_str()] = schemaForField(connections[0]->data.scope, field);
    }
  }

  return schema;
}

py::dict SchemaBuilder::schemaForField(const std::string &scope, Embag::RosMsgTypes::ros_msg_field &field) const {
  auto &primitive_type_map = Embag::RosMsgTypes::ros_msg_field::primitive_type_map_;
  auto field_def = py::dict{};

  // Not an array
  if (field.array_size_ == 0) {
    // Primitive type
    if (primitive_type_map.find(field.type_name_) != primitive_type_map.end()) {
      field_def["type"] = field.type_name_;
    } else {
      // Embedded type
      auto children = ordered_dict_();
      auto embedded_type = msg_def_->getEmbeddedType(scope, field);
      for (const auto &member : embedded_type.members_) {
        if (member.which() == 0) {  // ros_msg_field
          auto embedded_field = boost::get<Embag::RosMsgTypes::ros_msg_field>(member);
          children[embedded_field.field_name_.c_str()] = schemaForField(embedded_type.getScope(), embedded_field);
        }
      }

      field_def["type"] = "object";
      field_def["children"] = children;
    }
  } else {
    // Array type
    if (primitive_type_map.find(field.type_name_) != primitive_type_map.end()) {
      // Primitive type
      field_def["member_type"] = field.type_name_;
    } else {
      // This is an array of embedded types
      auto children = ordered_dict_();
      auto embedded_type = msg_def_->getEmbeddedType(scope, field);
      for (const auto &member : embedded_type.members_) {
        if (member.which() == 0) {  // ros_msg_field
          auto embedded_field = boost::get<Embag::RosMsgTypes::ros_msg_field>(member);
          children[embedded_field.field_name_.c_str()] = schemaForField(embedded_type.getScope(), embedded_field);
        }
      }

      field_def["children"] = children;
    }

    field_def["type"] = "array";
  }

  return field_def;
}
