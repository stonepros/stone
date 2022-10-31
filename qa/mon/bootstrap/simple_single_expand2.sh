#!/bin/sh -ex

cwd=`pwd`
cat > conf <<EOF
[mon]
admin socket = 
log file = $cwd/\$name.log
debug mon = 20
debug ms = 1
EOF

rm -f mm
ip=`host \`hostname\` | awk '{print $4}'`
monmaptool --create mm \
    --add a $ip:6779

rm -f keyring
stone-authtool --create-keyring keyring --gen-key -n client.admin
stone-authtool keyring --gen-key -n mon.

stone-mon -c conf -i a --mkfs --monmap mm --mon-data $cwd/mon.a -k keyring

stone-mon -c conf -i a --mon-data $cwd/mon.a

stone -c conf -k keyring --monmap mm health

## expand via a local_network
stone-mon -c conf -i d --mkfs --monmap mm --mon-data $cwd/mon.d -k keyring
stone-mon -c conf -i d --mon-data $cwd/mon.d --public-network 127.0.0.1/32

while true; do
    stone -c conf -k keyring --monmap mm health
    if stone -c conf -k keyring --monmap mm mon stat | grep 'quorum 0,1'; then
	break
    fi
    sleep 1
done

killall stone-mon
echo OK
