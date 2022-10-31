:orphan:

=================================
 stone-mon -- stone monitor daemon
=================================

.. program:: stone-mon

Synopsis
========

| **stone-mon** -i *monid* [ --mon-data *mondatapath* ]


Description
===========

**stone-mon** is the cluster monitor daemon for the Stone distributed
file system. One or more instances of **stone-mon** form a Paxos
part-time parliament cluster that provides extremely reliable and
durable storage of cluster membership, configuration, and state.

The *mondatapath* refers to a directory on a local file system storing
monitor data. It is normally specified via the ``mon data`` option in
the configuration file.

Options
=======

.. option:: -f, --foreground

   Foreground: do not daemonize after startup (run in foreground). Do
   not generate a pid file. Useful when run via :doc:`stone-run <stone-run>`\(8).

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

.. option:: --mkfs

   Initialize the ``mon data`` directory with seed information to form
   and initial stone file system or to join an existing monitor
   cluster.  Three pieces of information must be provided:

   - The cluster fsid.  This can come from a monmap (``--monmap <path>``) or
     explicitly via ``--fsid <uuid>``.
   - A list of monitors and their addresses.  This list of monitors
     can come from a monmap (``--monmap <path>``), the ``mon host``
     configuration value (in *stone.conf* or via ``-m
     host1,host2,...``), or (for backward compatibility) the deprecated ``mon addr`` lines in *stone.conf*.  If this
     monitor is to be part of the initial monitor quorum for a new
     Stone cluster, then it must be included in the initial list,
     matching either the name or address of a monitor in the list.
     When matching by address, either the ``public addr`` or ``public
     subnet`` options may be used.
   - The monitor secret key ``mon.``.  This must be included in the
     keyring provided via ``--keyring <path>``.

.. option:: --keyring

   Specify a keyring for use with ``--mkfs``.

.. option:: --no-config-file

    Signal that we don't want to rely on a *stone.conf*, either user provided
    or the default, to run the daemon.  This will entail providing all
    necessary options to the daemon as arguments.

Availability
============

**stone-mon** is part of Stone, a massively scalable, open-source, distributed storage system. Please refer
to the Stone documentation at http://stone.com/docs for more
information.


See also
========

:doc:`stone <stone>`\(8),
:doc:`stone-mds <stone-mds>`\(8),
:doc:`stone-osd <stone-osd>`\(8)
