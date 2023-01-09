TARGET = qtpeakcanbus

QT = core serialbus

HEADERS += \
    peakcanbackend.h \
    peakcanbackend_p.h \
    peakcan_symbols_p.h

SOURCES += \
    main.cpp \
    peakcanbackend.cpp

DISTFILES = plugin.json

PLUGIN_TYPE = canbus
PLUGIN_CLASS_NAME = PeakCanBusPlugin
load(qt_plugin)
