QT = waylandcompositor waylandcompositor-private core-private gui-private

OTHER_FILES += wayland-egl.json

SOURCES += \
    main.cpp

TARGET = qt-plugin-wayland-egl

include(../../../../hardwareintegration/compositor/wayland-egl/wayland-egl.pri)

PLUGIN_TYPE = wayland-graphics-integration-server
PLUGIN_CLASS_NAME = QWaylandEglClientBufferIntegrationPlugin
load(qt_plugin)
