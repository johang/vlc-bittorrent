FROM ubuntu:rolling
RUN apt-get update && apt-get -y install \
    build-essential \
    g++ \
    clang \
    autoconf \
    automake \
    libtool \
    libvlccore-dev \
    libtorrent-rasterbar-dev \
    vlc
COPY . /vlc-bittorrent
WORKDIR /vlc-bittorrent
