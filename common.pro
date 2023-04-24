CONFIG += qt warn_on
CONFIG -= app_bundle gui
LANGUAGE = C++
QMAKE_CXXFLAGS_RELEASE -= -O2

android {
} else {
exists(/usr/include/sched.h) {
DEFINES += SPOTON_LITE_DAEMON_SCHEDULING_ENABLED
}
}

contains(QMAKE_HOST.arch, armv7l) {
QMAKE_CXXFLAGS_RELEASE += -march=armv7
}

contains(QMAKE_HOST.arch, ppc) {
QMAKE_CXXFLAGS_RELEASE += -mcpu=powerpc -mtune=powerpc
}

android {
DEFINES += SPOTON_LITE_DAEMON_DTLS_SUPPORTED
DEFINES -= SPOTON_LITE_DAEMON_OPENSSL_SUPPORTED
QMAKE_CXXFLAGS  += -Wall \
                   -Wcast-align \
                   -Wcast-qual \
                   -Werror \
                   -Werror=format-security \
                   -Wextra \
                   -Wfloat-equal \
                   -Wformat=2 \
                   -Wno-deprecated-declarations \
                   -Woverloaded-virtual \
                   -Wpointer-arith \
                   -Wstack-protector \
                   -Wstrict-overflow=5 \
                   -Wundef \
                   -Wzero-as-null-pointer-constant \
                   -fPIE \
                   -fstack-protector-all \
                   -fwrapv \
                   -pedantic \
                   -std=c++11
} else:freebsd-* {
QMAKE_CXX = clang++
QMAKE_CXXFLAGS_RELEASE += -O3 \
                          -Wall \
                          -Wcast-align \
                          -Wcast-qual \
                          -Wdouble-promotion \
                          -Wextra \
                          -Wformat=2 \
                          -Wno-deprecated-declarations \
                          -Woverloaded-virtual \
                          -Wpointer-arith \
                          -Wstack-protector \
                          -Wstrict-overflow=1 \
                          -Wundef \
                          -Wzero-as-null-pointer-constant \
                          -fPIE \
                          -fstack-protector-all \
                          -fwrapv \
                          -pedantic \
                          -std=c++17
} else:linux-* {
DEFINES += SPOTON_LITE_DAEMON_UDP_MULTICASTING_IPC
QMAKE_CXXFLAGS_RELEASE += -O3 \
                          -Wall \
			  -Warray-bounds=2 \
                          -Wcast-qual \
                          -Wdouble-promotion \
                          -Wextra \
                          -Wfloat-equal \
                          -Wformat=2 \
                          -Wlogical-op \
                          -Wno-deprecated-declarations \
                          -Wno-unused-variable \
                          -Woverloaded-virtual \
                          -Wpointer-arith \
                          -Wundef \
                          -Wstack-protector \
                          -Wstrict-overflow=1 \
                          -Wundef \
                          -fPIE \
                          -fstack-protector-all \
                          -funroll-loops \
                          -fwrapv \
                          -pedantic \
                          -pie \
                          -std=c++17
} else:macx {
QMAKE_CXXFLAGS_RELEASE += -O3 \
                          -Wall \
                          -Wcast-align \
                          -Wcast-qual \
                          -Wdouble-promotion \
                          -Wextra \
                          -Wformat=2 \
                          -Wno-deprecated-declarations \
                          -Wno-unused-variable \
                          -Woverloaded-virtual \
                          -Wpointer-arith \
                          -Wstack-protector \
                          -Wundef \
                          -Wzero-as-null-pointer-constant \
                          -fPIE \
                          -fstack-protector-all \
                          -fwrapv \
                          -pedantic \
                          -std=c++17
} else:netbsd-* {
QMAKE_CXXFLAGS_RELEASE += -O3 \
                          -Wall \
                          -Wcast-qual \
                          -Wdouble-promotion \
                          -Wextra \
                          -Wformat-overflow=2 \
                          -Wformat-truncation=2 \
                          -Wformat=2 \
                          -Wno-deprecated-declarations \
                          -Wno-unused-variable \
                          -Woverloaded-virtual \
                          -Wpointer-arith \
                          -Wstack-protector \
                          -Wundef \
                          -Wzero-as-null-pointer-constant \
                          -fPIE \
                          -fwrapv \
                          -pedantic \
                          -pie \
                          -std=c++17
} else:openbsd-* {
QMAKE_CXXFLAGS_RELEASE += -O3 \
                          -Wall \
                          -Wcast-qual \
                          -Wextra \
                          -Wformat=2 \
                          -Wno-deprecated-declarations \
                          -Wno-unused-variable \
                          -Woverloaded-virtual \
                          -Wpointer-arith \
                          -Wstack-protector \
                          -Wundef \
                          -fPIE \
                          -fstack-protector-all \
                          -fwrapv \
                          -pedantic \
                          -pie \
                          -std=c++17
} else:unix {
QMAKE_CXXFLAGS_RELEASE += -O3 \
                          -Wall \
                          -Wcast-qual \
                          -Wdouble-promotion \
                          -Wextra \
                          -Wformat-overflow=2 \
                          -Wformat-truncation=2 \
                          -Wformat=2 \
                          -Wno-deprecated-declarations \
                          -Wno-unused-variable \
                          -Woverloaded-virtual \
                          -Wpointer-arith \
                          -Wstack-protector \
                          -Wundef \
                          -Wzero-as-null-pointer-constant \
                          -fPIE \
                          -fwrapv \
                          -pedantic \
                          -pie \
                          -std=c++11\7
}

unix {
purge.commands = rm -f */*~ *~
}

QMAKE_DISTCLEAN += -r temp
QMAKE_EXTRA_TARGETS = purge
QT += network sql
TEMPLATE = app

greaterThan(QT_MAJOR_VERSION, 4) {
openbsd-* {
} else {
}
QT += concurrent
} else {
DEFINES += nullptr=NULL
}

libshalisp.target = spot-on-lite-daemon-sha.a
libshalisp.commands = cd Source && ecl -norc -eval "'(require :asdf)'" -eval "'(push \"./\" asdf:*central-registry*)'" -eval "'(asdf:make-build :spot-on-lite-daemon-sha :type :static-library :move-here \"./\" :init-name \"init_lib_SPOTON_LITE_DAEMON_SHA\")'" -eval "'(quit)'" && cd ..
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

android {
} else:macx {
exists(/opt/homebrew/opt/openssl@3) {
DEFINES += SPOTON_LITE_DAEMON_OPENSSL_SUPPORTED
INCLUDEPATH += /opt/homebrew/opt/openssl@3/include
LIBS += -L/opt/homebrew/opt/openssl@3/lib -lcrypto -lssl
}
exists(/usr/local/opt/openssl) {
DEFINES += SPOTON_LITE_DAEMON_OPENSSL_SUPPORTED
INCLUDEPATH += /usr/local/opt/openssl/include
LIBS += -L/usr/local/opt/openssl/lib -lcrypto -lssl
}
} else {
exists(/usr/include/openssl) {
DEFINES += SPOTON_LITE_DAEMON_DTLS_SUPPORTED \
           SPOTON_LITE_DAEMON_OPENSSL_SUPPORTED
LIBS += -lcrypto -lssl
}
}

QMAKE_CLEAN += Source/spot-on-lite-daemon-sha.a \
               Source/spot-on-lite-daemon-sha.fas \
               Source/spot-on-lite-daemon-sha.lib
INCLUDEPATH += Source
MOC_DIR = temp/moc
OBJECTS_DIR = temp/obj
RCC_DIR = temp/rcc
