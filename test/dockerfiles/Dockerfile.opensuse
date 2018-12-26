ARG IMAGE=opensuse/leap:latest
FROM $IMAGE
COPY . /vlc-bittorrent
WORKDIR /vlc-bittorrent
RUN zypper update -y && zypper install -y \
    autoconf \
    automake \
    libtool \
    make \
    file \
    clang \
    gcc \
    libboost_system*-devel \
    libtorrent-rasterbar-devel \
    vlc-devel
CMD autoreconf -i && ./configure && make && (make check || (cat test/*.log && false))
