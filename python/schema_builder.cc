#include "schema_builder.h"
#include "lib/message_def_parser.h"

SchemaBuilder::SchemaBuilder(Embag::View &view, const std::string &topic) {
  msg_def_ = view.msgDefForTopic(topic);
  auto connections = view.connectionsByTopicMap().find(topic)->second;
  outer_scope_ = connections[0].scope;
}

SchemaBuilder::SchemaBuilder(Embag::Bag &bag, const std::string &topic) {
  msg_def_ = bag.msgDefForTopic(topic);
  auto connections = bag.connectionsForTopic(topic);
  outer_scope_ = connections[0]->data.scope;
}

SchemaBuilder::SchemaBuilder(const std::string &message_type,
                             const std::string &message_definition) {
  msg_def_ = Embag::parseMsgDef(message_definition);
  const size_t slash_pos = message_type.find_first_of('/');
  outer_scope_ = message_type.substr(0, slash_pos);
}

py::object SchemaBuilder::generateSchema() {
  auto schema = ordered_dict_();
  for (const auto &member : msg_def_->members) {
    if (member.which() == 0) { // ros_msg_field
      auto field = boost::get<Embag::RosMsgTypes::ros_msg_field>(member);
      schema[field.field_name.c_str()] = schemaForField(outer_scope_, field);
    }
  }
  return schema;
}

py::dict SchemaBuilder::schemaForField(const std::string &scope, const Embag::RosMsgTypes::ros_msg_field &field) const {
  auto &primitive_type_map = Embag::RosMsgTypes::ros_msg_field::primitive_type_map_;
  auto field_def = py::dict{};

  // Not an array
  if (field.array_size == 0) {
    // Primitive type
    if (primitive_type_map.find(field.type_name) != primitive_type_map.end()) {
      field_def["type"] = field.type_name;
    } else {
      // Embedded type
      auto children = ordered_dict_();
      auto embedded_type = msg_def_->getEmbeddedType(scope, field);
      for (const auto &member : embedded_type.members) {
        if (member.which() == 0) {  // ros_msg_field
          auto embedded_field = boost::get<Embag::RosMsgTypes::ros_msg_field>(member);
          children[embedded_field.field_name.c_str()] = schemaForField(embedded_type.getScope(), embedded_field);
        }
      }

      field_def["type"] = "object";
      field_def["children"] = children;
    }
  } else {
    // Array type
    if (primitive_type_map.find(field.type_name) != primitive_type_map.end()) {
      // Primitive type
      field_def["member_type"] = field.type_name;
    } else {
      // This is an array of embedded types
      auto children = ordered_dict_();
      auto embedded_type = msg_def_->getEmbeddedType(scope, field);
      for (const auto &member : embedded_type.members) {
        if (member.which() == 0) {  // ros_msg_field
          auto embedded_field = boost::get<Embag::RosMsgTypes::ros_msg_field>(member);
          children[embedded_field.field_name.c_str()] = schemaForField(embedded_type.getScope(), embedded_field);
        }
      }

      field_def["children"] = children;
    }

    field_def["type"] = "array";
  }

  return field_def;
}
