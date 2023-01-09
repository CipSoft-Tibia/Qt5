TEMPLATE = app
TARGET = contactlist
QT += quick

HEADERS += \
    contactmodel.h

SOURCES += \
    main.cpp \
    contactmodel.cpp

RESOURCES += \
    ContactDelegate.ui.qml \
    ContactDialog.qml \
    ContactForm.ui.qml \
    contactlist.qml \
    ContactView.ui.qml \
    designer/Backend/ContactModel.qml \
    SectionDelegate.ui.qml

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH = $$PWD/designer

OTHER_FILES += \
    designer/Backend/*.qml

target.path = $$[QT_INSTALL_EXAMPLES]/quickcontrols2/contactlist
INSTALLS += target
