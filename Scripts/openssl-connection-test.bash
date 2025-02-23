#!/usr/bin/env bash

# Alexis Megas.

# A test script.

for i in {1..256}
do
    openssl s_client -connect 127.0.0.1:5710 1>/dev/null 2>/dev/null &
done

sleep 10
