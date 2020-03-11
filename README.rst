|buildstatus|_

Monolinux example project
=========================

A `Monolinux`_ example project using QEMU.

Build and run
=============

With Docker
-----------

Run the commands below to create a file system, build the Linux kernel
and run everything in QEMU.

.. code-block:: shell

   $ ./rundocker.sh
   $ make -j4 run

Exit QEMU with Ctrl-A X.

Without Docker
--------------

Install all prerequisites:

.. code-block:: shell

   $ sudo apt install curl qemu-system-x86 flex bison libelf-dev gettext \
                      autoconf autogen
   $ tar xf docker/x86_64-linux-musl-cross.tgz

Source the development environment setup script.

.. code-block:: shell

   $ source setup.sh

Run the commands below to create a file system, build the Linux kernel
and run everything in QEMU.

.. code-block:: shell

   $ make -j4 run

Exit QEMU with Ctrl-A X.

.. |buildstatus| image:: https://travis-ci.org/eerimoq/monolinux-example-project.svg
.. _buildstatus: https://travis-ci.org/eerimoq/monolinux-example-project

.. _Monolinux: https://github.com/eerimoq/monolinux
