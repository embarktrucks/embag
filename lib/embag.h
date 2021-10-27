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
#include "util.h"

namespace Embag {

// Forward declaration
class View;

class Bag {
 public:
  Bag(const std::string &path) {
    bag_impl_ = make_unique<BagFromFile>(this, path);
  }

  Bag(std::shared_ptr<const std::string>bytes) {
    bag_impl_ = make_unique<BagFromBytes>(this, bytes);
  }

  ~Bag() {
    bag_impl_->close();
  }

  void close() {
    bag_impl_->close();
  }

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

  std::shared_ptr<RosMsgTypes::MsgDef> msgDefForTopic(const std::string &topic) {
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

  class BagImpl {
   public:
    explicit BagImpl(Bag *bag) : bag_(bag) {}

    virtual void close() {};

    virtual ~BagImpl() = default;

   protected:
    Bag *bag_ = nullptr;
  };

  class BagFromFile : public BagImpl {
   public:
    BagFromFile(Bag *bag, const std::string &path) : BagImpl(bag) {
      open(path);
    }

    void open(const std::string &path);
    void close();

   private:
    boost::iostreams::stream<boost::iostreams::mapped_file_source> bag_stream_;
  };

  class BagFromBytes : public BagImpl {
   public:
    BagFromBytes(Bag *bag, std::shared_ptr<const std::string>bytes) : BagImpl(bag), bytes_(bytes) {
      open(bytes->data(), bytes->size());
    }

    void open(const char* bytes, size_t length);
    void close();

   private:
    std::shared_ptr<const std::string> bytes_;
    boost::iostreams::stream<boost::iostreams::array_source> bag_stream_;
  };

  template <typename T>
  bool readStream(boost::iostreams::stream<T> &stream, const char* buffer, const size_t buffer_size);
  template <typename T>
  bool readRecords(boost::iostreams::stream<T> &stream);
  template <typename T>
  RosBagTypes::record_t readRecord(boost::iostreams::stream<T> &stream);
  static std::unique_ptr<std::unordered_map<std::string, std::string>> readFields(const char *p, uint64_t len);
  static RosBagTypes::header_t readHeader(const RosBagTypes::record_t &record);
  void parseMsgDefForTopic(const std::string &topic);

  char* bag_bytes_ = nullptr;
  size_t bag_bytes_size_ = 0;

  std::unique_ptr<BagImpl> bag_impl_;

  // Bag data
  std::vector<RosBagTypes::connection_record_t> connections_;
  std::unordered_map<std::string, std::vector<RosBagTypes::connection_record_t *>> topic_connection_map_;
  std::vector<RosBagTypes::chunk_info_t> chunk_infos_;
  std::vector<RosBagTypes::chunk_t> chunks_;
  uint64_t index_pos_ = 0;
  std::unordered_map<std::string, std::shared_ptr<RosMsgTypes::MsgDef>> message_schemata_;

  friend class View;
};
}
