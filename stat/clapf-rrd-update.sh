#!/bin/sh

export PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:/usr/local/rrdtool/bin

RRDTOOL=rrdtool

if [ $# -ne 2 ]; then echo "usage: $0 <rrd file> <timestamp>:<ham>:<spam>"; exit; fi

$RRDTOOL update $1 $2

