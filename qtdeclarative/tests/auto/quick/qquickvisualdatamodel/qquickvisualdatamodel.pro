CONFIG += testcase
TARGET = tst_qquickvisualdatamodel
macx:CONFIG -= app_bundle

SOURCES += tst_qquickvisualdatamodel.cpp

include (../../shared/util.pri)
include (../shared/util.pri)

TESTDATA = data/*

QT += core-private gui-private qml-private quick-private testlib qmlmodels-private
qtHaveModule(widgets): QT += widgets
