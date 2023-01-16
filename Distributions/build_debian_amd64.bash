#!/usr/bin/env bash
# Alexis Megas.

if [ ! -x /usr/bin/dpkg-deb ]; then
    echo "Please install dpkg-deb."
    exit
fi

if [ ! -x /usr/bin/fakeroot ]; then
    echo "Please install fakeroot."
    exit 1
fi

make distclean 2>/dev/null
mkdir -p ./usr/local/spot-on-lite/Documentation
make -j $(nproc)
cp -p ./Icons/monitor.png ./usr/local/spot-on-lite/.
cp -p ./Scripts/spot-on-lite-daemon.sh ./usr/local/spot-on-lite/.
cp -p ./Spot-On-Lite* ./usr/local/spot-on-lite/.
cp -p ./TO-DO ./usr/local/spot-on-lite/Documentation/.
cp -p ./spot-on-lite-daemon.conf ./usr/local/spot-on-lite/.
cp -p ./spot-on-lite-monitor.sh ./usr/local/spot-on-lite/.
cp -pr ./Documentation/* ./usr/local/spot-on-lite/Documentation/.

mkdir -p spot-on-lite-debian/usr/local
cp -pr ./DEBIAN spot-on-lite-debian/.
cp -r ./usr/local/spot-on-lite spot-on-lite-debian/usr/local/.
fakeroot dpkg-deb --build spot-on-lite-debian Spot-On-Lite-2023.01.15_amd64.deb
mv Spot-On-Lite-2023.01.15_amd64.deb ~/Desktop/.
rm -fr ./usr
rm -fr spot-on-lite-debian
make distclean
