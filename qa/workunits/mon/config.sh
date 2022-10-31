#!/bin/bash -ex

function expect_false()
{
	set -x
	if "$@"; then return 1; else return 0; fi
}

stone config dump

# value validation
stone config set mon.a debug_asok 22
stone config set mon.a debug_asok 22/33
stone config get mon.a debug_asok | grep 22
stone config set mon.a debug_asok 1/2
expect_false stone config set mon.a debug_asok foo
expect_false stone config set mon.a debug_asok -10
stone config rm mon.a debug_asok

stone config set global log_graylog_port 123
expect_false stone config set global log_graylog_port asdf
stone config rm global log_graylog_port

stone config set mon mon_cluster_log_to_stderr true
stone config get mon.a mon_cluster_log_to_stderr | grep true
stone config set mon mon_cluster_log_to_stderr 2
stone config get mon.a mon_cluster_log_to_stderr | grep true
stone config set mon mon_cluster_log_to_stderr 1
stone config get mon.a mon_cluster_log_to_stderr | grep true
stone config set mon mon_cluster_log_to_stderr false
stone config get mon.a mon_cluster_log_to_stderr | grep false
stone config set mon mon_cluster_log_to_stderr 0
stone config get mon.a mon_cluster_log_to_stderr | grep false
expect_false stone config set mon mon_cluster_log_to_stderr fiddle
expect_false stone config set mon mon_cluster_log_to_stderr ''
stone config rm mon mon_cluster_log_to_stderr

expect_false stone config set mon.a osd_pool_default_type foo
stone config set mon.a osd_pool_default_type replicated
stone config rm mon.a osd_pool_default_type

# scoping
stone config set global debug_asok 33
stone config get mon.a debug_asok | grep 33
stone config set mon debug_asok 11
stone config get mon.a debug_asok | grep 11
stone config set mon.a debug_asok 22
stone config get mon.a debug_asok | grep 22
stone config rm mon.a debug_asok
stone config get mon.a debug_asok | grep 11
stone config rm mon debug_asok
stone config get mon.a debug_asok | grep 33
#  nested .-prefix scoping
stone config set client.foo debug_asok 44
stone config get client.foo.bar debug_asok | grep 44
stone config get client.foo.bar.baz debug_asok | grep 44
stone config set client.foo.bar debug_asok 55
stone config get client.foo.bar.baz debug_asok | grep 55
stone config rm client.foo debug_asok
stone config get client.foo.bar.baz debug_asok | grep 55
stone config rm client.foo.bar debug_asok
stone config get client.foo.bar.baz debug_asok | grep 33
stone config rm global debug_asok

# help
stone config help debug_asok | grep debug_asok

# show
stone config set osd.0 debug_asok 33
while ! stone config show osd.0 | grep debug_asok | grep 33 | grep mon
do
    sleep 1
done
stone config set osd.0 debug_asok 22
while ! stone config show osd.0 | grep debug_asok | grep 22 | grep mon
do
    sleep 1
done

stone tell osd.0 config set debug_asok 99
while ! stone config show osd.0 | grep debug_asok | grep 99
do
    sleep 1
done
stone config show osd.0 | grep debug_asok | grep 'override  mon'
stone tell osd.0 config unset debug_asok
stone tell osd.0 config unset debug_asok

stone config rm osd.0 debug_asok
while stone config show osd.0 | grep debug_asok | grep mon
do
    sleep 1
done
stone config show osd.0 | grep -c debug_asok | grep 0

stone config set osd.0 osd_scrub_cost 123
while ! stone config show osd.0 | grep osd_scrub_cost | grep mon
do
    sleep 1
done
stone config rm osd.0 osd_scrub_cost

# show-with-defaults
stone config show-with-defaults osd.0 | grep debug_asok

# assimilate
t1=`mktemp`
t2=`mktemp`
cat <<EOF > $t1
[osd.0]
keyring = foo
debug_asok = 66
EOF
stone config assimilate-conf -i $t1 | tee $t2

grep keyring $t2
expect_false grep debug_asok $t2
rm -f $t1 $t2

expect_false stone config reset
expect_false stone config reset -1
# we are at end of testing, so it's okay to revert everything
stone config reset 0

echo OK
