# Embag: A really fast, simple bag file reader
This library reads [ROS](https://wiki.ros.org) [bag files](http://wiki.ros.org/Bags/Format/2.0) quickly without their [message descriptions](http://wiki.ros.org/msg) and without any ROS dependencies.  In fact, its only dependency is [lz4](https://github.com/lz4/lz4).

## Installation
TBD

## Usage

```c++
Embag::View view{};
auto bag = std::make_shared<Embag::Bag>(filename);
view.addBag(bag);

for (const auto &message : view.getMessages({"/fun/topic", "/another/topic"})) {
  std::cout << message->timestamp.secs << " : " << message->topic << std::endl;
  std::cout << message->data()["fun_array"][0].as<std::string>("fun_field") << std::endl;
}
```

## Building
First, you'll need [Bazel](https://docs.bazel.build/versions/master/install-ubuntu.html#step-1-add-bazel-distribution-uri-as-a-package-source).   You'll also need to install the [lz4](https://github.com/lz4/lz4) dev package:

    sudo apt install liblz4-dev

To build, run:

    # Embag echo demo binary
    bazel build //embag_echo:embag_echo
    # Run with
    bazel-bin/embag_echo/embag_echo --bag /path/to/sweet.bag --topic /awesome/topic

    # Shared object file
    bazel build //lib:embag.so

    # Debian package
    bazel build //lib:embag-debian

## Thank you...
This library was heavily influenced by:
- [bag_rdr: a zero-copy ROS bag parser library](https://github.com/starship-technologies/bag_rdr)
- [Java Bag Reader](https://github.com/swri-robotics/bag-reader-java)
