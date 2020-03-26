#pragma once

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <queue>

#include "embag.h"
#include "ros_message.h"
#include "ros_value.h"
#include "ros_bag_types.h"

namespace Embag {

// Forward declaration
class Bag;

class View {
 public:
  View () = default;

  explicit View(Bag* bag) {
    bags_.emplace_back(bag);
  };

  struct iterator {
    View& view_;
    explicit iterator(View &view) : view_(view) {};
    iterator(View &view, size_t chunk_count);
    iterator(const iterator& other) : view_(other.view_) {};

    iterator& operator=(const iterator&& other) {
      msg_queue_ = other.msg_queue_;
      bag_count_ = other.bag_count_;

      return *this;
    }

    bool operator==(const iterator& other) const {
      return msg_queue_.size() == other.msg_queue_.size();
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

    // TODO: Move this outside of iterator?
    struct bag_wrapper_t {
      Bag *bag;
      size_t chunk_index = 0;
      size_t chunk_count = 0;
      size_t processed_bytes = 0;
      uint32_t uncompressed_size = 0;
      std::string current_buffer;
      std::vector<const RosBagTypes::chunk_t *> chunks_to_parse;
      std::unordered_set<uint32_t> connection_ids;


      uint32_t current_connection_id = 0;
      char *current_message_data = nullptr;
      uint32_t current_message_len = 0;
      RosValue::ros_time_t current_timestamp{};
    };

    static header_t readHeader(const RosBagTypes::record_t &record);
    void readMessage(std::shared_ptr<bag_wrapper_t> bag_wrapper);

    size_t bag_count_ = 0;

    // Lambda that compares message timestamps
    struct compare_t {
      bool operator()(std::shared_ptr<bag_wrapper_t> &left, std::shared_ptr<bag_wrapper_t> &right) {
        const auto &left_ts = left->current_timestamp;
        const auto &right_ts = right->current_timestamp;

        if (left_ts.secs > right_ts.secs) {
          return true;
        } else if (left_ts.secs == right_ts.secs && left_ts.nsecs > right_ts.nsecs) {
          return true;
        }
        return false;
      };
    };
    std::priority_queue<std::shared_ptr<bag_wrapper_t>, std::vector<std::shared_ptr<bag_wrapper_t>>, compare_t> msg_queue_;
  };

  iterator begin();
  iterator end();

  // Message iterators
  View getMessages();
  View getMessages(const std::string &topic);
  View getMessages(std::initializer_list<std::string> topics);

  // Bag set manipulation
  View addBag(Bag &bag);
  // FIXME: this doesn't work because we can't copy bag objects
  View addBags(std::initializer_list<Bag> bags);

 private:
  // TODO: Is storing pointers like this a good idea?
  std::vector<Bag*> bags_;
  std::unordered_map<Bag *, std::shared_ptr<iterator::bag_wrapper_t>> bag_wrappers_;
};
}
