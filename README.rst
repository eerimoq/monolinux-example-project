|buildstatus|_

Monolinux example project
=========================

An Monolinux example project.

Build and run
=============

Install all prerequisites:

.. code-block:: shell

   $ sudo apt install curl qemu-system-x86 flex bison libelf-dev
   $ wget https://musl.cc/x86_64-linux-musl-cross.tgz
   $ tar xf x86_64-linux-musl-cross.tgz

Source the development environment setup script.

.. code-block:: shell

   $ source setup.sh

Run the commands below to create a file system, build the Linux kernel
and run everything in QEMU.

.. code-block:: shell

   $ make -C examples/hello_world run
   ...
   Hello world!
   Hello world!
   Hello world!
   ...

Exit QEMU with Ctrl-A C and then q <Enter>.

File tree
=========

Project files and folders.

.. code-block:: text

   monolinux-example-project/   - this repository
   ├── configs/                 - a few Linux kernel configs
   ├── examples/                - example applications
   ├── LICENSE                  - license
   ├── make/                    - build system
   │   └── packages/            - packages build specifications
   ├── ml/                      - the Monolinux C library
   └── setup.sh                 - development environment setup script

And after build.

.. code-block:: text

   monolinux-example-project/
   ├── app/
   │   ├── build/                   - all build output
   │   │   ├── app                  - the one and only executable
   │   │   ├── initramfs/           - unpacked ramfs
   │   │   ├── initramfs.cpio       - packed ramfs
   │   │   ├── linux/               - Linux source and build output
   │   │   ├── packages/            - packages source and objects
   │   │   │   └── curl/
   │   │   └── root/                - headers and libraries container
   │   │       ├── bin/
   │   │       ├── include/         - include files
   │   │       │   └── curl/
   │   │       │       └── curl.h
   │   │       ├── lib/             - static libraries
   │   │       │   └── libcurl.a
   │   │       └── share/
   │   ├── main.c
   │   └── Makefile
   ├── Makefile
   ├── monolinux/
   ├── README.rst
   └── setup.sh

.. |buildstatus| image:: https://travis-ci.org/eerimoq/monolinux-example-project.svg
.. _buildstatus: https://travis-ci.org/eerimoq/monolinux-example-project
