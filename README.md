# Embag: A really fast, simple bag file reader
This library reads [ROS](https://wiki.ros.org) [bag files](http://wiki.ros.org/Bags/Format/2.0) quickly without their [message descriptions](http://wiki.ros.org/msg) and without any ROS dependencies.  In fact, its only dependency is [lz4](https://github.com/lz4/lz4).

## Installation
TBD

## Usage
```c++
Embag reader{filename};

for (const auto &message : reader.getView().getMessages({"/fun/topic", "/another/topic"})) {
  std::cout << message->topic << " at " << message->timestamp.secs << "." << message->timestamp.nsecs << std::endl;
  std::cout << message->data("fun_array")->at(0)->getValue<std::string>("fun_field") << std::endl;
}

reader.close();
```

## Building
First, you'll need [Bazel](https://docs.bazel.build/versions/master/install-ubuntu.html#step-1-add-bazel-distribution-uri-as-a-package-source).   You'll also need to install the [lz4](https://github.com/lz4/lz4) dev package:

    sudo apt install liblz4-dev

To build, run:

    bazel build //embag_echo:embag_echo
    
## Thank you...
This library was heavily influenced by:
- [bag_rdr: a zero-copy ROS bag parser library](https://github.com/starship-technologies/bag_rdr)
- [Java Bag Reader](https://github.com/swri-robotics/bag-reader-java)