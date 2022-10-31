=============
iSCSI Targets
=============

Traditionally, block-level access to a Stone storage cluster has been
limited to QEMU and ``librbd``, which is a key enabler for adoption
within OpenStack environments. Starting with the Stone Luminous release,
block-level access is expanding to offer standard iSCSI support allowing
wider platform usage, and potentially opening new use cases.

-  Red Hat Enterprise Linux/CentOS 7.5 (or newer); Linux kernel v4.16 (or newer)

-  A working Stone Storage cluster, deployed with ``ceph-ansible`` or using the command-line interface

-  iSCSI gateways nodes, which can either be colocated with OSD nodes or on dedicated nodes

-  Separate network subnets for iSCSI front-end traffic and Stone back-end traffic

A choice of using Ansible or the command-line interface are the
available deployment methods for installing and configuring the Stone
iSCSI gateway:

.. toctree::
  :maxdepth: 1

  Using Ansible <iscsi-target-ansible>
  Using the Command Line Interface <iscsi-target-cli>
