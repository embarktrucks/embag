#!/bin/bash -e

mkdir /tmp/embag /tmp/pip_build /tmp/out
cp -r lib /tmp/embag
cp -r pip_package/* README.md LICENSE /tmp/pip_build

# Build embag for various version of Python
for version in 3.5 3.6 3.7 3.8
  do
    # Build embag libs and echo test binary
    PYTHON_PATH=`which python$version`
    PYTHON_BIN_PATH=$PYTHON_PATH bazel build //python:libembag.so //embag_echo:embag_echo && \

    # FIXME: This may not run with the correct version of python...
    bazel test test:* --test_output=all

    # Build wheel
    cp bazel-bin/python/libembag.so /tmp/pip_build/embag
    (cd /tmp/pip_build && $PYTHON_PATH setup.py bdist_wheel && \
     $PYTHON_PATH -m pip install dist/embag*.whl && \
     $PYTHON_PATH -c 'import embag; embag.View(); print("Successfully loaded embag!")' &&\
     cp dist/* /tmp/out && \
     rm -rf build dist)
  done

