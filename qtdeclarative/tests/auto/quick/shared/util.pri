
QT += core-private gui-private  qml-private quick-private

HEADERS += $$PWD/visualtestutil.h \
           $$PWD/geometrytestutil.h \
           $$PWD/viewtestutil.h
SOURCES += $$PWD/visualtestutil.cpp \
           $$PWD/geometrytestutil.cpp \
           $$PWD/viewtestutil.cpp

DEFINES += QT_QMLTEST_DATADIR=\\\"$${_PRO_FILE_PWD_}/data\\\"
