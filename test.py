import embag as rosbag

bag = rosbag.Bag('truck-201_run-15955_2020-03-12-00-06-09_slim_107.bag')
#bag = rosbag.Bag('truck-201_run-15955_2020-03-12-00-06-09_luminar_107.bag')

#for topic, msg, t in bag.read_messages(topics=['/luminar_pointcloud']):
#for topic, msg, t in bag.read_messages(topics=['/applanix/nav']):
for topic, msg, t in bag.read_messages(topics=['/perception/tracked_lanes']):
    print(topic)
    print(t)
    print(msg.header.stamp.to_sec())


#view = rosbag.View().addBag(bag)
#for msg in view.getMessages(['/perception/tracked_lanes']):
#    print(msg.timestamp.to_sec())
#    print(msg.data()['header']['stamp'].to_sec())
