TEMPLATE = app

TARGET = tst_renderqueue

QT += 3dcore 3dcore-private 3drender 3drender-private testlib

CONFIG += testcase

SOURCES += tst_renderqueue.cpp

include(../../core/common/common.pri)
