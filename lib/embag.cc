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

  //std::cout << "Record with head len " << record.header_len << " data len " << record.data_len << std::endl;

  return record;
}

std::map<std::string, std::string> read_fields(const char* p, const uint64_t len) {
  std::map<std::string, std::string> fields;
  const char *end = p + len;

  while (p < end) {
    const uint32_t field_len = *(reinterpret_cast<const uint32_t *>(p));

    p += sizeof(uint32_t);

    // FIXME: these are copies...
    std::string buffer(p, field_len);
    const auto sep = buffer.find("=");

    const auto name = buffer.substr(0, sep);
    const auto value = buffer.substr(sep + 1);

    fields[name] = value;

    //std::cout << "Field: " << name << " -> " << value << std::endl;

    p += field_len;
  }

  return fields;
}

Embag::header_t Embag::read_header(const record_t &record) {
  header_t header;

  header.fields = read_fields(record.header, record.header_len);

  return header;
}

// TODO: move this stuff elsewhere?
// Parser structures and binding
struct ros_msg_field {
  std::string type_name;
  std::string field_name;
};

struct ros_embedded_msg_def {
  std::string type_name;
  std::vector<ros_msg_field> fields;
};

struct ros_msg_def {
  std::vector<ros_msg_field> fields;
  std::vector<ros_embedded_msg_def> embedded_types;
};

BOOST_FUSION_ADAPT_STRUCT(
  ros_msg_def,
  (std::vector<ros_msg_field>, fields)
  (std::vector<ros_embedded_msg_def>, embedded_types)
)

BOOST_FUSION_ADAPT_STRUCT(
  ros_msg_field,
  (std::string, type_name)
  (std::string, field_name)
)

BOOST_FUSION_ADAPT_STRUCT(
  ros_embedded_msg_def,
  (std::string, type_name)
  (std::vector<ros_msg_field>, fields)
)

// Parser
template <typename Iterator>
struct ros_msg_skipper : boost::spirit::qi::grammar<Iterator> {
  ros_msg_skipper() : ros_msg_skipper::base_type(skip) {
    using boost::spirit::ascii::char_;
    using boost::spirit::eol;
    using boost::spirit::eoi;
    using boost::spirit::qi::repeat;

    comment = '#' >> *(char_ - eol) >> (eol|eoi);
    separator = repeat(80)['='] >> '\n';

    skip = comment | (eol >> eol) | separator;
  }

  boost::spirit::qi::rule<Iterator> skip;
  boost::spirit::qi::rule<Iterator> comment;
  boost::spirit::qi::rule<Iterator> separator;
};

template <typename Iterator, typename Skipper = ros_msg_skipper<Iterator>>
struct ros_msg_grammar : boost::spirit::qi::grammar<Iterator, ros_msg_def(), boost::spirit::qi::locals<std::string>, Skipper> {
  ros_msg_grammar() : ros_msg_grammar::base_type(msg) {
    using boost::spirit::qi::lit;
    using boost::spirit::qi::lexeme;
    using boost::spirit::ascii::char_;
    using boost::spirit::ascii::space;
    using boost::spirit::ascii::string;
    using boost::spirit::eol;
    using boost::spirit::eoi;
    using namespace boost::spirit::qi::labels;

    field_type %= lexeme[+(char_ - space)];
    field_name %= lexeme[+(char_ - '\n')];

    field = field_type >> -(space >> field_name);

    embedded_type_name %= lit("MSG: ") >> lexeme[(char_ - '\n')];

    embedded_type = embedded_type_name >> field >> *(lit('\n') >> field);

    msg =
        field >> *(lit('\n') >> field)
        >> *(embedded_type);

    BOOST_SPIRIT_DEBUG_NODE(field);
    BOOST_SPIRIT_DEBUG_NODE(field_type);
    BOOST_SPIRIT_DEBUG_NODE(field_name);
  }

  boost::spirit::qi::rule<Iterator, ros_msg_def(), boost::spirit::qi::locals<std::string>, Skipper> msg;
  boost::spirit::qi::rule<Iterator, ros_msg_field(), Skipper> field;
  boost::spirit::qi::rule<Iterator, std::string(), Skipper> field_type;
  boost::spirit::qi::rule<Iterator, std::string(), Skipper> field_name;
  boost::spirit::qi::rule<Iterator, ros_embedded_msg_def(), Skipper> embedded_type;
  boost::spirit::qi::rule<Iterator, std::string(), Skipper> embedded_type_name;
};


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
      case header_t::op::BAG_HEADER: {
        uint32_t connection_count;
        uint32_t chunk_count;
        uint64_t index_pos;

        header.get_field("conn_count", connection_count);
        header.get_field("chunk_count", chunk_count);
        header.get_field("index_pos", index_pos);

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

        header.get_field("compression", chunk.compression);
        header.get_field("size", chunk.size);

        chunks_.emplace_back(chunk);

        break;
      }
      case header_t::op::INDEX_DATA: {
        uint32_t version;
        uint32_t connection_id;
        uint32_t msg_count;

        // TODO: check these values
        header.get_field("ver", version);
        header.get_field("conn", connection_id);
        header.get_field("count", msg_count);

        index_block_t index_block;
        index_block.into_chunk = &chunks_.back();
        // TODO: set memory reference

        connections_[connection_id].blocks.emplace_back(index_block);

        break;
      }
      case header_t::op::CONNECTION: {
        uint32_t connection_id;
        std::string topic;

        header.get_field("conn", connection_id);
        header.get_field("topic", topic);

        // TODO: check these variables along with md5sum
        connection_data_t connection_data;
        connection_data.topic = topic;
        if (!(connection_id > 0 && topic.size() > 0))
          continue;
        if (!((connection_id >= 0) && (size_t(connection_id) < connections_.size())))
          continue;

        const auto fields = read_fields(record.data, record.data_len);

        connection_data.type = fields.at("type");
        connection_data.md5sum = fields.at("md5sum");
        connection_data.message_definition = fields.at("message_definition");
        std::cout << "Msg def for topic " << connection_data.topic << ":\n" << connection_data.message_definition << std::endl;
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
        ros_msg_skipper skipper;

        std::string::const_iterator iter = connection_data.message_definition.begin();
        std::string::const_iterator end = connection_data.message_definition.end();
        bool r = phrase_parse(iter, end, grammar, skipper, ast);

        if (r && iter == end) {
          std::cout << "-------------------------\n";
          std::cout << "Parsing succeeded\n";
          std::cout << ast.fields[0].field_name << std::endl;
          std::cout << "-------------------------\n";
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
        break;
      }
      case header_t::op::CHUNK_INFO: {
        uint32_t ver;
        uint64_t chunk_pos;
        uint64_t start_time;
        uint64_t end_time;
        uint32_t count;

        header.get_field("ver", ver);
        header.get_field("chunk_pos", chunk_pos);
        header.get_field("start_time", start_time);
        header.get_field("end_time", end_time);
        header.get_field("count", count);

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