QT += waylandclient-private

include(../../../../hardwareintegration/client/xcomposite-glx/xcomposite-glx.pri)

OTHER_FILES += xcomposite-glx.json

SOURCES += \
    main.cpp

PLUGIN_TYPE = wayland-graphics-integration-client
PLUGIN_CLASS_NAME = QWaylandXCompositeGlxClientBufferPlugin
load(qt_plugin)
