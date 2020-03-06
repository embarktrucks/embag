#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <iostream>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/variant.hpp>

// LZ4
#include <lz4frame.h>

#include "ros_value.h"
#include "bag_vew.h"

// TODO: where does this go?
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

class BagView;
class Embag {
 public:
  explicit Embag(const std::string filename) : filename_(filename) {
    const LZ4F_errorCode_t code = LZ4F_createDecompressionContext(&lz4_ctx_.ctx, LZ4F_VERSION);
    if (LZ4F_isError(code)) {
      // FIXME
      //std::cout << "Received error code from LZ4F_createDecompressionContext: " << code << std::endl;
    }
  }

  bool open();

  bool close();

  bool readRecords();

  void printAllMsgs();

  void printMsg(const std::unique_ptr<RosValue> &message, const std::string &path = "");

  BagView getView();

  // Schema stuff
  // TODO: move this elsewhere?
  struct ros_msg_field {
    std::string type_name;
    int32_t array_size;
    std::string field_name;
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

  struct connection_data_t {
    std::string topic;
    std::string type;
    std::string scope;
    std::string md5sum;
    std::string message_definition;
    std::string callerid;
    bool latching = false;
  };

  typedef boost::iostreams::stream<boost::iostreams::array_source> message_stream;

 private:
  const std::string MAGIC_STRING = "#ROSBAG V";

  struct record_t {
    uint32_t header_len;
    const char *header;
    uint32_t data_len;
    const char *data;
  };

  struct header_t {
    std::unordered_map<std::string, std::string> fields;
    enum class op {
      BAG_HEADER   = 0x03,
      CHUNK        = 0x05,
      CONNECTION   = 0x07,
      MESSAGE_DATA = 0x02,
      INDEX_DATA   = 0x04,
      CHUNK_INFO   = 0x06,
      UNSET        = 0xff,
    };

    const op getOp() const {
      return header_t::op(*(fields.at("op").data()));
    }

    const void getField(const std::string& name, std::string& value) const {
      value = fields.at(name);
    }

    template <typename T>
    const void getField(const std::string& name, T& value) const {
      value = *reinterpret_cast<const T*>(fields.at(name).data());
    }
    const bool checkField(const std::string& name) const {
      return fields.find(name) != fields.end();
    }
  };

  struct chunk_info_t {
    uint64_t start_time;
    uint64_t end_time;
    uint32_t message_count;
  };

  struct chunk_t {
    uint64_t offset = 0;
    chunk_info_t info;
    std::string compression;
    uint32_t uncompressed_size;
    record_t record;

    chunk_t(record_t r) {
      record = r;
    };
  };

  struct index_block_t {
    chunk_t* into_chunk;
  };

  struct connection_record_t {
    std::vector<index_block_t> blocks;
    std::string topic;
    connection_data_t data;
  };

  record_t readRecord();
  static header_t readHeader(const record_t &record);
  bool decompressLz4Chunk(const char *src, size_t src_size, char *dst, size_t dst_size);
  std::unique_ptr<RosValue> parseMessage(uint32_t connection_id, record_t message);

  std::string filename_;
  boost::iostreams::stream <boost::iostreams::mapped_file_source> bag_stream_;

  // Bag data
  std::vector<connection_record_t> connections_;
  std::unordered_map<std::string, connection_record_t> topic_connection_map_;
  std::vector<chunk_t> chunks_;
  uint64_t index_pos_ = 0;
  std::unordered_map<std::string, ros_msg_def> message_schemata_;

  // FIXME: We really should store this in BagView but I can't figure out the circular deps :(
  std::vector<chunk_t *> chunks_to_parse_;

  lz4f_ctx lz4_ctx_;

  friend class BagView;
};
