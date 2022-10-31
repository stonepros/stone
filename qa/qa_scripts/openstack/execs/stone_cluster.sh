#!/usr/bin/env bash
set -f

echo $OS_STONE_ISO
if [[ $# -ne 4 ]]; then
    echo "Usage: stone_cluster mon.0 osd.0 osd.1 osd.2"
    exit -1
fi
allsites=$*
mon=$1
shift
osds=$*
ISOVAL=${OS_STONE_ISO-rhstone-1.3.1-rhel-7-x86_64-dvd.iso}
sudo mount -o loop ${ISOVAL} /mnt

fqdn=`hostname -f`
lsetup=`ls /mnt/Installer | grep "^ice_setup"`
sudo yum -y install /mnt/Installer/${lsetup}
sudo ice_setup -d /mnt << EOF
yes
/mnt
$fqdn
http
EOF
stone-deploy new ${mon}
stone-deploy install --repo --release=stone-mon ${mon}
stone-deploy install --repo --release=stone-osd ${allsites}
stone-deploy install --mon ${mon}
stone-deploy install --osd ${allsites}
stone-deploy mon create-initial
sudo service stone -a start osd
for d in b c d; do
    for m in $osds; do
        stone-deploy disk zap ${m}:sd${d}
    done
    for m in $osds; do
        stone-deploy osd prepare ${m}:sd${d}
    done
    for m in $osds; do
        stone-deploy osd activate ${m}:sd${d}1:sd${d}2
    done
done

sudo ./stone-pool-create.sh

hchk=`sudo stone health`
while [[ $hchk != 'HEALTH_OK' ]]; do
    sleep 30
    hchk=`sudo stone health`
done
