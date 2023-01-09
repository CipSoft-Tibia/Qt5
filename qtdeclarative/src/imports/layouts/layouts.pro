CXX_MODULE = qml
TARGET  = qquicklayoutsplugin
TARGETPATH = QtQuick/Layouts
IMPORT_VERSION = 1.$$QT_MINOR_VERSION

QT *= qml-private quick-private gui-private core-private

SOURCES += plugin.cpp \
    qquicklayout.cpp \
    qquicklinearlayout.cpp \
    qquickstacklayout.cpp \
    qquickgridlayoutengine.cpp \
    qquicklayoutstyleinfo.cpp

HEADERS += \
    qquicklayout_p.h \
    qquicklinearlayout_p.h \
    qquickstacklayout_p.h \
    qquickgridlayoutengine_p.h \
    qquicklayoutstyleinfo_p.h

CONFIG += qmltypes install_qmltypes

load(qml_plugin)
