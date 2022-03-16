CXX_MODULE = gamepad
TARGET  = declarative_gamepad
TARGETPATH = QtGamepad
IMPORT_VERSION = 1.$$QT_MINOR_VERSION

QT += qml quick gamepad

load(qml_plugin)

OTHER_FILES += qmldir

SOURCES += \
    qtgamepad.cpp \
    qgamepadmouseitem.cpp

HEADERS += \
    qgamepadmouseitem.h
