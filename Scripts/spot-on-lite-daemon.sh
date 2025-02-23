#!/usr/bin/env sh

# Alexis Megas.

if [ -r /opt/spot-on-lite ] && [ -x /opt/spot-on-lite ]
then
    cd /opt/spot-on-lite && \
    ./Spot-On-Lite-Daemon \
    --configuration-file /opt/spot-on-lite/spot-on-lite-daemon.conf

    rc=$?

    if [ $rc -eq 0 ]
    then
	echo "Spot-On-Lite-Daemon was launched."
    else
	echo "Could not launch Spot-On-Lite-Daemon."
	exit $rc
    fi
else
    echo "The directory /opt/spot-on-lite is not valid."
    exit 1
fi

exit 0
