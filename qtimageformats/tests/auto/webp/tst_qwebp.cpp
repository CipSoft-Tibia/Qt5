// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QtGui/QtGui>

class tst_qwebp : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void readImage_data();
    void readImage();
    void readAnimation_data();
    void readAnimation();
    void writeImage_data();
    void writeImage();
};

void tst_qwebp::initTestCase()
{
    if (!QImageReader::supportedImageFormats().contains("webp"))
        QSKIP("The image format handler is not installed.");
}

void tst_qwebp::readImage_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QSize>("size");
    QTest::addColumn<bool>("alpha");

    QTest::newRow("kollada") << QString("kollada") << QSize(436, 160) << true;
    QTest::newRow("kollada_lossless") << QString("kollada_lossless") << QSize(436, 160) << true;
    QTest::newRow("kollada_noalpha") << QString("kollada_noalpha") << QSize(436, 160) << false;
}

void tst_qwebp::readImage()
{
    QFETCH(QString, fileName);
    QFETCH(QSize, size);
    QFETCH(bool, alpha);

    const QString path = QStringLiteral(":/images/") + fileName + QStringLiteral(".webp");
    QImageReader reader(path);
    QVERIFY(reader.canRead());
    QImage image = reader.read();
    QVERIFY2(!image.isNull(), qPrintable(reader.errorString()));
    QCOMPARE(image.size(), size);
    QCOMPARE(image.hasAlphaChannel(), alpha);
}

void tst_qwebp::readAnimation_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QSize>("size");
    QTest::addColumn<int>("imageCount");
    QTest::addColumn<int>("loopCount");
    QTest::addColumn<QColor>("bgColor");
    QTest::addColumn<QList<QRect> >("imageRects");
    QTest::addColumn<QList<int> >("imageDelays");

    QTest::newRow("kollada")
        << QString("kollada")
        << QSize(436, 160)
        << 1
        << 0
        << QColor()
        << (QList<QRect>() << QRect(0, 0, 436, 160))
        << (QList<int>() << 0);
    QTest::newRow("kollada_animation")
        << QString("kollada_animation")
        << QSize(536, 260)
        << 2
        << 2
        << QColor(128, 128, 128, 128)
        << (QList<QRect>() << QRect(0, 0, 436, 160) << QRect(100, 100, 436, 160))
        << (QList<int>() << 1000 << 1200);
}

void tst_qwebp::readAnimation()
{
    QFETCH(QString, fileName);
    QFETCH(QSize, size);
    QFETCH(int, imageCount);
    QFETCH(int, loopCount);
    QFETCH(QColor, bgColor);
    QFETCH(QList<QRect>, imageRects);
    QFETCH(QList<int>, imageDelays);

    const QString path = QStringLiteral(":/images/") + fileName + QStringLiteral(".webp");
    QImageReader reader(path);
    QVERIFY(reader.canRead());
    QCOMPARE(reader.size(), size);
    QCOMPARE(reader.imageCount(), imageCount);
    QCOMPARE(reader.loopCount(), loopCount);
    QCOMPARE(reader.backgroundColor(), bgColor);

    for (int i = 0; i < reader.imageCount(); ++i) {
        QImage image = reader.read();
        QVERIFY2(!image.isNull(), qPrintable(reader.errorString()));
        QCOMPARE(image.size(), size);
        QCOMPARE(reader.currentImageNumber(), i);
        QCOMPARE(reader.currentImageRect(), imageRects[i]);
        QCOMPARE(reader.nextImageDelay(), imageDelays[i]);
    }

    QVERIFY(reader.read().isNull());
    QCOMPARE(reader.canRead(), !reader.supportsAnimation());
}

void tst_qwebp::writeImage_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("postfix");
    QTest::addColumn<int>("quality");
    QTest::addColumn<QSize>("size");
    QTest::addColumn<bool>("alpha");
    QTest::addColumn<bool>("needcheck");

    QTest::newRow("kollada-75") << QString("kollada") << QString(".png") << 75 << QSize(436, 160) << true << false;
    QTest::newRow("kollada-100") << QString("kollada") << QString(".png") << 100 << QSize(436, 160) << true << true;
    QTest::newRow("kollada_noalpha-75") << QString("kollada_noalpha") << QString(".webp") << 75 << QSize(436, 160) << false << false;
    QTest::newRow("kollada_noalpha-100") << QString("kollada_noalpha") << QString(".webp") << 100 << QSize(436, 160) << false << true;
}

void tst_qwebp::writeImage()
{
    QFETCH(QString, fileName);
    QFETCH(QString, postfix);
    QFETCH(int, quality);
    QFETCH(QSize, size);
    QFETCH(bool, alpha);
    QFETCH(bool, needcheck);

    const QString path = QString("%1-%2.webp").arg(fileName).arg(quality);
    const QString sourcePath = QStringLiteral(":/images/") + fileName + postfix;

    QImage image(sourcePath);
    QVERIFY(!image.isNull());
    QVERIFY(image.size() == size);

    QImageWriter writer(path, QByteArrayLiteral("webp"));
    QVERIFY2(writer.canWrite(), qPrintable(writer.errorString()));
    writer.setQuality(quality);
    QVERIFY2(writer.write(image), qPrintable(writer.errorString()));

    QImage reread(path);
    QVERIFY(!reread.isNull());
    QVERIFY(reread.size() == size);
    QVERIFY(reread.hasAlphaChannel() == alpha);

    if (needcheck)
        QVERIFY(image == reread);
}

QTEST_MAIN(tst_qwebp)
#include "tst_qwebp.moc"
