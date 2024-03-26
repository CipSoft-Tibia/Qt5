// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtQuickTest/quicktest.h>
class tst_character: public QObject
{
    Q_OBJECT
private slots:
    void skiptest() { QSKIP("This test will fail, skipping."); };
};
int main(int argc, char **argv)
{
    if (!qEnvironmentVariableIsEmpty("QEMU_LD_PREFIX")) {
        qWarning("This test is unstable on QEMU, so it will be skipped.");
        tst_character skip;
        return QTest::qExec(&skip, argc, argv);
    }
    QTEST_SET_MAIN_SOURCE_PATH
    return quick_test_main(argc, argv, "tst_character", QUICK_TEST_SOURCE_DIR);
}
#include "tst_character.moc"
