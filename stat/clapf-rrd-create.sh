#!/bin/sh

export PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:/usr/local/rrdtool/bin

RRDTOOL=rrdtool

if [ $# -ne 2 ]; then echo "usage: $0 <rrd file> <start timestamp>"; exit; fi

$RRDTOOL create $1 \
--start $2 \
--step 3600 \
DS:ds0:GAUGE:200000000:U:U \
DS:ds1:GAUGE:200000000:U:U \
RRA:AVERAGE:0.5:1:16800 \
RRA:AVERAGE:0.5:6:72000 \
RRA:MAX:0.5:1:16800 \
RRA:MAX:0.5:6:72000
