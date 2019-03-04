ARG IMAGE=ubuntu:latest
FROM $IMAGE
COPY . /vlc-bittorrent
WORKDIR /vlc-bittorrent
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get -y install \
    autoconf \
    automake \
    build-essential \
    clang \
    g++ \
    libtool \
    libtorrent-rasterbar-dev \
    libvlccore-dev \
    libvlc-dev \
    vlc
CMD autoreconf -i && ./configure && make && (make check || (cat test/*.log && false))
