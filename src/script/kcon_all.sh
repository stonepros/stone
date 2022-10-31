#!/bin/sh -x

p() {
 echo "$*" > /sys/kernel/debug/dynamic_debug/control
}

echo 9 > /proc/sysrq-trigger
p 'module stone +p'
p 'module libstone +p'
p 'module rbd +p'
