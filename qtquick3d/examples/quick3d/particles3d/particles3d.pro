QT += quick quick3d

target.path = $$[QT_INSTALL_EXAMPLES]/quick3d/particles3d
INSTALLS += target

SOURCES += \
    main.cpp

RESOURCES += \
    qml.qrc

OTHER_FILES += \
    doc/src/*.*
