#!/bin/sh -x

p() {
 echo "$*" > /sys/kernel/debug/dynamic_debug/control
}

echo 9 > /proc/sysrq-trigger
p 'module stone +p'
p 'module libstone +p'
p 'module rbd +p'
p 'file net/stone/messenger.c -p'
p 'file' `grep -- --- /sys/kernel/debug/dynamic_debug/control | grep stone | awk '{print $1}' | sed 's/:/ line /'` '+p'
p 'file' `grep -- === /sys/kernel/debug/dynamic_debug/control | grep stone | awk '{print $1}' | sed 's/:/ line /'` '+p'
