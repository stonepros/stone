#!/usr/bin/env bash

set -x

tmp=/tmp/stonetest-mon-caps-madness

exit_on_error=1

[[ ! -z $TEST_EXIT_ON_ERROR ]] && exit_on_error=$TEST_EXIT_ON_ERROR

if [ `uname` = FreeBSD ]; then
    ETIMEDOUT=60
else
    ETIMEDOUT=110
fi

expect()
{
  cmd=$1
  expected_ret=$2

  echo $cmd
  eval $cmd >&/dev/null
  ret=$?

  if [[ $ret -ne $expected_ret ]]; then
    echo "Error: Expected return $expected_ret, got $ret"
    [[ $exit_on_error -eq 1 ]] && exit 1
    return 1
  fi

  return 0
}

expect "stone auth get-or-create client.bazar > $tmp.bazar.keyring" 0
expect "stone -k $tmp.bazar.keyring --user bazar quorum_status" 13
stone auth del client.bazar

c="'allow command \"auth ls\", allow command quorum_status'"
expect "stone auth get-or-create client.foo mon $c > $tmp.foo.keyring" 0
expect "stone -k $tmp.foo.keyring --user foo quorum_status" 0
expect "stone -k $tmp.foo.keyring --user foo auth ls" 0
expect "stone -k $tmp.foo.keyring --user foo auth export" 13
expect "stone -k $tmp.foo.keyring --user foo auth del client.bazar" 13
expect "stone -k $tmp.foo.keyring --user foo osd dump" 13

# monitor drops the subscribe message from client if it does not have enough caps
# for read from mon. in that case, the client will be waiting for mgrmap in vain,
# if it is instructed to send a command to mgr. "pg dump" is served by mgr. so,
# we need to set a timeout for testing this scenario.
#
# leave plenty of time here because the mons might be thrashing.
export STONE_ARGS='--rados-mon-op-timeout=300'
expect "stone -k $tmp.foo.keyring --user foo pg dump" $ETIMEDOUT
export STONE_ARGS=''

stone auth del client.foo
expect "stone -k $tmp.foo.keyring --user foo quorum_status" 13

c="'allow command service with prefix=list, allow command quorum_status'"
expect "stone auth get-or-create client.bar mon $c > $tmp.bar.keyring" 0
expect "stone -k $tmp.bar.keyring --user bar quorum_status" 0
expect "stone -k $tmp.bar.keyring --user bar auth ls" 13
expect "stone -k $tmp.bar.keyring --user bar auth export" 13
expect "stone -k $tmp.bar.keyring --user bar auth del client.foo" 13
expect "stone -k $tmp.bar.keyring --user bar osd dump" 13

# again, we'll need to timeout.
export STONE_ARGS='--rados-mon-op-timeout=300'
expect "stone -k $tmp.bar.keyring --user bar pg dump" $ETIMEDOUT
export STONE_ARGS=''

stone auth del client.bar
expect "stone -k $tmp.bar.keyring --user bar quorum_status" 13

rm $tmp.bazar.keyring $tmp.foo.keyring $tmp.bar.keyring

# invalid caps health warning
cat <<EOF | stone auth import -i -
[client.bad]
  caps mon = this is wrong
  caps osd = does not parse
  caps mds = also does not parse
EOF
stone health | grep AUTH_BAD_CAP
stone health detail | grep client.bad
stone auth rm client.bad
expect "stone auth health | grep AUTH_BAD_CAP" 1

echo OK
