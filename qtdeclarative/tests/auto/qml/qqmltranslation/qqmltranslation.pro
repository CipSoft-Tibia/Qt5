CONFIG += testcase
TARGET = tst_qqmltranslation
macx:CONFIG -= app_bundle

SOURCES += tst_qqmltranslation.cpp
RESOURCES += data/translation.qrc

include (../../shared/util.pri)

TESTDATA = data/*

QT += core-private gui-private qml-private testlib
