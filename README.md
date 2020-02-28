# Embag: A really fast, simple bag file reader
This library is under active development and probably not useful at this point.

## Building
First, you'll need [Bazel](https://docs.bazel.build/versions/master/install-ubuntu.html#step-1-add-bazel-distribution-uri-as-a-package-source).   You'll also need to install the lz4 dev package:

    sudo apt install liblz4-dev

To build, run:


    bazel build //embag_echo:embag_echo
