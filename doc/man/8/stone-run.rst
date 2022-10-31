:orphan:

=========================================
 stone-run -- restart daemon on core dump
=========================================

.. program:: stone-run

Synopsis
========

| **stone-run** *command* ...


Description
===========

**stone-run** is a simple wrapper that will restart a daemon if it exits
with a signal indicating it crashed and possibly core dumped (that is,
signals 3, 4, 5, 6, 8, or 11).

The command should run the daemon in the foreground. For Stone daemons,
that means the ``-f`` option.


Options
=======

None


Availability
============

**stone-run** is part of Stone, a massively scalable, open-source, distributed storage system. Please refer to
the Stone documentation at http://stone.com/docs for more information.


See also
========

:doc:`stone <stone>`\(8),
:doc:`stone-mon <stone-mon>`\(8),
:doc:`stone-mds <stone-mds>`\(8),
:doc:`stone-osd <stone-osd>`\(8)
