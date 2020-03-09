#include "bag_vew.h"
#include "ros_value.h"
#include "ros_msg.h"

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

// TODO: move this
typedef boost::iostreams::stream<boost::iostreams::array_source> message_stream;

std::unique_ptr<RosValue> BagView::iterator::operator*() const {
  // TODO: move this out of the hot path
  const auto &connection = view_.bag_.connections_[current_connection_id_];
  const auto &msg_def = view_.bag_.message_schemata_[connection.topic];

  // TODO: streaming this data means copying it into basic types.  It would be faster to just set pointers appropriately...
  message_stream stream{current_message_data_, current_message_len_};

  RosMsg msg{stream, connection.data, msg_def};

  return msg.parse();
}

void BagView::iterator::readMessage() {
  if (current_buffer_.empty()) {
    const auto &chunk = view_.chunks_to_parse_[chunk_index_];
    current_buffer_.resize(chunk->uncompressed_size, 0);
    // TODO: this really should be a function in chunks
    view_.bag_.decompressLz4Chunk(chunk->record.data, chunk->record.data_len, &current_buffer_[0], chunk->uncompressed_size);
    uncompressed_size_ = chunk->uncompressed_size;
  }

  if (processed_bytes_ < uncompressed_size_) {
    const size_t offset = processed_bytes_;

    // TODO: clean this up
    RosBagTypes::record_t record{};
    size_t p = sizeof(record.header_len);
    std::memcpy(&record.header_len, current_buffer_.c_str() + offset, p);
    record.header = &current_buffer_.c_str()[offset + p];
    p += record.header_len;
    std::memcpy(&record.data_len, current_buffer_.c_str() + offset + p, sizeof(record.data_len));
    p += sizeof(record.data_len);
    record.data = &current_buffer_.c_str()[offset + p];
    p += record.data_len;
    const auto header = Embag::readHeader(record);

    const auto op = header.getOp();
    switch (op) {
      case RosBagTypes::header_t::op::MESSAGE_DATA: {
        uint32_t connection_id;
        RosValue::ros_time_t timestamp;
        header.getField("conn", connection_id);
        header.getField("time", timestamp);

        if (view_.connection_ids_.count(connection_id)) {
          current_message_data_ = const_cast<char *>(record.data);
          current_message_len_ = record.data_len;
          current_connection_id_ = connection_id;
          processed_bytes_ += p;
        } else {
          processed_bytes_ += p;
          // FIXME: Recursion may be expensive here...
          readMessage();
        }

        break;
      }
      case RosBagTypes::header_t::op::CONNECTION: {
        // TODO: not entirely sure what to do with these...
        // Move to the next record for now
        processed_bytes_ += p;
        // FIXME: Recursion may be expensive here...
        readMessage();
        break;
      }
      default: {
        throw std::runtime_error("Found unknown record type: " + std::to_string(static_cast<int>(op)));
      }
    }
  } else {
    chunk_index_++;
    chunk_count_--;
    current_buffer_.clear();
    processed_bytes_ = 0;
  }
}

BagView::iterator& BagView::iterator::operator++() {
  readMessage();

  return *this;
}

BagView BagView::getMessages() {
  chunks_to_parse_.clear();
  for (auto &chunk : bag_.chunks_) {
    chunks_to_parse_.emplace_back(&chunk);
  }

  connection_ids_.clear();
  for (size_t i = 0; i < bag_.connections_.size(); i++) {
    connection_ids_.emplace(i);
  }

  std::cout << "Found " << chunks_to_parse_.size() << " chunks to parse (of " << bag_.chunks_.size() << ") for all topics " << std::endl;

  return *this;
}

BagView BagView::getMessages(const std::string &topic) {
  return getMessages({topic});
}

// TODO: handle the case where we find no messages
BagView BagView::getMessages(std::initializer_list<std::string> topics) {
  chunks_to_parse_.clear();
  connection_ids_.clear();

  for (const auto &topic : topics) {
    if (!bag_.topic_connection_map_.count(topic)) {
      std::cout << "WARNING: unable to find topic in bag: " << topic << std::endl;
      continue;
    }

    auto &connection_record = bag_.topic_connection_map_[topic];
    for (const auto &block : connection_record.blocks) {
      chunks_to_parse_.emplace_back(block.into_chunk);
    }

    connection_ids_.emplace(connection_record.id);
  }

  return *this;
}
