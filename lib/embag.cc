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

bool Bag::open() {
  boost::iostreams::mapped_file_source mapped_file_source{filename_};
  bag_stream_.open(mapped_file_source);

  // First, check for the magic string indicating this is indeed a bag file
  std::string buffer(MAGIC_STRING.size(), 0);

  bag_stream_.read(&buffer[0], MAGIC_STRING.size());

  if (buffer != MAGIC_STRING) {
    bag_stream_.close();
    throw std::runtime_error("This file doesn't appear to be a bag file... ");
  }

  // Next, parse the version
  buffer.resize(3);
  bag_stream_.read(&buffer[0], 3);

  // Only version 2.0 is supported at the moment
  if ("2.0" != buffer) {
    bag_stream_.close();
    throw std::runtime_error("Unsupported bag file version: " + buffer);
  }

  // The version is followed by a newline
  buffer.resize(1);
  bag_stream_.read(&buffer[0], 1);
  if ("\n" != buffer) {
    throw std::runtime_error("Unable to find newline after version string, perhaps this bag file is corrupted?");
  }

  readRecords();

  return true;
}

bool Bag::close() {
  if (bag_stream_.is_open()) {
    bag_stream_.close();
    return true;
  }

  return false;
}

RosBagTypes::record_t Bag::readRecord() {
  RosBagTypes::record_t record{};
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

  return record;
}

std::unique_ptr<std::unordered_map<std::string, std::string>> Bag::readFields(const char *p, const uint64_t len) {
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

bool Bag::readRecords() {
  const int64_t file_size = bag_stream_->size();

  while (true) {
    const auto pos = bag_stream_.tellg();
    if (pos == -1 || pos == file_size) {
      break;
    }
    const auto record = readRecord();
    const auto header = readHeader(record);

    const auto op = header.getOp();

    switch (op) {
      case RosBagTypes::header_t::op::BAG_HEADER: {
        uint32_t connection_count;
        uint32_t chunk_count;
        uint64_t index_pos;

        header.getField("conn_count", connection_count);
        header.getField("chunk_count", chunk_count);
        header.getField("index_pos", index_pos);

        // TODO: check these values are nonzero and index_pos is > 64
        connections_.resize(connection_count);
        chunks_.reserve(chunk_count);
        index_pos_ = index_pos;

        break;
      }
      case RosBagTypes::header_t::op::CHUNK: {
        RosBagTypes::chunk_t chunk{record};
        chunk.offset = pos;

        header.getField("compression", chunk.compression);
        header.getField("size", chunk.uncompressed_size);

        if (!(chunk.compression == "lz4" || chunk.compression == "bz2" || chunk.compression == "none")) {
          throw std::runtime_error("Unsupported compression type: " + chunk.compression);
        }

        chunks_.emplace_back(chunk);

        break;
      }
      case RosBagTypes::header_t::op::INDEX_DATA: {
        uint32_t version;
        uint32_t connection_id;
        uint32_t msg_count;

        // TODO: check these values
        header.getField("ver", version);
        header.getField("conn", connection_id);
        header.getField("count", msg_count);

        RosBagTypes::index_block_t index_block{};
        index_block.into_chunk = &chunks_.back();

        connections_[connection_id].blocks.emplace_back(index_block);

        break;
      }
      case RosBagTypes::header_t::op::CONNECTION: {
        uint32_t connection_id;
        std::string topic;

        header.getField("conn", connection_id);
        header.getField("topic", topic);

        if (topic.empty())
          continue;

        // TODO: check these variables along with md5sum
        RosBagTypes::connection_data_t connection_data;
        connection_data.topic = topic;

        const auto fields = readFields(record.data, record.data_len);

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

        break;
      }
      case RosBagTypes::header_t::op::MESSAGE_DATA: {
        // Message data is usually found in chunks
        break;
      }
      case RosBagTypes::header_t::op::CHUNK_INFO: {
        uint32_t ver;
        uint64_t chunk_pos;
        RosValue::ros_time_t start_time;
        RosValue::ros_time_t end_time;
        uint32_t count;

        header.getField("ver", ver);
        header.getField("chunk_pos", chunk_pos);
        header.getField("start_time", start_time);
        header.getField("end_time", end_time);
        header.getField("count", count);

        // TODO: It might make sense to save this data in a map or reverse the search.
        // At the moment there are only a few chunks so this doesn't really take long
        auto chunk_it = std::find_if(chunks_.begin(), chunks_.end(), [&chunk_pos](const RosBagTypes::chunk_t &c) {
          return c.offset == chunk_pos;
        });

        if (chunk_it == chunks_.end()) {
          throw std::runtime_error("Unable to find chunk for chunk info at pos: " + std::to_string(chunk_pos));
        }

        chunk_it->info.start_time = start_time;
        chunk_it->info.end_time = end_time;
        chunk_it->info.message_count = count;

        break;
      }
      case RosBagTypes::header_t::op::UNSET:
      default:throw std::runtime_error("Unknown record operation: " + std::to_string(uint8_t(op)));
    }
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
