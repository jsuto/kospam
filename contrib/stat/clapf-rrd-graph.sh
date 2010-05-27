#!/bin/sh

export PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:/usr/local/rrdtool/bin

RRDTOOL=rrdtool
ONEMONTH=2678400
ONEWEEK=604800
ONEDAY=86400
SIZEX=600
SIZEY=280

if [ $# -ne 4 ]; then echo "usage: $0 <rrd file> <image file> <title> <day|week|month>"; exit; fi

RRDFILE=$1
TITLE=$3
NOW=`date +%s`

if test "$4" = "week"
then
	START=`expr $NOW - $ONEWEEK`
elif test "$4" = "day"
then
	START=`expr $NOW - $ONEDAY`
else
	START=`expr $NOW - $ONEMONTH`
fi


$RRDTOOL graph $2 \
        --start $START --end $NOW \
	--width $SIZEX --height $SIZEY \
	--imgformat PNG \
	--title "$TITLE" \
        DEF:dds0=$RRDFILE:ds0:MAX \
	DEF:dds1=$RRDFILE:ds1:MAX \
	"CDEF:reqs=dds0,1,*" \
	"CDEF:traff=dds1,1,*" \
	"AREA:reqs#1ac090" \
        "LINE1:reqs#1ac090:Ham" \
	"LINE2:traff#d03080:Spam" \
	> /dev/null

