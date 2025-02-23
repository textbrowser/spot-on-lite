include (common.pro)

HEADERS = Source/spot-on-lite-daemon.h \
          Source/spot-on-lite-daemon-child.h \
          Source/spot-on-lite-daemon-tcp-listener.h \
          Source/spot-on-lite-daemon-udp-listener.h
PROJECTNAME = Spot-On-Lite-Daemon
RESOURCES =
SOURCES = Source/spot-on-lite-daemon.cc \
          Source/spot-on-lite-daemon-child.cc \
          Source/spot-on-lite-daemon-main.cc \
          Source/spot-on-lite-daemon-sha.cc \
          Source/spot-on-lite-daemon-tcp-listener.cc \
          Source/spot-on-lite-daemon-udp-listener.cc
TARGET = Spot-On-Lite-Daemon
