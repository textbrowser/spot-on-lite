CONFIG += qt warn_on
CONFIG -= app_bundle
LANGUAGE = C++
QMAKE_CXXFLAGS_RELEASE -= -O2

freebsd-* {
QMAKE_CXX = clang++
QMAKE_CXXFLAGS_RELEASE += -fPIE -fstack-protector-all -fwrapv \
                          -march=generic -O3 \
			  -Wall -Wcast-align -Wcast-qual -Wextra \
			  -Woverloaded-virtual -Wpointer-arith \
                          -Wstack-protector -Wstrict-overflow=5 -pedantic \
			  -std=c++11
} else:linux-* {
QMAKE_CXXFLAGS_RELEASE += -fPIE -fstack-protector-all -funroll-loops -fwrapv \
                          -pie -O3 \
                          -Wall -Wcast-qual -Wextra \
                          -Wno-class-memaccess -Wno-unused-variable \
                          -Woverloaded-virtual -Wpointer-arith \
                          -Wstack-protector -pedantic -std=c++11
} else:macx {
QMAKE_CXXFLAGS_RELEASE += -fPIE -fstack-protector-all -fwrapv -O3 \
                          -Wall -Wcast-align -Wcast-qual -Wextra \
                          -Wno-unused-variable \
                          -Woverloaded-virtual -Wpointer-arith \
                          -Wstack-protector -pedantic -std=c++11
} else:netbsd-* {
INCLUDEPATH += /usr/pkg/qt4/include/QtCore \
               /usr/pkg/qt4/include/QtNetwork \
               /usr/pkg/qt4/include/QtSql
LIBS += -L/usr/pkg/qt4/lib
QMAKE_CXXFLAGS_RELEASE += -fPIE -fwrapv -pie -O3 \
                          -Wall -Wcast-qual -Wextra \
                          -Wno-unused-variable \
                          -Woverloaded-virtual -Wpointer-arith \
                          -Wstack-protector -pedantic -std=c++11
QMAKE_MOC = /usr/pkg/qt4/bin/moc
} else:openbsd-* {
QMAKE_CXXFLAGS_RELEASE += -fPIE -fstack-protector-all -fwrapv \
                          -mcpu=ultrasparc -mtune=ultrasparc -pie -O3 \
                          -Wall -Wcast-qual -Wextra \
                          -Wno-unused-variable \
                          -Woverloaded-virtual -Wpointer-arith \
                          -Wstack-protector
} else:unix {
QMAKE_CXXFLAGS_RELEASE += -fPIE -fwrapv -pie -O3 \
                          -Wall -Wcast-qual -Wextra \
                          -Wno-unused-variable \
                          -Woverloaded-virtual -Wpointer-arith \
                          -Wstack-protector -pedantic -std=c++11
}

unix {
purge.commands = rm -f */*~ *~
}

QMAKE_DISTCLEAN += -r temp
QMAKE_EXTRA_TARGETS = purge
QT += network sql
TEMPLATE = app

greaterThan(QT_MAJOR_VERSION, 4) {
QMAKE_CXXFLAGS_RELEASE += -Wzero-as-null-pointer-constant
QT += concurrent
} else {
DEFINES += nullptr=NULL
}

libshalisp.target = spot-on-lite-daemon-sha.a
libshalisp.commands = cd Source && ecl -norc -eval "'(require :asdf)'" -eval "'(push \"./\" asdf:*central-registry*)'" -eval "'(asdf:make-build :spot-on-lite-daemon-sha :type :static-library :move-here \"./\" :init-name \"init_lib_SPOT_ON_LITE_DAEMON_SHA\")'" -eval "'(quit)'" && cd ..
libshalisp.depends =

exists(/usr/bin/ecl) {
DEFINES += SPOTON_LITE_DAEMON_CHILD_ECL_SUPPORTED
LIBS += -lecl
LIBS += Source/spot-on-lite-daemon-sha.a
PRE_TARGETDEPS += spot-on-lite-daemon-sha.a
QMAKE_CFLAGS += `ecl-config --cflags`
QMAKE_CXXFLAGS += `ecl-config --cflags`
QMAKE_EXTRA_TARGETS += libshalisp
QMAKE_LFLAGS += `ecl-config --ldflags`
}

exists(/usr/local/bin/ecl) {
DEFINES += SPOTON_LITE_DAEMON_CHILD_ECL_SUPPORTED
LIBS += -lecl
LIBS += Source/spot-on-lite-daemon-sha.a
PRE_TARGETDEPS += spot-on-lite-daemon-sha.a
QMAKE_CFLAGS += `ecl-config --cflags`
QMAKE_CXXFLAGS += `ecl-config --cflags`
QMAKE_EXTRA_TARGETS += libshalisp
QMAKE_LFLAGS += `ecl-config --ldflags`
}

macx {
INCLUDEPATH += /usr/local/opt/openssl/include
LIBS += -L/usr/local/opt/openssl/lib
}

QMAKE_CLEAN += Source/spot-on-lite-daemon-sha.a \
               Source/spot-on-lite-daemon-sha.fas \
               Source/spot-on-lite-daemon-sha.lib

INCLUDEPATH += Source
LIBS += -lcrypto -lssl
MOC_DIR = temp/moc
OBJECTS_DIR = temp/obj
RCC_DIR = temp/rcc
