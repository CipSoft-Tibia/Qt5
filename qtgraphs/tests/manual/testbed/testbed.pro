!include( ../tests.pri ) {
    error( "Couldn't find the tests.pri file!" )
}

SOURCES += main.cpp

RESOURCES += testbed.qrc

OTHER_FILES += qml/testbed/*
