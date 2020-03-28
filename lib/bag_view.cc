#include "bag_view.h"
#include "ros_message.h"
#include "ros_value.h"
#include "message_parser.h"
#include "util.h"

namespace Embag {
View::iterator View::begin() {
  return iterator{*this, iterator::begin_cond_t{}};
}

View::iterator View::end() {
  return iterator{*this};
}

View::iterator::iterator(View &view, begin_cond_t begin_cond) : view_(view) {
  // Read a message from each bag into the corresponding bag wrapper
  for (auto &pair : view_.bag_wrappers_) {
    readMessage(pair.second);
  }
}

std::unique_ptr<RosMessage> View::iterator::operator*() const {
  // Take the first wrapper from the priority queue
  auto wrapper = msg_queue_.top();

  const auto &connection = wrapper->bag->connections_[wrapper->current_connection_id];
  const auto &msg_def = wrapper->bag->message_schemata_[connection.topic];

  // TODO: streaming this data means copying it into basic types.  It would be faster to just set pointers...
  message_stream stream{wrapper->current_message_data, wrapper->current_message_len};

  auto message = make_unique<RosMessage>();
  MessageParser msg{stream, connection.data, msg_def};

  message->data_ = msg.parse();
  message->topic = connection.topic;
  message->timestamp = wrapper->current_timestamp;
  message->md5 = connection.data.md5sum;

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

    const auto value = strstr(p, "=");

    if (value == nullptr) {
      throw std::runtime_error("Unable to find '=' in header field - perhaps this bag is corrupt...");
    }

    // Compare the first char here for optimization reasons since this code gets pretty hot
    if (*p == 'o') {        // op
      header.op = RosBagTypes::header_t::op(*(value + 1));
    } else if (*p == 'c') { // conn
      header.connection_id = *reinterpret_cast<const uint32_t *>(value + 1);
    } else if (*p == 't') { // time
      header.timestamp = *reinterpret_cast<const RosValue::ros_time_t *>(value + 1);
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
  while (bag_wrapper->chunk_index < bag_wrapper->chunks_to_parse.size()) {
    if (bag_wrapper->current_buffer.empty()) {
      const auto &chunk = bag_wrapper->chunks_to_parse[bag_wrapper->chunk_index];
      bag_wrapper->current_buffer.reserve(chunk->uncompressed_size);
      // TODO: this really should be a function in chunks
      bag_wrapper->bag->decompressLz4Chunk(chunk->record.data,
                                           chunk->record.data_len,
                                           &bag_wrapper->current_buffer[0],
                                           chunk->uncompressed_size);
      bag_wrapper->uncompressed_size = chunk->uncompressed_size;
    }

    while (bag_wrapper->processed_bytes < bag_wrapper->uncompressed_size) {
      RosBagTypes::record_t record{};

      // TODO: just use pointers instead of copying memory?
      std::memcpy(&record.header_len,
                  bag_wrapper->current_buffer.c_str() + bag_wrapper->processed_bytes,
                  sizeof(record.header_len));
      bag_wrapper->processed_bytes += sizeof(record.header_len);
      record.header = &bag_wrapper->current_buffer.c_str()[bag_wrapper->processed_bytes];
      bag_wrapper->processed_bytes += record.header_len;

      std::memcpy(&record.data_len,
                  bag_wrapper->current_buffer.c_str() + bag_wrapper->processed_bytes,
                  sizeof(record.data_len));
      bag_wrapper->processed_bytes += sizeof(record.data_len);
      record.data = &bag_wrapper->current_buffer.c_str()[bag_wrapper->processed_bytes];
      bag_wrapper->processed_bytes += record.data_len;

      const auto header = readHeader(record);

      switch (header.op) {
        case RosBagTypes::header_t::op::MESSAGE_DATA: {
          // Check if this is a topic we're interested in
          if (bag_wrapper->connection_ids.count(header.connection_id) == 0) {
            continue;
          }

          bag_wrapper->current_message_data = const_cast<char *>(record.data);
          bag_wrapper->current_message_len = record.data_len;
          bag_wrapper->current_connection_id = header.connection_id;
          bag_wrapper->current_timestamp = header.timestamp;

          msg_queue_.push(bag_wrapper);
          return;
        }
        case RosBagTypes::header_t::op::CONNECTION: {
          // FIXME: not entirely sure what to do with these so we'll move to the next record...
          continue;
        }
        default: {
          throw std::runtime_error("Found unknown record type: " + std::to_string(static_cast<int>(header.op)));
        }
      }
    }

    bag_wrapper->chunk_index++;
    bag_wrapper->current_buffer.clear();
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

    for (const auto &chunk : bag->chunks_) {
      bag_wrappers_[bag]->chunks_to_parse.emplace_back(&chunk);
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
  bag_wrappers_.clear();

  for (const auto& bag : bags_) {
    bag_wrappers_[bag] = std::make_shared<iterator::bag_wrapper_t>();
    bag_wrappers_[bag]->bag = bag;

    for (const auto &topic : topics) {
      if (!bag->topic_connection_map_.count(topic)) {
        continue;
      }

      const auto connection_record = bag->topic_connection_map_.at(topic);
      for (const auto &block : connection_record->blocks) {
        bag_wrappers_[bag]->chunks_to_parse.emplace_back(block.into_chunk);
      }

      bag_wrappers_[bag]->connection_ids.emplace(connection_record->id);
    }
  }

  return *this;
}

View View::getMessages(std::initializer_list<std::string> topics) {
  return getMessages(std::vector<std::string>(topics.begin(), topics.end()));
}

View View::addBag(std::shared_ptr<Bag> bag) {
  bags_.emplace_back(bag);
  return *this;
}
}