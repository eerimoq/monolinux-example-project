FROM ubuntu:20.04

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
    python3-pip \
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

ENV LC_ALL C.UTF-8
ENV LANG C.UTF-8

RUN pip3 install \
    pexpect \
    mqttools==0.39.1 \
    pbtools \
    messi \
    systest==5.9.0 \
    bunga==0.42.0

ENV HOSTNAME=monolinux-example-project