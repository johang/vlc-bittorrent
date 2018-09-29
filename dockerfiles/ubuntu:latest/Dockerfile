FROM ubuntu:latest
RUN apt-get update && apt-get -y install \
    build-essential \
    g++ \
    clang \
    autoconf \
    automake \
    libtool \
    libvlccore-dev \
    libvlc-dev \
    libtorrent-rasterbar-dev \
    vlc
COPY . /vlc-bittorrent
WORKDIR /vlc-bittorrent
