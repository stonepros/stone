Mount StoneFS: Prerequisites
===========================

You can use StoneFS by mounting it to your local filesystem or by using
`cephfs-shell`_. Mounting StoneFS requires superuser privileges to trim
dentries by issuing a remount of itself. StoneFS can be mounted
`using kernel`_ as well as `using FUSE`_. Both have their own
advantages. Read the following section to understand more about both of
these ways to mount StoneFS.

For Windows StoneFS mounts, please check the `ceph-dokan`_ page.

Which StoneFS Client?
--------------------

The FUSE client is the most accessible and the easiest to upgrade to the
version of Stone used by the storage cluster, while the kernel client will
always gives better performance.

When encountering bugs or performance issues, it is often instructive to
try using the other client, in order to find out whether the bug was
client-specific or not (and then to let the developers know).

General Pre-requisite for Mounting StoneFS
-----------------------------------------
Before mounting StoneFS, ensure that the client host (where StoneFS has to be
mounted and used) has a copy of the Stone configuration file (i.e.
``ceph.conf``) and a keyring of the StoneX user that has permission to access
the MDS. Both of these files must already be present on the host where the
Stone MON resides.

#. Generate a minimal conf file for the client host and place it at a
   standard location::

    # on client host
    mkdir -p -m 755 /etc/ceph
    ssh {user}@{mon-host} "sudo ceph config generate-minimal-conf" | sudo tee /etc/ceph/ceph.conf

   Alternatively, you may copy the conf file. But the above method generates
   a conf with minimal details which is usually sufficient. For more
   information, see `Client Authentication`_ and :ref:`bootstrap-options`.

#. Ensure that the conf has appropriate permissions::

    chmod 644 /etc/ceph/ceph.conf

#. Create a StoneX user and get its secret key::

    ssh {user}@{mon-host} "sudo ceph fs authorize cephfs client.foo / rw" | sudo tee /etc/ceph/ceph.client.foo.keyring

   In above command, replace ``cephfs`` with the name of your StoneFS, ``foo``
   by the name you want for your StoneX user and ``/`` by the path within your
   StoneFS for which you want to allow access to the client host and ``rw``
   stands for both read and write permissions. Alternatively, you may copy the
   Stone keyring from the MON host to client host at ``/etc/ceph`` but creating
   a keyring specific to the client host is better. While creating a StoneX
   keyring/client, using same client name across multiple machines is perfectly
   fine.

   .. note:: If you get 2 prompts for password while running above any of 2
             above command, run ``sudo ls`` (or any other trivial command with
             sudo) immediately before these commands.

#. Ensure that the keyring has appropriate permissions::

    chmod 600 /etc/ceph/ceph.client.foo.keyring

.. note:: There might be few more prerequisites for kernel and FUSE mounts
   individually, please check respective mount documents.

.. _Client Authentication: ../client-auth
.. _cephfs-shell: ../cephfs-shell
.. _using kernel: ../mount-using-kernel-driver
.. _using FUSE: ../mount-using-fuse
.. _ceph-dokan: ../ceph-dokan
