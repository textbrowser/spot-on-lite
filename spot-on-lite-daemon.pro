include (common.pro)

CXX = clang++-6.0
INCLUDEPATH += Source

HEADERS = Source/spot-on-lite-daemon.h \
          Source/spot-on-lite-daemon-tcp-listener.h \
          Source/spot-on-lite-daemon-udp-listener.h

RESOURCES =

SOURCES = Source/spot-on-lite-daemon.cc \
          Source/spot-on-lite-daemon-main.cc \
          Source/spot-on-lite-daemon-tcp-listener.cc \
          Source/spot-on-lite-daemon-udp-listener.cc

PROJECTNAME = Spot-On-Lite-Daemon
TARGET = Spot-On-Lite-Daemon
