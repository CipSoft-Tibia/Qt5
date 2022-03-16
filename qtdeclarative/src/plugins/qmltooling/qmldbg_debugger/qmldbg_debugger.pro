TARGET = qmldbg_debugger
QT = qml-private core-private packetprotocol-private

SOURCES += \
    $$PWD/qqmldebuggerservicefactory.cpp \
    $$PWD/qqmlenginedebugservice.cpp \
    $$PWD/qqmlwatcher.cpp \
    $$PWD/qv4debugservice.cpp \
    $$PWD/qv4debugger.cpp \
    $$PWD/qv4debuggeragent.cpp \
    $$PWD/qv4datacollector.cpp \
    $$PWD/qv4debugjob.cpp

HEADERS += \
    $$PWD/qqmldebuggerservicefactory.h \
    $$PWD/qqmlenginedebugservice.h \
    $$PWD/qqmlwatcher.h \
    $$PWD/qv4debugservice.h \
    $$PWD/qv4debugger.h \
    $$PWD/qv4debuggeragent.h \
    $$PWD/qv4datacollector.h \
    $$PWD/qv4debugjob.h

OTHER_FILES += \
    $$PWD/qqmldebuggerservice.json

PLUGIN_TYPE = qmltooling
PLUGIN_CLASS_NAME = QQmlDebuggerServiceFactory
load(qt_plugin)
