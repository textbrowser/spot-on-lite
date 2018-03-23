#!/bin/bash
# A test script.

for i in `seq 1 100`;
do
    nc -d rosemary-ipv4.tilaa.cloud 4710 1>/dev/null &
done
