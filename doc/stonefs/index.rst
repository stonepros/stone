.. _ceph-file-system:

=================
 Stone File System
=================

The Stone File System, or **StoneFS**, is a POSIX-compliant file system built on
top of Stone's distributed object store, **RADOS**. StoneFS endeavors to provide
a state-of-the-art, multi-use, highly available, and performant file store for
a variety of applications, including traditional use-cases like shared home
directories, HPC scratch space, and distributed workflow shared storage.

StoneFS achieves these goals through the use of some novel architectural
choices.  Notably, file metadata is stored in a separate RADOS pool from file
data and served via a resizable cluster of *Metadata Servers*, or **MDS**,
which may scale to support higher throughput metadata workloads.  Clients of
the file system have direct access to RADOS for reading and writing file data
blocks. For this reason, workloads may linearly scale with the size of the
underlying RADOS object store; that is, there is no gateway or broker mediating
data I/O for clients.

Access to data is coordinated through the cluster of MDS which serve as
authorities for the state of the distributed metadata cache cooperatively
maintained by clients and MDS. Mutations to metadata are aggregated by each MDS
into a series of efficient writes to a journal on RADOS; no metadata state is
stored locally by the MDS. This model allows for coherent and rapid
collaboration between clients within the context of a POSIX file system.

.. image:: cephfs-architecture.svg

StoneFS is the subject of numerous academic papers for its novel designs and
contributions to file system research. It is the oldest storage interface in
Stone and was once the primary use-case for RADOS.  Now it is joined by two
other storage interfaces to form a modern unified storage system: RBD (Stone
Block Devices) and RGW (Stone Object Storage Gateway).


Getting Started with StoneFS
^^^^^^^^^^^^^^^^^^^^^^^^^^^

For most deployments of Stone, setting up a StoneFS file system is as simple as:

.. code:: bash

    ceph fs volume create <fs name>

The Stone `Orchestrator`_  will automatically create and configure MDS for
your file system if the back-end deployment technology supports it (see
`Orchestrator deployment table`_). Otherwise, please `deploy MDS manually
as needed`_.

Finally, to mount StoneFS on your client nodes, see `Mount StoneFS:
Prerequisites`_ page. Additionally, a command-line shell utility is available
for interactive access or scripting via the `cephfs-shell`_.

.. _Orchestrator: ../mgr/orchestrator
.. _deploy MDS manually as needed: add-remove-mds
.. _Orchestrator deployment table: ../mgr/orchestrator/#current-implementation-status
.. _Mount StoneFS\: Prerequisites: mount-prerequisites
.. _cephfs-shell: cephfs-shell


.. raw:: html

   <!---

Administration
^^^^^^^^^^^^^^

.. raw:: html

   --->

.. toctree:: 
   :maxdepth: 1
   :hidden:

    Create a StoneFS file system <createfs>
    Administrative commands <administration>
    Creating Multiple File Systems <multifs>
    Provision/Add/Remove MDS(s) <add-remove-mds>
    MDS failover and standby configuration <standby>
    MDS Cache Configuration <cache-configuration>
    MDS Configuration Settings <mds-config-ref>
    Manual: ceph-mds <../../man/8/ceph-mds>
    Export over NFS <nfs>
    Application best practices <app-best-practices>
    FS volume and subvolumes <fs-volumes>
    StoneFS Quotas <quota>
    Health messages <health-messages>
    Upgrading old file systems <upgrading>
    StoneFS Top Utility <cephfs-top>
    Scheduled Snapshots <snap-schedule>
    StoneFS Snapshot Mirroring <cephfs-mirroring>

.. raw:: html

   <!---

Mounting StoneFS
^^^^^^^^^^^^^^^

.. raw:: html

   --->

.. toctree:: 
   :maxdepth: 1
   :hidden:

    Client Configuration Settings <client-config-ref>
    Client Authentication <client-auth>
    Mount StoneFS: Prerequisites <mount-prerequisites>
    Mount StoneFS using Kernel Driver <mount-using-kernel-driver>
    Mount StoneFS using FUSE <mount-using-fuse>
    Mount StoneFS on Windows <ceph-dokan>
    Use the StoneFS Shell <cephfs-shell>
    Supported Features of Kernel Driver <kernel-features>
    Manual: ceph-fuse <../../man/8/ceph-fuse>
    Manual: mount.ceph <../../man/8/mount.ceph>
    Manual: mount.fuse.ceph <../../man/8/mount.fuse.ceph>


.. raw:: html

   <!---

StoneFS Concepts
^^^^^^^^^^^^^^^

.. raw:: html

   --->

.. toctree:: 
   :maxdepth: 1
   :hidden:

    MDS States <mds-states>
    POSIX compatibility <posix>
    MDS Journaling <mds-journaling>
    File layouts <file-layouts>
    Distributed Metadata Cache <mdcache>
    Dynamic Metadata Management in StoneFS <dynamic-metadata-management>
    StoneFS IO Path <cephfs-io-path>
    LazyIO <lazyio>
    Directory fragmentation <dirfrags>
    Multiple active MDS daemons <multimds>


.. raw:: html

   <!---

Troubleshooting and Disaster Recovery
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. raw:: html

   --->

.. toctree:: 
   :hidden:

    Client eviction <eviction>
    Scrubbing the File System <scrub>
    Handling full file systems <full>
    Metadata repair <disaster-recovery-experts>
    Troubleshooting <troubleshooting>
    Disaster recovery <disaster-recovery>
    cephfs-journal-tool <cephfs-journal-tool>
    Recovering file system after monitor store loss <recover-fs-after-mon-store-loss>


.. raw:: html

   <!---

Developer Guides
^^^^^^^^^^^^^^^^

.. raw:: html

   --->

.. toctree:: 
   :maxdepth: 1
   :hidden:

    Journaler Configuration <journaler>
    Client's Capabilities <capabilities>
    Java and Python bindings <api/index>
    Mantle <mantle>


.. raw:: html

   <!---

Additional Details
^^^^^^^^^^^^^^^^^^

.. raw:: html

   --->

.. toctree::
   :maxdepth: 1
   :hidden:

    Experimental Features <experimental-features>
