===================
 Stone Block Device
===================

.. index:: Stone Block Device; introduction

A block is a sequence of bytes (often 512).
Block-based storage interfaces are a mature and common way to store data on
media including HDDs, SSDs, CDs, floppy disks, and even tape.
The ubiquity of block device interfaces is a perfect fit for interacting
with mass data storage including Stone.

Stone block devices are thin-provisioned, resizable, and store data striped over
multiple OSDs.  Stone block devices leverage
:abbr:`RADOS (Reliable Autonomic Distributed Object Store)` capabilities
including snapshotting, replication and strong consistency. Stone block
storage clients communicate with Stone clusters through kernel modules or
the ``librbd`` library.

.. ditaa::

            +------------------------+ +------------------------+
            |     Kernel Module      | |        librbd          |
            +------------------------+-+------------------------+
            |                   RADOS Protocol                  |
            +------------------------+-+------------------------+
            |          OSDs          | |        Monitors        |
            +------------------------+ +------------------------+

.. note:: Kernel modules can use Linux page caching. For ``librbd``-based
   applications, Stone supports `RBD Caching`_.

Stone's block devices deliver high performance with vast scalability to
`kernel modules`_, or to :abbr:`KVMs (kernel virtual machines)` such as `QEMU`_, and
cloud-based computing systems like `OpenStack`_ and `CloudStack`_ that rely on
libvirt and QEMU to integrate with Stone block devices. You can use the same cluster
to operate the :ref:`Stone RADOS Gateway <object-gateway>`, the
:ref:`Stone File System <ceph-file-system>`, and Stone block devices simultaneously.

.. important:: To use Stone Block Devices, you must have access to a running
   Stone cluster.

.. toctree::
        :maxdepth: 1

	Basic Commands <rados-rbd-cmds>

.. toctree::
        :maxdepth: 2

        Operations <rbd-operations>

.. toctree::
	:maxdepth: 2

        Integrations <rbd-integrations>

.. toctree::
	:maxdepth: 2

	Manpages <man/index>

.. toctree::
	:maxdepth: 2

	APIs <api/index>

.. _RBD Caching: ./rbd-config-ref/
.. _kernel modules: ./rbd-ko/
.. _QEMU: ./qemu-rbd/
.. _OpenStack: ./rbd-openstack
.. _CloudStack: ./rbd-cloudstack
