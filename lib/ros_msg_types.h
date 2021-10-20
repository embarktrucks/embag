#pragma once

#include <unordered_map>
#include <boost/variant.hpp>

#include "ros_value.h"

namespace Embag {
class RosMsgTypes{
 public:
  struct ros_embedded_msg_def;

  // Schema stuff
  // TODO: move this stuff elsewhere?
  typedef std::unordered_map<std::string, std::pair<RosValue::Type, size_t>> primitive_type_map_t;
  struct ros_msg_field {
    static primitive_type_map_t primitive_type_map_;

    std::string type_name;
    int32_t array_size;
    std::string field_name;

    const std::pair<RosValue::Type, size_t>& getTypeInfo() {
      if (!type_info_set) {
        if (primitive_type_map_.count(type_name)) {
          type_info = primitive_type_map_.at(type_name);
        } else {
          type_info = {RosValue::Type::object, 0};
        }

        type_info_set = true;
      }

      return type_info;
    }

    Embag::RosMsgTypes::ros_embedded_msg_def& getDefinition(
      const std::unordered_map<std::string, RosMsgTypes::ros_embedded_msg_def*> &definition_map,
      const std::string &scope
    ) {
      if (definition == nullptr) {
        if (definition_map.count(type_name)) {
          definition = definition_map.at(type_name);
        }

        // ROS allows a type to lack its scope when referenced
        const auto scoped_name = scope + '/' + type_name;
        if (definition_map.count(scoped_name)) {
          definition = definition_map.at(scoped_name);
        }

        if (definition == nullptr) {
          throw std::runtime_error("Unable to find embedded type: " + type_name + " in scope " + scope);
        }
      }

      return *definition;
    }

   private:
    // Holds the type and size information about the field.
    // If this field is an array, holds the type info of the items within the array.
    // If this field is an object, the size will be 0.
    std::pair<RosValue::Type, size_t> type_info;
    bool type_info_set = false;

    // TODO: This can be stored in union with the size_t to reduce space
    // If this is an object, cache the associated ros_embedded_msg_def
    ros_embedded_msg_def* definition = nullptr;
  };

  struct ros_msg_constant {
    std::string type_name;
    std::string constant_name;
    std::string value;
  };

  typedef boost::variant<ros_msg_field, ros_msg_constant> ros_msg_member;

  struct ros_msg_def_base {
   private:
    std::shared_ptr<std::unordered_map<std::string, size_t>> field_indexes_;
    std::unordered_map<std::string, const ros_msg_member*> member_map_;

    const std::string& getMemberName(const ros_msg_member &member) {
      switch (member.which()) {
        case 0:
          return boost::get<RosMsgTypes::ros_msg_field>(member).field_name;
        case 1:
          return boost::get<RosMsgTypes::ros_msg_constant>(member).constant_name;
      }
    }

   public:
    std::vector<ros_msg_member> members;

    std::shared_ptr<std::unordered_map<std::string, size_t>> getFieldIndexes() {
      if (!field_indexes_) {
        size_t num_fields = 0;
        for (const auto &member : members) {
          if (member.which() == 0) {
            ++num_fields;
          }
        }

        field_indexes_ = std::make_shared<std::unordered_map<std::string, size_t>>();
        field_indexes_->reserve(num_fields);
        size_t field_num = 0;
        for (const auto &member : members) {
          if (member.which() == 0) {
            field_indexes_->insert(std::make_pair(boost::get<RosMsgTypes::ros_msg_field>(member).field_name, field_num++));
          }
        }
      }

      return field_indexes_;
    }
  };

  struct ros_embedded_msg_def : public ros_msg_def_base {
   public:
    std::string type_name;
    std::string scope;
    bool scope_set = false;

    // TODO: make this less dumb
    std::string getScope() {
      if (scope_set) {
        return scope;
      }

      scope_set = true;
      const size_t slash_pos = type_name.find_first_of('/');
      if (slash_pos != std::string::npos) {
        scope = type_name.substr(0, slash_pos);
      }

      return scope;
    }
  };

  struct ros_msg_def : public ros_msg_def_base {
   public:
    std::vector<ros_embedded_msg_def> embedded_types;

    // This speeds up searching for embedded types during parsing.
    bool map_set = false;
    std::unordered_map<std::string, RosMsgTypes::ros_embedded_msg_def*> embedded_type_map_;

    Embag::RosMsgTypes::ros_embedded_msg_def& getEmbeddedType(const std::string &scope,
                                                              Embag::RosMsgTypes::ros_msg_field &field) {
      if (!map_set) {
        for (auto &embedded_type : embedded_types) {
          embedded_type_map_[embedded_type.type_name] = &embedded_type;
        }
        map_set = true;
      }

      return field.getDefinition(embedded_type_map_, scope);
    }
  };
};
}
