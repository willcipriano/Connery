FROM debian:buster
WORKDIR /Connery
COPY . .
RUN apt-get update
RUN apt-get -y install make
RUN apt-get -y install sudo
RUN make -f Buildfile local.install
RUN make -f Buildfile local.build
ENTRYPOINT make -f Buildfile local.run