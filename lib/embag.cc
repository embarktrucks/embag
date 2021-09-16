#include <iostream>

#include "embag.h"
#include "util.h"
#include "message_def_parser.h"

namespace Embag {

RosMsgTypes::primitive_type_map_t RosMsgTypes::ros_msg_field::primitive_type_map_ = {
    {"bool", RosValue::Type::ros_bool},
    {"int8", RosValue::Type::int8},
    {"uint8", RosValue::Type::uint8},
    {"int16", RosValue::Type::int16},
    {"uint16", RosValue::Type::uint16},
    {"uint8", RosValue::Type::uint8},
    {"int32", RosValue::Type::int32},
    {"uint32", RosValue::Type::uint32},
    {"int64", RosValue::Type::int64},
    {"uint64", RosValue::Type::uint64},
    {"float32", RosValue::Type::float32},
    {"float64", RosValue::Type::float64},
    {"string", RosValue::Type::string},
    {"time", RosValue::Type::ros_time},
    {"duration", RosValue::Type::ros_duration},

    // Deprecated types
    {"byte", RosValue::Type::int8},
    {"char", RosValue::Type::uint8},
};

void Bag::BagFromFile::open(const std::string &path) {
  boost::iostreams::mapped_file_source mapped_file_source{path};
  bag_stream_.open(mapped_file_source);

  bag_->readStream(bag_stream_, bag_stream_->data(), bag_stream_->size());
}

void Bag::BagFromFile::close() {
  if (bag_stream_.is_open()) {
    bag_stream_.close();
  }
}

void Bag::BagFromBytes::open(const char *bytes, size_t length) {
  boost::iostreams::array_source array_source{bytes, length};
  bag_stream_.open(array_source);

  bag_->readStream(bag_stream_, bytes, length);
}

void Bag::BagFromBytes::close() {
  if (bag_stream_.is_open()) {
    bag_stream_.close();
  }
}

template <typename T>
bool Bag::readStream(boost::iostreams::stream<T> &stream, const char* buffer, size_t buffer_size) {
  bag_bytes_ = const_cast<char *>(buffer);
  bag_bytes_size_ = buffer_size;

  // First, check for the magic string indicating this is indeed a bag file
  std::string temp(MAGIC_STRING.size(), 0);

  stream.read(&temp[0], MAGIC_STRING.size());

  if (temp != MAGIC_STRING) {
    throw std::runtime_error("This file doesn't appear to be a bag file...");
  }

  // Next, parse the version
  temp.resize(3);
  stream.read(&temp[0], 3);

  // Only version 2.0 is supported at the moment
  if ("2.0" != temp) {
    throw std::runtime_error("Unsupported bag file version: " + temp);
  }

  // The version is followed by a newline
  temp.resize(1);
  stream.read(&temp[0], 1);
  if ("\n" != temp) {
    throw std::runtime_error("Unable to find newline after version string, perhaps this bag file is corrupted?");
  }

  readRecords(stream);

  return true;
}

template <typename T>
RosBagTypes::record_t Bag::readRecord(boost::iostreams::stream<T> &stream) {
  RosBagTypes::record_t record{};
  // Read header length
  stream.read(reinterpret_cast<char *>(&record.header_len), sizeof(record.header_len));

  // Set the header data pointer and skip to the next section
  record.header = bag_bytes_ + stream.tellg();
  stream.seekg(record.header_len, std::ios_base::cur);

  // Read data length
  stream.read(reinterpret_cast<char *>(&record.data_len), sizeof(record.data_len));

  // Set the data pointer and skip to the next record
  record.data = bag_bytes_ + stream.tellg();
  stream.seekg(record.data_len, std::ios_base::cur);

  return record;
}

std::unique_ptr<std::unordered_map<std::string, std::string>> Bag::readFields(const char *p, uint64_t len) {
  auto fields = make_unique<std::unordered_map<std::string, std::string>>();
  const char *end = p + len;

  while (p < end) {
    const uint32_t field_len = *(reinterpret_cast<const uint32_t *>(p));

    p += sizeof(uint32_t);

    // FIXME: these are copies...
    std::string buffer(p, field_len);
    const auto sep = buffer.find('=');

    if (sep == std::string::npos) {
      throw std::runtime_error("Unable to find '=' in header field - perhaps this bag is corrupt...");
    }

    const auto name = buffer.substr(0, sep);
    auto value = buffer.substr(sep + 1);

    (*fields)[name] = std::move(value);

    p += field_len;
  }

  return fields;
}

RosBagTypes::header_t Bag::readHeader(const RosBagTypes::record_t &record) {
  RosBagTypes::header_t header;

  header.fields = readFields(record.header, record.header_len);

  return header;
}

template <typename T>
bool Bag::readRecords(boost::iostreams::stream<T> &stream) {
  // Docs on the RosBag 2.0 format: http://wiki.ros.org/Bags/Format/2.0

  /**
   * Read the BAG_HEADER record. So long as the file is not corrupted, this is guaranteed
   * to be the first record in the bag
   */
  const auto pos = stream.tellg();
  uint32_t connection_count;
  uint32_t chunk_count;
  uint64_t index_pos;

  const auto bag_header_record = readRecord(stream);
  const auto bag_header_header = readHeader(bag_header_record);

  bag_header_header.getField("conn_count", connection_count);
  bag_header_header.getField("chunk_count", chunk_count);
  bag_header_header.getField("index_pos", index_pos);

  // TODO: check these values are nonzero and index_pos is > 64
  connections_.resize(connection_count);
  chunk_infos_.reserve(chunk_count);
  chunks_.reserve(chunk_count);
  index_pos_ = index_pos;

  /**
   * Read the CONNECTION records. As per the docs, these are located immediately after the
   * CHUNK section of the rosbag. The location is denoted by the index_pos field of the bag header
   */
  stream.seekg(index_pos_, std::ios_base::beg);
  for (size_t i = 0; i < connection_count; i++) {
    const auto conn_record = readRecord(stream);
    const auto conn_header = readHeader(conn_record);

    uint32_t connection_id;
    std::string topic;

    conn_header.getField("conn", connection_id);
    conn_header.getField("topic", topic);

    if (topic.empty())
      continue;

    // TODO: check these variables along with md5sum
    RosBagTypes::connection_data_t connection_data;
    connection_data.topic = topic;

    const auto fields = readFields(conn_record.data, conn_record.data_len);

    connection_data.type = fields->at("type");
    const size_t slash_pos = connection_data.type.find_first_of('/');
    if (slash_pos != std::string::npos) {
      connection_data.scope = connection_data.type.substr(0, slash_pos);
    }
    connection_data.md5sum = fields->at("md5sum");
    connection_data.message_definition = fields->at("message_definition");
    if (fields->find("callerid") != fields->end()) {
      connection_data.callerid = fields->at("callerid");
    }
    if (fields->find("latching") != fields->end()) {
      connection_data.latching = fields->at("latching") == "1";
    }

    connections_[connection_id].id = connection_id;
    connections_[connection_id].topic = topic;
    connections_[connection_id].data = connection_data;
    topic_connection_map_[topic].emplace_back(&connections_[connection_id]);
  }

  /**
   * Read the CHUNK_INFO records. These are guaranteed to be immediately after the CONNECTION records,
   * so no need to seek the file pointer
   */
  for (size_t i = 0; i < chunk_count; i++) {
    const auto chunk_info_record = readRecord(stream);
    const auto chunk_info_header = readHeader(chunk_info_record);

    RosBagTypes::chunk_info_t chunk_info;

    uint32_t ver;
    uint64_t chunk_pos;
    RosValue::ros_time_t start_time;
    RosValue::ros_time_t end_time;
    uint32_t count;

    chunk_info_header.getField("ver", ver);
    chunk_info_header.getField("chunk_pos", chunk_pos);
    chunk_info_header.getField("start_time", start_time);
    chunk_info_header.getField("end_time", end_time);
    chunk_info_header.getField("count", count);

    chunk_info.chunk_pos = chunk_pos;
    chunk_info.start_time = start_time;
    chunk_info.end_time = end_time;
    chunk_info.message_count = count;

    chunk_infos_[i] = chunk_info;
  }

  /**
   * Now that we have some chunk metadata from the CHUNK_INFO records, process the CHUNK records from
   * earlier in the file. Each CHUNK_INFO knows the position of its corresponding chunk.
   */
  for (size_t i = 0; i < chunk_count; i++) {
    const auto info = chunk_infos_[i];

    // TODO: The chunk infos are not necessarily Revisit this logic if seeking back and forth across the file causes a slowdown
    stream.seekg(info.chunk_pos, std::ios_base::beg);

    const auto chunk_record = readRecord(stream);
    const auto chunk_header = readHeader(chunk_record);

    RosBagTypes::chunk_t chunk{chunk_record};
    chunk.offset = stream.tellg();

    chunk_header.getField("compression", chunk.compression);
    chunk_header.getField("size", chunk.uncompressed_size);

    if (!(chunk.compression == "lz4" || chunk.compression == "bz2" || chunk.compression == "none")) {
      throw std::runtime_error("Unsupported compression type: " + chunk.compression);
    }

    chunk.info = info;

    chunks_.emplace_back(chunk);

    // Each chunk is followed by an INDEX_DATA record, so parse that out here
    const auto index_data_record = readRecord(stream);
    const auto index_data_header = readHeader(index_data_record);

    uint32_t version;
    uint32_t connection_id;
    uint32_t msg_count;

    // TODO: check these values
    index_data_header.getField("ver", version);
    index_data_header.getField("conn", connection_id);
    index_data_header.getField("count", msg_count);

    RosBagTypes::index_block_t index_block{};

    // NOTE: It seems like it would be simpler to just do &chunk here right? WRONG.
    //       C++ resuses the same memory location for the chunk variable for each loop, so
    //       if you use &chunk, all `into_chunk` values will be exactly the same
    index_block.into_chunk = &chunks_[i];

    connections_[connection_id].blocks.emplace_back(index_block);
  }

  return true;
}

void Bag::parseMsgDefForTopic(const std::string &topic) {
  const auto it = topic_connection_map_.find(topic);
  if (it == topic_connection_map_.end()) {
    throw std::runtime_error("Unable to find topic in bag: " + topic);
  }

  const auto connections = it->second;
  if (connections.empty()) {
    throw std::runtime_error("No connection data for topic: " + topic);
  }

  const auto connection_data = connections.front()->data;
  message_schemata_[topic] = parseMsgDef(connection_data.message_definition);
}
}
