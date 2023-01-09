QT       += core gui quick widgets quickwidgets

TARGET = quickwidget
TEMPLATE = app

CONFIG += qmltypes
QML_IMPORT_NAME = QuickWidgetExample
QML_IMPORT_MAJOR_VERSION = 1

SOURCES += main.cpp fbitem.cpp
HEADERS += fbitem.h

RESOURCES += quickwidget.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/quickwidgets/quickwidget
INSTALLS += target
