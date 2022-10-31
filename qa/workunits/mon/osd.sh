#!/bin/sh -x

set -e

ua=`uuidgen`
ub=`uuidgen`

# should get same id with same uuid
na=`stone osd create $ua`
test $na -eq `stone osd create $ua`

nb=`stone osd create $ub`
test $nb -eq `stone osd create $ub`
test $nb -ne $na

stone osd rm $na
stone osd rm $na
stone osd rm $nb
stone osd rm 1000

na2=`stone osd create $ua`

echo OK

