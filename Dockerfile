FROM ubuntu:20.04 AS runtime
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
  libcurlpp0 \
  libjsoncpp1 \
  libzmq5

FROM runtime AS build
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
  build-essential \
  cmake \
  libboost-test-dev \
  libcurl4-openssl-dev \
  libcurlpp-dev \
  libjsoncpp-dev \
  libzmq3-dev \
  pkg-config    

#RUN mkdir src

COPY . /src/
WORKDIR /src

RUN mkdir build \
  && cd build \
  && cmake -DCMAKE_PREFIX_PATH=../deps/cppzmq .. \
  && make

FROM runtime as login_server
COPY --from=build /src/build/login_server/login_server .
ENTRYPOINT ./login_server

FROM runtime as peer
COPY --from=build /src/build/peer/peer .
ENTRYPOINT ["./peer"]

FROM runtime as tests
RUN apt-get update && apt-get install -y \
  cmake \
  make
COPY --from=build /src/build/tests/*_test /src/build/tests/CTestTestfile.cmake ./tests/
COPY --from=build /src/build/Makefile /src/build/CTestTestfile.cmake ./
ENTRYPOINT ["make", "test"]
