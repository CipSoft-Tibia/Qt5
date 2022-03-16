QT += gui-private waylandclient-private
CONFIG += wayland-scanner

QMAKE_USE += wayland-client

qtConfig(xkbcommon): \
    QMAKE_USE += xkbcommon

WAYLANDCLIENTSOURCES += \
    ../../../3rdparty/protocol/ivi-application.xml \
    ../../../3rdparty/protocol/ivi-controller.xml

HEADERS += \
    qwaylandivishellintegration.h \
    qwaylandivisurface_p.h

SOURCES += \
    main.cpp \
    qwaylandivishellintegration.cpp \
    qwaylandivisurface.cpp

OTHER_FILES += \
    ivi-shell.json

PLUGIN_TYPE = wayland-shell-integration
PLUGIN_CLASS_NAME = QWaylandIviShellIntegrationPlugin
load(qt_plugin)
