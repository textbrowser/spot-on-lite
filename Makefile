UNAME := $(shell uname)

ifeq ($(UNAME), FreeBSD)
	QMAKE=$(shell which qmake || which qmake6)
else ifeq ($(UNAME), NetBSD)
	QMAKE=qmake
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

purge: Makefile.daemon Makefile.daemon_child Makefile.monitor
	$(MAKE) -f Makefile.daemon purge
	$(MAKE) -f Makefile.daemon_child purge
	$(MAKE) -f Makefile.monitor purge
