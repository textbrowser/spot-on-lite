#!/usr/bin/env sh

# Alexis Megas.

cd /opt/spot-on-lite && \
exec ./Spot-On-Lite-Daemon \
     --configuration-file /opt/spot-on-lite/spot-on-lite-daemon.conf
exit $?
