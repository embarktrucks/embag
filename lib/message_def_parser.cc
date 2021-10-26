#include "message_def_parser.h"

#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/include/adapt_struct.hpp>


// Parser structures and binding - must exist in global namespace
BOOST_FUSION_ADAPT_STRUCT(
    Embag::RosMsgTypes::ros_msg_field,
    type_name_,
    array_size_,
    field_name_,
)

BOOST_FUSION_ADAPT_STRUCT(
    Embag::RosMsgTypes::ros_msg_constant,
    type_name,
    constant_name,
    value,
)

BOOST_FUSION_ADAPT_STRUCT(
    Embag::RosMsgTypes::ros_embedded_msg_def,
    type_name_,
    members_,
)

BOOST_FUSION_ADAPT_STRUCT(
    Embag::RosMsgTypes::ros_msg_def,
    members_,
    embedded_types_,
)

namespace Embag {

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
    line_noise = eol >> *space;

    skip = line_noise | comment | separator | eol | blank_lines;
  }

  qi::rule<Iterator> skip;
  qi::rule<Iterator> comment;
  qi::rule<Iterator> separator;
  qi::rule<Iterator> blank_lines;
  qi::rule<Iterator> line_noise;
};

// ROS message parsing
// See http://wiki.ros.org/msg for details on the format
template<typename Iterator, typename Skipper = ros_msg_skipper<Iterator>>
struct ros_msg_grammar : qi::grammar<Iterator, RosMsgTypes::ros_msg_def(), Skipper> {
  ros_msg_grammar() : ros_msg_grammar::base_type(msg) {
    using qi::lit;
    using qi::lexeme;
    using boost::spirit::ascii::char_;
    using boost::spirit::ascii::space;
    using boost::spirit::qi::uint_;
    using boost::spirit::eol;
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

std::shared_ptr<RosMsgTypes::ros_msg_def> parseMsgDef(const std::string &def) {
  std::string::const_iterator iter = def.begin();
  const std::string::const_iterator end = def.end();

  const ros_msg_grammar<std::string::const_iterator> grammar;
  const ros_msg_skipper<std::string::const_iterator> skipper;

  auto ast = std::make_shared<RosMsgTypes::ros_msg_def>();
  const bool r = phrase_parse(iter, end, grammar, skipper, *ast);

  if (r && iter == end) {
    return ast;
  }

  const std::string::const_iterator some = iter + std::min(30, int(end - iter));
  const std::string context(iter, (some > end) ? end : some);

  throw std::runtime_error("Message definition parsing failed at: " + context);
}
}