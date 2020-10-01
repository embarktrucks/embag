#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <bzlib.h>

#include "ros_value.h"
#include "decompression.h"

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

    op getOp() const {
      return header_t::op(*(fields->at("op").data()));
    }

    void getField(const std::string &name, std::string &value) const {
      value = fields->at(name);
    }

    template<typename T>
    void getField(const std::string &name, T &value) const {
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

    void decompress(char *dst) const {
      if (compression == "lz4") {
        decompressLz4Chunk(dst);
      } else if (compression == "bz2") {
        decompressBz2Chunk(dst);
      }
    }

    void decompressLz4Chunk(char *dst) const {
      size_t src_bytes_left = record.data_len;
      size_t dst_bytes_left = uncompressed_size;

      while (dst_bytes_left && src_bytes_left) {
        size_t src_bytes_read = src_bytes_left;
        size_t dst_bytes_written = dst_bytes_left;
        auto& lz4_ctx = Lz4DecompressionCtx::getInstance();
        const size_t ret = LZ4F_decompress(lz4_ctx.context(), dst, &dst_bytes_written, record.data, &src_bytes_read, nullptr);
        if (LZ4F_isError(ret)) {
          throw std::runtime_error("chunk::decompress: lz4 decompression returned " + std::to_string(ret) + ", expected "
                                       + std::to_string(src_bytes_read));
        }

        src_bytes_left -= src_bytes_read;
        dst_bytes_left -= dst_bytes_written;
      }

      if (src_bytes_left || dst_bytes_left) {
        throw std::runtime_error("chunk::decompress: lz4 decompression left " + std::to_string(src_bytes_left) + "/"
                                     + std::to_string(dst_bytes_left) + " bytes in buffer");
      }
    };

    void decompressBz2Chunk(char *dst) const {
      unsigned int dst_bytes_left = uncompressed_size;
      char * source = const_cast<char *>(record.data);
      const auto r = BZ2_bzBuffToBuffDecompress(dst, &dst_bytes_left, source, record.data_len, 0,0);
      if (r != BZ_OK) {
        throw std::runtime_error("Failed decompress bz2 chunk, bz2 error code: " + std::to_string(r));
      }
    }
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
