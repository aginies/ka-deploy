#!/bin/bash

# Options passed to the ka-d-client -- i.e. how to find the server
# giving it explicitely :
#KAOPT="-h icluster70"
# or using a session name and a brodcast
# IDEA : maybe this option could be overriden by a kaopt= in the kernel command line ?
KACLIENT="/users/grappes/sderr/Ka/ka-deploy/src/ka-d-client"

kadserver=/users/grappes/sderr/Ka/ka-deploy/src/ka-d-server
KA_SESSION_BASE="-s kainstall"

inc_ka_session()
{
	(( ka_call_num++ ))
	cur_ka_session=$KA_SESSION_BASE$ka_call_num
}

compression=none

countdown()
{
        t=$1
        while [ $t -ne 0 ]; do
                /bin/echo -n "$t "
                sleep 1
                # move the cursor back
                # I use now tr instead of sed because busybox's sed adds a non-wanted carriage return
                # busybox's tr does not seem to know about the [: :] stuff (?)
                /bin/echo -n "$t " | tr " 0-9" $'\x08'
                t=`expr $t - 1`
        done
        # backspace only moves the cursor back, so at this point there's still a "1" to erase
        if [ "$1" -ne 0 ] ;then
                /bin/echo -n "  "$'\x08\x08'
        fi
}


fail()
{
	echo $*
	echo Aborted
	exit 1
}

imagedir=$1
description=$2

case $compression in 
	gzip1)
		comp="gzip -1 |"
		decomp="gunzip"
		;;
		
	gzip9)
		comp="gzip -9 |"
		decomp="gunzip"
		;;
	bzip2)
		comp="bzip2 |"
		decomp="bunzip"
		;;
	compress)
		comp="compress"
		decomp="uncompress"
		;;
	none)
		comp=""
		decomp=""
		;;
	*)
		echo Unkown compression mode
		comp=""
		decomp=""
		;;
esac

mkdir "$imagedir" || fail "could not create dir"
cd "$imagedir" || fail "could not enter dir"

# receive the partition table, fstab, etc from the source node
mkdir partfiles
echo "Receiving partitions information" 
inc_ka_session
$KACLIENT -w $cur_ka_session -e "( cd partfiles && tar xvf - )" || fail "ka client failed"

test -f partfiles/streams || fail "Missing streams file"
delay=3
streamnum=1
echo Compression = "-$comp/$decomp-"
for stream in `cat partfiles/streams`; do
		echo copy of $stream is about to start
		countdown $delay
		#$KACLIENT -w $KAOPT -e "$comp dd of=stream$streamnum bs=65536" || fail "ka client failed"
		inc_ka_session
		$KACLIENT -w $cur_ka_session -e "$comp split -b 1000m - stream$streamnum" || fail "ka client failed"
		(( streamnum++ ))
done

# maybe there is a last dummy ka-deploy for synchronization
if test -f partfiles/delay; then
	sleep 1
	inc_ka_session
	$KACLIENT -w $cur_ka_session -e "cat" || fail
fi


echo "$description" > description
cat > compression << EOF 
# compression
$comp
# decompression
$decomp
EOF

pwd
ls -l

