#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <iostream>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/variant.hpp>

#include <lz4frame.h>

#include "ros_value.h"
#include "bag_view.h"
#include "ros_bag_types.h"

namespace Embag {

// TODO: move to threaded decompression to make things even faster
struct lz4f_ctx {
  LZ4F_decompressionContext_t ctx{nullptr};
  ~lz4f_ctx() {
    if (ctx)
      LZ4F_freeDecompressionContext(ctx);
  }
  operator LZ4F_decompressionContext_t() {
    return ctx;
  }
};

// Forward declaration
class View;

class Bag {
 public:
  explicit Bag(const std::string filename) : filename_(filename) {
    const LZ4F_errorCode_t code = LZ4F_createDecompressionContext(&lz4_ctx_.ctx, LZ4F_VERSION);
    if (LZ4F_isError(code)) {
      // FIXME
      //std::cout << "Received error code from LZ4F_createDecompressionContext: " << code << std::endl;
    }

    open();
  }

  bool open();
  bool close();

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
  };

 private:
  const std::string MAGIC_STRING = "#ROSBAG V";

  bool readRecords();
  RosBagTypes::record_t readRecord();
  static std::unique_ptr<std::unordered_map<std::string, std::string>> readFields(const char *p, uint64_t len);
  static RosBagTypes::header_t readHeader(const RosBagTypes::record_t &record);
  void decompressLz4Chunk(const char *src, size_t src_size, char *dst, size_t dst_size);

  std::string filename_;
  boost::iostreams::stream<boost::iostreams::mapped_file_source> bag_stream_;

  // Bag data
  std::vector<RosBagTypes::connection_record_t> connections_;
  std::unordered_map<std::string, RosBagTypes::connection_record_t *> topic_connection_map_;
  std::vector<RosBagTypes::chunk_t> chunks_;
  uint64_t index_pos_ = 0;
  std::unordered_map<std::string, std::shared_ptr<ros_msg_def>> message_schemata_;

  lz4f_ctx lz4_ctx_;

  friend class View;
};
}