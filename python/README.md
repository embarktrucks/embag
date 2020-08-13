# Embag Python API
There are two Python APIs available in Embag.  The first is meant to be a drop-in replacement for [rosbag](http://wiki.ros.org/rosbag/Code%20API#Python_API):

```python
import embag as rosbag

bag = rosbag.Bag('/path/to/file.bag')
for topic, msg, t in bag.read_messages(topics=['/cool/topic']):
    print(msg)
 
bag.close()
```
Unfortunately, this API comes with a few caveats:
- It's slightly slower than the second, more native API.
- You can't iterate over more than one bag file (something ROS's C++ API allows you to do.)
- Special ROS primitives (such as `Time` and `Duration`) haven't been built out yet.  You can call `to_sec()` on them but arithmetic operators won't work.

Embag also offers an API more like the [C++ API](http://wiki.ros.org/rosbag/Code%20API#C.2B-.2B-_API):
```python
import embag

bag1 = embag.Bag('/path/to/file1.bag')
bag2 = embag.Bag('/path/to/file2.bag')

view = rosbag.View().addBag(bag1).addBag(bag2)
for msg in view.getMessages(['/cool/topic']):
    print(msg.timestamp.to_sec())
    print(msg)

bag.close()
```
