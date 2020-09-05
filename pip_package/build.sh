#!/bin/bash -e

# Build embag for various version of Python
for version in cp35-cp35m \
               cp36-cp36m \
               cp37-cp37m \
               cp38-cp38 \
               cp39-cp39
  do
    # Link the correct version of python
    ln -sf /opt/python/$version/bin/python /usr/bin/python3
    # Build embag libs and echo test binary
    (cd /tmp/embag && \
     PYTHON_BIN_PATH=/opt/python/$version/bin/python bazel build //python:libembag.so //embag_echo:embag_echo && \
     bazel test test:* --test_output=all)

    # Build wheel
    cp /tmp/embag/bazel-bin/python/libembag.so /tmp/pip_build/embag
    (cd /tmp/pip_build && /opt/python/$version/bin/python setup.py bdist_wheel && \
     auditwheel repair /tmp/pip_build/dist/embag*.whl --plat manylinux2014_x86_64 && \
     /opt/python/$version/bin/pip install wheelhouse/embag*.whl && \
     /opt/python/$version/bin/python -c 'import embag; embag.View(); print("Successfully loaded embag!")' &&\
     cp wheelhouse/* /tmp/out && \
     rm wheelhouse/* && \
     rm -rf build dist)
  done

