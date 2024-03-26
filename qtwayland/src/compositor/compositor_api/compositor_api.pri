INCLUDEPATH += compositor_api

HEADERS += \
    compositor_api/qwaylandcompositor.h \
    compositor_api/qwaylandcompositor_p.h \
    compositor_api/qwaylandclient.h \
    compositor_api/qwaylandsurface.h \
    compositor_api/qwaylandsurface_p.h \
    compositor_api/qwaylandseat.h \
    compositor_api/qwaylandseat_p.h \
    compositor_api/qwaylandkeyboard.h \
    compositor_api/qwaylandkeyboard_p.h \
    compositor_api/qwaylandkeymap.h \
    compositor_api/qwaylandkeymap_p.h \
    compositor_api/qwaylandpointer.h \
    compositor_api/qwaylandpointer_p.h \
    compositor_api/qwaylandtouch.h \
    compositor_api/qwaylandtouch_p.h \
    compositor_api/qwaylandoutput.h \
    compositor_api/qwaylandoutput_p.h \
    compositor_api/qwaylandoutputmode.h \
    compositor_api/qwaylandoutputmode_p.h \
    compositor_api/qwaylandbufferref.h \
    compositor_api/qwaylanddestroylistener.h \
    compositor_api/qwaylanddestroylistener_p.h \
    compositor_api/qwaylandview.h \
    compositor_api/qwaylandview_p.h \
    compositor_api/qwaylandresource.h \
    compositor_api/qwaylandsurfacegrabber.h \
    compositor_api/qwaylandoutputmode_p.h

SOURCES += \
    compositor_api/qwaylandcompositor.cpp \
    compositor_api/qwaylandclient.cpp \
    compositor_api/qwaylandsurface.cpp \
    compositor_api/qwaylandseat.cpp \
    compositor_api/qwaylandkeyboard.cpp \
    compositor_api/qwaylandkeymap.cpp \
    compositor_api/qwaylandpointer.cpp \
    compositor_api/qwaylandtouch.cpp \
    compositor_api/qwaylandoutput.cpp \
    compositor_api/qwaylandoutputmode.cpp \
    compositor_api/qwaylandbufferref.cpp \
    compositor_api/qwaylanddestroylistener.cpp \
    compositor_api/qwaylandview.cpp \
    compositor_api/qwaylandresource.cpp \
    compositor_api/qwaylandsurfacegrabber.cpp

qtConfig(im) {
    HEADERS += \
        compositor_api/qwaylandinputmethodcontrol.h \
        compositor_api/qwaylandinputmethodcontrol_p.h
    SOURCES += \
        compositor_api/qwaylandinputmethodcontrol.cpp
}

QT += core-private gui-private

qtConfig(draganddrop) {
    HEADERS += \
        compositor_api/qwaylanddrag.h
    SOURCES += \
        compositor_api/qwaylanddrag.cpp
}

qtConfig(wayland-compositor-quick) {
    SOURCES += \
        compositor_api/qwaylandmousetracker.cpp \
        compositor_api/qwaylandquickcompositor.cpp \
        compositor_api/qwaylandquicksurface.cpp \
        compositor_api/qwaylandquickoutput.cpp \
        compositor_api/qwaylandquickitem.cpp

    HEADERS += \
        compositor_api/qwaylandcompositorquickextensions_p.h \
        compositor_api/qwaylandmousetracker_p.h \
        compositor_api/qwaylandquickchildren.h \
        compositor_api/qwaylandquickcompositor.h \
        compositor_api/qwaylandquicksurface.h \
        compositor_api/qwaylandquicksurface_p.h \
        compositor_api/qwaylandquickoutput.h \
        compositor_api/qwaylandquickitem.h \
        compositor_api/qwaylandquickitem_p.h

    qtConfig(opengl) {
        SOURCES += \
            compositor_api/qwaylandquickhardwarelayer.cpp
        HEADERS += \
            compositor_api/qwaylandquickhardwarelayer_p.h
    }

    QT += qml qml-private quick quick-private
}
