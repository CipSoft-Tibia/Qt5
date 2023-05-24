!include( ../examples.pri ) {
    error( "Couldn't find the examples.pri file!" )
}

DEFINES += QMAKE_BUILD

QT += graphs

SOURCES += main.cpp \
           datasource.cpp
HEADERS += datasource.h

RESOURCES += surfacegallery.qrc

OTHER_FILES += doc/src/* \
               doc/images/* \
               qml/surfacegallery/*
