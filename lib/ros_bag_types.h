#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "ros_value.h"

namespace Embag {
struct RosBagTypes {
  struct connection_data_t {
    std::string topic;
    std::string type;
    std::string scope;
    std::string md5sum;
    std::string message_definition;
    std::string callerid;
    bool latching = false;
  };

  struct record_t {
    uint32_t header_len;
    const char *header;
    uint32_t data_len;
    const char *data;
  };

  struct header_t {
    std::unique_ptr<std::unordered_map<std::string, std::string>> fields;

    enum class op {
      BAG_HEADER = 0x03,
      CHUNK = 0x05,
      CONNECTION = 0x07,
      MESSAGE_DATA = 0x02,
      INDEX_DATA = 0x04,
      CHUNK_INFO = 0x06,
      UNSET = 0xff,
    };

    const op getOp() const {
      return header_t::op(*(fields->at("op").data()));
    }

    const void getField(const std::string &name, std::string &value) const {
      value = fields->at(name);
    }

    template<typename T>
    const void getField(const std::string &name, T &value) const {
      value = *reinterpret_cast<const T *>(fields->at(name).data());
    }
  };

  struct chunk_info_t {
    RosValue::ros_time_t start_time;
    RosValue::ros_time_t end_time;
    uint32_t message_count = 0;
  };

  struct chunk_t {
    uint64_t offset = 0;
    chunk_info_t info;
    std::string compression;
    uint32_t uncompressed_size = 0;
    record_t record{};

    explicit chunk_t(record_t r) {
      record = r;
    };
  };

  struct index_block_t {
    chunk_t *into_chunk;
  };

  struct connection_record_t {
    uint32_t id;
    std::vector<index_block_t> blocks;
    std::string topic;
    connection_data_t data;
  };
};
}
