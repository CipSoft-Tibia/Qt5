TEMPLATE = app

QT = testlib quick3d-private quick3druntimerender-private

requires(qtConfig(private_tests))

CONFIG += testcase
SOURCES = tst_qquick3dmaterials.cpp

RESOURCES += \
    qquick3dmaterials.qrc
