Alerts module
=============

The alerts module can send simple alert messages about cluster health
via e-mail.  In the future, it will support other notification methods
as well.

:note: This module is *not* intended to be a robust monitoring
       solution.  The fact that it is run as part of the Stone cluster
       itself is fundamentally limiting in that a failure of the
       stone-mgr daemon prevents alerts from being sent.  This module
       can, however, be useful for standalone clusters that exist in
       environments where existing monitoring infrastructure does not
       exist.

Enabling
--------

The *alerts* module is enabled with::

  stone mgr module enable alerts

Configuration
-------------

To configure SMTP, all of the following config options must be set::

  stone config set mgr mgr/alerts/smtp_host *<smtp-server>*
  stone config set mgr mgr/alerts/smtp_destination *<email-address-to-send-to>*
  stone config set mgr mgr/alerts/smtp_sender *<from-email-address>*

By default, the module will use SSL and port 465.  To change that,::

  stone config set mgr mgr/alerts/smtp_ssl false   # if not SSL
  stone config set mgr mgr/alerts/smtp_port *<port-number>*  # if not 465

To authenticate to the SMTP server, you must set the user and password::

  stone config set mgr mgr/alerts/smtp_user *<username>*
  stone config set mgr mgr/alerts/smtp_password *<password>*

By default, the name in the ``From:`` line is simply ``Stone``.  To
change that (e.g., to identify which cluster this is),::

  stone config set mgr mgr/alerts/smtp_from_name 'Stone Cluster Foo'

By default, the module will check the cluster health once per minute
and, if there is a change, send a message.  To change that
frequency,::

  stone config set mgr mgr/alerts/interval *<interval>*   # e.g., "5m" for 5 minutes

Commands
--------

To force an alert to be send immediately,::

  stone alerts send
