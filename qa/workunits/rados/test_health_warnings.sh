#!/usr/bin/env bash

set -uex

# number of osds = 10
crushtool -o crushmap --build --num_osds 10 host straw 2 rack straw 2 row straw 2 root straw 0
stone osd setcrushmap -i crushmap
stone osd tree
stone tell osd.* injectargs --osd_max_markdown_count 1024 --osd_max_markdown_period 1
stone osd set noout

wait_for_healthy() {
  while stone health | grep down
  do
    sleep 1
  done
}

test_mark_two_osds_same_host_down() {
  stone osd set noup
  stone osd down osd.0 osd.1
  stone health detail
  stone health | grep "1 host"
  stone health | grep "2 osds"
  stone health detail | grep "osd.0"
  stone health detail | grep "osd.1"
  stone osd unset noup
  wait_for_healthy
}

test_mark_two_osds_same_rack_down() {
  stone osd set noup
  stone osd down osd.8 osd.9
  stone health detail
  stone health | grep "1 host"
  stone health | grep "1 rack"
  stone health | grep "1 row"
  stone health | grep "2 osds"
  stone health detail | grep "osd.8"
  stone health detail | grep "osd.9"
  stone osd unset noup
  wait_for_healthy
}

test_mark_all_but_last_osds_down() {
  stone osd set noup
  stone osd down $(stone osd ls | sed \$d)
  stone health detail
  stone health | grep "1 row"
  stone health | grep "2 racks"
  stone health | grep "4 hosts"
  stone health | grep "9 osds"
  stone osd unset noup
  wait_for_healthy
}

test_mark_two_osds_same_host_down_with_classes() {
    stone osd set noup
    stone osd crush set-device-class ssd osd.0 osd.2 osd.4 osd.6 osd.8
    stone osd crush set-device-class hdd osd.1 osd.3 osd.5 osd.7 osd.9
    stone osd down osd.0 osd.1
    stone health detail
    stone health | grep "1 host"
    stone health | grep "2 osds"
    stone health detail | grep "osd.0"
    stone health detail | grep "osd.1"
    stone osd unset noup
    wait_for_healthy
}

test_mark_two_osds_same_host_down
test_mark_two_osds_same_rack_down
test_mark_all_but_last_osds_down
test_mark_two_osds_same_host_down_with_classes

exit 0
