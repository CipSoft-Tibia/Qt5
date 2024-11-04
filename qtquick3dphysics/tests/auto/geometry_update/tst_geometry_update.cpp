// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQuickTest/quicktest.h>
class tst_geometry_update : public QObject
{
    Q_OBJECT
private slots:
    void skiptest() { QSKIP("This test will fail, skipping."); };
};
int main(int argc, char **argv)
{
    if (!qEnvironmentVariableIsEmpty("QEMU_LD_PREFIX")) {
        qWarning("This test is unstable on QEMU, so it will be skipped.");
        tst_geometry_update skip;
        return QTest::qExec(&skip, argc, argv);
    }
    QTEST_SET_MAIN_SOURCE_PATH
    return quick_test_main(argc, argv, "tst_geometry_update", QUICK_TEST_SOURCE_DIR);
}

#include "tst_geometry_update.moc"
