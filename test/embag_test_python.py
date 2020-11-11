import python.libembag as embag
from collections import OrderedDict
import struct
import unittest


class EmbagTest(unittest.TestCase):
    def setUp(self):
        self.bag = embag.Bag('test/test.bag')
        self.view = embag.View('test/test.bag')
        self.known_topics = {'/base_pose_ground_truth', '/base_scan'}

    def tearDown(self):
        self.assertTrue(self.bag.close())

    def testSchema(self):
        known_schema = OrderedDict([
            ('header', {'type': 'object',
                        'children': OrderedDict([
                            ('seq', {'type': 'uint32'}),
                            ('stamp', {'type': 'time'}),
                            ('frame_id', {'type': 'string'})
                        ])}),
            ('angle_min', {'type': 'float32'}),
            ('angle_max', {'type': 'float32'}),
            ('angle_increment', {'type': 'float32'}),
            ('time_increment', {'type': 'float32'}),
            ('scan_time', {'type': 'float32'}),
            ('range_min', {'type': 'float32'}),
            ('range_max', {'type': 'float32'}),
            ('ranges', {'member_type': 'float32', 'type': 'array'}),
            ('intensities', {'member_type': 'float32', 'type': 'array'})
        ])

        schema = self.bag.getSchema('/base_scan')
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
            self.assertNotEqual(topic, '')
            self.assertGreater(t, 0)

            if topic in unseen_topics:
                unseen_topics.remove(topic)

            if topic == '/base_scan':
                self.assertEqual(msg.header.seq, base_scan_seq)
                base_scan_seq += 1
                self.assertEqual(msg.header.frame_id, 'base_laser_link')
                self.assertEqual(msg.scan_time, 0.0)

                # Test binary data
                values = struct.unpack('<%df' % (len(msg.ranges) / 4), msg.ranges)
                for v in values:
                    self.assertNotEqual(v, 0)

            if topic == '/base_pose_ground_truth':
                self.assertEqual(msg.header.seq, base_pose_seq)
                base_pose_seq += 1
                self.assertEqual(msg.header.frame_id, 'odom')
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

        for msg in self.view.getMessages('/base_scan'):
            self.assertEqual(msg.topic, '/base_scan')

        unseen_topics = self.known_topics.copy()
        for msg in self.view.getMessages(list(self.known_topics)):
            if msg.topic in unseen_topics:
                unseen_topics.remove(msg.topic)

        self.assertEqual(len(unseen_topics), 0)

        base_scan_seq = 601
        base_pose_seq = 601
        for msg in self.view.getMessages(list(self.known_topics)):
            self.assertTrue(msg.topic in self.known_topics)
            self.assertNotEqual(msg.topic, '')
            self.assertGreater(msg.timestamp.to_sec(), 0)

            if msg.topic == '/base_scan':
                msg_dict = msg.dict()
                self.assertEqual(msg_dict['header']['seq'], base_scan_seq)
                base_scan_seq += 1
                self.assertEqual(msg_dict['header']['frame_id'], 'base_laser_link')
                self.assertEqual(msg_dict['scan_time'], 0.0)

                # Test binary data
                dict_values = struct.unpack('<%df' % (len(msg_dict['ranges']) / 4), msg_dict['ranges'])
                self.assertNotEqual(len(dict_values), 0)
                for v in dict_values:
                    self.assertNotEqual(v, 0)

                # Compare with direct API
                data_values = msg['ranges'].as_blob().to_list()
                self.assertEqual(len(dict_values), len(data_values))

                for i in range(len(dict_values)):
                    self.assertEqual(dict_values[i], data_values[i])

            # We can also access fields directly - this is the fastest option
            if msg.topic == '/base_pose_ground_truth':
                self.assertEqual(msg['header']['seq'].as_uint32(), base_pose_seq)
                self.assertGreater(msg['header']['stamp'].as_ros_time().to_sec(), 0.0)
                base_pose_seq += 1
                self.assertEqual(msg['header']['frame_id'].as_string(), 'odom')
                self.assertNotEqual(msg['pose']['pose']['position']['x'].as_double(), 0.0)

                unpack_values = struct.unpack('<%dd' % msg['pose']['covariance'].as_blob().size, msg['pose']['covariance'].as_blob().bytes())
                self.assertNotEqual(len(unpack_values), 0)
                for v in unpack_values:
                    self.assertEqual(v, 0)

                self.assertNotEqual(len(msg['pose']['covariance'].as_blob()), 0)
                print(msg['pose']['covariance'].as_blob()[1])

                array_values = msg['pose']['covariance'].as_blob().to_list()
                self.assertNotEqual(len(array_values), 0)
                for v in array_values:
                    self.assertEqual(v, 0)

                for i in range(len(unpack_values)):
                    self.assertEqual(unpack_values[i], array_values[i])

                with self.assertRaises(RuntimeError):
                    # This should fail because the x is a float
                    foo = msg['pose']['pose']['position']['x'].as_int16()


if __name__ == '__main__':
    unittest.main()
