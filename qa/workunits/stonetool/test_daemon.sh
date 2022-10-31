#!/usr/bin/env bash

set -ex

expect_false()
{
	set -x
	if "$@"; then return 1; else return 0; fi
}

echo note: assuming mon.a is on the current host

# can set to 'sudo ./stone' to execute tests from current dir for development
STONE=${STONE:-'sudo stone'}

${STONE} daemon mon.a version | grep version

# get debug_ms setting and strip it, painfully for reuse
old_ms=$(${STONE} daemon mon.a config get debug_ms | \
	grep debug_ms | sed -e 's/.*: //' -e 's/["\}\\]//g')
${STONE} daemon mon.a config set debug_ms 13
new_ms=$(${STONE} daemon mon.a config get debug_ms | \
	grep debug_ms | sed -e 's/.*: //' -e 's/["\}\\]//g')
[ "$new_ms" = "13/13" ]
${STONE} daemon mon.a config set debug_ms $old_ms
new_ms=$(${STONE} daemon mon.a config get debug_ms | \
	grep debug_ms | sed -e 's/.*: //' -e 's/["\}\\]//g')
[ "$new_ms" = "$old_ms" ]

# unregistered/non-existent command
expect_false ${STONE} daemon mon.a bogus_command_blah foo

set +e
OUTPUT=$(${STONE} -c /not/a/stone.conf daemon mon.a help 2>&1)
# look for EINVAL
if [ $? != 22 ] ; then exit 1; fi
if ! echo "$OUTPUT" | grep -q '.*open.*/not/a/stone.conf'; then 
	echo "didn't find expected error in bad conf search"
	exit 1
fi
set -e

echo OK
