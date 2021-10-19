#pragma once

#include <unordered_map>
#include <boost/variant.hpp>

#include "ros_value.h"

namespace Embag {
class RosMsgTypes{
 public:
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

   private:
    // Holds the type and size information about the field.
    // If this field is an array, holds the type info of the items within the array.
    // If this field is an object, the size will be 0.
    std::pair<RosValue::Type, size_t> type_info;
    bool type_info_set = false;
  };

  struct ros_msg_constant {
    std::string type_name;
    std::string constant_name;
    std::string value;
  };

  typedef boost::variant<ros_msg_field, ros_msg_constant> ros_msg_member;

  struct ros_msg_def_base {
   private:
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

    ros_msg_member getMember(const std::string &member_name) {
      if (member_map_.size() != members.size()) {
        member_map_.reserve(members.size());
        for (const auto &member : members) {
          member_map_[getMemberName(member)] = &member;
        }
      }

      if (member_map_.count(member_name)) {
        return *member_map_[member_name];
      }

      throw std::runtime_error("Unable to find member: " + member_name);
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
                                                              const Embag::RosMsgTypes::ros_msg_field &field) {
      if (!map_set) {
        for (auto &embedded_type : embedded_types) {
          embedded_type_map_[embedded_type.type_name] = &embedded_type;
        }
        map_set = true;
      }

      if (embedded_type_map_.count(field.type_name)) {
        return *embedded_type_map_[field.type_name];
      }

      // ROS allows a type to lack its scope when referenced
      const auto scoped_name = scope + '/' + field.type_name;
      if (embedded_type_map_.count(scoped_name)) {
        return *embedded_type_map_[scoped_name];
      }

      throw std::runtime_error("Unable to find embedded type: " + field.type_name + " in scope " + scope);
    }
  };
};
}
