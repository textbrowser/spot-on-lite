cache()
include (common.pro)

CONFIG		+= qt release warn_on
DEFINES         += QT_DEPRECATED_WARNINGS
LANGUAGE	= C++
QMAKE_DISTCLEAN += .qmake.*
QT		+= gui sql widgets

FORMS           = UI/spot-on-lite-monitor.ui
HEADERS		= Source/spot-on-lite-monitor.h
RESOURCES	= Icons/icons.qrc
SOURCES		= Source/spot-on-lite-monitor.cc
TRANSLATIONS    =
UI_HEADERS_DIR  = .

PROJECTNAME	= Spot-On-Lite-Monitor
TARGET		= Spot-On-Lite-Monitor
TEMPLATE	= app

DISTFILES += \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle.properties \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/res/values/libs.xml

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
