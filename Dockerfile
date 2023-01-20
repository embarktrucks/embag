FROM quay.io/pypa/manylinux2014_x86_64

ARG BAZEL_VERSION

ENV USE_BAZEL_VERSION=$BAZEL_VERSION

RUN yum install npm git python-devel python2-pip python3-pip gdb -y -q && \
    npm install -g npm@9.2.0 \
    npm install -g @bazel/bazelisk && \
    python -m pip install --upgrade "pip < 21.0" --user && \
    python -m pip install wheel --user && \
    python3 -m pip install --upgrade "pip < 21.0" --user && \
    python3 -m pip install wheel --user

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
