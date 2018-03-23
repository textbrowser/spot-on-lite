#!/bin/bash
# A test script.

for i in `seq 1 100`;
do
    openssl s_client -connect rosemary-ipv4.tilaa.cloud:4710 &
done
sleep 10
