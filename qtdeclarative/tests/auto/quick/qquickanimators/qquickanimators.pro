QT += core-private gui-private qml-private quick-private testlib
TARGET=tst_qquickanimators
CONFIG += testcase
macx: CONFIG -= app_bundle
SOURCES += tst_qquickanimators.cpp

include (../../shared/util.pri)
include (../shared/util.pri)

TESTDATA = data/*
DISTFILES += data/*
