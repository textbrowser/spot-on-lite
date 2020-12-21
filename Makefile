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

all: Makefile.daemon Makefile.daemon_child
	$(MAKE) -f Makefile.daemon
	$(MAKE) -f Makefile.daemon_child

Makefile.daemon: spot-on-lite-daemon.pro
Makefile.daemon_child: spot-on-lite-daemon-child.pro
	$(QMAKE) -o Makefile.daemon spot-on-lite-daemon.pro
	$(QMAKE) -o Makefile.daemon_child spot-on-lite-daemon-child.pro

clean: Makefile.daemon Makefile.daemon_child
	$(MAKE) -f Makefile.daemon clean
	$(MAKE) -f Makefile.daemon_child clean

distclean: clean purge
	$(MAKE) -f Makefile.daemon distclean
	$(MAKE) -f Makefile.daemon_child distclean

purge: Makefile.daemon Makefile.daemon_child
	$(MAKE) -f Makefile.daemon purge
	$(MAKE) -f Makefile.daemon_child purge
