#include "gtest/gtest.h"
#include "lib/embag.h"
#include "lib/view.h"

#include <set>
#include <unordered_set>
#include <vector>

TEST(EmbagTest, OpenCloseBag) {
  Embag::Bag bag{"test/test.bag"};
  ASSERT_TRUE(bag.close());
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
void validateSchema(testSchema& test_schema, const std::vector<Embag::RosMsgTypes::ros_msg_member>& members) {
  ASSERT_EQ(members.size(), test_schema.size());

  size_t i = 0;
  for (const auto& named_type : test_schema) {
    const auto name = named_type.first;
    const auto type = named_type.second;

    const auto field = boost::get<Embag::RosMsgTypes::ros_msg_field>(members[i++]);
    ASSERT_EQ(field.field_name, name);
    ASSERT_EQ(field.type_name, type);
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

  validateSchema(top_level_types, def->members);

  // Test recursion on Header embedded type
  const testSchema header_types = {
      {"seq", "uint32"},
      {"stamp", "time"},
      {"frame_id", "string"},
  };

  const auto header_field = boost::get<Embag::RosMsgTypes::ros_msg_field>(def->members[0]);
  const auto header = def->getEmbeddedType("", header_field);
  validateSchema(header_types, header.members);

  // Test array type
  const auto array_field = boost::get<Embag::RosMsgTypes::ros_msg_field>(def->members.back());
  ASSERT_EQ(array_field.array_size, -1);  // -1 is an array of undefined length
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
    ASSERT_NE(message->raw_data, nullptr);
    ASSERT_GT(message->raw_data_len, 0);

    if (unseen_topics.count(message->topic)) {
      unseen_topics.erase(message->topic);
    }

    // For each topic, we'll test a few fields to make sure they're read from the bag correctly
    if (message->topic == "/base_scan") {
      ASSERT_GE(message->timestamp.to_sec(), last_scan_ts);
      last_scan_ts = message->timestamp.to_sec();
      ASSERT_EQ(message->md5, "90c7ef2dc6895d81024acba2ac42f369");
      ASSERT_EQ(message->data()["header"]["seq"].as<uint32_t>(), scan_seq++);
      ASSERT_EQ(message->data()["header"]["frame_id"].as<std::string>(), "base_laser_link");
      ASSERT_EQ(message->data()["scan_time"].as<float>(), 0.0);

      // Arrays are exposed at blobs
      ASSERT_EQ(message->data()["ranges"].getType(), Embag::RosValue::Type::blob);
      const auto blob = message->data()["ranges"].getBlob();
      ASSERT_EQ(blob.type, Embag::RosValue::Type::float32);
      ASSERT_GT(blob.size, 0);
      ASSERT_EQ(blob.size, 90);
      ASSERT_EQ(blob.byte_size, 90 * sizeof(float));

      const auto *ranges = (float *) blob.data.data();
      for (size_t i = 0; i < blob.size; i++) {
        ASSERT_NE(*(ranges + i), 0.0);
      }
    }

    if (message->topic == "/base_pose_ground_truth") {
      ASSERT_GE(message->timestamp.to_sec(), last_pose_ts);
      last_pose_ts = message->timestamp.to_sec();
      ASSERT_EQ(message->md5, "cd5e73d190d741a2f92e81eda573aca7");
      ASSERT_EQ(message->data()["header"]["seq"].as<uint32_t>(), pose_seq++);
      ASSERT_EQ(message->data()["header"]["frame_id"].as<std::string>(), "odom");
      ASSERT_NE(message->data()["pose"]["pose"]["position"]["x"].as<double>(), 0.0);

      ASSERT_EQ(message->data()["pose"]["covariance"].getType(), Embag::RosValue::Type::blob);
      const auto blob = message->data()["pose"]["covariance"].getBlob();
      const auto *values = (float *) blob.data.data();
      for (size_t i = 0; i < blob.size; i++) {
        ASSERT_EQ(*(values + i), 0.0);
      }
    }
  }

  ASSERT_EQ(unseen_topics.size(), 0);
}

TEST_F(ViewTest, MessagesForTopic) {
  for (const auto &message : view_.getMessages("/base_scan")) {
    ASSERT_EQ(message->topic, "/base_scan");
  }

  for (const auto &message : view_.getMessages({"/base_scan"})) {
    ASSERT_EQ(message->topic, "/base_scan");
  }
}

// TODO: test multi-bag message sorting
