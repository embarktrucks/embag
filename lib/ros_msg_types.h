#pragma once

#include <unordered_map>
#include <boost/variant.hpp>

#include "ros_value.h"

namespace Embag {
class RosMsgTypes{
 public:
  // Schema stuff
  // TODO: move this stuff elsewhere?
  typedef std::unordered_map<std::string, RosValue::Type> primitive_type_map_t;
  struct ros_msg_field {
    static primitive_type_map_t primitive_type_map_;

    std::string type_name;
    int32_t array_size;
    std::string field_name;

    bool type_set = false;
    RosValue::Type ros_type;

    RosValue::Type get_ros_type() {
      if (!type_set) {
        ros_type = primitive_type_map_.at(type_name);
        type_set = true;
      }
      return ros_type;
    }
  };

  struct ros_msg_constant {
    std::string type_name;
    std::string constant_name;
    std::string value;
  };

  typedef boost::variant<ros_msg_field, ros_msg_constant> ros_msg_member;

  struct ros_embedded_msg_def {
    std::string type_name;
    std::vector<ros_msg_member> members;
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

  struct ros_msg_def {
    std::vector<ros_msg_member> members;
    std::vector<ros_embedded_msg_def> embedded_types;

    // This speeds up searching for embedded types during parsing.
    bool map_set = false;
    std::unordered_map<std::string, const RosMsgTypes::ros_embedded_msg_def*> embedded_type_map_;

    Embag::RosMsgTypes::ros_embedded_msg_def getEmbeddedType(const std::string &scope,
                                                             const Embag::RosMsgTypes::ros_msg_field &field) {
      if (!map_set) {
        for (const auto &embedded_type : embedded_types) {
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

      throw std::runtime_error("Unable to find embedded type: " + field.type_name);
    }
  };
};
}
