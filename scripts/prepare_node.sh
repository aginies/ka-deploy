#!/bin/sh
# first arg must be the name of the bootloader: lilo or grub

. /ka2/replication.conf

#echo "Removing computing interfaces"
#rm -f $CHROOT/etc/sysconfig/network-scripts/ifcfg-eth1 >/dev/null 2>&1

echo "- Removing duplicated dhcp cache"
rm -vf $CHROOT/etc/dhcpc/* >/dev/null 2>&1

echo "- Writing modprobe.conf"
/usr/bin/perl $KADIR/gen_modprobe_conf.pl >$CHROOT/etc/modprobe.conf
echo "********************"
cat $CHROOT/etc/modprobe.conf
echo "********************"

echo "- Remove udev network rules"
rm -vf $CHROOT/etc/udev/rules.d/70-persistent*


echo "- Fix /dev in $CHROOT"
cp -v $KADIR/udev_creation.sh $CHROOT/sbin
rm -rf $CHROOT/dev/*
mount -o bind /proc $CHROOT/proc
mount -o bind /sys $CHROOT/sys
chroot $CHROOT /sbin/udev_creation.sh

echo "- Create new /dev"
mkdir $CHROOT/dev2
cp -af $CHROOT/dev/* $CHROOT/dev2/*
chroot $CHROOT killall udevd
umount $CHROOT/dev/
cp -af $CHROOT/dev2/* $CHROOT/dev/

echo "- Running mkinitrd"
$KADIR/make_initrd_${1}

echo "- Umount /sys and /proc"
umount $CHROOT/sys
umount $CHROOT/proc
# kill ifplugd to be able to umount $CHROOT/dev
killall5 -9 ifplugd
umount $CHROOT/dev
