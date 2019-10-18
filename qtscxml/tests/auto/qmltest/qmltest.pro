TEMPLATE = app
TARGET = tst_scxmlqml
CONFIG += qmltestcase

SOURCES += \
    $$PWD/tst_scxmlqml.cpp

RESOURCES += tst_scxmlqml.qrc

OTHER_FILES += \
    $$PWD/*.qml

TESTDATA += \
    $$PWD/tst_*
