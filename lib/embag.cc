#include "embag.h"

#include <iostream>

bool Embag::open() {
  boost::iostreams::mapped_file_source mapped_file_source(filename_);
  bag_stream_.open(mapped_file_source);

  // First, check for the magic string indicating this is indeed a bag file
  std::string buffer(MAGIC_STRING.size(), 0);

  bag_stream_.read(&buffer[0], MAGIC_STRING.size());

  if (buffer != MAGIC_STRING) {
    std::cout << "ERROR: This file doesn't appear to be a bag file... " << buffer << std::endl;
    bag_stream_.close();
    return false;
  }

  // Next, parse the version
  buffer.resize(3);
  bag_stream_.read(&buffer[0], 3);

  // Only version 2.0 is supported at the moment
  if ("2.0" != buffer) {
    std::cout << "ERROR: Unsupported bag file version: " << buffer << std::endl;
    bag_stream_.close();
    return false;
  }

  // The version is followed by a newline
  buffer.resize(1);
  bag_stream_.read(&buffer[0], 1);
  if ("\n" != buffer) {
    std::cout << "ERROR: Unable to find newline after version string, perhaps this bag file is corrupted?" << std::endl;
    return false;
  }

  // At this point the stream is positioned at the first record so we're done here
  return true;
}

bool Embag::close() {
  if (bag_stream_.is_open()) {
    bag_stream_.close();
    return true;
  }

  return false;
}

Embag::record_t Embag::read_record() {
  Embag::record_t record;
  // Read header length
  bag_stream_.read(reinterpret_cast<char *>(&record.header_len), sizeof(record.header_len));

  // Set the header data pointer and skip to the next section
  record.header = bag_stream_->data() + bag_stream_.tellg();
  bag_stream_.seekg(record.header_len, std::ios_base::cur);

  // Read data length
  bag_stream_.read(reinterpret_cast<char *>(&record.data_len), sizeof(record.data_len));

  // Set the data pointer and skip to the next record
  record.data = bag_stream_->data() + bag_stream_.tellg();
  bag_stream_.seekg(record.data_len, std::ios_base::cur);

  std::cout << "Record with head len " << record.header_len << " data len " << record.data_len << std::endl;

  return record;
}

Embag::header_t Embag::read_header(const record_t &record) {
  header_t header;

  char *p = const_cast<char *>(record.header);
  const char *header_end = record.header + record.header_len;

  while (p < header_end) {
    uint32_t field_len = *(reinterpret_cast<uint32_t *>(p));

    p += sizeof(uint32_t);

    // FIXME: these are copies...
    std::string buffer(reinterpret_cast<char *>(p), field_len);
    const auto sep = buffer.find("=");

    const auto name = buffer.substr(0, sep);
    const auto value = buffer.substr(sep + 1);

    header.fields[name] = value;

    std::cout << "Field: " << name << " -> " << value << std::endl;

    p += field_len;
  }

  return header;
}

bool Embag::read_records() {
  const int64_t file_size = bag_stream_->size();

  while (true) {
    const auto pos = bag_stream_.tellg();
    if (pos == -1 || pos == file_size) {
      break;
    }
    const auto record = read_record();
    const auto header = read_header(record);

    const auto op = header.get_op();

    switch(op) {
      case header_t::op::BAG_HEADER:
        uint32_t connection_count;
        uint32_t chunk_count;
        uint64_t index_pos;

        header.get_field("conn_count", connection_count);
        header.get_field("chunk_count", chunk_count);
        header.get_field("index_pos", index_pos);

        std::cout << "conn: " << connection_count << " chunk: " << chunk_count << " index " << index_pos << std::endl;

        // TODO: check these values are nonzero and index_pos is > 64
        connections_.resize(connection_count);
        chunks_.reserve(chunk_count);
        index_pos_ = index_pos;

        break;
      case header_t::op::CHUNK:
        chunks_.emplace_back(record);
        break;
      case header_t::op::INDEX_DATA:
        break;
      case header_t::op::CONNECTION:
        break;
      case header_t::op::MESSAGE_DATA:
        break;
      case header_t::op::CHUNK_INFO:
        break;
      case header_t::op::UNSET:
      default:
        std::cout << "ERROR: Unknown record operation: " << uint8_t(op) << std::endl;
    }
  }

  return true;
}