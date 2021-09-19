#pragma once

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
      std::string current_buffer;

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
      char *current_message_data = nullptr;
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

  std::unordered_map<std::string, std::vector<RosBagTypes::connection_record_t *>> connectionsByTopicMap() const {
    std::unordered_map<std::string, std::vector<RosBagTypes::connection_record_t *>> connections_by_topic;
    for (const auto &bag : bags_) {
      for (const auto &item : bag->connectionsByTopicMap()) {
        const auto &topic = item.first;
        const auto &new_connections = item.second;
        auto &existing_connections = connections_by_topic[topic];
        for (auto *new_c : new_connections) {
          bool already_exists = std::any_of(
              existing_connections.cbegin(), existing_connections.cend(),
              [new_c](RosBagTypes::connection_record_t *existing_c) {
                return new_c->data.type == existing_c->data.type &&
                       new_c->data.md5sum == existing_c->data.md5sum &&
                       new_c->data.callerid == existing_c->data.callerid &&
                       new_c->data.latching == existing_c->data.latching;
              });
          if (!already_exists) {
            existing_connections.push_back(new_c);
          }
        }
      }
    }
    return connections_by_topic;
  }

 private:
  std::vector<std::shared_ptr<Bag>> bags_;
  std::unordered_map<std::shared_ptr<Bag>, std::shared_ptr<iterator::bag_wrapper_t>> bag_wrappers_;
};
}
