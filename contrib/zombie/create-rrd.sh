#!/bin/sh

export PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:/usr/local/rrdtool/bin

RRDTOOL=rrdtool

if [ $# -ne 2 ]; then echo "usage: $0 <rrd file> <start timestamp>"; exit; fi

$RRDTOOL create $1 \
--start $2 \
--step 3600 \
DS:ds0:GAUGE:200000000:U:U \
DS:ds1:GAUGE:200000000:U:U \
DS:ds2:GAUGE:200000000:U:U \
RRA:MAX:0.5:1:8760 \
RRA:MAX:0.5:6:4380

