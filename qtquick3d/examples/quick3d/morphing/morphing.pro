QT += quick quick3d

target.path = $$[QT_INSTALL_EXAMPLES]/quick3d/morphing
INSTALLS += target

SOURCES += \
    main.cpp

RESOURCES += \
    qml.qrc

OTHER_FILES += \
    main.qml \
    RealSlider.qml

OTHER_FILES += \
    doc/src/*.*
