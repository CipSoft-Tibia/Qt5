!include( ../manual.pri ) {
    error( "Couldn't find the manual.pri file!" )
}

QT += 3dcore 3drender 3dinput 3dquick qml quick 3dquickextras

SOURCES += \
    main.cpp

OTHER_FILES += \
    Scene.qml \
    main.qml

RESOURCES += \
    pointlinesize.qrc
