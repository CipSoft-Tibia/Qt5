TARGET     = Qt3DCore
MODULE     = 3dcore

QT         = core-private gui-private network

# Qt3D is free of Q_FOREACH - make sure it stays that way:
DEFINES += QT_NO_FOREACH

gcov {
    QMAKE_CXXFLAGS += -fprofile-arcs -ftest-coverage
    QMAKE_LFLAGS += -fprofile-arcs -ftest-coverage
}

include(core.pri)
include(aspects/aspects.pri)
include(changes/changes.pri)
include(jobs/jobs.pri)
include(nodes/nodes.pri)
include(transforms/transforms.pri)
include(resources/resources.pri)
include(services/services.pri)

load(qt_module)
