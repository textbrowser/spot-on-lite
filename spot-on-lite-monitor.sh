#!/usr/bin/env sh

export AA_ENABLEHIGHDPISCALING=1
export AA_USEHIGHDPIPIXMAPS=1
export QT_AUTO_SCREEN_SCALE_FACTOR=1

# Disable https://en.wikipedia.org/wiki/MIT-SHM.

export QT_X11_NO_MITSHM=1

if [ -r ./Spot-On-Lite-Monitor ] && [ -x ./Spot-On-Lite-Monitor ]
then
    echo "Launching local Spot-On-Lite-Monitor."
    exec ./Spot-On-Lite-Monitor "$@"
    exit $?
elif [ -r /usr/local/spot-on-lite/Spot-On-Lite-Monitor ] && \
     [ -x /usr/local/spot-on-lite/Spot-On-Lite-Monitor ]
then
    cd /usr/local/spot-on-lite && exec ./Spot-On-Lite-Monitor "$@"
    exit $?
else
    exit 1
fi
