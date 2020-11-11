#import embag as rosbag
import embag


#bag = rosbag.Bag('_2019-05-29-18-10-41_190.bag')
#bag = rosbag.Bag('truck-201_run-15955_2020-03-12-00-06-09_slim_107.bag')
#bag = rosbag.Bag('truck-201_run-15955_2020-03-12-00-06-09_luminar_107.bag')

#for topic, msg, t in bag.read_messages(topics=['/luminar_pointcloud']):
#for topic, msg, t in bag.read_messages(topics=['/perception/tracked_lanes']): 
#for topic, msg, t in bag.read_messages(topics=['/applanix/nav']):
#for topic, msg, t in bag.read_messages(topics=['/diagnostics']):
#    print(topic)
#    print(t)
#    print(msg)

#bag.close()

view = embag.View().addBag('/home/jason/Downloads/truck-203_run-16938_2020-06-29-21-50-28_slim_6.bag')
for msg in view.getMessages('/diagnostics'):
#for msg in view.getMessages('/self_driving_state'):
    print(msg.topic)
    print(msg.timestamp.to_sec())
    print(msg)
