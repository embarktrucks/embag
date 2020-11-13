#include <iostream>

#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include "embag.h"
#include "util.h"

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
}

// TODO: move this stuff elsewhere?
// Parser structures and binding - must exist in global namespace
BOOST_FUSION_ADAPT_STRUCT(
    Embag::RosMsgTypes::ros_msg_field,
    type_name,
    array_size,
    field_name,
)

BOOST_FUSION_ADAPT_STRUCT(
    Embag::RosMsgTypes::ros_msg_constant,
    type_name,
    constant_name,
    value,
)

BOOST_FUSION_ADAPT_STRUCT(
    Embag::RosMsgTypes::ros_embedded_msg_def,
    type_name,
    members,
)

BOOST_FUSION_ADAPT_STRUCT(
    Embag::RosMsgTypes::ros_msg_def,
    members,
    embedded_types,
)

namespace Embag {

// TODO namespace this stuff
namespace qi = boost::spirit::qi;
// A parser for all the things we don't care about (aka a skipper)
template<typename Iterator>
struct ros_msg_skipper : qi::grammar<Iterator> {
  ros_msg_skipper() : ros_msg_skipper::base_type(skip) {
    using boost::spirit::ascii::char_;
    using boost::spirit::eol;
    using qi::repeat;
    using qi::lit;
    using boost::spirit::ascii::space;

    comment = *space >> "#" >> *(char_ - eol);
    separator = repeat(80)['='] - eol;
    blank_lines = *lit(' ') >> eol;

    skip = comment | separator | eol | blank_lines;
  }

  qi::rule<Iterator> skip;
  qi::rule<Iterator> comment;
  qi::rule<Iterator> separator;
  qi::rule<Iterator> blank_lines;
};

// ROS message parsing
// See http://wiki.ros.org/msg for details on the format
template<typename Iterator, typename Skipper = ros_msg_skipper<Iterator>>
struct ros_msg_grammar : qi::grammar<Iterator, RosMsgTypes::ros_msg_def(), Skipper> {
  ros_msg_grammar() : ros_msg_grammar::base_type(msg) {
    // TODO clean these up
    using qi::lit;
    using qi::lexeme;
    using boost::spirit::ascii::char_;
    using boost::spirit::ascii::space;
    using boost::spirit::qi::uint_;
    using boost::spirit::eol;
    using boost::spirit::eps;
    using boost::spirit::attr;

    // Parse a message field in the form: type field_name
    // This handles array types as well, for example type[n] field_name
    array_size %= ('[' >> (uint_ | attr(-1)) >> ']') | attr(0);
    type %= (lit("std_msgs/") >> +(char_ - (lit('[') | space)) | +(char_ - (lit('[') | space)));
    field_name %= lexeme[+(char_ - (space | eol | '#'))];

    field = type >> array_size >> +lit(' ') >> field_name;

    // Parse a constant in the form: type constant_name=constant_value
    constant_name %= lexeme[+(char_ - (space | lit('=')))];
    constant_value %= lexeme[+(char_ - (space | eol | '#'))];
    constant = type >> +lit(' ') >> constant_name >> *lit(' ') >> lit('=') >> *lit(' ') >> constant_value;

    // Each line of a message definition can be a constant or a field declaration
    member = constant | field;

    // Embedded types include all the supporting sub types (aka non-primitives) of a top-level message definition
    // The std_msgs namespace is included in the global namespace so we remove it here
    embedded_type_name %= (lit("MSG: std_msgs/") | lit("MSG: ")) >> lexeme[+(char_ - eol)];
    embedded_type = embedded_type_name >> +(member - lit("MSG: "));

    // Finally, we put these rules together to parse the full definition
    msg = *(member - lit("MSG: "))
        >> *embedded_type;
  }

  qi::rule<Iterator, RosMsgTypes::ros_msg_def(), Skipper> msg;
  qi::rule<Iterator, RosMsgTypes::ros_msg_field(), Skipper> field;
  qi::rule<Iterator, std::string(), Skipper> type;
  qi::rule<Iterator, int32_t(), Skipper> array_size;
  qi::rule<Iterator, std::string(), Skipper> field_name;
  qi::rule<Iterator, RosMsgTypes::ros_embedded_msg_def(), Skipper> embedded_type;
  qi::rule<Iterator, std::string(), Skipper> embedded_type_name;
  qi::rule<Iterator, RosMsgTypes::ros_msg_constant(), Skipper> constant;
  qi::rule<Iterator, std::string(), Skipper> constant_name;
  qi::rule<Iterator, std::string(), Skipper> constant_value;
  qi::rule<Iterator, RosMsgTypes::ros_msg_member(), Skipper> member;
};

bool Bag::readRecords() {
  const int64_t file_size = bag_stream_->size();

  ros_msg_grammar<std::string::const_iterator> grammar;
  ros_msg_skipper<std::string::const_iterator> skipper;

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

        // Parse message definition
        auto ast = std::make_shared<RosMsgTypes::ros_msg_def>();

        std::string::const_iterator iter = connection_data.message_definition.begin();
        std::string::const_iterator end = connection_data.message_definition.end();
        const bool r = phrase_parse(iter, end, grammar, skipper, *ast);

        if (r && iter == end) {
          message_schemata_[topic] = std::move(ast);
        } else {
          const std::string::const_iterator some = iter + std::min(30, int(end - iter));
          const std::string context(iter, (some > end) ? end : some);

          throw std::runtime_error("Message definition parsing failed at: " + context);
        }

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
}
