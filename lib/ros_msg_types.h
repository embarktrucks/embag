#pragma once

#include <unordered_map>
#include <boost/variant.hpp>

#include "ros_value.h"

namespace Embag {
class RosMsgTypes{
 public:
  class ros_embedded_msg_def;

  // Schema stuff
  // TODO: move this stuff elsewhere?
  typedef std::unordered_map<std::string, std::pair<RosValue::Type, size_t>> primitive_type_map_t;
  class ros_msg_field {
   public:
    static primitive_type_map_t primitive_type_map_;

    std::string type_name_;
    int32_t array_size_;
    std::string field_name_;

    const std::pair<RosValue::Type, size_t>& getTypeInfo() {
      if (!type_info_set) {
        if (primitive_type_map_.count(type_name_)) {
          type_info = primitive_type_map_.at(type_name_);
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
        if (definition_map.count(type_name_)) {
          definition = definition_map.at(type_name_);
        }

        // ROS allows a type to lack its scope when referenced
        const auto scoped_name = scope + '/' + type_name_;
        if (definition_map.count(scoped_name)) {
          definition = definition_map.at(scoped_name);
        }

        if (definition == nullptr) {
          throw std::runtime_error("Unable to find embedded type: " + type_name_ + " in scope " + scope);
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

  class ros_msg_def_base {
   private:
    std::shared_ptr<std::unordered_map<std::string, size_t>> field_indexes_;
    std::unordered_map<std::string, const ros_msg_member*> member_map_;

    const std::string& getMemberName(const ros_msg_member &member) {
      switch (member.which()) {
        case 0:
          return boost::get<RosMsgTypes::ros_msg_field>(member).field_name_;
        case 1:
          return boost::get<RosMsgTypes::ros_msg_constant>(member).constant_name;
      }
    }

   public:
    std::vector<ros_msg_member> members_;

    std::shared_ptr<std::unordered_map<std::string, size_t>> getFieldIndexes() {
      if (!field_indexes_) {
        size_t num_fields = 0;
        for (const auto &member : members_) {
          if (member.which() == 0) {
            ++num_fields;
          }
        }

        field_indexes_ = std::make_shared<std::unordered_map<std::string, size_t>>();
        field_indexes_->reserve(num_fields);
        size_t field_num = 0;
        for (const auto &member : members_) {
          if (member.which() == 0) {
            field_indexes_->emplace(boost::get<RosMsgTypes::ros_msg_field>(member).field_name_, field_num++);
          }
        }
      }

      return field_indexes_;
    }
  };

  class ros_embedded_msg_def : public ros_msg_def_base {
   public:
    std::string type_name_;
    std::string scope_;
    bool scope_set_ = false;

    // TODO: make this less dumb
    std::string getScope() {
      if (scope_set_) {
        return scope_;
      }

      scope_set_ = true;
      const size_t slash_pos = type_name_.find_first_of('/');
      if (slash_pos != std::string::npos) {
        scope_ = type_name_.substr(0, slash_pos);
      }

      return scope_;
    }
  };

  class ros_msg_def : public ros_msg_def_base {
   public:
    std::vector<ros_embedded_msg_def> embedded_types_;

    // This speeds up searching for embedded types during parsing.
    bool map_set_ = false;
    std::unordered_map<std::string, RosMsgTypes::ros_embedded_msg_def*> embedded_type_map_;

    Embag::RosMsgTypes::ros_embedded_msg_def& getEmbeddedType(const std::string &scope,
                                                              Embag::RosMsgTypes::ros_msg_field &field) {
      if (!map_set_) {
        for (auto &embedded_type : embedded_types_) {
          embedded_type_map_[embedded_type.type_name_] = &embedded_type;
        }
        map_set_ = true;
      }

      return field.getDefinition(embedded_type_map_, scope);
    }
  };
};
}
