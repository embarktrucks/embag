#pragma once

#include <chrono>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <queue>

#include "embag.h"
#include "ros_message.h"
#include "ros_value.h"
#include "ros_bag_types.h"

namespace Embag {

class View {
 public:
  View () = default;

  explicit View(std::shared_ptr<Bag> bag) {
    bags_.emplace_back(bag);
  };

  explicit View(const std::string& filename) {
    addBag(filename);
  }

  struct iterator {
    struct begin_cond_t{};

    View* view_;
    iterator() : view_(nullptr) {};

    // Begin constructor
    iterator(View *view, begin_cond_t begin_cond);
    // End constructor
    explicit iterator(View *view) : view_(view) {};

    // Copy constructor
    iterator(const iterator& other) : view_(other.view_), msg_queue_(other.msg_queue_) {};

    iterator& operator=(const iterator&& other) {
      view_ = other.view_;
      msg_queue_ = other.msg_queue_;

      return *this;
    }

    // == is used to determine if the current iterator is equal to .end().  So, when we're done, we'll want this
    // to be true.
    bool operator==(const iterator& other) const {
      return msg_queue_.size() == other.msg_queue_.size();
    }

    bool operator!=(const iterator& other) const {
      return !(*this == other);
    }

    std::shared_ptr<RosMessage> operator*() const;

    iterator& operator++();

    struct header_t {
      RosBagTypes::header_t::op op = RosBagTypes::header_t::op::UNSET;
      uint32_t connection_id = 0;
      RosValue::ros_time_t timestamp;
    };

    // TODO: Move this outside of iterator?
    struct bag_wrapper_t {
      std::shared_ptr<Bag> bag;
      size_t processed_bytes = 0;
      uint32_t uncompressed_size = 0;
      std::shared_ptr<std::vector<char>> current_buffer;

      // Function for comparing bag offsets
      struct bag_offset_compare_t {
        bool operator()(const RosBagTypes::chunk_t *left, const RosBagTypes::chunk_t *right) const {
          return left->offset < right->offset;
        }
      };

      std::set<const RosBagTypes::chunk_t *, bag_offset_compare_t> chunks_to_parse;
      std::set<const RosBagTypes::chunk_t *, bag_offset_compare_t>::iterator chunk_iter;
      std::unordered_set<uint32_t> connection_ids;


      uint32_t current_connection_id = 0;
      std::shared_ptr<std::vector<char>> current_message_buffer;
      size_t current_message_data_offset;
      uint32_t current_message_len = 0;
      RosValue::ros_time_t current_timestamp{};
    };

    static header_t readHeader(const RosBagTypes::record_t &record);
    void readMessage(std::shared_ptr<bag_wrapper_t> bag_wrapper);

    // Function for comparing message timestamps
    struct timestamp_compare_t {
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

    std::priority_queue<std::shared_ptr<bag_wrapper_t>, std::vector<std::shared_ptr<bag_wrapper_t>>, timestamp_compare_t> msg_queue_;
  };

  iterator begin();
  iterator end();

  // Message iterators
  View getMessages();
  View getMessages(const std::string &topic);
  View getMessages(const std::vector<std::string> &topics);
  View getMessages(std::initializer_list<std::string> topics);
  View getMessages(const std::vector<std::string> &topics, std::chrono::nanoseconds start_time, std::chrono::nanoseconds end_time);

  void setQuery(const std::vector<std::string> &topics, std::chrono::nanoseconds start_time, std::chrono::nanoseconds end_time);
  RosValue::ros_time_t getStartTime();
  RosValue::ros_time_t getEndTime();

  // Bag set manipulation
  View addBag(const std::string &filename);
  View addBag(std::shared_ptr<Bag> bag);

  std::vector<std::string> topics() {
    std::unordered_set<std::string> topics;
    for (const auto& bag : bags_) {
      const auto new_topics = bag->topics();
      topics.insert(new_topics.begin(), new_topics.end());
    }

    return std::vector<std::string>(topics.begin(), topics.end());
  }

  std::unordered_map<std::string, std::vector<RosBagTypes::connection_data_t>> connectionsByTopicMap() const {
    std::unordered_map<std::string, std::vector<RosBagTypes::connection_data_t>> connections_by_topic;
    for (const auto &bag : bags_) {
      for (const auto &item : bag->connectionsByTopicMap()) {
        const auto &topic = item.first;
        const auto &new_conns = item.second;
        auto &existing_conns = connections_by_topic[topic];
        for (const auto &new_c : new_conns) {
          auto existing_it = std::find(existing_conns.begin(), existing_conns.end(), new_c);
          if (existing_it == existing_conns.end()) {
            existing_conns.push_back(new_c);
          } else {
            existing_it->message_count += new_c.message_count;
          }
        }
      }
    }
    return connections_by_topic;
  }

 private:
  std::chrono::nanoseconds start_time_, end_time_;
  std::vector<std::shared_ptr<Bag>> bags_;
  std::unordered_map<std::shared_ptr<Bag>, std::shared_ptr<iterator::bag_wrapper_t>> bag_wrappers_;
};
}
