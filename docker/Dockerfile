# Builds a Docker image that contains tfd and runs the Treatfighter GSP
# on the modern XayaX / Polygon stack (no Charon).
#
# xaya/libxayagame is a Debian-based image with all Xaya libraries
# (libxayagame, libxayautil, ...) and their dependencies pre-installed.

FROM xaya/libxayagame AS build

RUN apt-get update && apt-get -y install \
  autoconf \
  autoconf-archive \
  automake \
  build-essential \
  git \
  libtool \
  pkg-config \
  libbenchmark-dev \
  libgtest-dev \
  libgmock-dev \
  libgflags-dev \
  libgoogle-glog-dev \
  libjsoncpp-dev \
  libmicrohttpd-dev \
  libsqlite3-dev \
  libprotobuf-dev \
  protobuf-compiler \
  libgmp-dev

# Build and install tfd.
WORKDIR /usr/src/tfgsp
COPY . .
RUN make distclean || true
RUN ./autogen.sh && ./configure && make
# Run the full C++ suite as a build gate: unit tests, the golden-replay
# regression, and the reorg tests (in-process + production-path).  The live
# Polygon end-to-end + reorg tests are separate (polygon-test/, run against the
# forked-evm-testing harness), not part of the image build.
RUN make check || (cat src/test-suite.log proto2/test-suite.log database/test-suite.log 2>/dev/null; exit 1)
RUN make install-strip

# Collect the binary and its shared library dependencies into /jail
# (cpld is a helper script provided by the xaya/libxayagame image).
WORKDIR /jail
RUN mkdir -p bin lib64 \
  && cp /usr/local/bin/tfd bin/ \
  && cpld bin/tfd lib64

# Add extra files needed (the RPC method spec) and the entrypoint.
RUN mkdir share \
  && cp /usr/local/share/tf-rpc.json share/
COPY docker/entrypoint.sh bin/
RUN chmod +x bin/entrypoint.sh

# Construct final runtime image (must be glibc/Debian to match cpld'd libs).
FROM debian:13-slim
# curl is required by the docker-compose healthcheck (getnullstate probe).
RUN apt-get update && apt-get -y install --no-install-recommends curl \
  && rm -rf /var/lib/apt/lists/*
COPY --from=build /jail /usr/local/
ENV LD_LIBRARY_PATH="/usr/local/lib64"
ENV XAYAGAME_DIR="/xayagame"
LABEL description="Treatfighter game-state processor (XayaX/Polygon)"
RUN mkdir -p /log /xayagame
VOLUME ["/log"]
ENV GLOG_log_dir="/log"
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
