include( ../../tests.pri )
TEMPLATE = app

QT += charts
QT += core gui widgets

SOURCES += main.cpp \
    mainwidget.cpp \
    customtablemodel.cpp \
    pentool.cpp

HEADERS += \
    mainwidget.h \
    customtablemodel.h \
    pentool.h
