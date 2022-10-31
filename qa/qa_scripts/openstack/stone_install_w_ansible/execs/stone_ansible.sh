#! /usr/bin/env bash
stonenodes=$*
monnode=$1
sudo yum -y install stone-ansible
cd
sudo ./edit_ansible_hosts.sh $stonenodes
mkdir stone-ansible-keys
cd /usr/share/stone-ansible/group_vars/
if [ -f ~/ip_info ]; then
    source ~/ip_info
fi
mon_intf=${mon_intf:-'eno1'}
pub_netw=${pub_netw:-'10.8.128.0\/21'}
sudo cp all.sample all
sudo sed -i 's/#stone_origin:.*/stone_origin: distro/' all
sudo sed -i 's/#fetch_directory:.*/fetch_directory: ~\/stone-ansible-keys/' all
sudo sed -i 's/#stone_stable:.*/stone_stable: true/' all
sudo sed -i 's/#stone_stable_rh_storage:.*/stone_stable_rh_storage: false/' all
sudo sed -i 's/#stone_stable_rh_storage_cdn_install:.*/stone_stable_rh_storage_cdn_install: true/' all
sudo sed -i 's/#stonex:.*/stonex: true/' all
sudo sed -i "s/#monitor_interface:.*/monitor_interface: ${mon_intf}/" all
sudo sed -i 's/#journal_size:.*/journal_size: 1024/' all
sudo sed -i "s/#public_network:.*/public_network: ${pub_netw}/" all
sudo cp osds.sample osds
sudo sed -i 's/#fetch_directory:.*/fetch_directory: ~\/stone-ansible-keys/' osds
sudo sed -i 's/#crush_location:/crush_location:/' osds
sudo sed -i 's/#osd_crush_location:/osd_crush_location:/' osds
sudo sed -i 's/#stonex:/stonex:/' osds
sudo sed -i 's/#devices:/devices:/' osds
sudo sed -i 's/#journal_collocation:.*/journal_collocation: true/' osds
cd
sudo ./edit_groupvars_osds.sh
cd /usr/share/stone-ansible
sudo cp site.yml.sample site.yml
ansible-playbook site.yml
ssh $monnode ~/stone-pool-create.sh
