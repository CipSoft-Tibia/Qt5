TEMPLATE = app
TARGET = tst_fusion
CONFIG += qmltestcase
QT += quickcontrols2

DEFINES += TST_CONTROLS_DATA=\\\"$$QQC2_SOURCE_TREE/tests/auto/controls/data\\\"

SOURCES += \
    $$PWD/tst_fusion.cpp

OTHER_FILES += \
    $$PWD/../data/*.qml

TESTDATA += \
    $$PWD/../data/tst_*
