CONFIG += testcase
TARGET = tst_qjsengine
QT +=  qml qml-private widgets testlib gui-private
macx:CONFIG -= app_bundle
SOURCES += tst_qjsengine.cpp
RESOURCES += qjsengine.qrc
RESOURCES += testmodule.mjs modulewithlexicals.mjs importerror1.mjs exporterror1.mjs

TESTDATA = script/*
