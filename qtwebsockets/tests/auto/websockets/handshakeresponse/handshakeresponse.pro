CONFIG += console
CONFIG += testcase
CONFIG -= app_bundle

TEMPLATE = app

TARGET = tst_handshakeresponse

QT = testlib websockets-private

SOURCES += tst_handshakeresponse.cpp

requires(qtConfig(private_tests))
