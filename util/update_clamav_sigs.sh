#!/bin/sh
##
## update_clamav_sigs.sh, 2007.08.24, SJ
##
## this is just an example, how to refresh the antivirus database
## and let clapf know when it should reload the signatures
## This is only necessary if you are using libclamav

/usr/local/bin/freshclam --quiet --user clamav --datadir=/usr/local/share/clamav
if [ $? -eq 0 ]
then
	killall -ALRM clapf
fi
