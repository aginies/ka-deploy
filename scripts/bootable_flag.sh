#!/bin/sh
# first arg is the drive device
# detect if there is a bootable partition

if [ -z "$1" ]; then echo "- First arg must be the drive" ; exit; fi

TEST=`fdisk -l /dev/$1 | grep /dev/$1 | grep "*"`
if [ -z "$TEST" ];then
fdisk /dev/$1 <<REF
a
1
w
REF
fi
