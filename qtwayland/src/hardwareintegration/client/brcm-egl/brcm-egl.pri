INCLUDEPATH += $$PWD

QMAKE_USE += egl wayland-client libdl

SOURCES += $$PWD/qwaylandbrcmeglintegration.cpp \
           $$PWD/qwaylandbrcmglcontext.cpp \
           $$PWD/qwaylandbrcmeglwindow.cpp

HEADERS += $$PWD/qwaylandbrcmeglintegration.h \
           $$PWD/qwaylandbrcmglcontext.h \
           $$PWD/qwaylandbrcmeglwindow.h

CONFIG += wayland-scanner-client-wayland-protocol-include
WAYLANDCLIENTSOURCES += $$PWD/../../../extensions/brcm.xml
