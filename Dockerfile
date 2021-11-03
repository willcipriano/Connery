FROM debian:buster
RUN apt-get update
RUN apt-get -y install build-essential
RUN apt-get -y install cmake
RUN apt-get -y install libcurl4-openssl-dev
RUN apt-get -y install libedit-dev
RUN apt-get -y install libjson-c-dev
WORKDIR /Connery
COPY src/. .
RUN cmake .
RUN cmake --build .
ENTRYPOINT "./Connery" && /bin/bash