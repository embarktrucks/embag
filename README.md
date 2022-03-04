# Embag: A really fast, simple bag file reader
[![embag CI status](https://github.com/embarktrucks/embag/workflows/pypi_build/badge.svg)](https://github.com/embarktrucks/embag/actions)
[![PyPI version](https://badge.fury.io/py/embag.svg)](https://badge.fury.io/py/embag)

This library reads [ROS](https://wiki.ros.org) [bag files](http://wiki.ros.org/Bags/Format/2.0) quickly without their [message descriptions](http://wiki.ros.org/msg) and without any dependencies.

## Python API
Embag is available on [PyPI](https://pypi.org/project/embag/) and supports Linux and MacOS.  To use it, simply install with `pip`:

    pip install embag
    
### Usage
See the [python directory](https://github.com/embarktrucks/embag/tree/master/python) for details.
The [tests](https://github.com/embarktrucks/embag/tree/master/test) include more usage examples.

## C++ API
Embag is a native library and is available from Gemfury as a Debian package.  You can also build it from source.  To install the `.deb` package for the first time:

    sudo sh -c "echo 'deb [trusted=yes] https://apt.fury.io/embark/ /' >> /etc/apt/sources.list.d/fury.list"
    sudo apt update
    sudo apt install embag

### Building
First, you'll need [Bazel](https://bazel.build).  It's often easiest to use [Bazelisk](https://github.com/bazelbuild/bazelisk), a convenient wrapper that installs it for you.
To build, run:

    # Embag echo demo binary
    bazel build //embag_echo:embag_echo
    # Run with
    bazel-bin/embag_echo/embag_echo --bag /path/to/sweet.bag --topic /awesome/topic

    # Shared object file
    bazel build //lib:libembag.so

    # Debian package
    bazel build //lib:embag-debian

    # Python Wheel (for example, you might use a specific installation of Python)
    PYTHON_BIN_PATH=/usr/local/bin/python3.7.5 bazel build //python:wheel
    
To test, run:

    # This will run both the C++ and Python tests against a small bag file
    bazel test test:* --test_output=all

NOTE: If you're testing the python2 or python3 interface, you'll need to ensure that your system has numpy installed for each respective python version.

### Usage
To use the C++ API:
```c++
Embag::View view{filename};
view.addBag("another.bag");  # Views support reading from multiple bags

for (const auto &message : view.getMessages({"/fun/topic", "/another/topic"})) {
  std::cout << message->timestamp->to_sec() << " : " << message->topic << std::endl;
  std::cout << message->data()["fun_array"][0]["fun_field"]->as<std::string>() << std::endl;
}
```
See the [tests](https://github.com/embarktrucks/embag/tree/master/test) for more usage examples.

## Benchmarks
These tests were performed on a 473M bag file compressed with lz4.  The bag contains a number of complex topics and each library was asked to print the topic, timestamp, and deserialized message.
Unfortunately, it's not possible to decode messages using the ROS C++ API without precompiled message definitions.

| Library            | 6128 messages | 1000 messages |
|--------------------|---------------|---------------|
| rospy              | 57.898s       | 1.815s        |
| embag (Python API) | 3.655s        | 1.093s        |
| embag (C++ API)    | 3.300s        | 1.007s        |


## Thank you...
This library was heavily influenced by:
- [bag_rdr: a zero-copy ROS bag parser library](https://github.com/starship-technologies/bag_rdr)
- [Java Bag Reader](https://github.com/swri-robotics/bag-reader-java)

## License (MIT)
Copyright 2020 Embark Trucks

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

