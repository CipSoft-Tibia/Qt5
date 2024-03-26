option(host_build)
include(moc_copy/moc.pri)
QT = core-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII QT_NO_CAST_FROM_BYTEARRAY QT_NO_URL_CAST_FROM_STRING
DEFINES += RO_INSTALL_HEADERS=$$shell_quote(\"$$clean_path($$[QT_INSTALL_HEADERS]/QtRemoteObjects)\")
msvc: QMAKE_CXXFLAGS += /wd4129

CONFIG += qlalr
QLALRSOURCES += $$QTRO_SOURCE_TREE/src/repparser/parser.g
INCLUDEPATH += $$QTRO_SOURCE_TREE/src/repparser

SOURCES += \
    main.cpp \
    repcodegenerator.cpp \
    cppcodegenerator.cpp \
    utils.cpp

HEADERS += \
    repcodegenerator.h \
    cppcodegenerator.h \
    utils.h

QMAKE_TARGET_DESCRIPTION = "Qt Remote Objects Compiler"
load(qt_tool)
