#!/usr/bin/env bash

set -ex

function expect_false()
{
	set -x
	if "$@"; then return 1; else return 0; fi
}

stone osd crush dump

# rules
stone osd crush rule dump
stone osd crush rule ls
stone osd crush rule list

stone osd crush rule create-simple foo default host
stone osd crush rule create-simple foo default host
stone osd crush rule create-simple bar default host

stone osd crush rm-device-class all
stone osd crush set-device-class ssd osd.0
stone osd crush set-device-class hdd osd.1
stone osd crush rule create-replicated foo-ssd default host ssd
stone osd crush rule create-replicated foo-hdd default host hdd
stone osd crush rule ls-by-class ssd | grep 'foo-ssd'
stone osd crush rule ls-by-class ssd | expect_false grep 'foo-hdd'
stone osd crush rule ls-by-class hdd | grep 'foo-hdd'
stone osd crush rule ls-by-class hdd | expect_false grep 'foo-ssd'

stone osd erasure-code-profile set ec-foo-ssd crush-device-class=ssd m=2 k=2
stone osd pool create ec-foo 2 erasure ec-foo-ssd
stone osd pool rm ec-foo ec-foo --yes-i-really-really-mean-it

stone osd crush rule ls | grep foo

stone osd crush rule rename foo foo-asdf
stone osd crush rule rename foo foo-asdf # idempotent
stone osd crush rule rename bar bar-asdf
stone osd crush rule ls | grep 'foo-asdf'
stone osd crush rule ls | grep 'bar-asdf'
stone osd crush rule rm foo 2>&1 | grep 'does not exist'
stone osd crush rule rm bar 2>&1 | grep 'does not exist'
stone osd crush rule rename foo-asdf foo
stone osd crush rule rename foo-asdf foo # idempotent
stone osd crush rule rename bar-asdf bar
stone osd crush rule ls | expect_false grep 'foo-asdf'
stone osd crush rule ls | expect_false grep 'bar-asdf'
stone osd crush rule rm foo
stone osd crush rule rm foo  # idempotent
stone osd crush rule rm bar

# can't delete in-use rules, tho:
stone osd pool create pinning_pool 1
expect_false stone osd crush rule rm replicated_rule
stone osd pool rm pinning_pool pinning_pool --yes-i-really-really-mean-it

# build a simple map
expect_false stone osd crush add-bucket foo osd
stone osd crush add-bucket foo root
o1=`stone osd create`
o2=`stone osd create`
stone osd crush add $o1 1 host=host1 root=foo
stone osd crush add $o1 1 host=host1 root=foo  # idemptoent
stone osd crush add $o2 1 host=host2 root=foo
stone osd crush add $o2 1 host=host2 root=foo  # idempotent
stone osd crush add-bucket bar root
stone osd crush add-bucket bar root  # idempotent
stone osd crush link host1 root=bar
stone osd crush link host1 root=bar  # idempotent
stone osd crush link host2 root=bar
stone osd crush link host2 root=bar  # idempotent

stone osd tree | grep -c osd.$o1 | grep -q 2
stone osd tree | grep -c host1 | grep -q 2
stone osd tree | grep -c osd.$o2 | grep -q 2
stone osd tree | grep -c host2 | grep -q 2
expect_false stone osd crush rm host1 foo   # not empty
stone osd crush unlink host1 foo
stone osd crush unlink host1 foo
stone osd tree | grep -c host1 | grep -q 1

expect_false stone osd crush rm foo  # not empty
expect_false stone osd crush rm bar  # not empty
stone osd crush unlink host1 bar
stone osd tree | grep -c host1 | grep -q 1   # now an orphan
stone osd crush rm osd.$o1 host1
stone osd crush rm host1
stone osd tree | grep -c host1 | grep -q 0
expect_false stone osd tree-from host1
stone osd tree-from host2
expect_false stone osd tree-from osd.$o2

expect_false stone osd crush rm bar   # not empty
stone osd crush unlink host2

stone osd crush add-bucket host-for-test host root=root-for-test rack=rack-for-test
stone osd tree | grep host-for-test
stone osd tree | grep rack-for-test
stone osd tree | grep root-for-test
stone osd crush rm host-for-test
stone osd crush rm rack-for-test
stone osd crush rm root-for-test

# reference foo and bar with a rule
stone osd crush rule create-simple foo-rule foo host firstn
expect_false stone osd crush rm foo
stone osd crush rule rm foo-rule

stone osd crush rm bar
stone osd crush rm foo
stone osd crush rm osd.$o2 host2
stone osd crush rm host2

stone osd crush add-bucket foo host
stone osd crush move foo root=default rack=localrack

stone osd crush create-or-move osd.$o1 1.0 root=default
stone osd crush move osd.$o1 host=foo
stone osd find osd.$o1 | grep host | grep foo

stone osd crush rm osd.$o1
stone osd crush rm osd.$o2

stone osd crush rm foo

# test reweight
o3=`stone osd create`
stone osd crush add $o3 123 root=default
stone osd tree | grep osd.$o3 | grep 123
stone osd crush reweight osd.$o3 113
expect_false stone osd crush reweight osd.$o3 123456
stone osd tree | grep osd.$o3 | grep 113
stone osd crush rm osd.$o3
stone osd rm osd.$o3

# test reweight-subtree
o4=`stone osd create`
o5=`stone osd create`
stone osd crush add $o4 123 root=default host=foobaz
stone osd crush add $o5 123 root=default host=foobaz
stone osd tree | grep osd.$o4 | grep 123
stone osd tree | grep osd.$o5 | grep 123
stone osd crush reweight-subtree foobaz 155
expect_false stone osd crush reweight-subtree foobaz 123456
stone osd tree | grep osd.$o4 | grep 155
stone osd tree | grep osd.$o5 | grep 155
stone osd crush rm osd.$o4
stone osd crush rm osd.$o5
stone osd rm osd.$o4
stone osd rm osd.$o5

# weight sets
# make sure we require luminous before testing weight-sets
stone osd set-require-min-compat-client luminous
stone osd crush weight-set dump
stone osd crush weight-set ls
expect_false stone osd crush weight-set reweight fooset osd.0 .9
stone osd pool create fooset 8
stone osd pool create barset 8
stone osd pool set barset size 3
expect_false stone osd crush weight-set reweight fooset osd.0 .9
stone osd crush weight-set create fooset flat
stone osd crush weight-set create barset positional
stone osd crush weight-set ls | grep fooset
stone osd crush weight-set ls | grep barset
stone osd crush weight-set dump
stone osd crush weight-set reweight fooset osd.0 .9
expect_false stone osd crush weight-set reweight fooset osd.0 .9 .9
expect_false stone osd crush weight-set reweight barset osd.0 .9
stone osd crush weight-set reweight barset osd.0 .9 .9 .9
stone osd crush weight-set ls | grep -c fooset | grep -q 1
stone osd crush weight-set rm fooset
stone osd crush weight-set ls | grep -c fooset | grep -q 0
stone osd crush weight-set ls | grep barset
stone osd crush weight-set rm barset
stone osd crush weight-set ls | grep -c barset | grep -q 0
stone osd crush weight-set create-compat
stone osd crush weight-set ls | grep '(compat)'
stone osd crush weight-set rm-compat

# weight set vs device classes
stone osd pool create cool 2
stone osd pool create cold 2
stone osd pool set cold size 2
stone osd crush weight-set create-compat
stone osd crush weight-set create cool flat
stone osd crush weight-set create cold positional
stone osd crush rm-device-class osd.0
stone osd crush weight-set reweight-compat osd.0 10.5
stone osd crush weight-set reweight cool osd.0 11.5
stone osd crush weight-set reweight cold osd.0 12.5 12.4
stone osd crush set-device-class fish osd.0
stone osd crush tree --show-shadow | grep osd\\.0 | grep fish | grep 10\\.
stone osd crush tree --show-shadow | grep osd\\.0 | grep fish | grep 11\\.
stone osd crush tree --show-shadow | grep osd\\.0 | grep fish | grep 12\\.
stone osd crush rm-device-class osd.0
stone osd crush set-device-class globster osd.0
stone osd crush tree --show-shadow | grep osd\\.0 | grep globster | grep 10\\.
stone osd crush tree --show-shadow | grep osd\\.0 | grep globster | grep 11\\.
stone osd crush tree --show-shadow | grep osd\\.0 | grep globster | grep 12\\.
stone osd crush weight-set reweight-compat osd.0 7.5
stone osd crush weight-set reweight cool osd.0 8.5
stone osd crush weight-set reweight cold osd.0 6.5 6.6
stone osd crush tree --show-shadow | grep osd\\.0 | grep globster | grep 7\\.
stone osd crush tree --show-shadow | grep osd\\.0 | grep globster | grep 8\\.
stone osd crush tree --show-shadow | grep osd\\.0 | grep globster | grep 6\\.
stone osd crush rm-device-class osd.0
stone osd pool rm cool cool --yes-i-really-really-mean-it
stone osd pool rm cold cold --yes-i-really-really-mean-it
stone osd crush weight-set rm-compat

# weight set vs device classes vs move
stone osd crush weight-set create-compat
stone osd crush add-bucket fooo host
stone osd crush move fooo root=default
stone osd crush add-bucket barr rack
stone osd crush move barr root=default
stone osd crush move fooo rack=barr
stone osd crush rm fooo
stone osd crush rm barr
stone osd crush weight-set rm-compat

# this sequence would crash at one point
stone osd crush weight-set create-compat
stone osd crush add-bucket r1 rack root=default
for f in `seq 1 32`; do
    stone osd crush add-bucket h$f host rack=r1
done
for f in `seq 1 32`; do
    stone osd crush rm h$f
done
stone osd crush rm r1
stone osd crush weight-set rm-compat

echo OK
