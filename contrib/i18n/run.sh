#!/bin/sh

if [ $# -ne 1 ]; then echo "usage: $0 <lang file>"; exit 1; fi

export PATH=$PATH:.

LANGFILE=$1
APHASH=../../aphash
NUM=299

for i in `i18n.pl < $LANGFILE | sort -rn | head -$NUM | cut -f2 -d ' ' `; do $APHASH $i; done | make_array.pl


