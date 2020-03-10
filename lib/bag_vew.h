#pragma once

#include <unordered_set>
#include <vector>

#include "embag.h"
#include "ros_message.h"
#include "ros_value.h"
#include "ros_bag_types.h"

// Forward declaration
class Embag;

class BagView {
 public:
  BagView(Embag& bag) : bag_(bag) {};

  struct iterator {
    const BagView& view_;
    explicit iterator(const BagView &view) : view_(view) {};
    iterator(const BagView &view, size_t chunk_count);
    iterator(const iterator& other) : view_(other.view_) {};

    iterator& operator=(const iterator&& other) {
      chunk_index_ = other.chunk_index_;
      chunk_count_ = other.chunk_count_;
      current_buffer_ = other.current_buffer_;
      processed_bytes_ = other.processed_bytes_;
      uncompressed_size_ = other.uncompressed_size_;

      return *this;
    }

    bool operator==(const iterator& other) const {
      return chunk_count_ == other.chunk_count_;
    }

    bool operator!=(const iterator& other) const {
      return !(*this == other);
    }

    std::unique_ptr<RosMessage> operator*() const;

    iterator& operator++();

    struct header_t {
      RosBagTypes::header_t::op op = RosBagTypes::header_t::op::UNSET;
      uint32_t connection_id = 0;
      RosValue::ros_time_t timestamp;
    };

    static header_t readHeader(const RosBagTypes::record_t &record);
    void readMessage();

    size_t chunk_index_ = 0;
    size_t chunk_count_ = 0;
    std::string current_buffer_;
    size_t processed_bytes_ = 0;
    uint32_t uncompressed_size_ = 0;

    // TODO: remove these once ros bag types are available?
    uint32_t current_connection_id_ = 0;
    char *current_message_data_ = nullptr;
    uint32_t current_message_len_ = 0;
    RosValue::ros_time_t current_timestamp_{};
  };

  iterator begin();
  iterator end();

  BagView getMessages();
  BagView getMessages(const std::string &topic);
  BagView getMessages(std::initializer_list<std::string> topics);

 private:
  Embag &bag_;
  std::vector<RosBagTypes::chunk_t *> chunks_to_parse_;
  std::unordered_set<uint32_t> connection_ids_;
};
