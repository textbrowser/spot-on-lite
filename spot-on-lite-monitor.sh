#!/usr/bin/env sh

# Alexis Megas.

export AA_ENABLEHIGHDPISCALING=1
export AA_USEHIGHDPIPIXMAPS=1
export QT_AUTO_SCREEN_SCALE_FACTOR=1

# Disable https://en.wikipedia.org/wiki/MIT-SHM.

export QT_X11_NO_MITSHM=1

if [ -r ./Spot-On-Lite-Monitor ] && [ -x ./Spot-On-Lite-Monitor ]
then
    echo "Launching a local Spot-On-Lite-Monitor."
    ./Spot-On-Lite-Monitor "$@"
    exit $?
elif [ -r /opt/spot-on-lite/Spot-On-Lite-Monitor ] && \
     [ -x /opt/spot-on-lite/Spot-On-Lite-Monitor ]
then
    echo "Launching an official Spot-On-Lite-Monitor."
    cd /opt/spot-on-lite && ./Spot-On-Lite-Monitor "$@"
    exit $?
else
    echo "Where is Spot-On-Lite-Monitor?"
    exit 1
fi
