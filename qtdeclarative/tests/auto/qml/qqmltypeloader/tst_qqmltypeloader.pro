CONFIG += testcase
TARGET = tst_qqmltypeloader
QT += qml testlib qml-private quick
macx:CONFIG -= app_bundle

SOURCES += \
    tst_qqmltypeloader.cpp \
    ../../shared/testhttpserver.cpp

HEADERS += \
    ../../shared/testhttpserver.h

include (../../shared/util.pri)

