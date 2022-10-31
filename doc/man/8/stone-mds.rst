:orphan:

=========================================
 stone-mds -- stone metadata server daemon
=========================================

.. program:: stone-mds

Synopsis
========

| **stone-mds** -i <*ID*> [flags]


Description
===========

**stone-mds** is the metadata server daemon for the Stone distributed file
system. One or more instances of stone-mds collectively manage the file
system namespace, coordinating access to the shared OSD cluster.

Each stone-mds daemon instance should have a unique name. The name is used
to identify daemon instances in the stone.conf.

Once the daemon has started, the monitor cluster will normally assign
it a logical rank, or put it in a standby pool to take over for
another daemon that crashes. Some of the specified options can cause
other behaviors.


Options
=======

.. option:: -f, --foreground

   Foreground: do not daemonize after startup (run in foreground). Do
   not generate a pid file. Useful when run via :doc:`stone-run
   <stone-run>`\(8).

.. option:: -d

   Debug mode: like ``-f``, but also send all log output to stderr.

.. option:: --setuser userorgid

   Set uid after starting.  If a username is specified, the user
   record is looked up to get a uid and a gid, and the gid is also set
   as well, unless --setgroup is also specified.

.. option:: --setgroup grouporgid

   Set gid after starting.  If a group name is specified the group
   record is looked up to get a gid.

.. option:: -c stone.conf, --conf=stone.conf

   Use *stone.conf* configuration file instead of the default
   ``/etc/stone/stone.conf`` to determine monitor addresses during
   startup.

.. option:: -m monaddress[:port]

   Connect to specified monitor (instead of looking through
   ``stone.conf``).

.. option:: --id/-i ID

   Set ID portion of the MDS name.

Availability
============

**stone-mds** is part of Stone, a massively scalable, open-source, distributed storage system. Please refer to the Stone documentation at
http://stone.com/docs for more information.


See also
========

:doc:`stone <stone>`\(8),
:doc:`stone-mon <stone-mon>`\(8),
:doc:`stone-osd <stone-osd>`\(8)
