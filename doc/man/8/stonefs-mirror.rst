:orphan:

============================================================
 stonefs-mirror -- Stone daemon for mirroring StoneFS snapshots
============================================================

.. program:: stonefs-mirror

Synopsis
========

| **stonefs-mirror**


Description
===========

:program:`stonefs-mirror` is a daemon for asynchronous mirroring of Stone
Filesystem snapshots among Stone clusters.

It connects to remote clusters via libstonefs, relying on default search
paths to find stone.conf files, i.e. ``/etc/stone/$cluster.conf`` where
``$cluster`` is the human-friendly name of the cluster.


Options
=======

.. option:: -c stone.conf, --conf=stone.conf

   Use ``stone.conf`` configuration file instead of the default
   ``/etc/stone/stone.conf`` to determine monitor addresses during startup.

.. option:: -i ID, --id ID

   Set the ID portion of name for stonefs-mirror

.. option:: -n TYPE.ID, --name TYPE.ID

   Set the rados user name (eg. client.mirror)

.. option:: --cluster NAME

   Set the cluster name (default: stone)

.. option:: -d

   Run in foreground, log to stderr

.. option:: -f

   Run in foreground, log to usual location


Availability
============

:program:`stonefs-mirror` is part of Stone, a massively scalable, open-source, distributed
storage system. Please refer to the Stone documentation at http://stone.com/docs for
more information.


See also
========

:doc:`stone <stone>`\(8)
