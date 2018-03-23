include (common.pro)

macx {
INCLUDEPATH += /usr/local/opt/openssl/include
LIBS += -L/usr/local/opt/openssl/lib
}

LIBS += -lcrypto -lssl

HEADERS = Source/spot-on-lite-daemon-child-tcp-client.h

RESOURCES =

SOURCES = Source/spot-on-lite-daemon-child-main.cc \
          Source/spot-on-lite-daemon-child-tcp-client.cc \
          Source/spot-on-lite-daemon-sha.cc

PROJECTNAME = Spot-On-Lite-Daemon-Child
TARGET = Spot-On-Lite-Daemon-Child
