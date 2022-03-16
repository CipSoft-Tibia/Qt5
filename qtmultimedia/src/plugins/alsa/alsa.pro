TARGET = qtaudio_alsa
QT += multimedia-private

LIBS += -lasound

HEADERS += \
    qalsaplugin.h \
    qalsaaudiodeviceinfo.h \
    qalsaaudioinput.h \
    qalsaaudiooutput.h

SOURCES += \
    qalsaplugin.cpp \
    qalsaaudiodeviceinfo.cpp \
    qalsaaudioinput.cpp \
    qalsaaudiooutput.cpp

OTHER_FILES += \
    alsa.json

PLUGIN_TYPE = audio
PLUGIN_CLASS_NAME = QAlsaPlugin
load(qt_plugin)
