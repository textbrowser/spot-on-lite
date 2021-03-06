UNAME := $(shell uname)

ifeq ($(UNAME), FreeBSD)
	QMAKE=qmake
else ifeq ($(UNAME), NetBSD)
	QMAKE=/usr/pkg/qt4/bin/qmake -spec netbsd-g++
else ifeq ($(UNAME), OpenBSD)
	QMAKE=qmake-qt5
else
	QMAKE=qmake
endif

all: Makefile.daemon Makefile.daemon_child Makefile.monitor
	$(MAKE) -f Makefile.daemon
	$(MAKE) -f Makefile.daemon_child
	$(MAKE) -f Makefile.monitor

Makefile.daemon: spot-on-lite-daemon.pro
Makefile.daemon_child: spot-on-lite-daemon-child.pro
Makefile.monitor: spot-on-lite-monitor.pro
	$(QMAKE) -o Makefile.daemon spot-on-lite-daemon.pro
	$(QMAKE) -o Makefile.daemon_child spot-on-lite-daemon-child.pro
	$(QMAKE) -o Makefile.monitor spot-on-lite-monitor.pro

clean: Makefile.daemon Makefile.daemon_child Makefile.monitor
	$(MAKE) -f Makefile.daemon clean
	$(MAKE) -f Makefile.daemon_child clean
	$(MAKE) -f Makefile.monitor clean

distclean: clean purge
	$(MAKE) -f Makefile.daemon distclean
	$(MAKE) -f Makefile.daemon_child distclean
	$(MAKE) -f Makefile.monitor distclean

install: all
	install -d --group=staff /usr/local/spot-on-lite
	cp -n *.conf /usr/local/spot-on-lite
	install --group=staff Spot-On-Lite-* /usr/local/spot-on-lite
	install --group=staff *.sh Scripts/spot-on-lite-daemon.sh \
		/usr/local/spot-on-lite
	chmod -x /usr/local/spot-on-lite/*.conf
	chown root:staff /usr/local/spot-on-lite/*.conf

purge: Makefile.daemon Makefile.daemon_child Makefile.monitor
	$(MAKE) -f Makefile.daemon purge
	$(MAKE) -f Makefile.daemon_child purge
	$(MAKE) -f Makefile.monitor purge

uninstall:
	rm /usr/local/spot-on-lite/*
	rmdir /usr/local/spot-on-lite
