CONFIG += wayland-scanner
WAYLANDSERVERSOURCES += \
    ../3rdparty/protocol/wayland.xml

HEADERS += \
    wayland_wrapper/qwlbuffermanager_p.h \
    wayland_wrapper/qwlclientbuffer_p.h \
    wayland_wrapper/qwlregion_p.h

SOURCES += \
    wayland_wrapper/qwlbuffermanager.cpp \
    wayland_wrapper/qwlclientbuffer.cpp \
    wayland_wrapper/qwlregion.cpp

qtConfig(wayland-datadevice) {
    HEADERS += \
        wayland_wrapper/qwldatadevice_p.h \
        wayland_wrapper/qwldatadevicemanager_p.h \
        wayland_wrapper/qwldataoffer_p.h \
        wayland_wrapper/qwldatasource_p.h

    SOURCES += \
        wayland_wrapper/qwldatadevice.cpp \
        wayland_wrapper/qwldatadevicemanager.cpp \
        wayland_wrapper/qwldataoffer.cpp \
        wayland_wrapper/qwldatasource.cpp
}

INCLUDEPATH += wayland_wrapper

qtConfig(xkbcommon): \
    QMAKE_USE += xkbcommon
