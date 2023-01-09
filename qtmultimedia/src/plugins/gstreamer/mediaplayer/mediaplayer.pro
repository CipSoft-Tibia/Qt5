TARGET = gstmediaplayer

include(../common.pri)

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/qgstreamerplayerservice.h \
    $$PWD/qgstreamerstreamscontrol.h \
    $$PWD/qgstreamermetadataprovider.h \
    $$PWD/qgstreameravailabilitycontrol.h \
    $$PWD/qgstreamerplayerserviceplugin.h

SOURCES += \
    $$PWD/qgstreamerplayerservice.cpp \
    $$PWD/qgstreamerstreamscontrol.cpp \
    $$PWD/qgstreamermetadataprovider.cpp \
    $$PWD/qgstreameravailabilitycontrol.cpp \
    $$PWD/qgstreamerplayerserviceplugin.cpp

OTHER_FILES += \
    mediaplayer.json

PLUGIN_TYPE = mediaservice
PLUGIN_CLASS_NAME = QGstreamerPlayerServicePlugin
load(qt_plugin)
