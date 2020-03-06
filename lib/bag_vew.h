#pragma once

#include "embag.h"

class Embag;
class BagView {
 public:

  BagView(Embag& bag) : bag_(bag) {};

  struct iterator {
    const BagView& view_;
    explicit iterator(const BagView &view) : view_(view) {};
    iterator(const BagView &view, size_t chunk_count);
    iterator(const iterator& other) : view_(other.view_), i(other.i) {};

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

    std::unique_ptr<RosValue> operator*() const;

    iterator& operator++();

    size_t chunk_index_ = 0;
    size_t chunk_count_ = 0;
    std::string current_buffer_;
    size_t processed_bytes_ = 0;
    uint32_t uncompressed_size_ = 0;
  };


  iterator begin();
  iterator end();

  BagView getMessages(const std::string &topic);

 private:
  Embag &bag_;

};
