  $ stone-conf -n osd.0 --show-config -c /dev/null | grep stone-osd
  admin_socket = /var/run/stone/stone-osd.0.asok
  log_file = /var/log/stone/stone-osd.0.log
  mon_debug_dump_location = /var/log/stone/stone-osd.0.tdump
  $ STONE_ARGS="--fsid 96a3abe6-7552-4635-a79b-f3c096ff8b95" stone-conf -n osd.0 --show-config -c /dev/null | grep fsid
  fsid = 96a3abe6-7552-4635-a79b-f3c096ff8b95
