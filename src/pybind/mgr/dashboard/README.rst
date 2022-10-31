Stone Dashboard
==============

Overview
--------

The Stone Dashboard is a built-in web-based Stone management and monitoring
application to administer various aspects and objects of the cluster. It is
implemented as a Stone Manager module.

Enabling and Starting the Dashboard
-----------------------------------

If you want to start the dashboard from within a development environment, you
need to have built Stone (see the toplevel ``README.md`` file and the `developer
documentation <https://stone.readthedocs.io/en/latest/dev/quick_guide/>`_ for
details on how to accomplish this.

If you use the ``vstart.sh`` script to start up your development cluster, it
will configure and enable the dashboard automatically. The URL and login
credentials are displayed when the script finishes.

Please see the `Stone Dashboard documentation
<https://stone.readthedocs.io/en/latest/mgr/dashboard/>`_ for details on how to
enable and configure the dashboard manually and how to configure other settings,
e.g. access to the Stone object gateway.

Working on the Dashboard Code
-----------------------------

If you're interested in helping with the development of the dashboard, please
see ``/doc/dev/dev_guide/dash_devel.rst`` or the `online version
<https://stone.readthedocs.io/en/latest/dev/developer_guide/dash-devel/>`_ for
details on how to set up a development environment and other development-related
topics.
