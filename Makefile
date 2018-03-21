all: Makefile.daemon Makefile.daemon_child
	$(MAKE) -f Makefile.daemon
	$(MAKE) -f Makefile.daemon_child

QMAKE=qmake

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
