#pragma once

#include <string>
#include <map>
#include <vector>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/variant.hpp>

// LZ4
#include <lz4frame.h>

// TODO: where does this go?
struct lz4f_ctx
{
  LZ4F_decompressionContext_t ctx{nullptr};
  ~lz4f_ctx() {
    if (ctx)
      LZ4F_freeDecompressionContext(ctx);
  }
  operator LZ4F_decompressionContext_t() {
    return ctx;
  }
};

class Embag {
 public:
  Embag(std::string filename) : filename_(filename) {
    const LZ4F_errorCode_t code = LZ4F_createDecompressionContext(&lz4_ctx_.ctx, LZ4F_VERSION);
    if (LZ4F_isError(code)) {
      // FIXME
      //std::cout << "Received error code from LZ4F_createDecompressionContext: " << code << std::endl;
    }
  }

  bool open();

  bool close();

  bool readRecords();

  void printMsgs();

  // Schema stuff
  // TODO: move this elsewhere?
  struct ros_msg_field {
    std::string type_name;
    int32_t array_size = 0;
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
  };

  struct ros_msg_def {
    std::vector<ros_msg_member> members;
    std::vector<ros_embedded_msg_def> embedded_types;
  };

 private:
  const std::string MAGIC_STRING = "#ROSBAG V";

  struct record_t {
    uint32_t header_len;
    const char *header;
    uint32_t data_len;
    const char *data;
  };

  struct header_t {
    std::map<std::string, std::string> fields;
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

  struct connection_data_t {
    std::string topic;
    std::string type;
    std::string md5sum;
    std::string message_definition;
    std::string callerid;
    bool latching = false;
  };

  struct connection_record_t {
    std::vector<index_block_t> blocks;
    std::string topic;
    connection_data_t data;
  };

  // TODO: do I need the types here?
  enum PRIMITIVE_TYPE {
    uint8,
    int8,
    uint32,
    int32,
    string,
    float64,
    time,
  };

  // TODO: it would be nice to not have to look this mapping up but establish it at parse time
  std::map<std::string, PRIMITIVE_TYPE> primitive_type_map_ = {
      {"uint8", uint8},
      {"int8", int8},
      {"uint32", uint32},
      {"int32", int32},
      {"string", string},
      {"float64", float64},
      {"time", time},
  };

  record_t readRecord();
  header_t readHeader(const record_t &record);
  bool decompressLz4Chunk(const char *src, const size_t src_size, char *dst, const size_t dst_size);
  void parseMessage(const uint32_t connection_id, record_t message);
  typedef boost::iostreams::stream<boost::iostreams::array_source> message_stream;
  void parseField(const ros_msg_def &msg_def, const ros_msg_field &field, message_stream &stream);
  void getPrimitiveField(const ros_msg_field &field, message_stream &stream);

  std::string filename_;
  boost::iostreams::stream <boost::iostreams::mapped_file_source> bag_stream_;

  // Bag data
  std::vector<connection_record_t> connections_;
  std::vector<chunk_t> chunks_;
  uint64_t index_pos_;
  std::map<std::string, ros_msg_def> message_schemata_;

  lz4f_ctx lz4_ctx_;
};
