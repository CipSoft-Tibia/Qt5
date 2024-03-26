// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QtGui/QtGui>

class tst_qmng: public QObject
{
    Q_OBJECT

private slots:
    void readImage_data();
    void readImage();
    void readCorruptImage_data();
    void readCorruptImage();
};

void tst_qmng::readImage_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QSize>("size");

    QTest::newRow("animation") << QString("animation.mng") << QSize(100, 100);
    QTest::newRow("ball") << QString("ball.mng") << QSize(32, 32);
    QTest::newRow("dutch") << QString("dutch.mng") << QSize(352, 264);
    QTest::newRow("fire") << QString("fire.mng") << QSize(30, 60);
}

void tst_qmng::readImage()
{
    QFETCH(QString, fileName);
    QFETCH(QSize, size);

    QString path = QString(":/mng/") + fileName;
    QBENCHMARK {
        QImageReader reader(path);
        QVERIFY(reader.canRead());
        QImage image = reader.read();
        QVERIFY(!image.isNull());
        QCOMPARE(image.size(), size);
    }
}

void tst_qmng::readCorruptImage_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("message");

    QTest::newRow("corrupt")
            << QString("corrupt.mng")
            << QString("MNG error 901: Application signalled I/O error; chunk IHDR; subcode 0:0");
}

void tst_qmng::readCorruptImage()
{
    QFETCH(QString, fileName);
    QFETCH(QString, message);

    QString path = QString(":/mng/") + fileName;
    QBENCHMARK {
        QImageReader reader(path);
        if (!message.isEmpty())
            QTest::ignoreMessage(QtWarningMsg, message.toLatin1());
        QVERIFY(reader.canRead());
        QImage image = reader.read();
        QVERIFY(image.isNull());
    }
}

QTEST_MAIN(tst_qmng)
#include "tst_qmng.moc"
