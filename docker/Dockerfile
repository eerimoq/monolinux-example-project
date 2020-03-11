FROM ubuntu:18.04

ENV DEBIAN_FRONTEND=noninteractive

COPY x86_64-linux-musl-cross.tgz .
RUN tar xf x86_64-linux-musl-cross.tgz -C /opt

RUN apt-get update && \
    apt-get install -y \
    build-essential \
    qemu-system-x86 \
    rsync \
    flex \
    bison \
    bc \
    python3 \
    git \
    gitk \
    libelf-dev \
    gettext \
    autoconf \
    autogen \
    autopoint \
    libtool \
    cpio \
    bash-completion \
    libncurses5-dev

ENV HOSTNAME=monolinux-example-project