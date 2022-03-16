TARGET     = Qt3DLogic
MODULE     = 3dlogic

QT         = core-private gui-private 3dcore 3dcore-private

# Qt3D is free of Q_FOREACH - make sure it stays that way:
DEFINES += QT_NO_FOREACH

gcov {
    QMAKE_CXXFLAGS += -fprofile-arcs -ftest-coverage
    QMAKE_LFLAGS += -fprofile-arcs -ftest-coverage
}

include(logic.pri)

load(qt_module)
