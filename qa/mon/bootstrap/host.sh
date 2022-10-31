#!/bin/sh -ex

cwd=`pwd`
cat > conf <<EOF
[global]
mon host = 127.0.0.1:6789

[mon]
admin socket = 
log file = $cwd/\$name.log
debug mon = 20
debug ms = 1
EOF

rm -f mm
fsid=`uuidgen`

rm -f keyring
stone-authtool --create-keyring keyring --gen-key -n client.admin
stone-authtool keyring --gen-key -n mon.

stone-mon -c conf -i a --mkfs --fsid $fsid --mon-data mon.a -k keyring

stone-mon -c conf -i a --mon-data $cwd/mon.a

stone -c conf -k keyring health

killall stone-mon
echo OK