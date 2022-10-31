:orphan:

==================================================
 stone-post-file -- post files for stone developers
==================================================

.. program:: stone-post-file

Synopsis
========

| **stone-post-file** [-d *description] [-u *user*] *file or dir* ...


Description
===========

**stone-post-file** will upload files or directories to stone.com for
later analysis by Stone developers.

Each invocation uploads files or directories to a separate directory
with a unique tag.  That tag can be passed to a developer or
referenced in a bug report (http://tracker.stone.com/).  Once the
upload completes, the directory is marked non-readable and
non-writeable to prevent access or modification by other users.

Warning
=======

Basic measures are taken to make posted data be visible only to
developers with access to stone.com infrastructure. However, users
should think twice and/or take appropriate precautions before
posting potentially sensitive data (for example, logs or data
directories that contain Stone secrets).


Options
=======

.. option:: -d *description*, --description *description*

   Add a short description for the upload.  This is a good opportunity
   to reference a bug number.  There is no default value.

.. option:: -u *user*

   Set the user metadata for the upload.  This defaults to `whoami`@`hostname -f`.

Examples
========

To upload a single log::

   stone-post-file /var/log/stone/stone-mon.`hostname`.log

To upload several directories::

   stone-post-file -d 'mon data directories' /var/log/stone/mon/*


Availability
============

**stone-post-file** is part of Stone, a massively scalable, open-source, distributed storage system. Please refer to
the Stone documentation at http://stone.com/docs for more information.

See also
========

:doc:`stone <stone>`\(8),
:doc:`stone-debugpack <stone-debugpack>`\(8),
