#!/bin/sh
# you need an ftp server with anonymous login

. /ka2/replication.conf

echo 'default login anonymous password user' > ~/.netrc
ftp -i $TFTPSERVER <<REF
lcd /tmp
mput ka_log*
quit
REF
