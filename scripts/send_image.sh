#! /bin/bash

unset LANG
unset LANGUAGE
unset LC_LANG
unset LC_MESSAGES

# NOTE



fail()
{
	echo "$*" 1>&2
	exit 1
}


say()
{
	echo "$*" 1>&2
}


nbnodes=1
kadserver=ka-d-server


KA_SESSION_BASE="-s kainstall"
ka_call_num=0


inc_ka_session()
{
	(( ka_call_num++ ))
	cur_ka_session=$KA_SESSION_BASE$ka_call_num
}




usage="
$0 : send a ka-deploy image 
Usage:
	$0 [options] image_dir

	options include:
		-d : delay (see same option in ka-d.sh)
		-n : number of client nodes (idem)
"
takembr=no
HD=/dev/hda
part_desc=""
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
      -d )
      	 release_delay=$2
	 shift; shift;
	 ;;
      -* )
        echo "${usage}" 1>&2; exit 1 ;;
      * )
        break ;;
   esac
done

imagedir="$1"

echo Image is $imagedir

cd $imagedir || fail "could not enter image directory"
test -d partfiles || fail "could not open image"

# read compression info
# print line 4
decomp=`cat compression | sed -n 4p`

if test -z "$decomp"; then
	echo Image is not compressed
else
	echo Image will be decompressed with $decomp
	decomp="| $decomp"
fi

tmpdir=/tmp/ka-d$$

mkdir $tmpdir
cp -a partfiles/* $tmpdir || fail "error reading image"


# maybe there is a delay coded IN the image
if [ $release_delay -eq 0 ]; then
	if test -f partfiles/delay; then
		release_delay=`cat partfiles/delay`
		echo Release day found = $release_delay
	fi
fi

echo Sending partition/filesystem/mount points informations...
inc_ka_session
$kadserver $cur_ka_session -n $nbnodes -e "(cd $tmpdir && tar c *)"


streamnum=1
for stream in `cat $tmpdir/streams`; do
	echo "Sending stream$streamnum ($stream)..."
	inc_ka_session
	$kadserver $cur_ka_session -n $nbnodes -e "cat stream$streamnum?? $decomp"
	(( streamnum++ ))
done



# if we want to release the clients with some delay...
if [ $release_delay -gt 0 ]; then
	inc_ka_session
	$kadserver $cur_ka_session -n $nbnodes -d $release_delay -e "echo Hello"
fi

rm -rf $tmpdir
