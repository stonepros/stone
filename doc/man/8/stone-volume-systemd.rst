:orphan:

=======================================================
 stone-volume-systemd -- systemd stone-volume helper tool
=======================================================

.. program:: stone-volume-systemd

Synopsis
========

| **stone-volume-systemd** *systemd instance name*


Description
===========
:program:`stone-volume-systemd` is a systemd helper tool that receives input
from (dynamically created) systemd units so that activation of OSDs can
proceed.

It translates the input into a system call to stone-volume for activation
purposes only.


Examples
========
Its input is the ``systemd instance name`` (represented by ``%i`` in a systemd
unit), and it should be in the following format::

    <stone-volume subcommand>-<extra metadata>

In the case of ``lvm`` a call could look like::

    /usr/bin/stone-volume-systemd lvm-0-8715BEB4-15C5-49DE-BA6F-401086EC7B41

Which in turn will call ``stone-volume`` in the following way::

    stone-volume lvm trigger  0-8715BEB4-15C5-49DE-BA6F-401086EC7B41

Any other subcommand will need to have implemented a ``trigger`` command that
can consume the extra metadata in this format.


Availability
============

:program:`stone-volume-systemd` is part of Stone, a massively scalable,
open-source, distributed storage system. Please refer to the documentation at
http://docs.stone.com/ for more information.


See also
========

:doc:`stone-osd <stone-osd>`\(8),
