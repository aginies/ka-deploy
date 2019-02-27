#!/bin/bash
# script to do the ka duplication getting data from the golden node

. /ka2/replication.conf

unset LANG
unset LANGUAGE
# needed for some for i in foo* loops
shopt -s nullglob

bash < /dev/tty2 >/dev/tty2 2>&1 &

# the mount_partition calls must be done in the same shell (NOT a subshell) because the global variable below has to be updated
# idea : maybe use a file instead of this variable (and since the tac function now exists, why not ?)
mounted_fs=""
delay=0
ka_call_num=0

inc_ka_session() {
    (( ka_call_num++ ))
    # maybe this option could be overriden by a kaopt= in the kernel command line ?
    KA_SESSION_BASE="-s kainstall"
    cur_ka_session=$KA_SESSION_BASE$ka_call_num
}

# run a command, hide its output and print OK if it suceeds, print FAILED and show the output otherwise.
runcom() {
    echo -n " $1..." 1>&2
    shift;
    out=`"$@" 2>&1` 
    ret=$?
    if [ $ret -eq 0 ]; then
	echo $C_S"OK"$C_N 1>&2
    else
	echo $C_F"Failed"$C_N 1>&2
	echo $C_W"$out"$C_N 1>&2
    fi
    return $ret
}

# 5 4 3 2 1 zero ignition
countdown() {
    t=$1
    while [ "$t" -ne 0 ]; do
        echo -n "$t "
        sleep 1
	# move the cursor back
	# I use now tr instead of sed because busybox's sed adds a non-wanted carriage return
	# busybox's tr does not seem to know about the [: :] stuff (?)
        echo -n "$t " | tr " 0-9" $'\x08'
	(( t-- ))
    done
    # backspace only moves the cursor back, so at this point there's still a "1" to erase
    if [ "$1" -ne 0 ] ;then
        echo -n "  "$'\x08\x08'
    fi
}

int_shell() {
    echo $C_H"- Starting an interactive shell"$C_N
    exec /bin/bash
}

fail() {
    echo $*
    echo $C_F"--- The installation program FAILED ---"$C_N
    echo " Check your configuration -- try to read previous error messages"
    echo " This machine is going to reboot (Ctrl-S to block it) (Ctrl-C for an interactive shell)"
    trap int_shell SIGINT
    cd $KADIR
    $KADIR/store_log.sh
    countdown 30
    do_reboot
}

do_reboot() {
    reboot
    sleep 1234567 # do not continue the install script (/sbin/reboot does not block)
}

run_chroot_command() {		
    $SBINDIR/chroot $CHROOT $*
}		

# mount a partition UNDER /disk !!! (/disk is prepended to $2)
mount_partition() {
    dev=$1
    point=$2
    echo -n "- Mounting ${C_H}${1}${C_N} as ${C_H}${CHROOT}${point}${C_N}" 
    mkdir -p $CHROOT/$point
    test -d $CHROOT/$point || return 1
    runcom "..." mount $dev $CHROOT/$point || return 1
    mounted_fs="$dev $mounted_fs"
    return 0
}

# umount all mounted partitions under /disk, in the reverse order
umount_partitions() {
    for dev in $mounted_fs; do
	retries=0
	while ! runcom "Umounting $dev" umount $dev -f; do
	    sleep 3
	    (( retries++ ))
	    if [ $retries -gt 3 ]; then
		echo "- Failed too many times. giving up."
		break
	    fi
	done
    done
}

write_MBRs() {
    # we must be in the partfiles directory also
    cd $TMPFILESDIR
    for file in MBR*; do
	drive=`echo $file | sed 's/MBR//'`
	runcom "Writing new MBR for $drive" dd if=$file of=/dev/$drive bs=1 count=446
    done
}


# recreate excluded directories
# read stdin like this : u=rwx g=rwx o=rwx uid gid filename
recreate_dirs() {
    while read line; do
	declare -a fields
	fields=( $line )
	file=$CHROOT/${fields[5]}
        # note : it is possible that the directory exists already, if it was a mount point
        # we need to set the permissions/users anyway
	mkdir -p $file
        # echo chmod ${fields[0]},${fields[1]},${fields[2]}  $file
	chmod ${fields[0]},${fields[1]},${fields[2]}  $file
		# argl !! chmod o+t does not work with busybox's chmod !
		# we have to handle it alone
	if echo ${fields[2]} | grep -q t; then
	    chmod +t $file
	fi
	chown ${fields[3]}.${fields[4]} $file
    done
}

make_partitions() {
    # we must be in the partfiles directory
    for file in partition_tab*; do
	if echo "$drive" | grep -q "cciss"; then
            drive="cciss/c0d0"
        else
            drive=`echo $file | sed 's/partition_tab//'`
        fi
	cat $file | runcom "Writing partition table for $drive using sfdisk" /sbin/sfdisk /dev/$drive -uS --force || fail "error with sfdisk"
    done
    
    for file in fdisk_commands*; do
        if echo "$drive" | grep -q "cciss"; then
            drive="cciss/c0d0"
        else
            drive=`echo $file | sed 's/fdisk_commands//'`
        fi
	runcom "Cleaning hard drive" dd if=/dev/zero of=/dev/$drive bs=1M count=5 || fail "Can t clean drive$drive"
        echo w | fdisk /dev/$drive
	cat $file | runcom "Writing partition table for $drive using fdisk" fdisk /dev/$drive || fail "error with fdisk"
    done
}


#######################
# MAIN
#######################

# Clear screen, fancy startup message.
echo $'\033'[2J$'\033'[H
echo "------| $C_H"Ka"$C_N |---- Install starting..."

# receive the partition table, fstab, etc from the source node
mkdir $TMPFILESDIR
inc_ka_session
echo Current session is $cur_ka_session
runcom "Receiving partitions information" $KADIR/ka-d-client -w $cur_ka_session -e "( cd $TMPFILESDIR && tar xvf - )" || fail

cd $TMPFILESDIR 
make_partitions

test -f $TMPFILESDIR/streams || fail "Missing streams file"
first_stream=`cat $TMPFILESDIR/streams | head -n 1`

if [ "$first_stream" = linux ]; then
    rcv_linux="yes"
else
    rcv_linux="no"
fi

# if we must receive a linux system, we need to format and mount the partitions
if [ $rcv_linux = yes ]; then
    # format partitions all partitions
    format_partitions() {
	while read line; do
	    declare -a fields
	    fields=( $line )
	    case ${fields[2]} in
		reiserfs )
		    runcom "Formatting ${fields[0]} as reiserfs" mkreiserfs -f ${fields[0]} || fail
		    ;;
		jfs )
		    runcom "Formatting ${fields[0]} as jfs" mkfs.jfs ${fields[0]} || fail
		    ;;
		xfs )
		    runcom "Formatting ${fields[0]} as xfs" mkfs.xfs -f ${fields[0]} || fail
		    ;;
		ext3 )
		    runcom "Formatting ${fields[0]} as ext3" mkfs.ext3 -v -j ${fields[0]} || fail
		    ;;
		ext4 )
		    runcom "Formatting ${fields[0]} as ext4" mkfs.ext4 -v -j ${fields[0]} || fail
		    ;;
		ext2 )
		    runcom "Formatting ${fields[0]} as ext2" mkfs.ext2 -v ${fields[0]} || fail
		    ;;
		swap ) 
		    runcom "Formatting ${fields[0]} as swap" mkswap ${fields[0]} || fail
		    ;;
	    esac
	done
    }
    format_partitions < $TMPFILESDIR/pfstab	
    # mount the partitions
    mount_partitions() {
	while read line; do
	    declare -a fields
	    fields=( $line )
	    case ${fields[2]} in
		reiserfs )
		    mount_partition ${fields[0]} ${fields[1]} || fail
		    ;;
		xfs )
		    mount_partition ${fields[0]} ${fields[1]} || fail
		    ;;
		jfs )
		    mount_partition ${fields[0]} ${fields[1]} || fail
		    ;;
		ext3 )
		    mount_partition ${fields[0]} ${fields[1]} || fail
		    ;;
		ext4 )
		    mount_partition ${fields[0]} ${fields[1]} || fail
		    ;;
		ext2 )
		    mount_partition ${fields[0]} ${fields[1]} || fail
		    ;;
	    esac
	done
    }
    # NOTE
    # I replaced cat truc | mount_partitions by mount_partitions < truc
    # because in the former case mount_partitions runs in a subshell and the $mounted_fs value is lost
    mount_partitions < $TMPFILESDIR/pfstab
    
    echo "++++++++++++++++++++++++++"
    mount
    echo "++++++++++++++++++++++++++"
    delay=0
else
    delay=10
fi

for stream in `cat $TMPFILESDIR/streams`; do
    if [ "$stream" = "linux" ]; then
	# partitions already formatted/mounted, just copy now
	# untar data from the master 'on the fly'
	echo -n "Linux copy is about to start "
	countdown $delay
	echo
	inc_ka_session
	$KADIR/ka-d-client -w $cur_ka_session -e "(cd $CHROOT; tar --extract --read-full-records --same-permissions --numeric-owner --sparse --file - ) 2>/dev/null" || fail
	runcom "Syncing disks" sync 
	echo Linux copy done.
	echo Creating excluded directories
	cat $TMPFILESDIR/excluded | recreate_dirs
    else
	# maybe receive some raw partition dumps
	echo Raw copy of $stream is about to start
	countdown $delay
	inc_ka_session
	/$KADIR/ka-d-client -w $cur_ka_session -e "dd of=$stream bs=65536" || fail
	delay=10
    fi
done


#write_MBRs
if [ -f $TMPFILESDIR/bootloader ] ; then
    BOOTLOADER=`cat $TMPFILESDIR/bootloader`
    INFO="(user choice)"
else
    BOOTLOADER="grub"
fi
echo "- bootloader is $BOOTLOADER $INFO"

# prepapre bootloader
BOOTPART=`cat $TMPFILESDIR/boot_part | cut -d ":" -f 1`
GET_UUID_GOLDEN_NODE=`cat $TMPFILESDIR/boot_part |  cut -d "=" -f 2 | cut -d " " -f 1 | sed -e 's/"//g'`
GET_UUID_LOCAL_BOOTPART=`blkid -o udev $BOOTPART | grep "ID_FS_UUID=" | cut -d "=" -f 2`
if test -f $CHROOT/etc/lilo.conf; then
        perl -pi -e "s/$GET_UUID_GOLDEN_NODE/$GET_UUID_LOCAL_BOOTPART/g" $CHROOT/etc/lilo.conf
fi
if test -f $CHROOT/boot/grub/menu.lst; then
        perl -pi -e "s/$GET_UUID_GOLDEN_NODE/$GET_UUID_LOCAL_BOOTPART/g" $CHROOT/boot/grub/menu.lst
fi

cp -vf $TMPFILESDIR/pfstab $CHROOT/etc/fstab
echo "none /proc proc defaults 0 0" >> $CHROOT/etc/fstab


cd $KADIR
$KADIR/prepare_node.sh "$BOOTLOADER"

# maybe there is a last dummy ka-deploy for synchronization
if test -f $TMPFILESDIR/delay; then
    sleep 1
    inc_ka_session
    runcom "Waiting source node signal to end installation" $KADIR/ka-d-client -w $cur_ka_session -e "cat" || fail
fi

sync
umount_partitions
# if you want to force a bootfslag on a disk, use this script
#$KADIR/bootable_flag.sh $FIRSTDRIVE

$KADIR/store_log.sh
echo -n " - Rebooting..."
countdown 3
do_reboot
