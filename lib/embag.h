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
#include "ros_bag_types.h"
#include "ros_msg_types.h"

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

  std::vector<std::string> topics() {
    std::vector<std::string> topics;
    for (auto item : topic_connection_map_) {
      topics.emplace_back(item.first);
    }
    return topics;
  }

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
  std::unordered_map<std::string, std::vector<RosBagTypes::connection_record_t *>> topic_connection_map_;
  std::vector<RosBagTypes::chunk_t> chunks_;
  uint64_t index_pos_ = 0;
  std::unordered_map<std::string, std::shared_ptr<RosMsgTypes::ros_msg_def>> message_schemata_;

  lz4f_ctx lz4_ctx_;

  friend class View;
};
}
