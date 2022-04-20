Spot-On-Lite supports Qt 5 LTS and 6 LTS. It has been tested on
Debian and OpenBSD. It compiles on Debian, Mac OS X, and OpenBSD.

Also included is a thread-safe Lisp implementation of SHA-512.
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
<li>ARM (Pi!)</li>
<li>Configurable.</li>
<li>Congestion control.</li>
<li>Cryptographic Discovery!</li>
<li>DTLS!</li>
<li>Debian, FreeBSD, OpenBSD.</li>
<li>Infinite listeners.</li>
<li>Monitor graphical interface.</li>
<li>Multiple instances of a daemon.</li>
<li>Per-process TCP clients.</li>
<li>Qt 5 LTS, Qt 6 LTS.</li>
<li>Sparc.</li>
<li>UDP unicast.</li>
<li>Vitals!</li>
</ul>