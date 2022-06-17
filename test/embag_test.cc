#include "gtest/gtest.h"
#include "lib/embag.h"
#include "lib/view.h"

#include <set>
#include <unordered_set>
#include <vector>
#include <fstream>

TEST(EmbagTest, OpenCloseBag) {
  Embag::Bag bag{"test/test.bag"};
  bag.close();
}

TEST(EmbagTest, OpenCloseOddPaddingBag) {
  // In some very rare cases rosbags will contain padding between the CHUNK and INDEX section of the file.
  // This bagfile is one such case.
  Embag::Bag oddly_padded_bag{"test/test_2.bag"};
  oddly_padded_bag.close();
}

class BagTest : public ::testing::Test {
 protected:
  Embag::Bag bag_{"test/test.bag"};

  const std::set<std::string> known_topics_ = {
      "/base_pose_ground_truth",
      "/base_scan",
      "/luminar_pointcloud",
  };
};

TEST_F(BagTest, TopicsInBag) {
  std::set<std::string> topic_set;
  for (const auto& topic : bag_.topics()) {
    topic_set.emplace(topic);
  }

  ASSERT_TRUE(topic_set == known_topics_);
}

TEST_F(BagTest, TopicInBag) {
  for (const auto& topic : known_topics_) {
    ASSERT_TRUE(bag_.topicInBag(topic));
  }
}

typedef const std::vector<std::pair<std::string, std::string>> testSchema;
void validateSchema(testSchema& test_schema, const std::vector<Embag::RosMsgTypes::MemberDef>& members) {
  ASSERT_EQ(members.size(), test_schema.size());

  size_t i = 0;
  for (const auto& named_type : test_schema) {
    const auto name = named_type.first;
    const auto type = named_type.second;

    const auto field = boost::get<Embag::RosMsgTypes::FieldDef>(members[i++]);
    ASSERT_EQ(field.name(), name);
    ASSERT_EQ(field.typeName(), type);
  }
}

TEST_F(BagTest, MsgDefForTopic) {
  const auto def = bag_.msgDefForTopic("/base_scan");

  const testSchema top_level_types = {
      {"header", "Header"},
      {"angle_min", "float32"},
      {"angle_max", "float32"},
      {"angle_increment", "float32"},
      {"time_increment", "float32"},
      {"scan_time", "float32"},
      {"range_min", "float32"},
      {"range_max", "float32"},
      {"ranges", "float32"},
      {"intensities", "float32"},
  };

  validateSchema(top_level_types, def->members());

  // Test recursion on Header embedded type
  const testSchema header_types = {
      {"seq", "uint32"},
      {"stamp", "time"},
      {"frame_id", "string"},
  };

  auto header_field = boost::get<Embag::RosMsgTypes::FieldDef>(def->members()[0]);
  const auto header = header_field.typeDefinition();
  validateSchema(header_types, header.members());

  // Test array type
  const auto array_field = boost::get<Embag::RosMsgTypes::FieldDef>(def->members().back());
  ASSERT_EQ(array_field.arraySize(), -1);  // -1 is an array of undefined length
}

TEST_F(BagTest, ConnectionsForTopic) {
  const auto connection_records = bag_.connectionsForTopic("/base_scan");
  ASSERT_EQ(connection_records.size(), 1);

  const auto &record = connection_records[0];
  ASSERT_EQ(record->blocks.size(), 1);
  for (const auto &block : record->blocks) {
    const auto &chunk = block.into_chunk;
    ASSERT_NE(chunk, nullptr);
    ASSERT_GT(chunk->offset, 0);
    ASSERT_GT(chunk->info.message_count, 0);
    ASSERT_EQ(chunk->compression, "lz4");
    ASSERT_GT(chunk->uncompressed_size, 0);
    ASSERT_GT(chunk->record.header_len, 0);
    ASSERT_NE(chunk->record.header, nullptr);
    ASSERT_GT(chunk->record.data_len, 0);
    ASSERT_NE(chunk->record.data, nullptr);
  }
  ASSERT_EQ(record->topic, "/base_scan");
  ASSERT_EQ(record->data.topic, "/base_scan");
  ASSERT_EQ(record->data.type, "sensor_msgs/LaserScan");
  ASSERT_EQ(record->data.scope, "sensor_msgs");
  ASSERT_EQ(record->data.md5sum, "90c7ef2dc6895d81024acba2ac42f369");
  ASSERT_EQ(record->data.message_definition.size(), 2123);
  ASSERT_EQ(record->data.callerid, "/play_1604515197096283663");
  ASSERT_EQ(record->data.latching, false);
}

class ViewTest : public ::testing::Test {
 protected:
  Embag::View view_{"test/test.bag"};

  const std::set<std::string> known_topics_ = {
      "/base_pose_ground_truth",
      "/base_scan",
      "/luminar_pointcloud",
  };
};


TEST_F(ViewTest, View) {
  const Embag::RosValue::ros_time_t start_time{1604515190, 231374463};
  const Embag::RosValue::ros_time_t end_time{1604515197, 820012098};

  ASSERT_EQ(view_.getStartTime(), start_time);
  ASSERT_EQ(view_.getEndTime(), end_time);

  const auto topics = view_.topics();
  const auto topic_set = std::unordered_set<std::string>(topics.begin(), topics.end());

  ASSERT_EQ(topic_set.size(), 3);
  ASSERT_EQ(topic_set.count("/base_pose_ground_truth"), 1);
  ASSERT_EQ(topic_set.count("/base_scan"), 1);
}

TEST_F(ViewTest, AllMessages) {
  std::unordered_set<std::string> unseen_topics = {
      "/base_pose_ground_truth",
      "/base_scan",
  };

  size_t scan_seq = 601;
  size_t pose_seq = 601;
  double last_scan_ts = 0;
  double last_pose_ts = 0;
  for (const auto &message : view_.getMessages()) {
    ASSERT_NE(message->topic, "");
    ASSERT_TRUE(message->raw_buffer);
    ASSERT_GT(message->raw_data_len, 0);

    if (unseen_topics.count(message->topic)) {
      unseen_topics.erase(message->topic);
    }

    // For each topic, we'll test a few fields to make sure they're read from the bag correctly
    if (message->topic == "/base_scan") {
      ASSERT_GE(message->timestamp.to_sec(), last_scan_ts);
      last_scan_ts = message->timestamp.to_sec();
      ASSERT_EQ(message->md5, "90c7ef2dc6895d81024acba2ac42f369");
      ASSERT_EQ(message->data()["header"]["seq"]->as<uint32_t>(), scan_seq++);
      ASSERT_EQ(message->data()["header"]["frame_id"]->as<std::string>(), "base_laser_link");
      ASSERT_EQ(message->data()["scan_time"]->as<float>(), 0.0);

      // Arrays are exposed at blobs
      ASSERT_EQ(message->data()["ranges"]->getType(), Embag::RosValue::Type::primitive_array);
      const auto array = message->data()["ranges"];
      ASSERT_EQ(array[0]->getType(), Embag::RosValue::Type::float32);
      ASSERT_EQ(array->size(), 90);

      for (size_t i = 0; i < array->size(); ++i) {
        ASSERT_NE(array[i]->as<float>(), 0.0);
      }
    }

    if (message->topic == "/base_pose_ground_truth") {
      ASSERT_GE(message->timestamp.to_sec(), last_pose_ts);
      last_pose_ts = message->timestamp.to_sec();
      ASSERT_EQ(message->md5, "cd5e73d190d741a2f92e81eda573aca7");
      ASSERT_EQ(message->data()["header"]["seq"]->as<uint32_t>(), pose_seq++);
      ASSERT_EQ(message->data()["header"]["frame_id"]->as<std::string>(), "odom");
      ASSERT_NE(message->data()["pose"]["pose"]["position"]["x"]->as<double>(), 0.0);

      ASSERT_EQ(message->data()["pose"]["covariance"]->getType(), Embag::RosValue::Type::primitive_array);
      const auto array = message->data()["pose"]["covariance"];

      for (size_t i = 0; i < array->size(); i++) {
        ASSERT_EQ(array[i]->as<float>(), 0.0);
      }
    }
  }

  ASSERT_EQ(unseen_topics.size(), 0);
}

TEST_F(ViewTest, MessagesForTopic) {
  uint32_t count;
  for (const auto &topic : known_topics_) {
    count = 0;
    for (const auto &message : view_.getMessages(topic)) {
      ASSERT_EQ(message->topic, topic);
      ++count;
    }
    ASSERT_GT(count, 0);

    count = 0;
    for (const auto &message : view_.getMessages({topic})) {
      ASSERT_EQ(message->topic, topic);
      ++count;
    }
    ASSERT_GT(count, 0);
  }
}

class StreamTest : public ::testing::Test {
 protected:
  std::string bag_path_ = "test/test.bag";
};

TEST_F(StreamTest, BagFromStream) {
  std::ifstream ifs{bag_path_};
  auto bytes = std::make_shared<const std::string>((std::istreambuf_iterator<char>(ifs) ), (std::istreambuf_iterator<char>()));
  auto bag = std::make_shared<Embag::Bag>(bytes);
  auto view = Embag::View{bag};

  for (const auto &message : view.getMessages("/base_scan")) {
    ASSERT_EQ(message->topic, "/base_scan");
  }

  for (const auto &message : view.getMessages({"/base_scan"})) {
    ASSERT_EQ(message->topic, "/base_scan");
  }

  bag->close();
}

class ArraysTest : public ::testing::Test {
 protected:
  Embag::View view_{"test/array_test.bag"};
};

TEST_F(ArraysTest, ArrayReading) {
  uint32_t index = 0;
  uint32_t inner_index = 0;
  for (const auto &message : view_.getMessages("/array_test")) {
    ASSERT_EQ(message->topic, "/array_test");
    ASSERT_EQ(message->timestamp.secs, index);
    ASSERT_EQ(message->timestamp.nsecs, 0);
    
    ASSERT_EQ(message->data()["index"]->as<uint32_t>(), index);
    // ASSERT_EQ(message->data()["index_as_text"], std::format("{}", index));

    // For each array type, confirm that both iteration and index access work
    // Tests a dynamically sized array of boolean primitives
    inner_index = 0;
    const auto dynamic_bool_array = message->data()["index_as_dynamic_bool_array"];
    for (auto item = dynamic_bool_array->beginValues<Embag::RosValue::Pointer>(); item != dynamic_bool_array->endValues<Embag::RosValue::Pointer>(); item++) {
      ASSERT_EQ((*item)->as<bool>(), index == inner_index++);
    }
    for (inner_index = 0; inner_index < 20; inner_index++) {
      ASSERT_EQ(dynamic_bool_array[inner_index]->as<bool>(), index == inner_index);
    }

    // Tests a statically sized array of boolean primitives
    inner_index = 0;
    const auto static_bool_array = message->data()["index_as_static_bool_array"];
    for (auto item = static_bool_array->beginValues<Embag::RosValue::Pointer>(); item != static_bool_array->endValues<Embag::RosValue::Pointer>(); item++) {
      ASSERT_EQ((*item)->as<bool>(), index == inner_index++);
    }
    for (inner_index = 0; inner_index < 20; inner_index++) {
      ASSERT_EQ(static_bool_array[inner_index]->as<bool>(), index == inner_index);
    }

    // Tests a dynamically sized array of doubles
    inner_index = 0;
    const auto dynamic_uint64_array = message->data()["index_multiples_as_dynamic_uint64_array"];
    for (auto item = dynamic_uint64_array->beginValues<Embag::RosValue::Pointer>(); item != dynamic_uint64_array->endValues<Embag::RosValue::Pointer>(); item++) {
      ASSERT_EQ((*item)->as<uint64_t>(), (uint64_t) index * inner_index++);
    }
    for (inner_index = 0; inner_index < 20; inner_index++) {
      ASSERT_EQ(dynamic_uint64_array[inner_index]->as<uint64_t>(), (uint64_t) index * inner_index);
    }

    // Tests a statically sized array of doubles
    inner_index = 0;
    const auto static_uint64_array = message->data()["index_multiples_as_static_uint64_array"];
    for (auto item = static_uint64_array->beginValues<Embag::RosValue::Pointer>(); item != static_uint64_array->endValues<Embag::RosValue::Pointer>(); item++) {
      ASSERT_EQ((*item)->as<uint64_t>(), (uint64_t) index * inner_index++);
    }
    for (inner_index = 0; inner_index < 20; inner_index++) {
      ASSERT_EQ(static_uint64_array[inner_index]->as<uint64_t>(), (uint64_t) index * inner_index);
    }

    // Tests an array of strings
    inner_index = 0;
    const auto string_array = message->data()["index_as_string_array"];
    for (auto item = string_array->beginValues<Embag::RosValue::Pointer>(); item != string_array->endValues<Embag::RosValue::Pointer>(); item++) {
      ASSERT_EQ((*item)->as<std::string>(), index == inner_index++ ? "true" : "false");
    }
    for (inner_index = 0; inner_index < 20; inner_index++) {
      ASSERT_EQ(string_array[inner_index]->as<std::string>(), index == inner_index ? "true" : "false");
    }

    // Tests an array of std_msgs/Bool objects
    inner_index = 0;
    const auto bool_object_array = message->data()["index_as_bool_object_array"];
    for (auto item = bool_object_array->beginValues<Embag::RosValue::Pointer>(); item != bool_object_array->endValues<Embag::RosValue::Pointer>(); item++) {
      ASSERT_EQ((*item)["data"]->as<bool>(), index == inner_index++);
    }
    for (inner_index = 0; inner_index < 20; inner_index++) {
      ASSERT_EQ(bool_object_array[inner_index]["data"]->as<bool>(), index == inner_index);
    }

    index++;
  }

  ASSERT_EQ(index, 20);
}

// TODO: test multi-bag message sorting
