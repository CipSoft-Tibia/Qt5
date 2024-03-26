// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QtGui/QtGui>

class tst_qtiff: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void readImage_data();
    void readImage();
    void readCorruptImage_data();
    void readCorruptImage();

private:
    QString prefix;
};

void tst_qtiff::initTestCase()
{
    prefix = ":/tiff/";
}

void tst_qtiff::readImage_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QSize>("size");

    QTest::newRow("grayscale-ref") << QString("grayscale-ref.tif") << QSize(320, 200);
    QTest::newRow("grayscale") << QString("grayscale.tif") << QSize(320, 200);
    QTest::newRow("image_100dpi") << QString("image_100dpi.tif") << QSize(22, 22);
    QTest::newRow("image") << QString("image.tif") << QSize(22, 22);
    QTest::newRow("indexed_orientation_1") << QString("indexed_orientation_1.tiff") << QSize(64, 64);
    QTest::newRow("indexed_orientation_2") << QString("indexed_orientation_2.tiff") << QSize(64, 64);
    QTest::newRow("indexed_orientation_3") << QString("indexed_orientation_3.tiff") << QSize(64, 64);
    QTest::newRow("indexed_orientation_4") << QString("indexed_orientation_4.tiff") << QSize(64, 64);
    QTest::newRow("indexed_orientation_5") << QString("indexed_orientation_5.tiff") << QSize(64, 64);
    QTest::newRow("indexed_orientation_6") << QString("indexed_orientation_6.tiff") << QSize(64, 64);
    QTest::newRow("indexed_orientation_7") << QString("indexed_orientation_7.tiff") << QSize(64, 64);
    QTest::newRow("indexed_orientation_8") << QString("indexed_orientation_8.tiff") << QSize(64, 64);
    QTest::newRow("mono_orientation_1") << QString("mono_orientation_1.tiff") << QSize(64, 64);
    QTest::newRow("mono_orientation_2") << QString("mono_orientation_2.tiff") << QSize(64, 64);
    QTest::newRow("mono_orientation_3") << QString("mono_orientation_3.tiff") << QSize(64, 64);
    QTest::newRow("mono_orientation_4") << QString("mono_orientation_4.tiff") << QSize(64, 64);
    QTest::newRow("mono_orientation_5") << QString("mono_orientation_5.tiff") << QSize(64, 64);
    QTest::newRow("mono_orientation_6") << QString("mono_orientation_6.tiff") << QSize(64, 64);
    QTest::newRow("mono_orientation_7") << QString("mono_orientation_7.tiff") << QSize(64, 64);
    QTest::newRow("mono_orientation_8") << QString("mono_orientation_8.tiff") << QSize(64, 64);
    QTest::newRow("original_indexed") << QString("original_indexed.tiff") << QSize(64, 64);
    QTest::newRow("original_mono") << QString("original_mono.tiff") << QSize(64, 64);
    QTest::newRow("original_rgb") << QString("original_rgb.tiff") << QSize(64, 64);
    QTest::newRow("rgba_adobedeflate_littleendian") << QString("rgba_adobedeflate_littleendian.tif") << QSize(200, 200);
    QTest::newRow("rgba_lzw_littleendian") << QString("rgba_lzw_littleendian.tif") << QSize(200, 200);
    QTest::newRow("rgba_nocompression_bigendian") << QString("rgba_nocompression_bigendian.tif") << QSize(200, 200);
    QTest::newRow("rgba_nocompression_littleendian") << QString("rgba_nocompression_littleendian.tif") << QSize(200, 200);
    QTest::newRow("rgba_packbits_littleendian") << QString("rgba_packbits_littleendian.tif") << QSize(200, 200);
    QTest::newRow("rgba_zipdeflate_littleendian") << QString("rgba_zipdeflate_littleendian.tif") << QSize(200, 200);
    QTest::newRow("rgb_orientation_1") << QString("rgb_orientation_1.tiff") << QSize(64, 64);
    QTest::newRow("rgb_orientation_2") << QString("rgb_orientation_2.tiff") << QSize(64, 64);
    QTest::newRow("rgb_orientation_3") << QString("rgb_orientation_3.tiff") << QSize(64, 64);
    QTest::newRow("rgb_orientation_4") << QString("rgb_orientation_4.tiff") << QSize(64, 64);
    QTest::newRow("rgb_orientation_5") << QString("rgb_orientation_5.tiff") << QSize(64, 64);
    QTest::newRow("rgb_orientation_6") << QString("rgb_orientation_6.tiff") << QSize(64, 64);
    QTest::newRow("rgb_orientation_7") << QString("rgb_orientation_7.tiff") << QSize(64, 64);
    QTest::newRow("rgb_orientation_8") << QString("rgb_orientation_8.tiff") << QSize(64, 64);
    QTest::newRow("teapot") << QString("teapot.tiff") << QSize(256, 256);
}

void tst_qtiff::readImage()
{
    QFETCH(QString, fileName);
    QFETCH(QSize, size);

    QString path = prefix + fileName;
    QBENCHMARK {
        QImageReader reader(path);
        QVERIFY(reader.canRead());
        QImage image = reader.read();
        QVERIFY(!image.isNull());
        QCOMPARE(image.size(), size);
    }
}

void tst_qtiff::readCorruptImage_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("message");

    QTest::newRow("corrupt-data") << QString("corrupt-data.tif") << QString();
}

void tst_qtiff::readCorruptImage()
{
    QFETCH(QString, fileName);
    QFETCH(QString, message);

    QString path = prefix + fileName;
    QBENCHMARK {
        QImageReader reader(path);
        if (!message.isEmpty())
            QTest::ignoreMessage(QtWarningMsg, message.toLatin1());
        QVERIFY(reader.canRead());
        QImage image = reader.read();
        QVERIFY(image.isNull());
    }
}

QTEST_MAIN(tst_qtiff)
#include "tst_qtiff.moc"
