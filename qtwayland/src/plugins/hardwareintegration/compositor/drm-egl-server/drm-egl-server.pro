QT = waylandcompositor waylandcompositor-private core-private gui-private

OTHER_FILES += drm-egl-server.json

SOURCES += \
    main.cpp

include($PWD/../../../../../hardwareintegration/compositor/drm-egl-server/drm-egl-server.pri)

PLUGIN_TYPE = wayland-graphics-integration-server
PLUGIN_CLASS_NAME = DrmEglServerBufferIntegrationPlugin
load(qt_plugin)
