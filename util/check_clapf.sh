#!/bin/sh
##
## check_clapf.sh, 2006.11.16, SJ
##
## periodicaly check clapf-related services and restart them if necessary

NUM=`ps uaxw | grep clapf | grep -c ^av`

if [ $NUM -eq 0 ]; then /bin/su av -c 'export TMPDIR=/opt/av; /usr/local/bin/clapf 2> /dev/null &' ; fi

