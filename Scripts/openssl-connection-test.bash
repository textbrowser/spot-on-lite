#!/usr/bin/env bash

# Alexis Megas.

# A test script.

for i in {1..256}
do
    openssl s_client -connect 127.0.0.1:4710 &
done

sleep 10
