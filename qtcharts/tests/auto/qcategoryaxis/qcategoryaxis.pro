!include( ../auto.pri ) {
    error( "Couldn't find the auto.pri file!" )
}
HEADERS += ../qabstractaxis/tst_qabstractaxis.h
SOURCES += tst_qcategoryaxis.cpp ../qabstractaxis/tst_qabstractaxis.cpp
