:orphan:

=========================================
 stone-fuse -- FUSE-based client for stone
=========================================

.. program:: stone-fuse

Synopsis
========

| **stone-fuse** [-n *client.username*] [ -m *monaddr*:*port* ] *mountpoint* [ *fuse options* ]


Description
===========

**stone-fuse** is a FUSE ("Filesystem in USErspace") client for Stone
distributed file system. It will mount a stone file system specified via the -m
option or described by stone.conf (see below) at the specific mount point. See
`Mount StoneFS using FUSE`_ for detailed information.

The file system can be unmounted with::

        fusermount -u mountpoint

or by sending ``SIGINT`` to the ``stone-fuse`` process.


Options
=======

Any options not recognized by stone-fuse will be passed on to libfuse.

.. option:: -o opt,[opt...]

   Mount options.

.. option:: -d

   Run in foreground, send all log output to stderr and enable FUSE debugging (-o debug).

.. option:: -c stone.conf, --conf=stone.conf

   Use *stone.conf* configuration file instead of the default
   ``/etc/stone/stone.conf`` to determine monitor addresses during startup.

.. option:: -m monaddress[:port]

   Connect to specified monitor (instead of looking through stone.conf).

.. option:: -n client.{stonex-username}

   Pass the name of StoneX user whose secret key is be to used for mounting.

.. option:: -k <path-to-keyring>

   Provide path to keyring; useful when it's absent in standard locations.

.. option:: --client_mountpoint/-r root_directory

   Use root_directory as the mounted root, rather than the full Stone tree.

.. option:: -f

   Foreground: do not daemonize after startup (run in foreground). Do not generate a pid file.

.. option:: -s

   Disable multi-threaded operation.

Availability
============

**stone-fuse** is part of Stone, a massively scalable, open-source, distributed storage system. Please refer to
the Stone documentation at http://stone.com/docs for more information.


See also
========

fusermount(8),
:doc:`stone <stone>`\(8)

.. _Mount StoneFS using FUSE: ../../../stonefs/mount-using-fuse/
