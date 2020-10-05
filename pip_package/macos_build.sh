#!/bin/bash -e

mkdir -p /tmp/embag /tmp/pip_build /tmp/out
cp -r lib /tmp/embag
cp -r pip_package/* README.md LICENSE /tmp/pip_build

# Build embag libs and echo test binary
bazel build //python:libembag.so //embag_echo:embag_echo && \
bazel test test:* --test_output=all

# Build wheel
cp bazel-bin/python/libembag.so /tmp/pip_build/embag
python -m pip install wheel
(cd /tmp/pip_build && python setup.py bdist_wheel && \
 python -m pip install dist/embag*.whl && \
 python -c 'import embag; embag.View(); print("Successfully loaded embag!")' &&\
 cp dist/* /tmp/out && \
 rm -rf build dist)

