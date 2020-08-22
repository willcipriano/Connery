FROM debian:buster
RUN apt-get update
RUN apt-get -y install build-essential
RUN apt-get -y install cmake
RUN apt-get -y install libcurl4-openssl-dev
RUN apt-get -y install libedit-dev
WORKDIR /Connery
COPY src/. .
RUN cmake .
RUN cp /Connery/cmake-build-debug/Connery /Connery/Connery
ENTRYPOINT ./Connery