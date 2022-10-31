#! /usr/bin/env bash
if [ $# -ne 5 ]; then
    echo 'Usage: stone_install.sh <admin-node> <mon-node> <osd-node> <osd-node> <osd-node>'
    exit -1
fi
allnodes=$*
adminnode=$1
shift
stonenodes=$*
monnode=$1
shift
osdnodes=$*
./multi_action.sh cdn_setup.sh $allnodes
./talknice.sh $allnodes
for mac in $allnodes; do
    ssh $mac sudo yum -y install yum-utils
done

source ./repolocs.sh
ssh $adminnode sudo yum-config-manager --add ${STONE_REPO_TOOLS}
ssh $monnode sudo yum-config-manager --add ${STONE_REPO_MON}
for mac in $osdnodes; do
    ssh $mac sudo yum-config-manager --add ${STONE_REPO_OSD}
done
ssh $adminnode sudo yum-config-manager --add ${INSTALLER_REPO_LOC}

for mac in $allnodes; do
    ssh $mac sudo sed -i 's/gpgcheck=1/gpgcheck=0/' /etc/yum.conf
done

source copy_func.sh
copy_file execs/stone_ansible.sh $adminnode . 0777 ubuntu:ubuntu
copy_file execs/edit_ansible_hosts.sh $adminnode . 0777 ubuntu:ubuntu
copy_file execs/edit_groupvars_osds.sh $adminnode . 0777 ubuntu:ubuntu
copy_file ../execs/stone-pool-create.sh $monnode . 0777 ubuntu:ubuntu
if [ -e ~/ip_info ]; then
    copy_file ~/ip_info $adminnode . 0777 ubuntu:ubuntu
fi
ssh $adminnode ./stone_ansible.sh $stonenodes
