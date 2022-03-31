#!/usr/bin/bash

mkdir -p ./usr/local/spot-on-lite/Documentation
make -j $(nproc)
cp -p ./Icons/monitor.png ./usr/local/spot-on-lite/.
cp -p ./Scripts/spot-on-lite-daemon.sh ./usr/local/spot-on-lite/.
cp -p ./Spot-On-Lite* ./usr/local/spot-on-lite/.
cp -p ./spot-on-lite-daemon.conf ./usr/local/spot-on-lite/.
cp -p ./spot-on-lite-monitor.sh ./usr/local/spot-on-lite/.
cp -pr ./Documentation/* ./usr/local/spot-on-lite/Documentation/.
