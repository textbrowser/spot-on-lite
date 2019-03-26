#!/bin/bash
# A test script.

for i in {1 .. 256};
do
    openssl s_client -connect tulip-ipv4.tilaa.cloud:4710 &
done

sleep 10
