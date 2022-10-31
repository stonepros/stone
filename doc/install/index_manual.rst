.. _install-manual:

=======================
 Installation (Manual)
=======================


Get Software
============

There are several methods for getting Stone software. The easiest and most common
method is to `get packages`_ by adding repositories for use with package
management tools such as the Advanced Package Tool (APT) or Yellowdog Updater,
Modified (YUM). You may also retrieve pre-compiled packages from the Stone
repository. Finally, you can retrieve tarballs or clone the Stone source code
repository and build Stone yourself.


.. toctree::
   :maxdepth: 1

	Get Packages <get-packages>
	Get Tarballs <get-tarballs>
	Clone Source <clone-source>
	Build Stone <build-ceph>
    	Stone Mirrors <mirrors>
	Stone Containers <containers>


Install Software
================

Once you have the Stone software (or added repositories), installing the software
is easy. To install packages on each :term:`Stone Node` in your cluster. You may
use  ``cephadm`` to install Stone for your storage cluster, or use package
management tools. You should install Yum Priorities for RHEL/CentOS and other
distributions that use Yum if you intend to install the Stone Object Gateway or
QEMU.

.. toctree::
   :maxdepth: 1

	Install cephadm <../cephadm/install>
    	Install Stone Storage Cluster <install-storage-cluster>
	Install Virtualization for Block <install-vm-cloud>


Deploy a Cluster Manually
=========================

Once you have Stone installed on your nodes, you can deploy a cluster manually.
The manual procedure is primarily for exemplary purposes for those developing
deployment scripts with Chef, Juju, Puppet, etc.

.. toctree::

	Manual Deployment <manual-deployment>
	Manual Deployment on FreeBSD <manual-freebsd-deployment>

Upgrade Software
================

As new versions of Stone become available, you may upgrade your cluster to take
advantage of new functionality. Read the upgrade documentation before you
upgrade your cluster. Sometimes upgrading Stone requires you to follow an upgrade
sequence.

.. toctree::
   :maxdepth: 2

.. _get packages: ../get-packages
