FROM quay.io/pypa/manylinux2014_x86_64

RUN yum install npm git -y -q && npm install -g @bazel/bazelisk

RUN mkdir -p /tmp/embag /tmp/pip_build /tmp/out
COPY WORKSPACE /tmp/embag
COPY lib /tmp/embag/lib
COPY python /tmp/embag/python
COPY embag_echo /tmp/embag/embag_echo
COPY lz4 /tmp/embag/lz4
COPY bz2 /tmp/embag/bz2
COPY test /tmp/embag/test
COPY pip_package README.md LICENSE /tmp/pip_build/

RUN /tmp/pip_build/build.sh
