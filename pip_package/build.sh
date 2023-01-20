#!/bin/bash -e

function build() {
  PYTHON_PARENT_FOLDER=$1
  PYTHON_VERSION=$2

  PYTHON_PATH="$PYTHON_PARENT_FOLDER/python$PYTHON_VERSION"

  # Install necessary dependencies
  "$PYTHON_PATH" -m pip install -r /tmp/pip_build/requirements.txt --user

  # Build embag libs and echo test binary
  (cd /tmp/embag &&
    PYTHON_BIN_PATH="$PYTHON_PATH" bazel build -c opt //python:libembag.so //embag_echo:embag_echo &&
    PYTHON_BIN_PATH="$PYTHON_PATH" bazel test -c opt //test:embag_test "//test:embag_test_python$PYTHON_VERSION" --cache_test_results=no --test_output=all)

  # Build wheel
  cp /tmp/embag/bazel-bin/python/libembag.so /tmp/pip_build/embag
  (cd /tmp/pip_build && "$PYTHON_PATH" setup.py bdist_wheel &&
    auditwheel repair /tmp/pip_build/dist/embag*.whl --plat manylinux2014_x86_64 &&
    "$PYTHON_PATH" -m pip install wheelhouse/embag*.whl --user &&
    "$PYTHON_PATH" -c 'import embag; embag.View(); print("Successfully loaded embag!")' &&
    cp wheelhouse/* /tmp/out &&
    rm wheelhouse/* &&
    rm -rf build dist)
}

# Build embag for various version of Python 3
for version in \
  cp36-cp36m \
  cp37-cp37m \
  cp38-cp38 \
  cp39-cp39; do
  # Link the correct version of python
  ln -sf /opt/python/$version/bin/python /usr/bin/python3

  build "/opt/python/$version/bin" 3
done
