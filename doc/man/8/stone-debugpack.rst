:orphan:

=============================================
 stone-debugpack -- stone debug packer utility
=============================================

.. program:: stone-debugpack

Synopsis
========

| **stone-debugpack** [ *options* ] *filename.tar.gz*


Description
===========

**stone-debugpack** will build a tarball containing various items that are
useful for debugging crashes. The resulting tarball can be shared with
Stone developers when debugging a problem.

The tarball will include the binaries for stone-mds, stone-osd, and stone-mon, radosgw, any
log files, the stone.conf configuration file, any core files we can
find, and (if the system is running) dumps of the current cluster state
as reported by 'stone report'.


Options
=======

.. option:: -c stone.conf, --conf=stone.conf

   Use *stone.conf* configuration file instead of the default
   ``/etc/stone/stone.conf`` to determine monitor addresses during
   startup.


Availability
============

**stone-debugpack** is part of Stone, a massively scalable, open-source, distributed storage system. Please
refer to the Stone documentation at http://stone.com/docs for more
information.


See also
========

:doc:`stone <stone>`\(8)
:doc:`stone-post-file <stone-post-file>`\(8)
