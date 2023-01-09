CONFIG += wayland-scanner
WAYLANDSERVERSOURCES += \
    ../extensions/server-buffer-extension.xml \
    ../extensions/hardware-integration.xml \

HEADERS += \
    hardware_integration/qwlclientbufferintegration_p.h \

SOURCES += \
    hardware_integration/qwlclientbufferintegration.cpp \


qtConfig(opengl) {
    HEADERS += \
        hardware_integration/qwlclientbufferintegrationfactory_p.h \
        hardware_integration/qwlclientbufferintegrationplugin_p.h \
        hardware_integration/qwlserverbufferintegration_p.h \
        hardware_integration/qwlserverbufferintegrationfactory_p.h \
        hardware_integration/qwlserverbufferintegrationplugin_p.h \
        hardware_integration/qwlhwintegration_p.h \
        hardware_integration/qwlhardwarelayerintegration_p.h \
        hardware_integration/qwlhardwarelayerintegrationfactory_p.h \
        hardware_integration/qwlhardwarelayerintegrationplugin_p.h \

    SOURCES += \
        hardware_integration/qwlclientbufferintegrationfactory.cpp \
        hardware_integration/qwlclientbufferintegrationplugin.cpp \
        hardware_integration/qwlserverbufferintegration.cpp \
        hardware_integration/qwlserverbufferintegrationfactory.cpp \
        hardware_integration/qwlserverbufferintegrationplugin.cpp \
        hardware_integration/qwlhwintegration.cpp \
        hardware_integration/qwlhardwarelayerintegration.cpp \
        hardware_integration/qwlhardwarelayerintegrationfactory.cpp \
        hardware_integration/qwlhardwarelayerintegrationplugin.cpp \
} else {
    system(echo "Qt-Compositor configured as raster only compositor")
}
