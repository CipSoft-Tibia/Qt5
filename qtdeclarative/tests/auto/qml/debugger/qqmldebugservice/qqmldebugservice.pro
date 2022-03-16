CONFIG += testcase
TARGET = tst_qqmldebugservice
osx:CONFIG -= app_bundle

HEADERS += ../shared/qqmldebugtestservice.h

SOURCES += tst_qqmldebugservice.cpp \
           ../shared/qqmldebugtestservice.cpp

include(../shared/debugutil.pri)

TESTDATA = data/*

OTHER_FILES += \
    data/test.qml

DEFINES += QT_QML_DEBUG_NO_WARNING

QT += qml-private testlib gui-private core-private
