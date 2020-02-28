#define BOOST_SPIRIT_DEBUG
#define BOOST_SPIRIT_DEBUG_OUT std::cout
#include "embag.h"

#include <iostream>

// Boost Magic
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/foreach.hpp>

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

Embag::record_t Embag::readRecord() {
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

  //std::cout << "Record with head len " << record.header_len << " data len " << record.data_len << std::endl;

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

    //std::cout << "Field: " << name << " -> " << value << std::endl;

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
    type %= +(char_ - (lit('[')|space));
    field_name %= lexeme[+(char_ - (space|eol|'#'))];

    field = type >> array_size >> +lit(' ') >> field_name;

    // Parse a constant in the form: type constant_name=constant_value
    constant_name %= lexeme[+(char_ - (space|lit('=')))];
    constant_value %= lexeme[+(char_ - (space|eol|'#'))];
    constant = type >> +lit(' ') >> constant_name >> *lit(' ') >> lit('=') >> *lit(' ') >> constant_value;

    // Each line of a message definition can be a constant or a field declaration
    member = constant | field;

    // Embedded types include all the supporting sub types (aka non-primitives) of a top-level message definition
    embedded_type_name %= lit("MSG: ") >> lexeme[+(char_ - eol)];
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

        //std::cout << "conn: " << connection_count << " chunk: " << chunk_count << " index " << index_pos << std::endl;

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

        index_block_t index_block;
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
        connection_data.md5sum = fields.at("md5sum");
        connection_data.message_definition = fields.at("message_definition");
        //std::cout << "Msg def for topic " << connection_data.topic << ":\n" << connection_data.message_definition << std::endl;
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
          //std::cout << "-------------------------\n";
          //std::cout << "Parsing succeeded\n";
          //std::cout << "name: " << ast.fields[0].field_name << std::endl;
          //std::cout << "type: " << ast.fields[0].type_name << std::endl;
          //std::cout << "embedded: " << ast.embedded_types[0].type_name << std::endl;
          //std::cout << "embedded field: " << ast.embedded_types[0].fields[0].field_name << std::endl;
          //std::cout << "-------------------------\n";
          message_schemata_[topic] = ast;
        } else {
          std::string::const_iterator some = iter + std::min(30, int(end - iter));
          std::string context(iter, (some>end)?end:some);
          std::cout << "-------------------------\n";
          std::cout << "Parsing failed\n";
          std::cout << "stopped at: \"" << context << "...\"\n";
          std::cout << "-------------------------\n";
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
        auto chunk_it = std::find_if(chunks_.begin(), chunks_.end(), [this, &chunk_pos] (const chunk_t& c) {
          return c.offset == chunk_pos;
        });

        if (chunk_it == chunks_.end()) {
          std::cout << "ERROR: unable to find chunk for chunk info at pos: " << chunk_pos << std::endl;
          return false;
        }

        chunk_it->info.start_time = start_time;
        chunk_it->info.end_time = end_time;
        chunk_it->info.message_count = count;

        break;
      }
      case header_t::op::UNSET:
      default:
        std::cout << "ERROR: Unknown record operation: " << uint8_t(op) << std::endl;
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
      std::cout << "chunk::decompress: lz4 decompression returned " << ret << ", expected " << src_bytes_read << std::endl;
      return false;
    }

    src_bytes_left -= src_bytes_read;
    dst_bytes_left -= dst_bytes_written;
  }

  if (src_bytes_left || dst_bytes_left) {
    std::cout << "chunk::decompress: lz4 decompression left " << src_bytes_left << "/" << dst_bytes_left << " bytes in buffer" << std::endl;
    return false;
  }

  return true;
}

void Embag::printMsgs() {
  std::cout << "Printing " << chunks_.size() << " chunks..." << std::endl;

  for (const auto chunk : chunks_) {
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

      Embag::record_t record;
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
          std::cout << "Found message data conn: " << connection_id << " topic: " << topic << std::endl;

          parseMessage(connection_id, record);
          break;
        }
        case header_t::op::CONNECTION: {
          uint32_t connection_id;
          header.getField("conn", connection_id);
          //std::cout << "Found connection data conn id: " << connection_id << std::endl;
          break;
        }
        default: {
          std::cout << "Found unknown record type: " << static_cast<int>(op) << std::endl;
        }
      }

      processed_bytes += p;
    }
  }
}

struct member_visitor : boost::static_visitor<boost::optional<Embag::ros_msg_field>> {
  boost::optional<Embag::ros_msg_field> operator()(Embag::ros_msg_field const& field) const {
    return field;
  }

  boost::optional<Embag::ros_msg_field> operator()(Embag::ros_msg_constant const& constant) const {
    // TODO: handle constants
    return boost::none;
  }
};


void Embag::parseMessage(const uint32_t connection_id, record_t message) {
  const auto &connection_data = connections_[connection_id].data;
  const auto &msg_def = message_schemata_[connection_data.topic];

  // TODO: streaming this data means copying it into basic types.  It would be faster to just set pointers appropriately...
  message_stream stream(message.data, message.data_len);

  for (const auto &member : msg_def.members) {
    const auto field = boost::apply_visitor(member_visitor(), member);
    if (field) {
      parseField(msg_def, *field, stream);
    }
  }
}

struct ros_time {
  uint32_t secs;
  uint32_t nsecs;
};

struct thing {
  uint32_t uint32_value;
  std::string string_value;
  ros_time time_value;
  double float64_value;
};

void Embag::parseField(const ros_msg_def &msg_def, const ros_msg_field &field, message_stream &stream) {
  /* Unlimited number of array objects */
  if (field.array_size == -1) {
    uint32_t array_len;
    stream.read(reinterpret_cast<char *>(&array_len), sizeof(array_len));
    std::cout << "Found array len: " << array_len << std::endl;

    if (primitive_type_map_.find(field.type_name) != primitive_type_map_.end()) {
      std::cout << "Found primitive field: " << field.field_name << " type: " << field.type_name << std::endl;
      getPrimitiveField(field, stream);
    } else {
      // Search the embedded types for this type
      // TODO: optimize this
      bool found = false;
      ros_embedded_msg_def found_embedded_type;
      for (const auto &embedded_type : msg_def.embedded_types) {
        if (embedded_type.type_name == field.type_name) {
          found = true;
          found_embedded_type = embedded_type;
          std::cout << "Found embedded type " << field.type_name << std::endl;
          break;
        }
      }
      if (!found) {
        std::cout << "Unable to find embedded type " << field.type_name << std::endl;
        return;
      }

      // We have the array size and type
      for (size_t i = 0; i < array_len; i++) {
        for (const auto &member : found_embedded_type.members) {
          const auto embedded_field = boost::apply_visitor(member_visitor(), member);
          if (embedded_field) {
            parseField(msg_def, *embedded_field, stream);
          }
        }
      }
    }
  } else if (field.array_size == 0) {
    if (primitive_type_map_.find(field.type_name) != primitive_type_map_.end()) {
      std::cout << "Found primitive field: " << field.field_name << " type: " << field.type_name << std::endl;
      getPrimitiveField(field, stream);
    }
  }
}

void Embag::getPrimitiveField(const ros_msg_field& field, message_stream &stream) {
  PRIMITIVE_TYPE type = primitive_type_map_.at(field.type_name);
  thing value;
  switch (type) {
    case uint32: {
      stream.read(reinterpret_cast<char *>(&value.uint32_value), sizeof(type));
      std::cout << "Read uint32: " << value.uint32_value << std::endl;
      break;
    }
    case string: {
      uint32_t string_len;
      stream.read(reinterpret_cast<char *>(&string_len), sizeof(string_len));
      value.string_value = std::string(string_len, 0);
      stream.read(&value.string_value[0], string_len);
      std::cout << "Read string: " << value.string_value << std::endl;
      break;
    }
    case time: {
      stream.read(reinterpret_cast<char *>(&value.time_value.secs), sizeof(value.time_value.secs));
      stream.read(reinterpret_cast<char *>(&value.time_value.nsecs), sizeof(value.time_value.nsecs));
      std::cout << "Read time: " << value.time_value.secs << "s + " << value.time_value.nsecs << std::endl;
      break;
    }
    case float64: {
      stream.read(reinterpret_cast<char *>(&value.float64_value), sizeof(value.float64_value));
      std::cout << "Read float64: " << value.float64_value << std::endl;
      break;
    }
  }
}
