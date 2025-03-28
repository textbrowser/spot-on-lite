#!/usr/bin/env bash

# Alexis Megas.

if [ ! -x /usr/bin/dpkg ]
then
    echo "Please install dpkg."
    exit 1
fi

if [ ! -x /usr/bin/dpkg-deb ]
then
    echo "Please install dpkg-deb."
    exit 1
fi

if [ ! -x /usr/bin/fakeroot ]
then
    echo "Please install fakeroot."
    exit 1
fi

if [ ! -r common.pro ]
then
    echo "Please execute $0 from the primary directory."
    exit 1
fi

make distclean 2>/dev/null
mkdir -p ./opt/spot-on-lite/Documentation
make -j $(nproc)
cp -p ./Icons/monitor.png ./opt/spot-on-lite/.
cp -p ./Scripts/spot-on-lite-daemon.sh ./opt/spot-on-lite/.
cp -p ./Spot-On-Lite* ./opt/spot-on-lite/.
cp -p ./spot-on-lite-daemon.conf ./opt/spot-on-lite/.
cp -p ./spot-on-lite-monitor.sh ./opt/spot-on-lite/.
cp -pr ./Documentation/* ./opt/spot-on-lite/Documentation/.
rm -fr ./opt/spot-on-lite-debian/Documentation/*.qrc
mkdir -p spot-on-lite-debian/opt

architecture="$(dpkg --print-architecture)"

if [ "$architecture" = "armhf" ]
then
    cp -pr ./Distributions/PiOS32 spot-on-lite-debian/DEBIAN
else
    cp -pr ./Distributions/PiOS64 spot-on-lite-debian/DEBIAN
fi

cp -r ./opt/spot-on-lite spot-on-lite-debian/opt/.
fakeroot dpkg-deb \
	 --build spot-on-lite-debian Spot-On-Lite-2025.03.05_$architecture.deb
rm -fr ./opt
rm -fr spot-on-lite-debian
make distclean
