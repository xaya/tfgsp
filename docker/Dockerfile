# Builds a Docker image that contains tfd and runs the Treatfighter GSP
# on the modern XayaX / Polygon stack (no Charon).
#
# xaya/libxayagame is a Debian-based image with all Xaya libraries
# (libxayagame, libxayautil, ...) and their dependencies pre-installed.
#
# Pinned by DIGEST, not the mutable :latest tag, for reproducible/auditable
# builds.  This is tag 6a6f0a2665... (pushed 2026-07-05).  It carries the
# WAL-autocheckpoint disable — sqlite3_wal_autocheckpoint(db, 0) — that keeps
# SQLITE_BUSY from ever reaching the fatal CHECK in sqlite3_snapshot_open()
# (the crash we hit on the fork), plus the SnapshotGate locking-gap fix and the
# getnullstate unblock.  NOTE: Docker Hub's :latest is stale (Mar 2026) and is a
# pre-amend snapshot that SHIPS THAT CRASH WITHOUT THE GUARD — do not use it.
# Bumping the base is deliberate: `make check` (golden replay) is the gate; if
# the golden goes red after a bump, that is a real consensus change to
# investigate — never GOLDEN_REGEN it away.
FROM xaya/libxayagame@sha256:56f0b3b28ea58350966b4248f0c7d62aea22c533c000071795361ef1a99d8dc7 AS build

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
# Git revision of the source that produced this image, passed by the build gate
# (`docker build --build-arg GIT_REV=$(git rev-parse HEAD)`) and surfaced as the
# standard OCI revision label for traceability.  Defaults to "unknown".
ARG GIT_REV=unknown
LABEL org.opencontainers.image.revision="${GIT_REV}"
RUN mkdir -p /log /xayagame
# Run the GSP as an unprivileged user (audit P2-23).  tfd needs no root
# capabilities: it only binds an unprivileged port (8600) and writes its
# SQLite datadir (/xayagame) and glog output (/log).  Both dirs are chowned
# to this uid BEFORE the VOLUME declaration so that a freshly created named
# volume inherits tfd ownership (Docker seeds an empty volume from the image
# dir's contents + permissions).
RUN groupadd -g 10001 tfd \
  && useradd -u 10001 -g 10001 -M -s /usr/sbin/nologin tfd \
  && chown -R tfd:tfd /log /xayagame
VOLUME ["/log"]
ENV GLOG_log_dir="/log"
USER tfd
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
