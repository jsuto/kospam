#!/bin/sh

RRDTOOL=/usr/local/bin/rrdtool
ONEMONTH=2678400
ONEWEEK=604800
ONEDAY=86400
SIZEX=600
SIZEY=280

if [ $# -ne 4 ]; then echo "usage: $0 <rrd file> <image file> <title> <day|week|month>"; exit; fi

RRDFILE=$1
NOW=`date +%s`
TITLE=$3

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
        "LINE1:reqs#1ac090:$TITLE" 

