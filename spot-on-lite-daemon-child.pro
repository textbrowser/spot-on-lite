include (common.pro)

exists(/usr/bin/ecl) {
DEFINES += SPOTON_LITE_DAEMON_CHILD_ECL_SUPPORTED
LIBS += -lecl
QMAKE_CXXFLAGS += `ecl-config --cflags`
QMAKE_LFLAGS += `ecl-config --ldflags`
}

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
