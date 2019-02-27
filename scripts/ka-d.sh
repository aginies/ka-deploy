#! /bin/bash

# NOTE
# this script will collect many options, write them in files under a temp directory, and then
# send a tarball of this directory to the destination nodes.
# these files are :
#	pfstab : list of mount points
#	fdisk_commands : list of fdisk commands to create a new partition table
#	 	OR 
#	partition_tab : raw dump of the partition table of this machine
#	excluded : list of exluded directories that the destination node will have to re-mkdir (such as /tmp) [ with permissions and owner ]
#	streams : list of data streams : first should be 'linux', then one line per raw partition dump
# 	MBR : master boot record, if we don't use lilo
#	delay : empty file, but, when it is present, the clients will have to use the -w flag
# UPDATE : fdisk_commands, partition_tab, and MBR are not appended by the drive name, such as hda
#	command: command to run (e.g. lilo)


# beware of locale
unset LANGUAGE
unset LANG
unset LC_LANG
unset LC_MESSAGES


nbnodes=1
kadserver=ka-d-server
# session name usefull to tell NODE we are in install session
KA_SESSION_BASE="-s kainstall"
ka_call_num=0
tar_opts=""
excluded=""
STAGE2_PATH="/mnt/ka"
excludedisk="TOTO"

usage="

$0 : clone this machine
Usage:
	-h, --help : display this message
	-n num : specify the number of (destination) nodes
	-x 'dir|dir2' : exclude directory 
        -X 'sdb|sdc' : exclude sdb for the replication
        -m drive : copy the master boot record (for windows) of this drive
	-M drive file : use 'file' as master boot record (must be 446 bytes long) for the specified drive
	-D partition : also copy partition 'partition'
	-p drive pdesc : use 'pdesc' file as partition scheme (see doc) for the specified drive
	-d delay : delay beteween the release of 2 clients (1/10 second)
	-r 'grub|lilo' : choose the bootloader (you can add mkinitrd options)

	ie: ka-d.sh -n 3 -p sda /tmp/desc -X sdb -r 'grub --with=ata_piix --with=piix'
	"

if [ ! -f $STAGE2_PATH/ka/ka-d-client ]; then
    echo "You should have a valid ka rescue boot image mounted in $STAGE2_PATH"
    echo "check that STAGE2_PATH/ka/ka-d-client exist"
    exit	
fi


# write message to STDERR
say() {
    echo "$*" 1>&2
}

# drive names can be given as sda or /dev/sda --> transform it into sda
dev_drive() {
    local drv
    drv=`echo "$*" | sed 's#^/dev/##'`
    if [ -z "$drv" ]; then
	say Missing drive name
        echo "${usage}" 1>&2;
	exit 1 
    fi
    echo "$drv"
}

takembr=""
part_desc_files=""
dump_partitions=""
release_delay=0
while test $# -gt 0 ; do
    case "$1" in
	-h | --help | --h* )
            echo "${usage}"; exit 0 ;;
	-n )
            shift
	    nbnodes=$1
            shift
            ;;
	-x )
            tar_opts="$tar_opts --exclude $2"
	    excluded="$excluded $2"
            shift;shift;
            ;;
	-X)
	    excludedisk=$2
	    shift;shift;
	    ;;
	-m )
      	    shift
	    drv=$1
	    drv=`dev_drive $drv`
	    test $? -eq 0 || exit 1
      	    takembr="$takembr $drv /dev/$drv"
	    shift
	    ;;
	-M )
      	    shift
	    drv=$1
	    mbrfile=$2
	    if ! test -r "$mbrfile"; then
		echo Cannot read MBR file $mbrfile
		exit 1
	    fi
	    drv=`dev_drive $drv`
	    test $? -eq 0 || exit 1
      	    takembr="$takembr $drv $mbrfile"
	    shift
	    shift
	    ;;
	-D )
      	    dump_partitions="$dump_partitions $2"
	    shift; shift
	    ;;
	-p )
      	    shift
	    drv=$1
	    partfile=$2
	    if ! test -r "$partfile"; then
		echo Cannot read file $mbrfile
		exit 1
	    fi
	    drv=`dev_drive $drv`
	    test $? -eq 0 || exit 1
      	    part_desc_files="$part_desc_files $drv $partfile"
	    shift
	    shift
	    ;;
	-d )
      	    release_delay=$2
	    shift; shift;
	    ;;
	-r )
      	    bootloader="$2"
	    shift; shift;
	    ;;
	
	-* )
            echo "${usage}" 1>&2; exit 1 ;;
	* )
        break ;;
    esac
done

echo takembr = $takembr
echo desc = $part_desc_files

if [ "$UID" -ne 0 ]; then
    echo '*************************************************************************************'
    echo '*************** This script should be executed by root only ! ***********************'
    echo '*************************************************************************************'
    sleep 3
    echo trying anyway....
fi

tmpdir=/tmp/ka-d$$
mkdir $tmpdir

inc_ka_session() {
    (( ka_call_num++ ))
    cur_ka_session=$KA_SESSION_BASE$ka_call_num
}

create_fdisk_commands() {
    com=""
    
    add_line() {
	com="$com""$*
"
    }
    # delete all existing partitions
    num_line=0
    num_prim=0
    num_logical=4
    while read line; do
	declare -a fields
	(( num_line++ ))
	iscomment=${line/##*/is_comment}
	if [ "$iscomment" = "is_comment" ]; then continue; fi # skip comment lines
	fields=( $line )
	logical=false
	if [ ${#fields[*]} -eq 3 ]; then
	    if [ ${fields[0]} = "logical" ]; then
		logical=true
		parttype=${fields[1]}
		size=${fields[2]}
	    else
		say Error line $num_line
		exit 1
	    fi
	else
	    parttype=${fields[0]}
	    size=${fields[1]}
	fi
	parttype=${parttype//linux/type83}
	parttype=${parttype//swap/type82}
	
		# n = new partition
	add_line n
	if [ $logical = true ]; then
	    (( num_logical++ ))
	    this_part=$num_logical
	    add_line l
	else
	    (( num_prim++ ))
	    if [ "$parttype" = "extended" ]; then
		add_line e
	    else
		add_line p
		this_part=$num_prim
	    fi
	    add_line $num_prim
	fi
	
		# accept fdisk's proposal for starting sector
	add_line
	
	if [ "$size" = fill ]; then 
			# accept fdisk's proposal for ending sector
	    add_line
	else
	    add_line +"$size"M
	fi
	
		# set the partition type
	if [ "$parttype" != "extended" ]; then
	    numtype=${parttype//type/}
	    if [ "type$numtype" != $parttype ]; then
		say Bad partition type line $num_line
		exit 1
	    fi

			# t = change partition type
	    add_line t
	    if [ "$this_part" != "1" ]; then
		add_line $this_part
	    fi
	    add_line $numtype
	    say "    Added partition $this_part : type $numtype"
	fi
    done	
    add_line p
    add_line w
    #add_line q
    echo "$com"
}

add_mnt() {
    local dev
    local mnt
    local type
    dev=$1
    mnt=$2
    type=$3
	# beware of nasty automount !
    if echo $dev | grep -q '^LABEL'; then
	say "$mnt is automounted, trying mount"
	dev=`mount | grep " $mnt " | cut -d ' ' -f 1`
	if [ "$dev" ]; then
	    say "found $dev"
	else
	    fail "$mnt device Not found !"
	fi
    fi
	echo $dev $mnt $type
}

fstab_read_lines() {
    while read line; do
	declare -a fields
	fields=( $line )
	
	case ${fields[2]} in
	    reiserfs | xfs | jfs | ext3 | ext4 | ext2 | swap )
		add_mnt ${fields[0]} ${fields[1]} ${fields[2]}
		;;
	esac
    done
}

take_all_MBRs() {
    local fld
    local dev
    local file
    dev=""
    file=""
	# information in $takembr is in pairs, and for each drive, first read drive name, then file name
    for fld in $takembr; do
	if [ -z "$dev" ]; then
	    dev=$fld
	else
	    file=$fld
	    echo reading MBR for $dev from $file
	    if ! dd of=$tmpdir/MBR$dev if=$file bs=1 count=446; then
		echo Could not read MBR from $file 
		exit 1
	    fi
	    dev=""
	fi
    done
}

take_partdesc_files() {
    local fld
    local dev
    local file
    dev=""
    file=""
	# information is in pairs, and for each drive, first read drive name, then file name
    for fld in $part_desc_files; do
	if [ -z "$dev" ]; then
	    dev=$fld
	else
	    file=$fld
	    echo + Reading partition table description for $dev
	    cat $file | create_fdisk_commands > $tmpdir/fdisk_commands$dev
	    test "$?" -eq 0 || exit 1
	    dev=""
	fi
    done
}

get_dev_from_uuid() {
	UUID=$1
	SDX_UUID=`ls -l /dev/disk/by-uuid/* | grep $UUID | cut -d ">" -f 2`
	#SDX_UUID=sda5
	return $SDX_UUID
}

# prepare a $tmpdir/pfstab

# map UUID to partition name (/dev/sdX)
cp -vf /etc/fstab $tmpdir/pfstab.tmp
for line in /etc/fstab
do
	# UUID=4e987291-e13e-4614-bdc8-05863173e2c2
	UUID=`grep UUID $line | cut -d " " -f 1 | cut -d "=" -f 2`
	for p in $UUID
	do
		SDX_UUID=`ls -l /dev/disk/by-uuid/* | grep $p | cut -d ">" -f 2 | sed -e 's|../||g' | sed -e 's| ||g'`
		#SDX_UUID=sda5
		# change UUID by SDname
		perl -pi -e "s|UUID=${p}|/dev/${SDX_UUID}|" $tmpdir/pfstab.tmp
	done
	# remove unwanted disk
	cat $tmpdir/pfstab.tmp | grep "^/dev/" | grep -vP "$excludedisk" | grep -v "cdrom" > $tmpdir/pfstab
done
# UUID map

echo "+ Mount points :"  
cat $tmpdir/pfstab | sed 's/^/     /'
echo "+ Hard drives :"
drives=`cat $tmpdir/pfstab | cut -d ' ' -f 1 | grep '^/dev/' | sed 's#/dev/\([a-z]*\)[0-9]*#\1#' | sort -u`
if echo "$drives" | grep -q "^md"; then
    drives=`echo "$drives" | grep -v "^md"`
fi

echo "$drives" | sed 's/^/     /'

take_partdesc_files

for drv in $drives ;do
    if ! test -f $tmpdir/fdisk_commands$drv; then
	echo + No description, reading raw partition table for $drv
	/sbin/sfdisk -d /dev/$drv | grep Id > $tmpdir/partition_tab$drv
    fi
done

# when excludind directories (such as /tmp), we want to exclude the dir's contents but not the directory itself
# tar will exclude both, so we take care to recreate the directory 
# UPDATED : we also take care of keeping the same permission/owner
touch $tmpdir/excluded
for i in $excluded; do
    if test -d $i; then
	ls=`ls -dln $i`
	perm=`echo "$ls" | awk ' {printf "u=%s g=%s o=%s\n",substr($1, 2, 3), substr($1, 5,3), substr($1, 8,3) } ' | tr -d -`
		# in ls output, t means t and x, s means s and x, S means s, T means t (chmodly speaking)
	perm=`echo "$perm" | sed -e 's/s/sx/g' -e 's/t/tx/g'`
	perm=`echo "$perm" | sed -e 's/S/s/g' -e 's/T/t/g'`
	fuid=`echo "$ls" | awk ' {printf "%s\n",$3} '`
	fgid=`echo "$ls" | awk ' {printf "%s\n",$4} '`
	echo $perm $fuid $fgid $i >> $tmpdir/excluded
    fi
done

if test -s $tmpdir/excluded; then
    echo + Excluded directories :
    cat $tmpdir/excluded | sed 's/^/     /'
fi

# find mount points to add to the tar command line
tar_dirs=`cat $tmpdir/pfstab | cut -d ' ' -f 2 | grep ^/`
# remove \n's
tar_dirs=`echo $tar_dirs`
echo + Included mount points : $tar_dirs

touch $tmpdir/streams
echo linux > $tmpdir/streams
# raw-dump some partitions (? windows ?)
if [ "$dump_partitions" ]; then
    echo + Additional partitions to be sent:
    for part in $dump_partitions; do
	echo "    $part"
	echo $part >> $tmpdir/streams
    done
fi

take_all_MBRs

if [ $release_delay -gt 0 ]; then
    echo $release_delay > $tmpdir/delay
fi


if [ "$bootloader" ]; then
    echo + Bootloader is: $bootloader
    echo "$bootloader" > $tmpdir/bootloader
fi


# retrieve the partition wich contains /boot to adjust lilo/grub conf
bootpart_dev=`cd /boot/ && df . | grep /dev | cut -d ' ' -f 1`
bootpart=`blkid $bootpart_dev`
echo "$bootpart" > $tmpdir/boot_part
/sbin/blkid > $tmpdir/blkid_golden_node

echo +++ Sending Stage2 +++
$kadserver -s getstage2 -n $nbnodes -e "(cd $STAGE2_PATH; tar --create --one-file-system --sparse $tar_opts . )"

echo " - Sending partition/filesystem/mount points informations..."
echo " +++ Running ka-deploy +++"
inc_ka_session
$kadserver $cur_ka_session -n $nbnodes -e "(cd $tmpdir && tar c *)"
echo " WAITING node (partition/format)"

rm -rf $tmpdir

echo " - Sending Linux filesystem..."
echo " +++ Running ka-deploy +++" 
inc_ka_session
$kadserver $cur_ka_session -n $nbnodes -e "(cd /; tar --create --one-file-system --sparse $tar_opts $tar_dirs)"

# raw-dump some partitions (? windows ?)
if [ "$dump_partitions" ]; then
    for part in $dump_partitions; do
	echo Sending $part
	echo +++ Running ka-deploy +++
	inc_ka_session
	$kadserver $cur_ka_session -n $nbnodes -e "dd if=$part"
    done
fi

# if we want to release the clients with some delay...
if [ $release_delay -gt 0 ]; then
    inc_ka_session
    $kadserver $cur_ka_session -n $nbnodes -d $release_delay -e "echo Hello"
fi

