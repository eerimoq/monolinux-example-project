|buildstatus|_

Monolinux example project
=========================

A `Monolinux`_ example project using QEMU.

Build and run
=============

Run the commands below to create a file system, build the Linux kernel
and run everything in QEMU.

.. code-block:: shell

   $ ./rundocker.sh
   $ make -j4 run

Exit QEMU with Ctrl-A X.

.. |buildstatus| image:: https://travis-ci.org/eerimoq/monolinux-example-project.svg
.. _buildstatus: https://travis-ci.org/eerimoq/monolinux-example-project

.. _Monolinux: https://github.com/eerimoq/monolinux
