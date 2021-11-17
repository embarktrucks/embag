#include "view.h"
#include "ros_message.h"
#include "ros_value.h"
#include "util.h"

namespace Embag {
View::iterator View::begin() {
  return iterator{this, iterator::begin_cond_t{}};
}

View::iterator View::end() {
  return iterator{this};
}

View::iterator::iterator(View *view, begin_cond_t begin_cond) : view_(view) {
  // Read a message from each bag into the corresponding bag wrapper
  for (auto &pair : view_->bag_wrappers_) {
    auto& wrapper = pair.second;
    wrapper->chunk_iter = wrapper->chunks_to_parse.begin();

    readMessage(wrapper);
  }
}

std::shared_ptr<RosMessage> View::iterator::operator*() const {
  // Take the first wrapper from the priority queue
  auto wrapper = msg_queue_.top();

  const auto &connection = wrapper->bag->connections_[wrapper->current_connection_id];
  const auto &msg_def = wrapper->bag->msgDefForTopic(connection.topic);

  auto message = std::make_shared<RosMessage>(wrapper->current_message_buffer, wrapper->current_message_data_offset);

  message->topic = connection.topic;
  message->timestamp = wrapper->current_timestamp;
  message->md5 = connection.data.md5sum;
  message->raw_data_len = wrapper->current_message_len;
  message->msg_def_ = msg_def;

  return message;
}

// This implementation of readHeader is faster but less flexible than the map-based version in Embag.
View::iterator::header_t View::iterator::readHeader(const RosBagTypes::record_t &record) {
  header_t header{};

  auto p = record.header;
  const char *end = record.header + record.header_len;

  while (p < end) {
    const uint32_t field_len = *(reinterpret_cast<const uint32_t *>(p));

    p += sizeof(uint32_t);

    const auto value_p = strstr(p, "=") + 1;

    if (value_p == nullptr) {
      throw std::runtime_error("Unable to find '=' in header field - perhaps this bag is corrupt...");
    }

    // Compare the first char here for optimization reasons since this code gets pretty hot
    if (*p == 'o') {        // op
      header.op = RosBagTypes::header_t::op(*value_p);
    } else if (*p == 'c') { // conn
      header.connection_id = *reinterpret_cast<const uint32_t *>(value_p);
    } else if (*p == 't') { // time
      header.timestamp = *reinterpret_cast<const RosValue::ros_time_t *>(value_p);
    }

    p += field_len;
  }

  return header;
}

/*
 * initialize: fill the queue with a message from each bag
 * store the message with the smallest timestamp in current_msg_ stuff and remove it from the queue
 * read another message from the bag that had its message removed from the queue
 */
void View::iterator::readMessage(std::shared_ptr<bag_wrapper_t> bag_wrapper) {
  while (bag_wrapper->chunk_iter != bag_wrapper->chunks_to_parse.end()) {
    if (!bag_wrapper->current_buffer) {
      const auto& chunk = *(bag_wrapper->chunk_iter);
      bag_wrapper->current_buffer = std::make_shared<std::vector<char>>(chunk->uncompressed_size);
      chunk->decompress(&bag_wrapper->current_buffer->at(0));
      bag_wrapper->uncompressed_size = chunk->uncompressed_size;
    }

    while (bag_wrapper->processed_bytes < bag_wrapper->uncompressed_size) {
      RosBagTypes::record_t record{};

      // TODO: just use pointers instead of copying memory?
      std::memcpy(&record.header_len,
                  &bag_wrapper->current_buffer->at(bag_wrapper->processed_bytes),
                  sizeof(record.header_len));
      bag_wrapper->processed_bytes += sizeof(record.header_len);
      record.header = &bag_wrapper->current_buffer->at(bag_wrapper->processed_bytes);
      bag_wrapper->processed_bytes += record.header_len;

      std::memcpy(&record.data_len,
                  &bag_wrapper->current_buffer->at(bag_wrapper->processed_bytes),
                  sizeof(record.data_len));
      bag_wrapper->processed_bytes += sizeof(record.data_len);
      record.data = &bag_wrapper->current_buffer->at(bag_wrapper->processed_bytes);
      bag_wrapper->processed_bytes += record.data_len;

      const auto header = readHeader(record);

      switch (header.op) {
        case RosBagTypes::header_t::op::MESSAGE_DATA: {
          // Check if this is a topic we're interested in
          if (bag_wrapper->connection_ids.count(header.connection_id) == 0) {
            continue;
          }

          bag_wrapper->current_message_buffer = bag_wrapper->current_buffer;
          bag_wrapper->current_message_data_offset = record.data - &bag_wrapper->current_message_buffer->at(0);
          bag_wrapper->current_message_len = record.data_len;
          bag_wrapper->current_connection_id = header.connection_id;
          bag_wrapper->current_timestamp = header.timestamp;

          msg_queue_.push(bag_wrapper);
          return;
        }
        case RosBagTypes::header_t::op::CONNECTION: {
          // TODO: not entirely sure what to do with these so we'll move to the next record...
          continue;
        }
        default: {
          throw std::runtime_error("Found unknown record type: " + std::to_string(static_cast<int>(header.op)));
        }
      }
    }

    bag_wrapper->chunk_iter++;
    bag_wrapper->current_buffer.reset();
    bag_wrapper->processed_bytes = 0;
  }
}

View::iterator &View::iterator::operator++() {
  auto wrapper = msg_queue_.top();
  msg_queue_.pop();

  readMessage(wrapper);

  return *this;
}

View View::getMessages() {
  bag_wrappers_.clear();

  for (const auto& bag : bags_) {
    bag_wrappers_[bag] = std::make_shared<iterator::bag_wrapper_t>();
    bag_wrappers_[bag]->bag = bag;

    for (const auto &chunk : bag->chunks_) {
      bag_wrappers_[bag]->chunks_to_parse.emplace(&chunk);
    }

    for (size_t i = 0; i < bag->connections_.size(); i++) {
      bag_wrappers_[bag]->connection_ids.emplace(i);
    }
  }

  return *this;
}

View View::getMessages(const std::string &topic) {
  return getMessages({topic});
}

View View::getMessages(const std::vector<std::string> &topics) {
  std::chrono::nanoseconds start_time_ns {getStartTime().to_nsec()};
  std::chrono::nanoseconds end_time_ns   {getEndTime().to_nsec()};
  return getMessages(topics, start_time_ns, end_time_ns);
}

View View::getMessages(const std::vector<std::string> &topics, std::chrono::nanoseconds start_time, std::chrono::nanoseconds end_time) {
  bag_wrappers_.clear();

  for (const auto& bag : bags_) {
    bag_wrappers_[bag] = std::make_shared<iterator::bag_wrapper_t>();
    bag_wrappers_[bag]->bag = bag;

    for (const auto &topic : topics) {
      if (!bag->topic_connection_map_.count(topic)) {
        continue;
      }

      for (const auto &connection_record : bag->topic_connection_map_.at(topic)) {
        for (const auto &block : connection_record->blocks) {
          if (block.into_chunk->info.end_time > start_time && block.into_chunk->info.start_time < end_time) {
            bag_wrappers_[bag]->chunks_to_parse.emplace(block.into_chunk);
          }
        }
        bag_wrappers_[bag]->connection_ids.emplace(connection_record->id);
      }
    }
  }

  return *this;
}

View View::getMessages(std::initializer_list<std::string> topics) {
  return getMessages(std::vector<std::string>(topics.begin(), topics.end()));
}

RosValue::ros_time_t View::getStartTime() {
  RosValue::ros_time_t start_time;
  start_time.secs = UINT32_MAX;
  start_time.nsecs = UINT32_MAX;

  for (const auto& bag : bags_) {
    const auto& bag_start = bag->chunks_.front().info.start_time;
    if (bag_start.secs < start_time.secs) {
      start_time = bag_start;
    } else if (bag_start.secs == start_time.secs && bag_start.nsecs < start_time.nsecs) {
      start_time = bag_start;
    }
  }

  return start_time;
}

RosValue::ros_time_t View::getEndTime() {
  RosValue::ros_time_t end_time;
  end_time.secs = 0;
  end_time.nsecs = 0;

  for (const auto& bag : bags_) {
    const auto& bag_end = bag->chunks_.back().info.end_time;
    if (bag_end.secs > end_time.secs) {
      end_time = bag_end;
    } else if (bag_end.secs == end_time.secs && bag_end.nsecs > end_time.nsecs) {
      end_time = bag_end;
    }
  }

  return end_time;
}

View View::addBag(const std::string &filename) {
  auto bag = std::make_shared<Embag::Bag>(filename);
  addBag(bag);
  return *this;
}

View View::addBag(std::shared_ptr<Bag> bag) {
  bags_.emplace_back(bag);
  return *this;
}
}