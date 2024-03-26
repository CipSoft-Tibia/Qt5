// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QtTest/QtTest>

#include <QtCore/QFileInfo>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

#include "../../../src/assistant/qhelpgenerator/qhelpprojectdata_p.h"
#include "../../../src/assistant/qhelpgenerator/helpgenerator.h"

class tst_QHelpGenerator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void generateHelp();
    // Check that two runs of the generator creates the same file twice
    void generateTwice();

private:
    void checkNamespace();
    void checkFilters();
    void checkIndices();
    void checkFiles();
    void checkMetaData();

    QString m_outputFile;
    QSqlQuery *m_query;
};

void tst_QHelpGenerator::initTestCase()
{
    QString path = QLatin1String(SRCDIR);
    m_outputFile = path + QLatin1String("/data/test.qch");
    if (QFile::exists(m_outputFile)) {
        QDir d;
        if (!d.remove(m_outputFile))
            QFAIL("Cannot remove old output file!");
    }
}

void tst_QHelpGenerator::generateHelp()
{
    // defined in profile
    QString path = QLatin1String(SRCDIR);

    QString inputFile(path + "/data/test.qhp");
    QHelpProjectData data;
    if (!data.readData(inputFile))
        QFAIL("Cannot read qthp file!");

    HelpGenerator generator;
    QCOMPARE(generator.generate(&data, m_outputFile), true);

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "testdb");
        db.setDatabaseName(m_outputFile);
        if (!db.open()) {
            QSqlDatabase::removeDatabase("testdb");
            QFAIL("Created database seems to be corrupt!");
        }
        m_query = new QSqlQuery(db);
        checkNamespace();
        checkFilters();
        checkIndices();
        checkFiles();
        checkMetaData();

        m_query->clear();
        delete m_query;
    }
    QSqlDatabase::removeDatabase("testdb");

    // check if db is still in use...
    initTestCase();
}

void tst_QHelpGenerator::checkNamespace()
{
    m_query->exec("SELECT Id, Name FROM NamespaceTable");
    if (m_query->next()
        && m_query->value(1).toString() == QLatin1String("trolltech.com.1.0.0.test"))
        return;
    QFAIL("Namespace Error!");
}

void tst_QHelpGenerator::checkFilters()
{
    m_query->exec("SELECT COUNT(Id) FROM FilterAttributeTable");
    if (!m_query->next() || m_query->value(0).toInt() != 3)
        QFAIL("FilterAttribute Error!");

    m_query->exec("SELECT a.Name FROM FilterAttributeTable a, FilterTable b, "
        "FilterNameTable c WHERE c.Id=b.NameId AND b.FilterAttributeID=a.Id "
        "AND c.Name=\'Custom Filter 2\'");
    QStringList lst;
    while (m_query->next())
        lst.append(m_query->value(0).toString());
    if (!lst.contains("test") || !lst.contains("filter2"))
        QFAIL("FilterAttribute Error!");
}

void tst_QHelpGenerator::checkIndices()
{
    m_query->exec("SELECT a.Name, b.Anchor FROM FileNameTable a, "
        "IndexTable b, IndexFilterTable c, FilterAttributeTable d "
        "WHERE a.FileID=b.FileId AND b.Id=c.IndexId "
        "AND c.FilterAttributeId=d.Id AND d.Name=\'filter1\' AND b.Name=\'foo\'");
    if (!m_query->next() || m_query->value(0).toString() != QLatin1String("test.html")
        || m_query->value(1).toString() != QLatin1String("foo"))
        QFAIL("Index Error!");

    m_query->exec("SELECT COUNT(a.Id) FROM IndexTable a, "
        "IndexFilterTable b, FilterAttributeTable c WHERE a.Id=b.IndexId "
        "AND b.FilterAttributeId=c.Id AND c.Name=\'filter2\'");
    if (!m_query->next() || m_query->value(0).toInt() != 3)
        QFAIL("Index Error!");
}

void tst_QHelpGenerator::checkFiles()
{
    m_query->exec("SELECT COUNT(a.FileId) FROM FileNameTable a, FolderTable b "
        "WHERE a.FolderId=b.Id AND b.Name=\'testFolder\'");
    if (!m_query->next() || m_query->value(0).toInt() != 6)
        QFAIL("File Error!");

    QStringList lst;
    lst << "classic.css" << "test.html" << "people.html" << "sub/about.html";
    m_query->exec("SELECT a.Name FROM FileNameTable a, FileFilterTable b, "
        "FilterAttributeTable c WHERE c.Id=b.FilterAttributeId "
        "AND b.FileId=a.FileID AND c.Name=\'filter1\'");
    while (m_query->next())
        lst.removeAll(m_query->value(0).toString());
    QCOMPARE(lst.size(), 0);

    QMap<int, QStringList> fileAtts;
    m_query->exec("SELECT a.Id, b.Name FROM FileAttributeSetTable a, "
        "FilterAttributeTable b WHERE a.FilterAttributeId=b.Id");
    while (m_query->next()) {
        int id = m_query->value(0).toInt();
        if (!fileAtts.contains(id))
            fileAtts.insert(id, QStringList());
        fileAtts[id].append(m_query->value(1).toString());
    }
    QCOMPARE(fileAtts.size(), 2);
    QVERIFY(fileAtts.value(1).contains("test"));
    QVERIFY(fileAtts.value(1).contains("filter1"));
    QVERIFY(!fileAtts.value(1).contains("filter2"));
    QVERIFY(fileAtts.value(2).contains("test"));
    QVERIFY(fileAtts.value(2).contains("filter2"));
}

void tst_QHelpGenerator::checkMetaData()
{
    m_query->exec("SELECT COUNT(Value) FROM MetaDataTable");
    if (!m_query->next())
        QFAIL("Meta Data Error");
    QCOMPARE(m_query->value(0).toInt(), 3);

    m_query->exec("SELECT Value FROM MetaDataTable WHERE Name=\'author\'");
    if (!m_query->next())
        QFAIL("Meta Data Error");
    QCOMPARE(m_query->value(0).toString(), QString("Digia Plc and/or its subsidiary(-ies)"));

}

void tst_QHelpGenerator::generateTwice()
{
    // defined in profile
    QString path = QLatin1String(SRCDIR);

    QString inputFile(path + "/data/test.qhp");
    QHelpProjectData data;
    if (!data.readData(inputFile))
        QFAIL("Cannot read qhp file!");

    HelpGenerator generator1;
    HelpGenerator generator2;
    QString outputFile1 = path + QLatin1String("/data/test1.qch");
    QString outputFile2 = path + QLatin1String("/data/test2.qch");
    QCOMPARE(generator1.generate(&data, outputFile1), true);
    QCOMPARE(generator2.generate(&data, outputFile2), true);

    QFile f1(outputFile1);
    QFile f2(outputFile2);
    QVERIFY(f1.open(QIODevice::ReadOnly));
    QVERIFY(f2.open(QIODevice::ReadOnly));

    QByteArray arr1 = f1.readAll();
    QByteArray arr2 = f2.readAll();

    QFile::remove(outputFile1);
    QFile::remove(outputFile2);
    QCOMPARE(arr1, arr2);
}

QTEST_MAIN(tst_QHelpGenerator)
#include "tst_qhelpgenerator.moc"
