:orphan:

==================================================
 stone-rbdnamer -- udev helper to name RBD devices
==================================================

.. program:: stone-rbdnamer


Synopsis
========

| **stone-rbdnamer** *num*


Description
===========

**stone-rbdnamer** prints the pool and image name for the given RBD devices
to stdout. It is used by `udev` (using a rule like the one below) to
set up a device symlink.


::

        KERNEL=="rbd[0-9]*", PROGRAM="/usr/bin/stone-rbdnamer %n", SYMLINK+="rbd/%c{1}/%c{2}"


Availability
============

**stone-rbdnamer** is part of Stone, a massively scalable, open-source, distributed storage system.  Please
refer to the Stone documentation at http://stone.com/docs for more
information.


See also
========

:doc:`rbd <rbd>`\(8),
:doc:`stone <stone>`\(8)
