include (common.pro)

macx {
INCLUDEPATH += /usr/local/opt/openssl/include
LIBS += -L/usr/local/opt/openssl/lib
}

LIBS += -lcrypto -lssl

HEADERS = spot-on-lite-daemon-child-tcp-client.h

RESOURCES =

SOURCES = spot-on-lite-daemon-child-main.cc \
          spot-on-lite-daemon-child-tcp-client.cc \
          spot-on-lite-daemon-sha.cc

PROJECTNAME = Spot-On-Lite-Daemon-Child
TARGET = Spot-On-Lite-Daemon-Child
