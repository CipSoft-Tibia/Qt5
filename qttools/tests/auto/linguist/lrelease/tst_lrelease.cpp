// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QByteArray>

#include <QtTest/QtTest>

class tst_lrelease : public QObject
{
    Q_OBJECT

public:
    tst_lrelease()
         : lrelease(QLibraryInfo::path(QLibraryInfo::BinariesPath) + "/lrelease")
         , dataDir(QFINDTESTDATA("testdata/"))
    {}

private:

private slots:
    void translate();
    void compressed();
    void idbased();
    void markuntranslated();
    void dupes();
    void noTranslations();

private:
    void doCompare(const QStringList &actual, const QString &expectedFn);

    QString lrelease;
    QString dataDir;
};

void tst_lrelease::doCompare(const QStringList &actual, const QString &expectedFn)
{
    QFile file(expectedFn);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QStringList expected = QString(file.readAll()).trimmed().split('\n');

    int i = 0, ei = expected.size(), gi = actual.size();
    for (; ; i++) {
        if (i == gi) {
            if (i == ei)
                return;
            gi = 0;
            break;
        } else if (i == ei) {
            ei = 0;
            break;
        } else if (!QRegularExpression(QRegularExpression::anchoredPattern(expected.at(i))).match(actual.at(i)).hasMatch()) {
            while ((ei - 1) >= i && (gi - 1) >= i &&
                     (QRegularExpression(QRegularExpression::anchoredPattern(expected.at(ei - 1))).match(actual.at(gi - 1))).hasMatch())
                ei--, gi--;
            break;
        }
    }
    QString diff;
    for (int j = qMax(0, i - 3); j < i; j++)
        diff += expected.at(j) + '\n';
    diff += "<<<<<<< got\n";
    for (int j = i; j < gi; j++) {
        diff += actual.at(j) + '\n';
        if (j >= i + 5) {
            diff += "...\n";
            break;
        }
    }
    diff += "=========\n";
    for (int j = i; j < ei; j++) {
        diff += expected.at(j) + '\n';
        if (j >= i + 5) {
            diff += "...\n";
            break;
        }
    }
    diff += ">>>>>>> expected\n";
    for (int j = ei; j < qMin(ei + 3, expected.size()); j++)
        diff += expected.at(j) + '\n';
    QFAIL(qPrintable("Output for " + expectedFn + " does not meet expectations:\n" + diff));
}

void tst_lrelease::translate()
{
    QVERIFY(!QProcess::execute(lrelease, QStringList() << (dataDir + "translate.ts")));

    QTranslator translator;
    QVERIFY(translator.load(dataDir + "translate.qm"));
    qApp->installTranslator(&translator);

    QCOMPARE(QObject::tr("\nnewline at the start"), QString("\nNEWLINE AT THE START"));
    QCOMPARE(QObject::tr("newline at the end\n"),   QString("NEWLINE AT THE END\n"));
    QCOMPARE(QObject::tr("newline and space at the end\n "),   QString("NEWLINE AND SPACE AT THE END\n "));
    QCOMPARE(QObject::tr("space and newline at the end \n"),   QString("SPACE AND NEWLINE AT THE END \n"));
    QCOMPARE(QObject::tr("\ttab at the start and newline at the end\n"),   QString("\tTAB AT THE START AND NEWLINE AT THE END\n"));
    QCOMPARE(QObject::tr("\n\tnewline and tab at the start"), QString("\n\tNEWLINE AND TAB AT THE START"));
    QCOMPARE(QObject::tr(" \tspace and tab at the start"), QString(" \tSPACE AND TAB AT THE START"));
    QCOMPARE(QObject::tr(" string that does not exist"), QString(" string that does not exist"));

    QCOMPARE(QCoreApplication::translate("CubeForm", "Test"), QString::fromLatin1("BBBB"));
    QCOMPARE(QCoreApplication::translate("", "Test", "Empty context"), QString("AAAA"));

    // Test plurals
    QString txed = QCoreApplication::translate("Plurals", "There are %n houses", 0, 0);
    QCOMPARE(QString::fromLatin1("[%1]").arg(txed), QString("[There are 0 houses]"));
    QCOMPARE(QCoreApplication::translate("Plurals", "There are %n houses", 0, 1), QString("There is 1 house"));
    QCOMPARE(QCoreApplication::translate("Plurals", "There are %n houses", 0, 2), QString("There are 2 houses"));
    QCOMPARE(QCoreApplication::translate("Plurals", "There are %n houses", 0, 3), QString("There are 3 houses"));


    // More plurals
    QCOMPARE(tr("There are %n cars", "More Plurals", 0) , QString("There are 0 cars"));
    QCOMPARE(tr("There are %n cars", "More Plurals", 1) , QString("There is 1 car"));
    QCOMPARE(tr("There are %n cars", "More Plurals", 2) , QString("There are 2 cars"));
    QCOMPARE(tr("There are %n cars", "More Plurals", 3) , QString("There are 3 cars"));


    QCOMPARE(QCoreApplication::translate("no_en", "Kj\xc3\xb8r K\xc3\xa5re, kj\xc3\xa6re"), QString::fromUtf8("Drive K\xc3\xa5re, dear"));
    QCOMPARE(QCoreApplication::translate("en_no", "Drive K\xc3\xa5re, dear"), QString::fromUtf8("Kj\xc3\xb8r K\xc3\xa5re, kj\xc3\xa6re"));
    QCOMPARE(QCoreApplication::translate("en_ch", "Chinese symbol:"), QString::fromUtf8("Chinese symbol:\xe7\xb0\x9f"));

//    printf("halo\r\nhallo");
  //  QCOMPARE(tr("This\r\nwill fail"), QString("THIS\nWILL FAIL"));    // \r\n =  0d 0a

    QCOMPARE(tr("Completely random string"),
             QString::fromLatin1("Super-lange Uebersetzung mit Schikanen\x9c"
                                 "Mittlere Uebersetung\x9c"
                                 "Kurze Uebers."));

    qApp->removeTranslator(&translator);
}

void tst_lrelease::compressed()
{
    QVERIFY(!QProcess::execute(lrelease, QStringList() << "-compress" << (dataDir + "compressed.ts")));

    QTranslator translator;
    QVERIFY(translator.load(dataDir + "compressed.qm"));
    qApp->installTranslator(&translator);

    QCOMPARE(QCoreApplication::translate("Context1", "Foo"), QString::fromLatin1("in first context"));
    QCOMPARE(QCoreApplication::translate("Context2", "Bar"), QString::fromLatin1("in second context"));

    QCOMPARE(QCoreApplication::translate("Action1", "Component Name"), QString::fromLatin1("translation in first context"));
    QCOMPARE(QCoreApplication::translate("Action2", "Component Name"), QString::fromLatin1("translation in second context"));
    QCOMPARE(QCoreApplication::translate("Action3", "Component Name"), QString::fromLatin1("translation in third context"));

}

void tst_lrelease::idbased()
{
    QVERIFY(!QProcess::execute(lrelease, QStringList() << "-idbased" << (dataDir + "idbased.ts")));

    QTranslator translator;
    QVERIFY(translator.load(dataDir + "idbased.qm"));
    qApp->installTranslator(&translator);

    QCOMPARE(qtTrId("test_id"), QString::fromLatin1("This is a test string."));
    QCOMPARE(qtTrId("untranslated_id"), QString::fromLatin1("This has no translation."));
}

void tst_lrelease::markuntranslated()
{
    QVERIFY(!QProcess::execute(lrelease, QStringList() << "-markuntranslated" << "#" << "-idbased" << (dataDir + "idbased.ts")));

    QTranslator translator;
    QVERIFY(translator.load(dataDir + "idbased.qm"));
    qApp->installTranslator(&translator);

    QCOMPARE(qtTrId("test_id"), QString::fromLatin1("This is a test string."));
    QCOMPARE(qtTrId("untranslated_id"), QString::fromLatin1("#This has no translation."));
}

void tst_lrelease::dupes()
{
    QProcess proc;
    proc.start(lrelease, QStringList() << (dataDir + "dupes.ts"), QIODevice::ReadWrite | QIODevice::Text);
    QVERIFY(proc.waitForFinished());
    QVERIFY(proc.exitStatus() == QProcess::NormalExit);
    doCompare(QString(proc.readAllStandardError()).trimmed().split('\n'), dataDir + "dupes.errors");
}

void tst_lrelease::noTranslations()
{
    QProcess proc;
    proc.start(lrelease, { dataDir + "no-translations.pro" });
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitStatus(), QProcess::NormalExit);
    QCOMPARE(proc.exitCode(), 0);
    auto stderrOutput = proc.readAllStandardError();
    QVERIFY(stderrOutput.contains("lrelease warning: Met no 'TRANSLATIONS' entry in project file"));
}

QTEST_MAIN(tst_lrelease)
#include "tst_lrelease.moc"
