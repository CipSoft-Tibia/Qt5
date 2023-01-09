TARGET = qttinycanbus

QT = core serialbus

HEADERS += \
    tinycanbackend.h \
    tinycanbackend_p.h \
    tinycan_symbols_p.h

SOURCES += main.cpp \
    tinycanbackend.cpp

DISTFILES = plugin.json

PLUGIN_TYPE = canbus
PLUGIN_CLASS_NAME = TinyCanBusPlugin
load(qt_plugin)
