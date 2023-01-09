CONFIG += testcase
TARGET = tst_accessibility
requires(qtConfig(accessibility))
SOURCES += tst_accessibility.cpp

macos:CONFIG -= app_bundle

QT += core-private gui-private qml-private quick-private quicktemplates2-private testlib

include (../shared/util.pri)

TESTDATA = data/*

OTHER_FILES += \
    data/defaults\*.qml \
    data/ordering\*.qml \
    data/override*.qml

