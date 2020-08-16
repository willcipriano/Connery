FROM debian:buster
RUN apt-get update
RUN apt-get -y install build-essential
COPY . .