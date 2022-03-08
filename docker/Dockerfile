# Builds a Docker image that contains tfd, and that will run the GSP
# process if executed.

FROM xaya/charon AS build
RUN apk add --no-cache \
  autoconf \
  autoconf-archive \
  automake \
  build-base \
  cmake \
  gflags-dev \
  git \
  libtool \
  pkgconfig

# Build and install the Google benchmark library from source.
WORKDIR /usr/src/benchmark
RUN git clone https://github.com/google/benchmark .
RUN git clone https://github.com/google/googletest
RUN cmake . && make && make install/strip

# Build and install tfd.
WORKDIR /usr/src/tfgsp
COPY . .
RUN make distclean || true
RUN ./autogen.sh && ./configure && make && make install-strip

# Collect the binary and required libraries.  Add in Charon.
WORKDIR /jail
RUN mkdir bin && cp /usr/local/bin/tfd /usr/local/bin/charon-* bin/
RUN for b in bin/*; do cpld $b lib64; done

# Add extra files needed (like the RPC method spec loaded with the
# Charon binaries).
RUN mkdir share \
  && cp /usr/local/share/tf-rpc.json share/ \
  && cp /usr/local/share/charon/letsencrypt.pem share/
COPY docker/entrypoint.sh bin/

# Construct final image.
FROM alpine
COPY --from=build /jail /usr/local/
ENV LD_LIBRARY_PATH "/usr/local/lib64"
ENV XAYAGAME_DIR "/xayagame"
LABEL description="Treatfighter game-state processor with Charon"
VOLUME ["/log"]
ENV GLOG_log_dir "/log"
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
