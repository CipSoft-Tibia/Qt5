CXX_MODULE = qml
TARGET = qtqmlstatemachine
TARGETPATH = QtQml/StateMachine
IMPORT_VERSION = 1.$$QT_MINOR_VERSION

QT = core-private qml-private

SOURCES = \
    $$PWD/finalstate.cpp \
    $$PWD/signaltransition.cpp \
    $$PWD/state.cpp \
    $$PWD/statemachine.cpp \
    $$PWD/timeouttransition.cpp \
    $$PWD/plugin.cpp

HEADERS = \
    $$PWD/childrenprivate.h \
    $$PWD/finalstate.h  \
    $$PWD/signaltransition.h  \
    $$PWD/state.h  \
    $$PWD/statemachine.h  \
    $$PWD/timeouttransition.h \
    $$PWD/statemachineforeign.h

CONFIG += qmltypes install_qmltypes

load(qml_plugin)
