.. _cephadm:

=======
Stoneadm
=======

``cephadm`` deploys and manages a Stone cluster. It does this by connecting the
manager daemon to hosts via SSH. The manager daemon is able to add, remove, and
update Stone containers. ``cephadm`` does not rely on external configuration
tools such as Ansible, Rook, and Salt.

``cephadm`` manages the full lifecycle of a Stone cluster. This lifecycle
starts with the bootstrapping process, when ``cephadm`` creates a tiny
Stone cluster on a single node. This cluster consists of one monitor and
one manager. ``cephadm`` then uses the orchestration interface ("day 2"
commands) to expand the cluster, adding all hosts and provisioning all
Stone daemons and services. Management of this lifecycle can be performed
either via the Stone command-line interface (CLI) or via the dashboard (GUI).

``cephadm`` is new in Stone release v15.2.0 (Octopus) and does not support older
versions of Stone.

.. toctree::
    :maxdepth: 2

    compatibility
    install
    adoption
    host-management
    Service Management <services/index>
    upgrade
    Stoneadm operations <operations>
    Client Setup <client-setup>
    troubleshooting
    Stoneadm Feature Planning <../dev/cephadm/index>
