// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QObject>
#include <QProcessEnvironment>

class tst_QProcessEnvironment: public QObject
{
    Q_OBJECT
private slots:
    void operator_eq();
    void clearAndIsEmpty();
    void clearAndInheritsFromParent();
    void insert();
    void emptyNull();
    void toStringList();
    void keys();
    void insertEnv();

    void caseSensitivity();
    void systemEnvironment();
    void putenv();
};

void tst_QProcessEnvironment::operator_eq()
{
    QProcessEnvironment e1;
    QCOMPARE(e1, e1);
    e1.clear();
    QCOMPARE(e1, e1);

    e1 = QProcessEnvironment();
    QProcessEnvironment e2;
    QCOMPARE(e1, e2);

    auto parentEnv = QProcessEnvironment(QProcessEnvironment::InheritFromParent);
    QVERIFY(parentEnv != e2);
    QVERIFY(e2 != parentEnv);

    e1.clear();
    QCOMPARE(e1, e2);

    e2.clear();
    QCOMPARE(e1, e2);

    e1.insert("FOO", "bar");
    QVERIFY(e1 != e2);

    e2.insert("FOO", "bar");
    QCOMPARE(e1, e2);

    e2.insert("FOO", "baz");
    QVERIFY(e1 != e2);

    QVERIFY(e2 != parentEnv);
    QVERIFY(parentEnv != e2);
}

void tst_QProcessEnvironment::clearAndIsEmpty()
{
    QProcessEnvironment e;
    QVERIFY(e.isEmpty());
    QVERIFY(!e.inheritsFromParent());
    e.insert("FOO", "bar");
    QVERIFY(!e.isEmpty());
    QVERIFY(!e.inheritsFromParent());
    e.clear();
    QVERIFY(e.isEmpty());
    QVERIFY(!e.inheritsFromParent());
}

void tst_QProcessEnvironment::clearAndInheritsFromParent()
{
    QProcessEnvironment e(QProcessEnvironment::InheritFromParent);
    QVERIFY(e.isEmpty());
    QVERIFY(e.inheritsFromParent());
    // Clearing null environment keeps it null
    e.clear();
    QVERIFY(e.isEmpty());
    QVERIFY(e.inheritsFromParent());
    e.insert("FOO", "bar");
    QVERIFY(!e.isEmpty());
    QVERIFY(!e.inheritsFromParent());
    e.clear();
    QVERIFY(e.isEmpty());
    QVERIFY(!e.inheritsFromParent());
}

void tst_QProcessEnvironment::insert()
{
    QProcessEnvironment e;
    e.insert("FOO", "bar");
    QVERIFY(!e.isEmpty());
    QVERIFY(e.contains("FOO"));
    QCOMPARE(e.value("FOO"), QString("bar"));

    e.remove("FOO");
    QVERIFY(!e.contains("FOO"));
    QVERIFY(e.value("FOO").isNull());

    e.clear();
    QVERIFY(!e.contains("FOO"));
}

void tst_QProcessEnvironment::emptyNull()
{
    QProcessEnvironment e;

    e.insert("FOO", "");
    QVERIFY(e.contains("FOO"));
    QVERIFY(e.value("FOO").isEmpty());
    QVERIFY(!e.value("FOO").isNull());

    e.insert("FOO", QString());
    QVERIFY(e.contains("FOO"));
    QVERIFY(e.value("FOO").isEmpty());
    // don't test if it's NULL, since we shall not make a guarantee

    e.remove("FOO");
    QVERIFY(!e.contains("FOO"));
}

void tst_QProcessEnvironment::toStringList()
{
    QProcessEnvironment e;
    QVERIFY(e.isEmpty());
    QVERIFY(e.toStringList().isEmpty());

    e.insert("FOO", "bar");
    QStringList result = e.toStringList();
    QVERIFY(!result.isEmpty());
    QCOMPARE(result.size(), 1);
    QCOMPARE(result.at(0), QString("FOO=bar"));

    e.clear();
    e.insert("BAZ", "");
    result = e.toStringList();
    QCOMPARE(result.at(0), QString("BAZ="));

    e.insert("FOO", "bar");
    e.insert("A", "bc");
    e.insert("HELLO", "World");
    result = e.toStringList();
    QCOMPARE(result.size(), 4);

    // order is not specified, so use contains()
    QVERIFY(result.contains("FOO=bar"));
    QVERIFY(result.contains("BAZ="));
    QVERIFY(result.contains("A=bc"));
    QVERIFY(result.contains("HELLO=World"));
}

void tst_QProcessEnvironment::keys()
{
    QProcessEnvironment e;
    QVERIFY(e.isEmpty());
    QVERIFY(e.keys().isEmpty());

    e.insert("FOO", "bar");
    QStringList result = e.keys();
    QCOMPARE(result.size(), 1);
    QCOMPARE(result.at(0), QString("FOO"));

    e.clear();
    e.insert("BAZ", "");
    result = e.keys();
    QCOMPARE(result.at(0), QString("BAZ"));

    e.insert("FOO", "bar");
    e.insert("A", "bc");
    e.insert("HELLO", "World");
    result = e.keys();
    QCOMPARE(result.size(), 4);

    // order is not specified, so use contains()
    QVERIFY(result.contains("FOO"));
    QVERIFY(result.contains("BAZ"));
    QVERIFY(result.contains("A"));
    QVERIFY(result.contains("HELLO"));
}

void tst_QProcessEnvironment::insertEnv()
{
    QProcessEnvironment e;
    e.insert("FOO", "bar");
    e.insert("A", "bc");
    e.insert("Hello", "World");

    QProcessEnvironment e2;
    e2.insert("FOO2", "bar2");
    e2.insert("A2", "bc2");
    e2.insert("Hello", "Another World");

    e.insert(e2);
    QStringList keys = e.keys();
    QCOMPARE(keys.size(), 5);

    QCOMPARE(e.value("FOO"), QString("bar"));
    QCOMPARE(e.value("A"), QString("bc"));
    QCOMPARE(e.value("Hello"), QString("Another World"));
    QCOMPARE(e.value("FOO2"), QString("bar2"));
    QCOMPARE(e.value("A2"), QString("bc2"));

    QProcessEnvironment e3;
    e3.insert("FOO2", "bar2");
    e3.insert("A2", "bc2");
    e3.insert("Hello", "Another World");

    e3.insert(e3); // mustn't deadlock

    QCOMPARE(e3, e2);
}

void tst_QProcessEnvironment::caseSensitivity()
{
    QProcessEnvironment e;
    e.insert("foo", "bar");

#ifdef Q_OS_WIN
    // Windows is case-insensitive, but case-preserving
    QVERIFY(e.contains("foo"));
    QVERIFY(e.contains("FOO"));
    QVERIFY(e.contains("FoO"));

    QCOMPARE(e.value("foo"), QString("bar"));
    QCOMPARE(e.value("FOO"), QString("bar"));
    QCOMPARE(e.value("FoO"), QString("bar"));

    // Per Windows, this overwrites the value, but keeps the name's original capitalization
    e.insert("Foo", "Bar");

    QStringList list = e.toStringList();
    QCOMPARE(list.length(), 1);
    QCOMPARE(list.at(0), QString("foo=Bar"));
#else
    // otherwise, it's case sensitive
    QVERIFY(e.contains("foo"));
    QVERIFY(!e.contains("FOO"));

    e.insert("FOO", "baz");
    QVERIFY(e.contains("FOO"));
    QCOMPARE(e.value("FOO"), QString("baz"));
    QCOMPARE(e.value("foo"), QString("bar"));

    QStringList list = e.toStringList();
    QCOMPARE(list.size(), 2);
    QVERIFY(list.contains("foo=bar"));
    QVERIFY(list.contains("FOO=baz"));
#endif
}

void tst_QProcessEnvironment::systemEnvironment()
{
#ifdef Q_OS_VXWORKS
    QSKIP("VxWorks do not have PATH environment variable");
#endif
    static const char envname[] = "THIS_ENVIRONMENT_VARIABLE_HOPEFULLY_DOESNT_EXIST";
    QByteArray path = qgetenv("PATH");
    QByteArray nonexistant = qgetenv(envname);
    QProcessEnvironment system = QProcessEnvironment::systemEnvironment();

    QVERIFY(nonexistant.isNull());

    // all other system have environments
    if (path.isEmpty())
        QFAIL("Could not find the PATH environment variable -- please correct the test environment");

    QVERIFY(system.contains("PATH"));
    QCOMPARE(system.value("PATH"), QString::fromLocal8Bit(path));

    QVERIFY(!system.contains(envname));

#ifdef Q_OS_WIN
    // check case-insensitive too
    QVERIFY(system.contains("path"));
    QCOMPARE(system.value("path"), QString::fromLocal8Bit(path));

    QVERIFY(!system.contains(QString(envname).toLower()));
#endif
}

void tst_QProcessEnvironment::putenv()
{
    static const char envname[] = "WE_RE_SETTING_THIS_ENVIRONMENT_VARIABLE";
    static bool testRan = false;

    if (testRan)
        QFAIL("You cannot run this test more than once, since we modify the environment");
    testRan = true;

    QByteArray valBefore = qgetenv(envname);
    if (!valBefore.isNull())
        QFAIL("The environment variable we set in the environment is already set! -- please correct the test environment");
    QProcessEnvironment eBefore = QProcessEnvironment::systemEnvironment();

    qputenv(envname, "Hello, World");
    QByteArray valAfter = qgetenv(envname);
    QCOMPARE(valAfter, QByteArray("Hello, World"));

    QProcessEnvironment eAfter = QProcessEnvironment::systemEnvironment();

    QVERIFY(!eBefore.contains(envname));
    QVERIFY(eAfter.contains(envname));
    QCOMPARE(eAfter.value(envname), QString("Hello, World"));

# ifdef Q_OS_WIN
    // check case-insensitive too
    QString lower = envname;
    lower = lower.toLower();
    QVERIFY(!eBefore.contains(lower));
    QVERIFY(eAfter.contains(lower));
    QCOMPARE(eAfter.value(lower), QString("Hello, World"));
# endif
}

QTEST_MAIN(tst_QProcessEnvironment)

#include "tst_qprocessenvironment.moc"
