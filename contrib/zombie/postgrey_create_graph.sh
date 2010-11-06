#!/bin/sh

export PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:/usr/local/rrdtool/bin

RRDTOOL=rrdtool
ONEMONTH=2678400
ONEWEEK=604800
ONEDAY=86400
#SIZEX=350
SIZEX=600
#SIZEY=200
SIZEY=300

RRDFILE=/var/lib/clapf/stat/postgrey.rrd

if [ $# -ne 3 ]; then echo "usage: $0 <image file> <title> <day|week|month>"; exit; fi

IMAGE=$1
TITLE=$2
TIME=$3

NOW=`date +%s`

if test "$TIME" = "week"
then
	START=`expr $NOW - $ONEWEEK`
elif test "$TIME" = "day"
then
	START=`expr $NOW - $ONEDAY`
else
	START=`expr $NOW - $ONEMONTH`
fi

$RRDTOOL graph $IMAGE \
        --start $START --end $NOW \
	--width $SIZEX --height $SIZEY \
	--imgformat PNG \
	--title "$TITLE" \
        DEF:dds0=$RRDFILE:ds0:MAX \
	DEF:dds1=$RRDFILE:ds1:MAX \
	DEF:dds2=$RRDFILE:ds2:MAX \
	DEF:dds3=$RRDFILE:ds3:MAX \
	"CDEF:greylisted=dds0,1,*" \
	"CDEF:passed=dds1,1,*" \
        "CDEF:notazombie=dds2,1,*" \
        "CDEF:whitelisted=dds3,1,*" \
	"AREA:greylisted#999999" \
        "LINE1:greylisted#999999:greylisted" \
	"LINE2:passed#990000:passed" \
        "LINE3:notazombie#006600:not a zombie" \
	"LINE4:whitelisted#00dd00:whitelisted" \
	> /dev/null

