:orphan:

==========================================
 stonefs-top -- Stone Filesystem Top Utility
==========================================

.. program:: stonefs-top

Synopsis
========

| **stonefs-top** [flags]


Description
===========

**stonefs-top** provides top(1) like functionality for Stone Filesystem.
Various client metrics are displayed and updated in realtime.

Stone Metadata Servers periodically send client metrics to Stone Manager.
``Stats`` plugin in Stone Manager provides an interface to fetch these metrics.

Options
=======

.. option:: --cluster

   Cluster: Stone cluster to connect. Defaults to ``stone``.

.. option:: --id

   Id: Client used to connect to Stone cluster. Defaults to ``fstop``.

.. option:: --selftest

   Perform a selftest. This mode performs a sanity check of ``stats`` module.

Descriptions of fields
======================

.. describe:: chit

   cap hit rate

.. describe:: rlat

   read latency

.. describe:: wlat

   write latency

.. describe:: mlat

   metadata latency

.. describe:: dlease

   dentry lease rate

.. describe:: ofiles

   number of opened files

.. describe:: oicaps

   number of pinned caps

.. describe:: oinodes

   number of opened inodes

.. describe:: rtio

   total size of read IOs

.. describe:: wtio

   total size of write IOs

.. describe:: raio

   average size of read IOs

.. describe:: waio

   average size of write IOs

.. describe:: rsp

   speed of read IOs compared with the last refresh

.. describe:: wsp

   speed of write IOs compared with the last refresh


Availability
============

**stonefs-top** is part of Stone, a massively scalable, open-source, distributed storage system. Please refer to the Stone documentation at
http://stone.com/ for more information.


See also
========

:doc:`stone <stone>`\(8),
:doc:`stone-mds <stone-mds>`\(8)
