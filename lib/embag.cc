#define BOOST_SPIRIT_DEBUG
#define BOOST_SPIRIT_DEBUG_OUT std::cout

#include "embag.h"
#include "ros_msg.h"

#include <iostream>

// Boost Magic
#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

bool Embag::open() {
  boost::iostreams::mapped_file_source mapped_file_source(filename_);
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

Embag::record_t Embag::readRecord() {
  Embag::record_t record{};
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


std::map<std::string, std::string> readFields(const char* p, const uint64_t len) {
  std::map<std::string, std::string> fields;
  const char *end = p + len;

  while (p < end) {
    const uint32_t field_len = *(reinterpret_cast<const uint32_t *>(p));

    p += sizeof(uint32_t);

    // FIXME: these are copies...
    std::string buffer(p, field_len);
    const auto sep = buffer.find("=");

    // TODO: check if = is std::string::npos

    const auto name = buffer.substr(0, sep);
    const auto value = buffer.substr(sep + 1);

    fields[name] = value;

    p += field_len;
  }

  return fields;
}

Embag::header_t Embag::readHeader(const record_t &record) {
  header_t header;

  header.fields = readFields(record.header, record.header_len);

  return header;
}

// TODO: move this stuff elsewhere?
// Parser structures and binding
BOOST_FUSION_ADAPT_STRUCT(
  Embag::ros_msg_field,
  type_name,
  array_size,
  field_name,
)

BOOST_FUSION_ADAPT_STRUCT(
  Embag::ros_msg_constant,
  type_name,
  constant_name,
  value,
)

BOOST_FUSION_ADAPT_STRUCT(
  Embag::ros_embedded_msg_def,
  type_name,
  members,
)

BOOST_FUSION_ADAPT_STRUCT(
  Embag::ros_msg_def,
  members,
  embedded_types,
)

// TODO namespace this stuff
namespace qi = boost::spirit::qi;
// A parser for all the things we don't care about (aka a skipper)
template <typename Iterator>
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

    //BOOST_SPIRIT_DEBUG_NODE(skip);
    //BOOST_SPIRIT_DEBUG_NODE(newlines);
  }

  qi::rule<Iterator> skip;
  qi::rule<Iterator> comment;
  qi::rule<Iterator> separator;
  qi::rule<Iterator> blank_lines;
};

// ROS message parsing
// See http://wiki.ros.org/msg for details on the format
template <typename Iterator, typename Skipper = ros_msg_skipper<Iterator>>
struct ros_msg_grammar : qi::grammar<Iterator, Embag::ros_msg_def(), Skipper> {
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
    type %= (lit("std_msgs/") >> +(char_ - (lit('[')|space)) | +(char_ - (lit('[')|space)));
    field_name %= lexeme[+(char_ - (space|eol|'#'))];

    field = type >> array_size >> +lit(' ') >> field_name;

    // Parse a constant in the form: type constant_name=constant_value
    constant_name %= lexeme[+(char_ - (space|lit('=')))];
    constant_value %= lexeme[+(char_ - (space|eol|'#'))];
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

    //BOOST_SPIRIT_DEBUG_NODE(array_size);
    //BOOST_SPIRIT_DEBUG_NODE(type);
    //BOOST_SPIRIT_DEBUG_NODE(field);
    //BOOST_SPIRIT_DEBUG_NODE(constant);
    //BOOST_SPIRIT_DEBUG_NODE(field_name);
    //BOOST_SPIRIT_DEBUG_NODE(member);
    //BOOST_SPIRIT_DEBUG_NODE(embedded_type);
  }

  qi::rule<Iterator, Embag::ros_msg_def(), Skipper> msg;
  qi::rule<Iterator, Embag::ros_msg_field(), Skipper> field;
  qi::rule<Iterator, std::string(), Skipper> type;
  qi::rule<Iterator, int32_t(), Skipper> array_size;
  qi::rule<Iterator, std::string(), Skipper> field_name;
  qi::rule<Iterator, Embag::ros_embedded_msg_def(), Skipper> embedded_type;
  qi::rule<Iterator, std::string(), Skipper> embedded_type_name;
  qi::rule<Iterator, Embag::ros_msg_constant(), Skipper> constant;
  qi::rule<Iterator, std::string(), Skipper> constant_name;
  qi::rule<Iterator, std::string(), Skipper> constant_value;
  qi::rule<Iterator, Embag::ros_msg_member(), Skipper> member;
};


bool Embag::readRecords() {
  const int64_t file_size = bag_stream_->size();

  while (true) {
    const auto pos = bag_stream_.tellg();
    if (pos == -1 || pos == file_size) {
      break;
    }
    const auto record = readRecord();
    const auto header = readHeader(record);

    const auto op = header.getOp();

    switch(op) {
      case header_t::op::BAG_HEADER: {
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
      case header_t::op::CHUNK: {
        chunk_t chunk(record);
        chunk.offset = pos;

        header.getField("compression", chunk.compression);
        header.getField("size", chunk.uncompressed_size);

        chunks_.emplace_back(chunk);

        break;
      }
      case header_t::op::INDEX_DATA: {
        uint32_t version;
        uint32_t connection_id;
        uint32_t msg_count;

        // TODO: check these values
        header.getField("ver", version);
        header.getField("conn", connection_id);
        header.getField("count", msg_count);

        index_block_t index_block{};
        index_block.into_chunk = &chunks_.back();
        // TODO: set memory reference

        connections_[connection_id].blocks.emplace_back(index_block);

        break;
      }
      case header_t::op::CONNECTION: {
        uint32_t connection_id;
        std::string topic;

        header.getField("conn", connection_id);
        header.getField("topic", topic);

        if (topic.empty())
          continue;

        // TODO: check these variables along with md5sum
        connection_data_t connection_data;
        connection_data.topic = topic;

        const auto fields = readFields(record.data, record.data_len);

        connection_data.type = fields.at("type");
        const size_t slash_pos = connection_data.type.find_first_of('/');
        if (slash_pos != std::string::npos) {
          connection_data.scope = connection_data.type.substr(0, slash_pos);
        }
        connection_data.md5sum = fields.at("md5sum");
        connection_data.message_definition = fields.at("message_definition");
        if (fields.find("callerid") != fields.end()) {
          connection_data.callerid = fields.at("callerid");
        }
        if (fields.find("latching") != fields.end()) {
          connection_data.latching = fields.at("latching") == "1";
        }

        connections_[connection_id].topic = topic;
        connections_[connection_id].data = connection_data;

        // Parse message definition
        typedef ros_msg_grammar<std::string::const_iterator> ros_msg_grammar;
        typedef ros_msg_skipper<std::string::const_iterator> ros_msg_skipper;
        ros_msg_grammar grammar; // Our grammar
        ros_msg_def ast;         // Our tree
        ros_msg_skipper skipper; // Things we're not interested in

        std::string::const_iterator iter = connection_data.message_definition.begin();
        std::string::const_iterator end = connection_data.message_definition.end();
        const bool r = phrase_parse(iter, end, grammar, skipper, ast);

        if (r && iter == end) {
          message_schemata_[topic] = ast;
        } else {
          const std::string::const_iterator some = iter + std::min(30, int(end - iter));
          const std::string context(iter, (some>end)?end:some);

          throw std::runtime_error("Message definition parsing failed at: " + context);
        }

        break;
      }
      case header_t::op::MESSAGE_DATA: {
        // Message data is usually found in chunks
        break;
      }
      case header_t::op::CHUNK_INFO: {
        uint32_t ver;
        uint64_t chunk_pos;
        uint64_t start_time;
        uint64_t end_time;
        uint32_t count;

        header.getField("ver", ver);
        header.getField("chunk_pos", chunk_pos);
        header.getField("start_time", start_time);
        header.getField("end_time", end_time);
        header.getField("count", count);

        // TODO: It might make sense to save this data in a map or reverse the search.
        // At the moment there are only a few chunks so this doesn't really take long
        auto chunk_it = std::find_if(chunks_.begin(), chunks_.end(), [&chunk_pos] (const chunk_t& c) {
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
      case header_t::op::UNSET:
      default:
        throw std::runtime_error( "Unknown record operation: " + std::to_string(uint8_t(op)));
    }
  }

  return true;
}

bool Embag::decompressLz4Chunk(const char *src, const size_t src_size, char *dst, const size_t dst_size) {
  size_t src_bytes_left = src_size;
  size_t dst_bytes_left = dst_size;

  while (dst_bytes_left && src_bytes_left) {
    size_t src_bytes_read = src_bytes_left;
    size_t dst_bytes_written = dst_bytes_left;
    const size_t ret = LZ4F_decompress(lz4_ctx_, dst, &dst_bytes_written, src, &src_bytes_read, nullptr);
    if (LZ4F_isError(ret)) {
      throw std::runtime_error("chunk::decompress: lz4 decompression returned " + std::to_string(ret) + ", expected " + std::to_string(src_bytes_read));
    }

    src_bytes_left -= src_bytes_read;
    dst_bytes_left -= dst_bytes_written;
  }

  if (src_bytes_left || dst_bytes_left) {
    throw std::runtime_error("chunk::decompress: lz4 decompression left " + std::to_string(src_bytes_left) + "/" + std::to_string(dst_bytes_left) + " bytes in buffer");
  }

  return true;
}

void Embag::topics() {

}

void Embag::pringAllMsgs() {
  std::cout << "Printing " << chunks_.size() << " chunks..." << std::endl;

  for (const auto &chunk : chunks_) {
    std::cout << "Offset: " << chunk.offset << std::endl;
    std::cout << "  Compression: " << chunk.compression << std::endl;
    std::cout << "  Compressed size: " << chunk.record.data_len << std::endl;
    std::cout << "  Uncompressed size: " << chunk.uncompressed_size << std::endl;
    std::cout << "  Duration: " << chunk.info.start_time  << " - " << chunk.info.end_time << std::endl;
    std::cout << "  Messages: " << chunk.info.message_count << std::endl;

    std::string buffer(chunk.uncompressed_size, 0);
    decompressLz4Chunk(chunk.record.data, chunk.record.data_len, &buffer[0], chunk.uncompressed_size);

    size_t processed_bytes = 0;
    while (processed_bytes < chunk.uncompressed_size) {
      const size_t offset = processed_bytes;

      Embag::record_t record{};
      size_t p = sizeof(record.header_len);
      std::memcpy(&record.header_len, buffer.c_str() + offset, p);
      record.header = &buffer.c_str()[offset + p];
      p += record.header_len;
      std::memcpy(&record.data_len, buffer.c_str() + offset + p, sizeof(record.data_len));
      p += sizeof(record.data_len);
      record.data = &buffer.c_str()[offset + p];
      p += record.data_len;
      const auto header = readHeader(record);

      const auto op = header.getOp();
      switch (op) {
        case header_t::op::MESSAGE_DATA: {
          uint32_t connection_id;
          header.getField("conn", connection_id);
          const std::string &topic = connections_[connection_id].topic;

          std::cout << "Message on " << topic << std::endl;

          RosValue message = parseMessage(connection_id, record);
          printMsg(message);

          std::cout << "----------------------------" << std::endl;
          break;
        }
        case header_t::op::CONNECTION: {
          uint32_t connection_id;
          header.getField("conn", connection_id);
          break;
        }
        default: {
          throw std::runtime_error("Found unknown record type: " + std::to_string(static_cast<int>(op)));
        }
      }

      processed_bytes += p;
    }
  }
}

void Embag::printMsg(const RosValue &field, const std::string &path) {
  switch (field.type) {
    case RosValue::Type::ros_bool: {
      std::cout << path << " -> " << (field.bool_value ? "true" : "false") << std::endl;
      break;
    }
    case RosValue::Type::int8: {
      std::cout << path << " -> " << +field.int8_value << std::endl;
      break;
    }
    case RosValue::Type::uint8: {
      std::cout << path << " -> " << +field.uint8_value << std::endl;
      break;
    }
    case RosValue::Type::int16: {
      std::cout << path << " -> " << +field.int16_value << std::endl;
      break;
    }
    case RosValue::Type::uint16: {
      std::cout << path << " -> " << +field.uint16_value << std::endl;
      break;
    }
    case RosValue::Type::int32: {
      std::cout << path << " -> " << +field.int32_value << std::endl;
      break;
    }
    case RosValue::Type::uint32: {
      std::cout << path << " -> " << +field.uint32_value << std::endl;
      break;
    }
    case RosValue::Type::int64: {
      std::cout << path << " -> " << +field.int64_value << std::endl;
      break;
    }
    case RosValue::Type::uint64: {
      std::cout << path << " -> " << +field.uint64_value << std::endl;
      break;
    }
    case RosValue::Type::float32: {
      std::cout << path << " -> " << +field.float32_value << std::endl;
      break;
    }
    case RosValue::Type::float64: {
      std::cout << path << " -> " << +field.float64_value << std::endl;
      break;
    }
    case RosValue::Type::string: {
      std::cout << path << " -> " << field.string_value << std::endl;
      break;
    }
    case RosValue::Type::ros_time: {
      std::cout << path << " -> " << field.time_value.secs <<  "s " << field.time_value.nsecs << "ns" << std::endl;
      break;
    }
    case RosValue::Type::ros_duration: {
      std::cout << path << " -> " << field.duration_value.secs <<  "s " << field.duration_value.nsecs << "ns" << std::endl;
      break;
    }
    case RosValue::Type::object: {
      for (const auto &object : field.objects) {
        if (path.empty()) {
          printMsg(object.second, object.first);
        } else {
          printMsg(object.second, path + "." + object.first);
        }
      }
      break;
    }
    case RosValue::Type::array: {
      size_t i = 0;
      for (const auto &item : field.values) {
        const std::string array_path = path + "[" + std::to_string(i) + "]";
        printMsg(item, array_path);
        i++;
      }
      break;
    }
  }
}


RosValue Embag::parseMessage(const uint32_t connection_id, record_t message) {
  const auto &connection = connections_[connection_id];
  const auto &msg_def = message_schemata_[connection.topic];

  // TODO: streaming this data means copying it into basic types.  It would be faster to just set pointers appropriately...
  message_stream stream(message.data, message.data_len);

  RosMsg msg(stream, connection.data, msg_def);

  RosValue parsed_message = msg.parse();

  return parsed_message;
}
