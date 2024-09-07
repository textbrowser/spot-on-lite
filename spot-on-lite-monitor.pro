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
    Android/AndroidManifest.xml \
    Android/build.gradle \
    Android/gradle.properties \
    Android/gradle/wrapper/gradle-wrapper.jar \
    Android/gradle/wrapper/gradle-wrapper.properties \
    Android/gradlew \
    Android/gradlew.bat \
    Android/res/values/libs.xml

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/Android
