#include "bag_vew.h"
#include "ros_message.h"
#include "ros_value.h"
#include "message_parser.h"
#include "util.h"

BagView::iterator BagView::begin() {
  return iterator{*this, chunks_to_parse_.size()};
}

BagView::iterator BagView::end() {
  return iterator{*this};
}

BagView::iterator::iterator(const BagView& view, size_t chunk_count) : view_(view), chunk_count_(chunk_count) {
  if (chunk_count == 0) {
    return;
  }

  readMessage();
}

std::unique_ptr<RosMessage> BagView::iterator::operator*() const {
  const auto &connection = view_.bag_.connections_[current_connection_id_];
  const auto &msg_def = view_.bag_.message_schemata_[connection.topic];

  // TODO: streaming this data means copying it into basic types.  It would be faster to just set pointers...
  message_stream stream{current_message_data_, current_message_len_};

  auto message = make_unique<RosMessage>();
  MessageParser msg{stream, connection.data, msg_def};

  message->data = msg.parse();
  message->topic = connection.topic;
  message->timestamp = current_timestamp_;

  return message;
}

// This implementation of readHeader is faster but less flexible than the map-based version in Embag.
BagView::iterator::header_t BagView::iterator::readHeader(const RosBagTypes::record_t &record) {
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

void BagView::iterator::readMessage() {
  if (current_buffer_.empty()) {
    const auto &chunk = view_.chunks_to_parse_[chunk_index_];
    current_buffer_.reserve(chunk->uncompressed_size);
    // TODO: this really should be a function in chunks
    view_.bag_.decompressLz4Chunk(chunk->record.data, chunk->record.data_len, &current_buffer_[0], chunk->uncompressed_size);
    uncompressed_size_ = chunk->uncompressed_size;
  }

  while (processed_bytes_ < uncompressed_size_) {
    RosBagTypes::record_t record{};

    // TODO: just use pointers instead of copying memory?
    std::memcpy(&record.header_len, current_buffer_.c_str() + processed_bytes_, sizeof(record.header_len));
    processed_bytes_ += sizeof(record.header_len);
    record.header = &current_buffer_.c_str()[processed_bytes_];
    processed_bytes_ += record.header_len;

    std::memcpy(&record.data_len, current_buffer_.c_str() + processed_bytes_, sizeof(record.data_len));
    processed_bytes_ += sizeof(record.data_len);
    record.data = &current_buffer_.c_str()[processed_bytes_];
    processed_bytes_ += record.data_len;

    const auto header = readHeader(record);

    switch (header.op) {
      case RosBagTypes::header_t::op::MESSAGE_DATA: {
        // Check if this is a topic we're interested in
        if (view_.connection_ids_.count(header.connection_id)) {
          current_message_data_ = const_cast<char *>(record.data);
          current_message_len_ = record.data_len;
          current_connection_id_ = header.connection_id;
          current_timestamp_ = header.timestamp;
        } else {
          continue;
        }

        break;
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

  chunk_index_++;
  chunk_count_--;
  current_buffer_.clear();
  processed_bytes_ = 0;
}

BagView::iterator& BagView::iterator::operator++() {
  readMessage();

  return *this;
}

BagView BagView::getMessages() {
  chunks_to_parse_.clear();
  connection_ids_.clear();

  for (auto &chunk : bag_.chunks_) {
    chunks_to_parse_.emplace_back(&chunk);
  }

  for (size_t i = 0; i < bag_.connections_.size(); i++) {
    connection_ids_.emplace(i);
  }

  return *this;
}

BagView BagView::getMessages(const std::string &topic) {
  return getMessages({topic});
}

BagView BagView::getMessages(std::initializer_list<std::string> topics) {
  chunks_to_parse_.clear();
  connection_ids_.clear();

  for (const auto &topic : topics) {
    if (!bag_.topic_connection_map_.count(topic)) {
      std::cout << "WARNING: unable to find topic in bag: " << topic << std::endl;
      continue;
    }

    auto connection_record = bag_.topic_connection_map_[topic];
    for (const auto &block : connection_record->blocks) {
      chunks_to_parse_.emplace_back(block.into_chunk);
    }

    connection_ids_.emplace(connection_record->id);
  }

  return *this;
}
