include (common.pro)

unix {
purge.commands = rm -f */*~ *~
}

CONFIG += qt warn_on

unix {
QMAKE_CXXFLAGS_RELEASE += -fPIE -fstack-protector-all -fwrapv \
                          -mtune=generic -pie \
                          -Wall -Wcast-align -Wcast-qual \
                          -Werror -Wextra \
                          -Wno-unused-variable \
                          -Woverloaded-virtual -Wpointer-arith \
                          -Wstack-protector
}

QMAKE_DISTCLEAN += -r temp
QMAKE_EXTRA_TARGETS = purge

LANGUAGE = C++
LIBS += -lcrypto -lssl
QT += network sql
TEMPLATE = app

greaterThan(QT_MAJOR_VERSION, 4) {
}

MOC_DIR = temp/moc
OBJECTS_DIR = temp/obj
RCC_DIR = temp/rcc

INCLUDEPATH += .

HEADERS = spot-on-lite-daemon-child-tcp-client.h

RESOURCES =

SOURCES = spot-on-lite-daemon-child-main.cc \
          spot-on-lite-daemon-child-tcp-client.cc

PROJECTNAME = Spot-On-Lite-Daemon-Child
TARGET = Spot-On-Lite-Daemon-Child
