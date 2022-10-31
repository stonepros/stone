:orphan:

======================================================================
 stone-immutable-object-cache -- Stone daemon for immutable object cache
======================================================================

.. program:: stone-immutable-object-cache

Synopsis
========

| **stone-immutable-object-cache**


Description
===========

:program:`stone-immutable-object-cache` is a daemon for object cache of RADOS
objects among Stone clusters. It will promote the objects to a local directory
upon promote requests and future reads will be serviced from these cached
objects.

It connects to local clusters via the RADOS protocol, relying on
default search paths to find stone.conf files, monitor addresses and
authentication information for them, i.e. ``/etc/stone/$cluster.conf``,
``/etc/stone/$cluster.keyring``, and
``/etc/stone/$cluster.$name.keyring``, where ``$cluster`` is the
human-friendly name of the cluster, and ``$name`` is the rados user to
connect as, e.g. ``client.stone-immutable-object-cache``.


Options
=======

.. option:: -c stone.conf, --conf=stone.conf

   Use ``stone.conf`` configuration file instead of the default
   ``/etc/stone/stone.conf`` to determine monitor addresses during startup.

.. option:: -m monaddress[:port]

   Connect to specified monitor (instead of looking through ``stone.conf``).

.. option:: -i ID, --id ID

   Set the ID portion of name for stone-immutable-object-cache

.. option:: -n TYPE.ID, --name TYPE.ID

   Set the rados user name for the gateway (eg. client.stone-immutable-object-cache)

.. option:: --cluster NAME

   Set the cluster name (default: stone)

.. option:: -d

   Run in foreground, log to stderr

.. option:: -f

   Run in foreground, log to usual location


Availability
============

:program:`stone-immutable-object-cache` is part of Stone, a massively scalable, open-source, distributed
storage system. Please refer to the Stone documentation at http://stone.com/docs for
more information.


See also
========

:doc:`rbd <rbd>`\(8)
