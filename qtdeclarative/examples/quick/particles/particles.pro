TEMPLATE = subdirs
SUBDIRS += affectors \
    customparticle \
    emitters \
    imageparticle \
    itemparticle \
    system

EXAMPLE_FILES = \
    images.qrc

#Install shared images too
qml.files = images
qml.path = $$[QT_INSTALL_EXAMPLES]/quick/particles
INSTALLS += qml
