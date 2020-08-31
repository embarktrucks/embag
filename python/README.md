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
- You can't iterate over more than one bag file (something ROS's C++ API allows you to do).
- Special ROS primitives (such as `Time` and `Duration`) haven't been built out yet.  For now, these values are provided as doubles.

Embag also offers an API more like the [C++ API](http://wiki.ros.org/rosbag/Code%20API#C.2B-.2B-_API):
```python
import embag
import struct

view = embag.View().addBag('/path/to/file1.bag').addBag('/path/to/file2.bag')
for msg in view.getMessages(['/cool/topic']):
    print(msg.timestamp.to_sec())
    print(msg)

    # There are a few ways to access fields, the first returns a dict of the message
    print(msg.dict())
    # You can also access individual fields much faster this way
    print(msg.data()['cool_field'])

    # Arrays are returned as Python bytes for performance reasons.
    # Assuming an array of floats (4 bytes per value), you can unpack them using:
    values = struct.unpack('<%df' % (len(msg.data()['field']) / 4), msg.data()['field'])


bag.close()
```

If you're interested in the schema of a particular topic in a bag, embag offers a simple, machine readable format:
```python
import embag
import pprint

pp = pprint.PrettyPrinter()

bag = embag.Bag('/path/to/file.bag')
pp.pprint(bag.getSchema('/cool/topic'))
bag.close()
```

