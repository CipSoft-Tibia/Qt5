TEMPLATE = app
CONFIG += benchmark
QT = core testlib core5compat

QMAKE_CXXFLAGS += -g
QMAKE_CFLAGS += -g

TARGET = tst_bench_qstringbuilder
SOURCES += main.cpp
