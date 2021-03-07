FROM debian:buster
RUN apt-get update
RUN apt-get -y install make
RUN apt-get -y install sudo
WORKDIR /Connery
COPY . .
RUN make -f Buildfile local.install
RUN cmake src/.
RUN make Connery
ENTRYPOINT "./Connery" && /bin/bash