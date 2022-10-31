:orphan:

====================================================
 mount.fuse.stone -- mount stone-fuse from /etc/fstab.
====================================================

.. program:: mount.fuse.stone

Synopsis
========

| **mount.fuse.stone** [-h] [-o OPTIONS [*OPTIONS* ...]]
                      device [*device* ...]
                      mountpoint [*mountpoint* ...]

Description
===========

**mount.fuse.stone** is a helper for mounting stone-fuse from
``/etc/fstab``.

To use mount.fuse.stone, add an entry in ``/etc/fstab`` like::

  DEVICE    PATH        TYPE        OPTIONS
  none      /mnt/stone   fuse.stone   stone.id=admin,_netdev,defaults  0 0
  none      /mnt/stone   fuse.stone   stone.name=client.admin,_netdev,defaults  0 0
  none      /mnt/stone   fuse.stone   stone.id=myuser,stone.conf=/etc/stone/foo.conf,_netdev,defaults  0 0

stone-fuse options are specified in the ``OPTIONS`` column and must begin
with '``stone.``' prefix. This way stone related fs options will be passed to
stone-fuse and others will be ignored by stone-fuse.

Options
=======

.. option:: stone.id=<username>

   Specify that the stone-fuse will authenticate as the given user.

.. option:: stone.name=client.admin

   Specify that the stone-fuse will authenticate as client.admin

.. option:: stone.conf=/etc/stone/foo.conf

   Sets 'conf' option to /etc/stone/foo.conf via stone-fuse command line.


Any valid stone-fuse options can be passed this way.   

Additional Info
===============

The old format /etc/fstab entries are also supported::

  DEVICE                              PATH        TYPE        OPTIONS
  id=admin                            /mnt/stone   fuse.stone   defaults   0 0
  id=myuser,conf=/etc/stone/foo.conf   /mnt/stone   fuse.stone   defaults   0 0

Availability
============

**mount.fuse.stone** is part of Stone, a massively scalable, open-source, distributed storage system. Please
refer to the Stone documentation at http://stone.com/docs for more
information.

See also
========

:doc:`stone-fuse <stone-fuse>`\(8),
:doc:`stone <stone>`\(8)
