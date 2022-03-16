TARGET     = QtSvg
QT         = core-private gui-private
qtHaveModule(widgets): QT += widgets-private

DEFINES   += QT_NO_USING_NAMESPACE
msvc:equals(QT_ARCH, i386): QMAKE_LFLAGS += /BASE:0x66000000
solaris-cc*:QMAKE_CXXFLAGS_RELEASE -= -O2

QMAKE_DOCS = $$PWD/doc/qtsvg.qdocconf

HEADERS += \
    qsvggraphics_p.h        \
    qsvghandler_p.h         \
    qsvgnode_p.h            \
    qsvgstructure_p.h       \
    qsvgstyle_p.h           \
    qsvgfont_p.h            \
    qsvgtinydocument_p.h    \
    qsvgrenderer.h          \
    qsvgwidget.h            \
    qgraphicssvgitem.h      \
    qsvggenerator.h \
    qtsvgglobal.h


SOURCES += \
    qsvggraphics.cpp        \
    qsvghandler.cpp         \
    qsvgnode.cpp            \
    qsvgstructure.cpp       \
    qsvgstyle.cpp           \
    qsvgfont.cpp            \
    qsvgtinydocument.cpp    \
    qsvgrenderer.cpp        \
    qsvgwidget.cpp          \
    qgraphicssvgitem.cpp    \
    qsvggenerator.cpp

qtConfig(system-zlib): \
    QMAKE_USE_PRIVATE += zlib
else: \
    QT_PRIVATE += zlib-private

load(qt_module)
