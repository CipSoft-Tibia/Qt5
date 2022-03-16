TARGET = qtsensors_generic
QT = core sensors

HEADERS += generictiltsensor.h

SOURCES += main.cpp\
           generictiltsensor.cpp
DEFINES += QTSENSORS_GENERICTILTSENSOR

HEADERS += genericorientationsensor.h\
           genericalssensor.h

SOURCES += genericorientationsensor.cpp\
           genericalssensor.cpp
DEFINES += QTSENSORS_GENERICORIENTATIONSENSOR QTSENSORS_GENERICALSSENSOR

!android {
    HEADERS += genericrotationsensor.h

    SOURCES += genericrotationsensor.cpp

    DEFINES += QTSENSORS_GENERICROTATIONSENSOR
}

OTHER_FILES = plugin.json

PLUGIN_TYPE = sensors
PLUGIN_CLASS_NAME = genericSensorPlugin
load(qt_plugin)
