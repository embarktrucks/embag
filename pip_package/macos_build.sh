#!/bin/bash -e

mkdir -p /tmp/embag /tmp/pip_build /tmp/out
cp -r lib /tmp/embag
cp -r pip_package/* README.md LICENSE /tmp/pip_build

# Build embag libs and echo test binary
bazel build //python:libembag.so //embag_echo:embag_echo && \
bazel test //test:embag_test //test:embag_test_python3 --test_output=all

# Build wheel
cp bazel-bin/python/libembag.so /tmp/pip_build/embag
python -m pip install wheel
(cd /tmp/pip_build && MACOSX_DEPLOYMENT_TARGET=11.0 python setup.py bdist_wheel && \
 MACOSX_DEPLOYMENT_TARGET=11.0 python -m pip install dist/embag*.whl && \
 python -c 'import embag; embag.View(); print("Successfully loaded embag!")' &&\
 cp dist/* /tmp/out && \
 rm -rf build dist)

