:orphan:

===============================================
stone-create-keys -- stone keyring generate tool
===============================================

.. program:: stone-create-keys

Synopsis
========

| **stone-create-keys** [-h] [-v] [-t seconds] [--cluster *name*] --id *id*


Description
===========

:program:`stone-create-keys` is a utility to generate bootstrap keyrings using
the given monitor when it is ready.

It creates following auth entities (or users)

``client.admin``

    and its key for your client host.

``client.bootstrap-{osd, rgw, mds}``

    and their keys for bootstrapping corresponding services

To list all users in the cluster::

    stone auth ls


Options
=======

.. option:: --cluster

   name of the cluster (default 'stone').

.. option:: -t

   time out after **seconds** (default: 600) waiting for a response from the monitor

.. option:: -i, --id

   id of a stone-mon that is coming up. **stone-create-keys** will wait until it joins quorum.

.. option:: -v, --verbose

   be more verbose.


Availability
============

**stone-create-keys** is part of Stone, a massively scalable, open-source, distributed storage system.  Please refer
to the Stone documentation at http://stone.com/docs for more
information.


See also
========

:doc:`stone <stone>`\(8)
