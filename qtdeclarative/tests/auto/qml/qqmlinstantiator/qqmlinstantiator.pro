CONFIG += testcase
TARGET = tst_qqmlinstantiator
macx:CONFIG -= app_bundle

SOURCES += tst_qqmlinstantiator.cpp
HEADERS += stringmodel.h

include (../../shared/util.pri)

TESTDATA = data/*

QT += core-private gui-private qml-private testlib qmlmodels-private
