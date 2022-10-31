
# should reflect daemon defaults

  $ stone-conf -n osd.0 --show-config-value log_file -c /dev/null
  /var/log/stone/stone-osd.0.log
  $ STONE_ARGS="--fsid 96a3abe6-7552-4635-a79b-f3c096ff8b95" stone-conf -n osd.0 --show-config-value fsid -c /dev/null
  96a3abe6-7552-4635-a79b-f3c096ff8b95
  $ stone-conf -n osd.0 --show-config-value INVALID -c /dev/null
  failed to get config option 'INVALID': option not found
  [1]

  $ cat > $TESTDIR/stone.conf <<EOF
  > [global]
  >     mon_host = \$public_network
  >     public_network = \$mon_host
  > EOF
  $ stone-conf --show-config-value mon_host -c $TESTDIR/stone.conf
  variable expansion loop at mon_host=$public_network
  expansion stack:
  public_network=$mon_host
  mon_host=$public_network
  $mon_host
  $ rm $TESTDIR/stone.conf

Name option test to strip the PID
=================================
  $ cat > $TESTDIR/stone.conf <<EOF
  > [client]
  >     admin socket = \$name.\$pid.asok
  > [global]
  >     admin socket = \$name.asok
  > EOF
  $ stone-conf --name client.admin --pid 133423 --show-config-value admin_socket -c $TESTDIR/stone.conf
  client.admin.133423.asok
  $ stone-conf --name mds.a --show-config-value admin_socket -c $TESTDIR/stone.conf
  mds.a.asok
  $ stone-conf --name osd.0 --show-config-value admin_socket -c $TESTDIR/stone.conf
  osd.0.asok
  $ rm $TESTDIR/stone.conf
