CONFIG += testcase
TARGET = tst_qquickpathview
macx:CONFIG -= app_bundle

SOURCES += tst_qquickpathview.cpp

include (../../shared/util.pri)
include (../shared/util.pri)

TESTDATA = data/*

QT += core-private gui-private  qml-private quick-private testlib
qtHaveModule(widgets): QT += widgets
