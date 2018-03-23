include (common.pro)

INCLUDEPATH += Source

HEADERS = Source/spot-on-lite-daemon.h \
          Source/spot-on-lite-daemon-tcp-listener.h

RESOURCES =

SOURCES = Source/spot-on-lite-daemon.cc \
          Source/spot-on-lite-daemon-main.cc \
          Source/spot-on-lite-daemon-tcp-listener.cc

PROJECTNAME = Spot-On-Lite-Daemon
TARGET = Spot-On-Lite-Daemon
