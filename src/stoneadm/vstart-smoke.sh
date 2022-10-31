#!/bin/bash -ex

# this is a smoke test, meant to be run against vstart.sh.

host="$(hostname)"

bin/init-stone stop || true
MON=1 OSD=1 MDS=0 MGR=1 ../src/vstart.sh -d -n -x -l --stoneadm

export STONE_DEV=1

bin/stone orch ls
bin/stone orch apply mds foo 1
bin/stone orch ls | grep foo
while ! bin/stone orch ps | grep mds.foo ; do sleep 1 ; done
bin/stone orch ps

bin/stone orch host ls

bin/stone orch rm crash
! bin/stone orch ls | grep crash
bin/stone orch apply crash '*'
bin/stone orch ls | grep crash

while ! bin/stone orch ps | grep crash ; do sleep 1 ; done
bin/stone orch ps | grep crash.$host | grep running
bin/stone orch ls | grep crash | grep 1/1
bin/stone orch daemon rm crash.$host
while ! bin/stone orch ps | grep crash ; do sleep 1 ; done

bin/stone orch daemon stop crash.$host
bin/stone orch daemon start crash.$host
bin/stone orch daemon restart crash.$host
bin/stone orch daemon reconfig crash.$host
bin/stone orch daemon redeploy crash.$host

bin/stone orch host ls | grep $host
bin/stone orch host label add $host fooxyz
bin/stone orch host ls | grep $host | grep fooxyz
bin/stone orch host label rm $host fooxyz
! bin/stone orch host ls | grep $host | grep fooxyz
bin/stone orch host set-addr $host $host

bin/stone stoneadm check-host $host
#! bin/stone stoneadm check-host $host 1.2.3.4
#bin/stone orch host set-addr $host 1.2.3.4
#! bin/stone stoneadm check-host $host
bin/stone orch host set-addr $host $host
bin/stone stoneadm check-host $host

bin/stone orch apply mgr 1
bin/stone orch rm mgr --force     # we don't want a mgr to take over for ours

bin/stone orch daemon add mon $host:127.0.0.1

while ! bin/stone mon dump | grep 'epoch 2' ; do sleep 1 ; done

bin/stone orch apply rbd-mirror 1

bin/stone orch apply node-exporter '*'
bin/stone orch apply prometheus 1
bin/stone orch apply alertmanager 1
bin/stone orch apply grafana 1

while ! bin/stone dashboard get-grafana-api-url | grep $host ; do sleep 1 ; done

bin/stone orch apply rgw foo --placement=1

bin/stone orch ps
bin/stone orch ls

# clean up
bin/stone orch rm mds.foo
bin/stone orch rm rgw.myrealm.myzone
bin/stone orch rm rbd-mirror
bin/stone orch rm node-exporter
bin/stone orch rm alertmanager
bin/stone orch rm grafana
bin/stone orch rm prometheus
bin/stone orch rm crash

bin/stone mon rm $host
! bin/stone orch daemon rm mon.$host
bin/stone orch daemon rm mon.$host --force

echo OK
