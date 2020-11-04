import python.libembag as embag
from collections import OrderedDict
import struct
import unittest


class EmbagTest(unittest.TestCase):
    def setUp(self):
        self.bag = embag.Bag('test/test.bag')
        self.view = embag.View('test/test.bag')
        self.known_topics = {
            "/base_pose_ground_truth",
            "/base_scan",
            "/luminar_pointcloud",
        }

    def tearDown(self):
        self.assertTrue(self.bag.close())

    def testSchema(self):
        known_schema = OrderedDict([
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

        schema = self.bag.getSchema('/luminar_pointcloud')
        self.assertDictEqual(schema, known_schema)

    def testTopicsInBag(self):
        topics = set(self.bag.topics())
        self.assertSetEqual(topics, self.known_topics)

    def testROSMessages(self):
        unseen_topics = self.known_topics.copy()

        base_scan_seq = 601
        base_pose_seq = 601
        for topic, msg, t in self.bag.read_messages(topics=list(self.known_topics)):
            self.assertTrue(topic in self.known_topics)
            self.assertNotEqual(topic, "")
            self.assertGreater(t, 0)

            if topic in unseen_topics:
                unseen_topics.remove(topic)

            if topic == '/base_scan':
                self.assertEqual(msg.header.seq, base_scan_seq)
                base_scan_seq += 1
                self.assertEqual(msg.header.frame_id, "base_laser_link")
                self.assertEqual(msg.scan_time, 0.0)

                # Test binary data
                values = struct.unpack('<%df' % (len(msg.ranges) / 4), msg.ranges)
                for v in values:
                    self.assertNotEqual(v, 0)

            if topic == '/base_pose_ground_truth':
                self.assertEqual(msg.header.seq, base_pose_seq)
                base_pose_seq += 1
                self.assertEqual(msg.header.frame_id, "odom")
                self.assertNotEqual(msg.pose.pose.position.x, 0.0)

                values = struct.unpack('<%df' % (len(msg.pose.covariance) / 4), msg.pose.covariance)
                for v in values:
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
                msg_dict = msg.dict()
                self.assertEqual(msg_dict['header']['seq'], base_scan_seq)
                base_scan_seq += 1
                self.assertEqual(msg_dict['header']['frame_id'], "base_laser_link")
                self.assertEqual(msg_dict['scan_time'], 0.0)

                # Test binary data
                values = struct.unpack('<%df' % (len(msg_dict['ranges']) / 4), msg_dict['ranges'])
                for v in values:
                    self.assertNotEqual(v, 0)

            # We can also access fields directly with data() - this is the fastest option
            if msg.topic == '/base_pose_ground_truth':
                msg_data = msg.data()
                self.assertEqual(msg_data['header']['seq'], base_pose_seq)
                base_pose_seq += 1
                self.assertEqual(msg_data['header']['frame_id'], "odom")
                self.assertNotEqual(msg_data['pose']['pose']['position']['x'], 0.0)

                values = struct.unpack('<%df' % (len(msg_data['pose']['covariance']) / 4), msg_data['pose']['covariance'])
                for v in values:
                    self.assertEqual(v, 0)


if __name__ == "__main__":
    unittest.main()
