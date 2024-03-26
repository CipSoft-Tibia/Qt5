// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qglobal.h>

// SCENARIO 1
// this is the "no harm done" version. Only operator% is active,
// with NO_CAST * defined
#undef QT_USE_QSTRINGBUILDER
#define QT_NO_CAST_FROM_ASCII
#define QT_NO_CAST_TO_ASCII

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringBuilder>
#include <QtTest/QTest>
#include <qstringref.h>

#define LITERAL "some literal"

void runScenario(); // Defined in stringbuilder.cpp #included below.

class tst_QStringBuilder1 : public QObject
{
    Q_OBJECT

private slots:
    void scenario() { runScenario(); }
};

#define P %
#include "stringbuilder.cpp"
#undef P

#include "tst_qstringbuilder1.moc"

QTEST_APPLESS_MAIN(tst_QStringBuilder1)
