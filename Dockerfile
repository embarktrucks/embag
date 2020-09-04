FROM quay.io/pypa/manylinux2014_x86_64

RUN yum install npm lz4-devel git -y
RUN npm install -g @bazel/bazelisk

RUN mkdir -p /tmp/embag /tmp/pip_build
COPY WORKSPACE /tmp/embag
ADD lib /tmp/embag/lib
ADD python /tmp/embag/python
ADD embag_echo /tmp/embag/embag_echo
ADD lz4 /tmp/embag/lz4
ADD test /tmp/embag/test

WORKDIR /tmp/embag
RUN PYTHON_BIN_PATH=/opt/python/cp37-cp37m/bin/python bazel build //python:libembag.so //embag_echo:embag_echo
RUN ln -s /opt/python/cp37-cp37m/bin/python /usr/bin/python3
RUN bazel test test:* --test_output=all

COPY pip_package /tmp/pip_build
RUN cp /tmp/embag/bazel-bin/python/libembag.so /tmp/pip_build/embag
WORKDIR /tmp/pip_build
RUN /opt/python/cp37-cp37m/bin/python setup.py bdist_wheel
RUN auditwheel repair /tmp/pip_build/dist/embag-0.0.19-cp37-cp37m-linux_x86_64.whl --plat manylinux2014_x86_64
RUN /opt/python/cp37-cp37m/bin/pip install wheelhouse/embag-0.0.19-cp37-cp37m-manylinux2014_x86_64.whl
RUN /opt/python/cp37-cp37m/bin/python -c 'import embag; embag.View()'
