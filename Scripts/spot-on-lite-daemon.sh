#!/usr/bin/env sh

cd /usr/local/spot-on-lite && exec ./Spot-On-Lite-Daemon --configuration-file /usr/local/spot-on-lite/spot-on-lite-daemon.conf
exit $?
