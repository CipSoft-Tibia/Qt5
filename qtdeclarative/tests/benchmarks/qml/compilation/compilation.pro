CONFIG += benchmark
TEMPLATE = app
TARGET = tst_compilation
QT += qml qml-private testlib core-private
macx:CONFIG -= app_bundle

CONFIG += release

SOURCES += tst_compilation.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"
