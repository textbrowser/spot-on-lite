include (common.pro)

INCLUDEPATH += .

HEADERS = spot-on-lite-daemon.h \
          spot-on-lite-daemon-tcp-listener.h

RESOURCES =

SOURCES = spot-on-lite-daemon.cc \
          spot-on-lite-daemon-main.cc \
          spot-on-lite-daemon-tcp-listener.cc

PROJECTNAME = Spot-On-Lite-Daemon
TARGET = Spot-On-Lite-Daemon
