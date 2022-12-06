Spot-On-Lite supports Qt 5 LTS and 6 LTS. It has been tested on
Debian and OpenBSD. Spot-On-Lite compiles on Debian, Mac OS X, and OpenBSD.

ARM, PowerPC, and Sparc are supported. Qt 5.5 is also supported for
PowerPC.

Included is a thread-safe Lisp implementation of SHA-512.
Super fast, super cute.

On Linux, limiting the number of processes may be achieved
via /etc/security/limits.conf.

spot-on-lite-daemon		hard	nproc		256

The LD_LIBRARY_PATH may require adjusting.

export LD_LIBRARY_PATH=/usr/local/Trolltech/Qt-5.12.x/lib

Debian administrators, please add spot-on-lite-daemon.debian.sh to
/etc/init.d and execute sudo update-rc.d spot-on-lite-daemon.debian.sh defaults.

Summary of Spot-On-Lite
<ul>
<li>AMD64 and ARMHF distributions.</li>
<li>ARM (Pi!)</li>
<li>Configurable.</li>
<li>Congestion control.</li>
<li>Cryptographic Discovery!</li>
<li>DTLS!</li>
<li>Debian, FreeBSD, Mac, OpenBSD.</li>
<li>Documentation manuals.</li>
<li>Infinite listeners.</li>
<li>Monitor interface.</li>
<li>Multiple instances of a daemon.</li>
<li>Per-process TCP clients.</li>
<li>PowerPC!</li>
<li>Qt 5 LTS, Qt 6 LTS.</li>
<li>Sparc.</li>
<li>UDP unicast.</li>
<li>Vitals!</li>
</ul>

```
/usr/local/spot-on-lite/Spot-On-Lite-Daemon --statistics
PID                 652
Bytes Accumulated   0
Bytes Read          0
Bytes Written       0
IP Information      
Memory              11,204
Name                ./Spot-On-Lite-Daemon
Uptime              1254994 Second(s)
Type                daemon
```

![alt text](https://github.com/textbrowser/spot-on-lite/blob/Images/spot-on-lite-monitor.png)
