|buildstatus|_

Monolinux example project
=========================

An `Monolinux`_ example project using QEMU.

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
   ├── 3pp/                     - third party sources
   ├── configs/                 - Linux kernel configuration
   ├── examples/                - example applications
   ├── LICENSE                  - license
   └── setup.sh                 - development environment setup script

.. |buildstatus| image:: https://travis-ci.org/eerimoq/monolinux-example-project.svg
.. _buildstatus: https://travis-ci.org/eerimoq/monolinux-example-project

.. _Monolinux: https://github.com/eerimoq/monolinux
