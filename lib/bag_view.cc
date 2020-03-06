#include "bag_vew.h"
#include "ros_value.h"

BagView::iterator BagView::begin() {
  return iterator{*this, bag_.chunks_to_parse_.size()};
}

BagView::iterator BagView::end() {
  return iterator{*this};
}

BagView::iterator::iterator(const BagView& view, size_t chunk_count) : view_(view), chunk_count_(chunk_count) {}


std::unique_ptr<RosValue> BagView::iterator::operator*() const {

  auto message = view_.bag_.parseMessage(connection_id, record);

}

BagView::iterator& BagView::iterator::operator++() {
  if (current_buffer_.empty()) {
    const auto &chunk = view_.bag_.chunks_to_parse_[chunk_index_];
    current_buffer_ = std::string(chunk->uncompressed_size, 0);
    // TODO: this really should be a function in chunks
    view_.bag_.decompressLz4Chunk(chunk->record.data, chunk->record.data_len, &current_buffer_[0], chunk->uncompressed_size);
    uncompressed_size_ = chunk->uncompressed_size;
  }

  if (processed_bytes_ < uncompressed_size_) {
    const size_t offset = processed_bytes_;

    Embag::record_t record{};
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
      case Embag::header_t::op::MESSAGE_DATA: {
        uint32_t connection_id;
        header.getField("conn", connection_id);
        const std::string &topic = view_.bag_.connections_[connection_id].topic;

        std::cout << "Message on " << topic << std::endl;

        //auto message = view_.bag_.parseMessage(connection_id, record);

        break;
      }
      case Embag::header_t::op::CONNECTION: {
        // TODO: not entirely sure what to do with these...
        uint32_t connection_id;
        header.getField("conn", connection_id);
        break;
      }
      default: {
        throw std::runtime_error("Found unknown record type: " + std::to_string(static_cast<int>(op)));
      }
    }

    processed_bytes_ += p;
  } else {
    // TODO: move to the next chunk
    chunk_count_ = 0;
  }

  return *this;
}

BagView BagView::getMessages(const std::string &topic) {
  auto &connection_record = bag_.topic_connection_map_[topic];

  bag_.chunks_to_parse_.clear();
  for (const auto &block : connection_record.blocks) {
    bag_.chunks_to_parse_.emplace_back(block.into_chunk);
  }

  std::cout << "Found " << bag_.chunks_to_parse_.size() << " chunks to parse (of " << bag_.chunks_.size() << ") for topic " << topic << std::endl;

  return *this;
}
