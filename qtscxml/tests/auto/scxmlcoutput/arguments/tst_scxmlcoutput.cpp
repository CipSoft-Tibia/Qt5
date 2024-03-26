// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest>
#include <QObject>

// This testcase tests the cmake scxmlc generation, and the testcase itself
// does mere basic compilation & statemachine instantiation; these fail
// if qscxmlc generation does not work as expected
#include "ids1.h"

class tst_scxmlcoutput: public QObject
{
    Q_OBJECT
private slots:
    void instantiates();
};

void tst_scxmlcoutput::instantiates() {
    MyClass statemachine;
    QVERIFY(statemachine.property("foo.bar").isValid());
}

QTEST_MAIN(tst_scxmlcoutput)
#include "tst_scxmlcoutput.moc"
