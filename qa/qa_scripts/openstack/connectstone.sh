#!/usr/bin/env bash
#
# Connect openstack node just installed to a stone cluster.
#
# Essentially implements:
#
# http://docs.stone.com/en/latest/rbd/rbd-openstack/
#
# The directory named files contains templates for the /etc/glance/glance-api.conf,
# /etc/cinder/cinder.conf, /etc/nova/nova.conf Openstack files
#
set -fv
source ./copy_func.sh
source ./fix_conf_file.sh
openstack_node=${1}
stone_node=${2}

scp $stone_node:/etc/stonepros/stone.conf ./stone.conf
ssh $openstack_node sudo mkdir /etc/stone
copy_file stone.conf $openstack_node /etc/stone 0644
rm -f stone.conf
ssh $openstack_node sudo yum -y install python-rbd
ssh $openstack_node sudo yum -y install stone-common
ssh $stone_node "sudo stone auth get-or-create client.cinder mon 'allow r' osd 'allow class-read object_prefix rbd_children, allow rwx pool=volumes, allow rwx pool=vms, allow rx pool=images'"
ssh $stone_node "sudo stone auth get-or-create client.glance mon 'allow r' osd 'allow class-read object_prefix rbd_children, allow rwx pool=images'"
ssh $stone_node "sudo stone auth get-or-create client.cinder-backup mon 'allow r' osd 'allow class-read object_prefix rbd_children, allow rwx pool=backups'"
ssh $stone_node sudo stone auth get-or-create client.glance mon 'allow r' osd 'allow class-read object_prefix rbd_children, allow rwx pool=images'
ssh $stone_node sudo stone auth get-or-create client.cinder-backup mon 'allow r' osd 'allow class-read object_prefix rbd_children, allow rwx pool=backups'
ssh $stone_node sudo stone auth get-or-create client.glance | ssh $openstack_node sudo tee /etc/stonepros/stone.client.glance.keyring
ssh $openstack_node sudo chown glance:glance /etc/stonepros/stone.client.glance.keyring
ssh $stone_node sudo stone auth get-or-create client.cinder | ssh $openstack_node sudo tee /etc/stonepros/stone.client.cinder.keyring
ssh $openstack_node sudo chown cinder:cinder /etc/stonepros/stone.client.cinder.keyring
ssh $stone_node sudo stone auth get-or-create client.cinder-backup | ssh $openstack_node sudo tee /etc/stonepros/stone.client.cinder-backup.keyring
ssh $openstack_node sudo chown cinder:cinder /etc/stonepros/stone.client.cinder-backup.keyring
ssh $stone_node sudo stone auth get-key client.cinder | ssh $openstack_node tee client.cinder.key
copy_file execs/libvirt-secret.sh $openstack_node .
secret_msg=`ssh $openstack_node sudo ./libvirt-secret.sh $openstack_node`
secret_virt=`echo $secret_msg | sed 's/.* set //'`
echo $secret_virt
fix_conf_file $openstack_node glance-api /etc/glance
fix_conf_file $openstack_node cinder /etc/cinder $secret_virt
fix_conf_file $openstack_node nova /etc/nova $secret_virt
copy_file execs/start_openstack.sh $openstack_node . 0755
ssh $openstack_node ./start_openstack.sh
