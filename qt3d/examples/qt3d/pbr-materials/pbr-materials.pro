!include( ../examples.pri ) {
    error( "Couldn't find the examples.pri file!" )
}

QT += 3dcore 3drender 3dinput 3dquick qml quick 3dquickextras

HEADERS += \

SOURCES += \
    main.cpp

OTHER_FILES += \
    *.qml \

RESOURCES += \
    materials.qrc \
    ../exampleresources/cubemaps.qrc \
    ../exampleresources/obj.qrc \
    ../exampleresources/textures.qrc
