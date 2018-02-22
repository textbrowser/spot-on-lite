macx {
QMAKE_CXXFLAGS_RELEASE += -fPIE -fstack-protector-all -fwrapv \
                          -mtune=generic \
                          -Wall -Wcast-align -Wcast-qual \
                          -Werror -Wextra \
                          -Wno-unused-variable \
                          -Woverloaded-virtual -Wpointer-arith \
                          -Wstack-protector
} else:unix {
QMAKE_CXXFLAGS_RELEASE += -fPIE -fstack-protector-all -fwrapv \
                          -mtune=generic -pie \
                          -Wall -Wcast-align -Wcast-qual \
                          -Werror -Wextra \
                          -Wno-unused-variable \
                          -Woverloaded-virtual -Wpointer-arith \
                          -Wstack-protector
}

unix {
purge.commands = rm -f */*~ *~
}

CONFIG += qt warn_on

QMAKE_DISTCLEAN += -r temp
QMAKE_EXTRA_TARGETS = purge

LANGUAGE = C++
QT += network sql
TEMPLATE = app

greaterThan(QT_MAJOR_VERSION, 4) {
QT += concurrent
}

MOC_DIR = temp/moc
OBJECTS_DIR = temp/obj
RCC_DIR = temp/rcc
