import python.libembag as embag
from collections import OrderedDict
import numpy as np
import struct
import sys
import unittest


class EmbagTest(unittest.TestCase):
    def setUp(self):
        self.bag_path = 'test/test.bag'
        self.bag = embag.Bag(self.bag_path)
        self.view = embag.View(self.bag_path)
        self.known_topics = {
            "/base_pose_ground_truth",
            "/base_scan",
            "/luminar_pointcloud",
        }
        self.known_connections = {
            "/base_pose_ground_truth": {'type': 'nav_msgs/Odometry',
                                        'scope': 'nav_msgs', 'md5sum': 'cd5e73d190d741a2f92e81eda573aca7',
                                        'callerid': '/play_1604515197096283663', 'latching': False, 'message_count': 5},
            "/base_scan": {'type': 'sensor_msgs/LaserScan', 'scope': 'sensor_msgs',
                           'md5sum': '90c7ef2dc6895d81024acba2ac42f369', 'callerid': '/play_1604515197096283663',
                           'latching': False, 'message_count': 5},
            "/luminar_pointcloud": {'type': 'sensor_msgs/PointCloud2',
                                    'scope': 'sensor_msgs', 'md5sum': '1158d486dd51d683ce2f1be655c3c181',
                                    'callerid': '/play_1604515189845695821', 'latching': False, 'message_count': 5},
        }
        self.known_pointcloud_schema = OrderedDict([
            ('header',
             {'type': 'object',
              'children': OrderedDict([('seq', {'type': 'uint32'}),
                                       ('stamp', {'type': 'time'}),
                                       ('frame_id', {'type': 'string'})])}),
            ('height', {'type': 'uint32'}),
            ('width', {'type': 'uint32'}),
            ('fields',
             {'type': 'array',
              'children': OrderedDict([('name', {'type': 'string'}),
                                       ('offset', {'type': 'uint32'}),
                                       ('datatype', {'type': 'uint8'}),
                                       ('count', {'type': 'uint32'})])}),
            ('is_bigendian', {'type': 'bool'}),
            ('point_step', {'type': 'uint32'}),
            ('row_step', {'type': 'uint32'}),
            ('data', {'member_type': 'uint8', 'type': 'array'}),
            ('is_dense', {'type': 'bool'})
        ])

    def tearDown(self):
        self.bag.close()

    def testSchema(self):
        schema = self.bag.getSchema('/luminar_pointcloud')
        self.assertDictEqual(schema, self.known_pointcloud_schema)

    def testTopicsInBag(self):
        topics = set(self.bag.topics())
        self.assertSetEqual(topics, self.known_topics)

    def testConnectionsInBag(self):
        self.checkConnectionsByTopic(self.bag.connectionsByTopic(), self.known_connections)

    def checkConnectionsByTopic(self, connections_from_bag, known_connections):
        as_dict = {}
        for topic, connections in connections_from_bag.items():
            self.assertEqual(len(connections), 1)
            c = connections[0]
            self.assertIn("MSG: ", c.message_definition)
            self.assertEqual(c.topic, topic)
            as_dict[topic] = dict(type=c.type, scope=c.scope, md5sum=c.md5sum, callerid=c.callerid,
                                  latching=c.latching, message_count=c.message_count)
        self.assertDictEqual(as_dict, known_connections)

    def testROSMessages(self):
        unseen_topics = self.known_topics.copy()

        base_scan_seq = 601
        base_pose_seq = 601
        for topic, msg, t in self.bag.read_messages(topics=list(self.known_topics)):
            self.assertTrue(topic in self.known_topics)
            self.assertNotEqual(topic, "")
            self.assertGreater(t.to_sec(), 0)

            if topic in unseen_topics:
                unseen_topics.remove(topic)

            if topic == '/base_scan':
                self.assertEqual(msg.header.seq, base_scan_seq)
                base_scan_seq += 1
                self.assertEqual(msg.header.frame_id, "base_laser_link")
                self.assertEqual(msg.scan_time, 0.0)

                # Test binary data
                for v in msg.ranges:
                    self.assertNotEqual(v, 0)

            if topic == '/base_pose_ground_truth':
                self.assertEqual(msg.get('header').get('seq'), base_pose_seq)
                base_pose_seq += 1
                self.assertEqual(msg.get('header').get('frame_id'), "odom")
                self.assertNotEqual(msg.get('pose').get('pose').get('position').get('x'), 0.0)

                for v in msg.get('pose').get('covariance'):
                    self.assertEqual(v, 0)

        self.assertEqual(len(unseen_topics), 0)

    def testViewMessages(self):
        unseen_topics = self.known_topics.copy()
        for msg in self.view.getMessages():
            if msg.topic in unseen_topics:
                unseen_topics.remove(msg.topic)

        self.assertEqual(len(unseen_topics), 0)

        for msg in self.view.getMessages("/base_scan"):
            self.assertEqual(msg.topic, "/base_scan")

        unseen_topics = self.known_topics.copy()
        for msg in self.view.getMessages(list(self.known_topics)):
            if msg.topic in unseen_topics:
                unseen_topics.remove(msg.topic)

        self.assertEqual(len(unseen_topics), 0)

        base_scan_seq = 601
        base_pose_seq = 601
        for msg in self.view.getMessages(list(self.known_topics)):
            self.assertTrue(msg.topic in self.known_topics)
            self.assertNotEqual(msg.topic, "")
            self.assertGreater(msg.timestamp.to_sec(), 0)

            if msg.topic == '/base_scan':
                msg_dict = msg.dict(types_to_unpack={embag.RosValueType.float32})
                self.assertEqual(msg_dict['header']['seq'], base_scan_seq)
                base_scan_seq += 1
                self.assertEqual(msg_dict['header']['frame_id'], "base_laser_link")
                self.assertEqual(msg_dict['scan_time'], 0.0)

                for v in msg_dict['ranges']:
                    self.assertNotEqual(v, 0)

            # We can also access fields directly with data() - this is the fastest option
            if msg.topic == '/base_pose_ground_truth':
                msg_data = msg.data()
                self.assertEqual(msg_data['header']['seq'], base_pose_seq)
                base_pose_seq += 1
                self.assertEqual(msg_data['header']['frame_id'], "odom")
                self.assertNotEqual(msg_data['pose']['pose']['position']['x'], 0.0)

                for v in msg_data['pose']['covariance']:
                    self.assertEqual(v, 0)

    def testObjectIterators(self):
        for topic, msg, t in self.bag.read_messages(topics=['/luminar_pointcloud']):
            assert {field_name for field_name in msg} == {field_name for field_name in self.known_pointcloud_schema}
            for field_name, value in msg.items():
                if isinstance(value, embag.RosValue):
                    assert str(msg[field_name]) == str(value)
                else:
                    assert msg[field_name] == value
            for field_name in msg.keys():
                assert field_name in self.known_pointcloud_schema
            assert set(str(v) for v in msg.values()) == {str(msg[field_name]) for field_name in msg}

    def testTopicsInView(self):
        topics = set(self.view.topics())
        self.assertSetEqual(topics, self.known_topics)

    def testConnectionsInView(self):
        self.checkConnectionsByTopic(self.view.connectionsByTopic(), self.known_connections)

    def testBagFromBytes(self):
        bag_stream = open(self.bag_path, 'rb')
        bag_bytes = bag_stream.read()
        bag = embag.Bag(bag_bytes, len(bag_bytes))
        self.view = embag.View(bag)

        self.testViewMessages()
        self.testTopicsInView()
        self.testConnectionsInView()
        bag_stream.close()

    def testBufferInfo(self):
        for msg in self.view.getMessages('/base_pose_ground_truth'):
            covariance_array = msg.data()['pose']['covariance']
            self.assertTrue(memoryview(covariance_array).readonly)
            for covariance in np.array(covariance_array, copy=False):
                self.assertEqual(covariance, 0)

    def testDictMemoryView(self):
        for msg in self.view.getMessages('/base_pose_ground_truth'):
            dict_memoryview = msg.dict(packed_types_as_memoryview=True)['pose']['covariance']
            assert isinstance(dict_memoryview, memoryview if sys.version_info >= (3,3) else np.ndarray)
            dict_bytes = msg.dict()['pose']['covariance']
            assert bytes(bytearray(dict_memoryview)) == dict_bytes
            dict_list = dict_memoryview.tolist()
            data_list = [v for v in msg.data()['pose']['covariance']]
            assert dict_list == data_list

    def testDictUnpacking(self):
        for msg in self.view.getMessages('/base_pose_ground_truth'):
            assert isinstance(msg.dict()['pose']['covariance'], bytes)
            assert isinstance(msg.dict(types_to_unpack={embag.RosValueType.float64})['pose']['covariance'], list)

    def testRosValueDict(self):
        # Validate that dict on RosValue functions the same as on messages
        for msg in self.view.getMessages('/luminar_pointcloud'):
            message_dict = msg.dict()
            # Below python3.3, primitive arrays in dict form are numpy arrays
            # so we need to compare them with numpy's utilities.
            # This comparison still works in python3.3 and above, so just use it no matter what
            # Test RosValue objects
            np.testing.assert_equal(
                message_dict['header'],
                msg.data()['header'].dict(),
            )
            # Test RosValue arrays
            np.testing.assert_equal(
                message_dict['fields'],
                msg.data()['fields'].dict(),
            )
            # Test RosValue primitive_arrays
            np.testing.assert_equal(
                message_dict['data'],
                msg.data()['data'].dict(),
            )

    def testROSTimeDicting(self):
        for msg in self.view.getMessages('/base_pose_ground_truth'):
            assert isinstance(msg.dict()['header']['stamp'], embag.RosTime)
            as_ros_time = msg.dict(ros_time_py_type=None)['header']['stamp']
            as_int = msg.dict(ros_time_py_type=int)['header']['stamp']
            as_float = msg.dict(ros_time_py_type=float)['header']['stamp']
            assert isinstance(as_ros_time, embag.RosTime)
            assert isinstance(as_int, int)
            assert isinstance(as_float, float)
            assert as_ros_time.to_nsec() == as_int
            assert as_ros_time.to_sec() == as_float

if __name__ == "__main__":
    unittest.main()
