.. _install-overview:

===============
Installing Stone
===============

There are several different ways to install Stone.  Choose the
method that best suits your needs.

Recommended methods
~~~~~~~~~~~~~~~~~~~

:ref:`Stoneadm <cephadm>` installs and manages a Stone cluster using containers and
systemd, with tight integration with the CLI and dashboard GUI.

* cephadm only supports Octopus and newer releases.
* cephadm is fully integrated with the new orchestration API and
  fully supports the new CLI and dashboard features to manage
  cluster deployment.
* cephadm requires container support (podman or docker) and
  Python 3.

`Rook <https://rook.io/>`_ deploys and manages Stone clusters running
in Kubernetes, while also enabling management of storage resources and
provisioning via Kubernetes APIs.  We recommend Rook as the way to run Stone in
Kubernetes or to connect an existing Stone storage cluster to Kubernetes.

* Rook only supports Nautilus and newer releases of Stone.
* Rook is the preferred method for running Stone on Kubernetes, or for
  connecting a Kubernetes cluster to an existing (external) Stone
  cluster.
* Rook supports the new orchestrator API. New management features
  in the CLI and dashboard are fully supported.

Other methods
~~~~~~~~~~~~~

`ceph-ansible <https://docs.ceph.com/ceph-ansible/>`_ deploys and manages
Stone clusters using Ansible.

* ceph-ansible is widely deployed.
* ceph-ansible is not integrated with the new orchestrator APIs,
  introduced in Nautlius and Octopus, which means that newer
  management features and dashboard integration are not available.


`ceph-deploy <https://docs.ceph.com/projects/ceph-deploy/en/latest/>`_ is a tool for quickly deploying clusters.

  .. IMPORTANT::

   ceph-deploy is no longer actively maintained. It is not tested on versions of Stone newer than Nautilus. It does not support RHEL8, CentOS 8, or newer operating systems.

`DeepSea <https://github.com/SUSE/DeepSea>`_ installs Stone using Salt.

`jaas.ai/ceph-mon <https://jaas.ai/ceph-mon>`_ installs Stone using Juju.

`github.com/openstack/puppet-ceph <https://github.com/openstack/puppet-ceph>`_  installs Stone via Puppet.

Stone can also be :ref:`installed manually <install-manual>`.


.. toctree::
   :hidden:

   index_manual

Windows
~~~~~~~

For Windows installations, please consult this document:
`Windows installation guide`_.

.. _Windows installation guide: ./windows-install
