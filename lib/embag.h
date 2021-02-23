#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <iostream>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/variant.hpp>

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
    open();
  }

  ~Bag() {
    close();
  }

  bool open();
  bool close();

  std::unordered_set<std::string> topics() const {
    std::unordered_set<std::string> topics;
    for (const auto& item : topic_connection_map_) {
      topics.emplace(item.first);
    }
    return topics;
  }

  bool topicInBag(const std::string &topic) const {
    return topic_connection_map_.count(topic) != 0;
  }

  std::shared_ptr<RosMsgTypes::ros_msg_def> msgDefForTopic(const std::string &topic) {
    const auto it = message_schemata_.find(topic);
    if (it == message_schemata_.end()) {
      parseMsgDefForTopic(topic);
      return message_schemata_[topic];
    } else {
      return it->second;
    }
  }

  std::vector<RosBagTypes::connection_record_t *> connectionsForTopic(const std::string &topic) {
    return topic_connection_map_[topic];
  }

 private:
  const std::string MAGIC_STRING = "#ROSBAG V";

  bool readRecords();
  RosBagTypes::record_t readRecord();
  static std::unique_ptr<std::unordered_map<std::string, std::string>> readFields(const char *p, uint64_t len);
  static RosBagTypes::header_t readHeader(const RosBagTypes::record_t &record);
  void parseMsgDefForTopic(const std::string &topic);

  std::string filename_;
  boost::iostreams::stream<boost::iostreams::mapped_file_source> bag_stream_;

  // Bag data
  std::vector<RosBagTypes::connection_record_t> connections_;
  std::unordered_map<std::string, std::vector<RosBagTypes::connection_record_t *>> topic_connection_map_;
  std::vector<RosBagTypes::chunk_t> chunks_;
  uint64_t index_pos_ = 0;
  std::unordered_map<std::string, std::shared_ptr<RosMsgTypes::ros_msg_def>> message_schemata_;

  friend class View;
};
}
