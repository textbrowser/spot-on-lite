CONFIG -= app_bundle
QMAKE_CXXFLAGS_RELEASE -= -O2

freebsd-* {
QMAKE_CXXFLAGS_RELEASE += -fPIE -fstack-protector-all -fwrapv \
                          -mtune=native -O3 \
			  -Wall -Wcast-align -Wcast-qual \
			  -Wextra \
			  -Woverloaded-virtual -Wpointer-arith \
                          -Wstack-protector -Wstrict-overflow=5
} else:linux-* {
QMAKE_CXXFLAGS_RELEASE += -fPIE -fstack-protector-all -fwrapv -pie -O3 \
                          -mtune=native \
                          -Wall -Wcast-qual \
                          -Wextra \
                          -Wno-unused-variable \
                          -Woverloaded-virtual -Wpointer-arith \
                          -Wstack-protector
} else:macx {
QMAKE_CXXFLAGS_RELEASE += -fPIE -fstack-protector-all -fwrapv \
                          -mtune=native -O3 \
                          -Wall -Wcast-align -Wcast-qual \
                          -Wextra \
                          -Wno-unused-variable \
                          -Woverloaded-virtual -Wpointer-arith \
                          -Wstack-protector
} else:netbsd-* {
INCLUDEPATH += /usr/pkg/qt4/include/QtCore \
               /usr/pkg/qt4/include/QtNetwork \
               /usr/pkg/qt4/include/QtSql
LIBS += -L/usr/pkg/qt4/lib
QMAKE_CXXFLAGS_RELEASE += -fPIE -fwrapv -pie -O3 \
                          -Wall -Wcast-qual \
                          -Wextra \
                          -Wno-unused-variable \
                          -Woverloaded-virtual -Wpointer-arith \
                          -Wstack-protector
QMAKE_MOC = /usr/pkg/qt4/bin/moc
} else:openbsd-* {
QMAKE_CXXFLAGS_RELEASE += -fPIE -fstack-protector-all -fwrapv -pie -O3 \
                          -Wall -Wcast-qual \
                          -Wextra \
                          -Wno-unused-variable \
                          -Woverloaded-virtual -Wpointer-arith \
                          -Wstack-protector
} else:unix {
QMAKE_CXXFLAGS_RELEASE += -fPIE -fwrapv -pie -O3 \
                          -Wall -Wcast-qual \
                          -Wextra \
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
