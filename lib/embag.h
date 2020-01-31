#pragma once

#include <string>
#include <map>
#include <vector>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

class Embag {
 public:
  const std::string MAGIC_STRING = "#ROSBAG V";

  Embag(std::string filename) : filename_(filename) {}

  bool open();

  bool close();

  bool read_records();

 private:

  struct record_t {
    uint32_t header_len;
    const char *header;
    uint32_t data_len;
    const char *data;
  };

  struct header_t {
    std::map<std::string, std::string> fields;
    enum class op {
      BAG_HEADER   = 0x03,
      CHUNK        = 0x05,
      CONNECTION   = 0x07,
      MESSAGE_DATA = 0x02,
      INDEX_DATA   = 0x04,
      CHUNK_INFO   = 0x06,
      UNSET        = 0xff,
    };

    const op get_op() const {
      return header_t::op(*(fields.at("op").data()));
    }

    template <typename T>
    const void get_field(const std::string& name, T& value) const {
      value = *reinterpret_cast<const T*>(fields.at(name).data());
      return;
    }
  };

  struct connection_record_t {

  };

  struct chunk_t {
    chunk_t(record_t r) {
      // TODO
    };
  };

  record_t read_record();
  header_t read_header(const record_t &record);
  std::string filename_;
  boost::iostreams::stream <boost::iostreams::mapped_file_source> bag_stream_;

  // Bag data
  std::vector<connection_record_t> connections_;
  std::vector<chunk_t> chunks_;
  uint64_t index_pos_;
};
