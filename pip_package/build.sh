#!/bin/bash -e

function build() {
  PYTHON_PATH=$1
  PYTHON_VERSION=$2

  # Build embag libs and echo test binary
  (cd /tmp/embag &&
    PYTHON_BIN_PATH="$PYTHON_PATH/python" bazel build //python:libembag.so //embag_echo:embag_echo &&
    bazel test //test:embag_test "//test:embag_test_python$PYTHON_VERSION" --cache_test_results=no --test_output=all)

  # Build wheel
  cp /tmp/embag/bazel-bin/python/libembag.so /tmp/pip_build/embag
  (cd /tmp/pip_build && "$PYTHON_PATH/python" setup.py bdist_wheel &&
    auditwheel repair /tmp/pip_build/dist/embag*.whl --plat manylinux2014_x86_64 &&
    "$PYTHON_PATH/pip" install wheelhouse/embag*.whl &&
    "$PYTHON_PATH/python" -c 'import embag; embag.View(); print("Successfully loaded embag!")' &&
    cp wheelhouse/* /tmp/out &&
    rm wheelhouse/* &&
    rm -rf build dist)
}

# Build embag for Python 2 (soon to be deprecated)
build "/usr/bin" 2

# Build embag for various version of Python 3
for version in cp35-cp35m \
  cp36-cp36m \
  cp37-cp37m \
  cp38-cp38 \
  cp39-cp39; do
  # Link the correct version of python
  ln -sf /opt/python/$version/bin/python /usr/bin/python3

  build "/opt/python/$version/bin" 3
done
