CONFIG += testcase
TARGET = tst_qqmllistmodelworkerscript
macx:CONFIG -= app_bundle

SOURCES += tst_qqmllistmodelworkerscript.cpp

include (../../shared/util.pri)

TESTDATA = data/*

QT += core-private gui-private qml-private quick-private testlib qmlmodels-private
