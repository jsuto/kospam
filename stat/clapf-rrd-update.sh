#!/bin/sh

RRDTOOL=/usr/local/bin/rrdtool

if [ $# -ne 2 ]; then echo "usage: $0 <rrd file> <timestamp>:<ham>:<spam>"; exit; fi

$RRDTOOL update $1 $2

