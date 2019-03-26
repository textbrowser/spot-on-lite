#!/bin/bash
# A test script.

for i in {1 .. 100};
do
    nc -d tulip-ipv4.tilaa.cloud 4710 1>/dev/null &
done
