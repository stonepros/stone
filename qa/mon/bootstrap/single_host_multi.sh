#!/bin/sh -ex

cwd=`pwd`
cat > conf <<EOF
[global]

[mon]
admin socket = 
log file = $cwd/\$name.log
debug mon = 20
debug ms = 1
mon host = 127.0.0.1:6789 127.0.0.1:6790 127.0.0.1:6791
EOF

rm -f mm
fsid=`uuidgen`

rm -f keyring
stone-authtool --create-keyring keyring --gen-key -n client.admin
stone-authtool keyring --gen-key -n mon.

stone-mon -c conf -i a --mkfs --fsid $fsid --mon-data $cwd/mon.a -k keyring --public-addr 127.0.0.1:6789
stone-mon -c conf -i b --mkfs --fsid $fsid --mon-data $cwd/mon.b -k keyring --public-addr 127.0.0.1:6790
stone-mon -c conf -i c --mkfs --fsid $fsid --mon-data $cwd/mon.c -k keyring --public-addr 127.0.0.1:6791

stone-mon -c conf -i a --mon-data $cwd/mon.a
stone-mon -c conf -i b --mon-data $cwd/mon.b
stone-mon -c conf -i c --mon-data $cwd/mon.c

stone -c conf -k keyring health -m 127.0.0.1
while true; do
    if stone -c conf -k keyring -m 127.0.0.1 mon stat | grep 'a,b,c'; then
	break
    fi
    sleep 1
done

killall stone-mon
echo OK