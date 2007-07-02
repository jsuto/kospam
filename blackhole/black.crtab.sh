#!/bin/sh

# purge files accessed 3 hours (=180 minutes) ago
find /opt/av/blackhole -type f -amin +180 | xargs rm -f
