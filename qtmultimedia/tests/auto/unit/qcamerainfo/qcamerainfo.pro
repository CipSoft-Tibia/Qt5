CONFIG += testcase
TARGET = tst_qcamerainfo

QT += multimedia-private testlib

include (../qmultimedia_common/mock.pri)
include (../qmultimedia_common/mockcamera.pri)

SOURCES += tst_qcamerainfo.cpp
