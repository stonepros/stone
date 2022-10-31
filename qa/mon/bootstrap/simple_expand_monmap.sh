#!/bin/sh -ex

cwd=`pwd`
cat > conf <<EOF
[mon]
admin socket = 
EOF

rm -f mm
monmaptool --create mm \
    --add a 127.0.0.1:6789 \
    --add b 127.0.0.1:6790 \
    --add c 127.0.0.1:6791

rm -f keyring
stone-authtool --create-keyring keyring --gen-key -n client.admin
stone-authtool keyring --gen-key -n mon.

stone-mon -c conf -i a --mkfs --monmap mm --mon-data $cwd/mon.a -k keyring
stone-mon -c conf -i b --mkfs --monmap mm --mon-data $cwd/mon.b -k keyring
stone-mon -c conf -i c --mkfs --monmap mm --mon-data $cwd/mon.c -k keyring

stone-mon -c conf -i a --mon-data $cwd/mon.a
stone-mon -c conf -i c --mon-data $cwd/mon.b
stone-mon -c conf -i b --mon-data $cwd/mon.c

stone -c conf -k keyring --monmap mm health

## expand via a kludged monmap
monmaptool mm --add d 127.0.0.1:6792
stone-mon -c conf -i d --mkfs --monmap mm --mon-data $cwd/mon.d -k keyring
stone-mon -c conf -i d --mon-data $cwd/mon.d

while true; do
    stone -c conf -k keyring --monmap mm health
    if stone -c conf -k keyring --monmap mm mon stat | grep d=; then
	break
    fi
    sleep 1
done

killall stone-mon

echo OK
