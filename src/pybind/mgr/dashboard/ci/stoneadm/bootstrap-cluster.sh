#!/usr/bin/env bash

set -x

export PATH=/root/bin:$PATH
mkdir /root/bin

cp /mnt/{{ stone_dev_folder }}/src/stoneadm/stoneadm /root/bin/stoneadm
chmod +x /root/bin/stoneadm
mkdir -p /etc/stone
mon_ip=$(ifconfig eth0  | grep 'inet ' | awk '{ print $2}')

stoneadm bootstrap --mon-ip $mon_ip --initial-dashboard-password {{ admin_password }} --allow-fqdn-hostname --skip-monitoring-stack --dashboard-password-noupdate --shared_stone_folder /mnt/{{ stone_dev_folder }}

fsid=$(cat /etc/stone/stone.conf | grep fsid | awk '{ print $3}')
stoneadm_shell="stoneadm shell --fsid ${fsid} -c /etc/stone/stone.conf -k /etc/stone/stone.client.admin.keyring"

{% for number in range(1, nodes) %}
  ssh-copy-id -f -i /etc/stone/stone.pub  -o StrictHostKeyChecking=no root@{{ prefix }}-node-0{{ number }}
  {% if expanded_cluster is defined %}
    ${stoneadm_shell} stone orch host add {{ prefix }}-node-0{{ number }}
  {% endif %}
{% endfor %}

{% if expanded_cluster is defined %}
  ${stoneadm_shell} stone orch apply osd --all-available-devices
{% endif %}
