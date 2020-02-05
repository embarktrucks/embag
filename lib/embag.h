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

  // TODO: convert these to classes?
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

    const void get_field(const std::string& name, std::string& value) const {
      value = fields.at(name);
    }

    template <typename T>
    const void get_field(const std::string& name, T& value) const {
      value = *reinterpret_cast<const T*>(fields.at(name).data());
    }
    const bool check_field(const std::string& name) const {
      return fields.find(name) != fields.end();
    }
  };

  struct chunk_info_t {
    uint64_t start_time;
    uint64_t end_time;
    uint32_t message_count;
  };

  struct chunk_t {
    uint64_t offset = 0;
    chunk_info_t info;
    std::string compression;
    uint32_t size;

    chunk_t(record_t r) {
      // TODO
    };
  };

  struct index_block_t {
    chunk_t* into_chunk;

  };

  struct connection_data_t {
    std::string topic;
    std::string type;
    std::string md5sum;
    std::string message_definition;
    std::string callerid;
    bool latching = false;
  };

  struct connection_record_t {
    std::vector<index_block_t> blocks;
    std::string topic;
    connection_data_t data;
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
