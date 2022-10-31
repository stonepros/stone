# remove the stone directories
sudo rm -rf /var/log/stone
sudo rm -rf /var/lib/stone
sudo rm -rf /etc/stone
sudo rm -rf /var/run/stone
# remove the stone packages
sudo apt-get -y  purge stone
sudo apt-get -y  purge stone-dbg
sudo apt-get -y  purge stone-mds
sudo apt-get -y  purge stone-mds-dbg
sudo apt-get -y  purge stone-fuse
sudo apt-get -y  purge stone-fuse-dbg
sudo apt-get -y  purge stone-common
sudo apt-get -y  purge stone-common-dbg
sudo apt-get -y  purge stone-resource-agents
sudo apt-get -y  purge librados2
sudo apt-get -y  purge librados2-dbg
sudo apt-get -y  purge librados-dev
sudo apt-get -y  purge librbd1
sudo apt-get -y  purge librbd1-dbg
sudo apt-get -y  purge librbd-dev
sudo apt-get -y  purge libstonefs2
sudo apt-get -y  purge libstonefs2-dbg
sudo apt-get -y  purge libstonefs-dev
sudo apt-get -y  purge radosgw
sudo apt-get -y  purge radosgw-dbg
sudo apt-get -y  purge obsync
sudo apt-get -y  purge python-rados
sudo apt-get -y  purge python-rbd
sudo apt-get -y  purge python-stonefs
