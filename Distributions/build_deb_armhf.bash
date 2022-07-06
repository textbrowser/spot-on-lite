#!/bin/bash

mkdir -p ./usr/local/spot-on-lite/Documentation
make -j $(nproc)
cp -p ./Icons/monitor.png ./usr/local/spot-on-lite/.
cp -p ./Scripts/spot-on-lite-daemon.sh ./usr/local/spot-on-lite/.
cp -p ./Spot-On-Lite* ./usr/local/spot-on-lite/.
cp -p ./TO-DO ./usr/local/spot-on-lite/Documentation/.
cp -p ./spot-on-lite-daemon.conf ./usr/local/spot-on-lite/.
cp -p ./spot-on-lite-monitor.sh ./usr/local/spot-on-lite/.
cp -pr ./Documentation/* ./usr/local/spot-on-lite/Documentation/.

mkdir -p spot-on-lite-raspberry/usr/local
cp -pr ./RASPBERRY spot-on-lite-raspberry/.
cp -r ./usr/local/spot-on-lite spot-on-lite-raspberry/usr/local/.
fakeroot dpkg-deb --build spot-on-lite-raspberry Spot-On-Lite-2022.07.05_armhf.deb
mv Spot-On-Lite-2022.07.05_armhf.deb ~/Desktop/.
rm -fr ./usr
rm -fr spot-on-lite-raspberry
make distclean
