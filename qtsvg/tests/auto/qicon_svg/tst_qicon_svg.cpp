// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtTest/QtTest>
#include <QImageReader>
#include <QtGui>

class tst_QIcon_Svg : public QObject
{
    Q_OBJECT
public:

public slots:
    void initTestCase();

private slots:
    void svgActualSize();
    void svg();
    void availableSizes();
    void isNull();
    void sizeInPercent();


private:
    QString prefix;
};

void tst_QIcon_Svg::initTestCase()
{
    prefix = QFINDTESTDATA("icons/");
    if (prefix.isEmpty())
        QFAIL("Can't find images directory!");
    if (!QImageReader::supportedImageFormats().contains("svg"))
        QFAIL("SVG support is not available");
    QCOMPARE(QImageReader::imageFormat(prefix + "triangle.svg"), QByteArray("svg"));
}

void tst_QIcon_Svg::svgActualSize()
{
    QIcon icon(prefix + "rect.svg");
    QCOMPARE(icon.actualSize(QSize(16, 2)), QSize(16, 2));
    QCOMPARE(icon.pixmap(QSize(16, 16)).size(), QSize(16, 2));

    QPixmap p(16, 16);
    p.fill(Qt::cyan);
    icon.addPixmap(p);

    QCOMPARE(icon.actualSize(QSize(16, 16)), QSize(16, 16));
    QCOMPARE(icon.pixmap(QSize(16, 16)).size(), QSize(16, 16));

    QCOMPARE(icon.actualSize(QSize(16, 14)), QSize(16, 2));
    QCOMPARE(icon.pixmap(QSize(16, 14)).size(), QSize(16, 2));
}

void tst_QIcon_Svg::svg()
{
    QIcon icon1(prefix + "heart.svg");
    QVERIFY(!icon1.pixmap(32).isNull());
    QImage img1 = icon1.pixmap(32).toImage();
    QVERIFY(!icon1.pixmap(32, QIcon::Disabled).isNull());
    QImage img2 = icon1.pixmap(32, QIcon::Disabled).toImage();

    icon1.addFile(prefix + "trash.svg", QSize(), QIcon::Disabled);
    QVERIFY(!icon1.pixmap(32, QIcon::Disabled).isNull());
    QImage img3 = icon1.pixmap(32, QIcon::Disabled).toImage();
    QVERIFY(img3 != img2);
    QVERIFY(img3 != img1);

    QPixmap pm(prefix + "image.png");
    icon1.addPixmap(pm, QIcon::Normal, QIcon::On);
    QVERIFY(!icon1.pixmap(128, QIcon::Normal, QIcon::On).isNull());
    QImage img4 = icon1.pixmap(128, QIcon::Normal, QIcon::On).toImage();
    QVERIFY(img4 != img3);
    QVERIFY(img4 != img2);
    QVERIFY(img4 != img1);

    QIcon icon2;
    icon2.addFile(prefix + "heart.svg");
    QVERIFY(icon2.pixmap(57).toImage() == icon1.pixmap(57).toImage());

    QIcon icon3(prefix + "trash.svg");
    icon3.addFile(prefix + "heart.svg");
    QVERIFY(icon3.pixmap(57).toImage() == icon1.pixmap(57).toImage());

    QIcon icon4(prefix + "heart.svg");
    icon4.addFile(prefix + "image.png", QSize(), QIcon::Active);
    QVERIFY(!icon4.pixmap(32).isNull());
    QVERIFY(!icon4.pixmap(32, QIcon::Active).isNull());
    QVERIFY(icon4.pixmap(32).toImage() == img1);
    QIcon pmIcon(pm);
    QVERIFY(icon4.pixmap(pm.size(), QIcon::Active).toImage() == pmIcon.pixmap(pm.size(), QIcon::Active).toImage());

#ifndef QT_NO_COMPRESS
    QIcon icon5(prefix + "heart.svgz");
    QVERIFY(!icon5.pixmap(32).isNull());
#endif
}

void tst_QIcon_Svg::availableSizes()
{
    {
        // checks that there are no availableSizes for scalable images.
        QIcon icon(prefix + "heart.svg");
        QList<QSize> availableSizes = icon.availableSizes();
        qDebug() << availableSizes;
        QVERIFY(availableSizes.isEmpty());
    }

    {
        // even if an a scalable image contain added pixmaps,
        // availableSizes still should be empty.
        QIcon icon(prefix + "heart.svg");
        icon.addFile(prefix + "image.png", QSize(32,32));
        QList<QSize> availableSizes = icon.availableSizes();
        QVERIFY(availableSizes.isEmpty());
    }
}

void tst_QIcon_Svg::isNull()
{
    {
        //checks that an invalid file results in the icon being null
        QIcon icon(prefix + "nonExistentFile.svg");
        QVERIFY(icon.isNull());
    }
    {
        //valid svg, we're not null
        QIcon icon(prefix + "heart.svg");
        QVERIFY(!icon.isNull());

        // check for non null of serialized/deserialized valid icon
        QByteArray buf;
        QDataStream out(&buf, QIODevice::WriteOnly);
        out << icon;

        QIcon icon2;
        QDataStream in(buf);
        in >> icon2;
        QVERIFY(!icon2.isNull());
    }
    {
        //invalid svg, but a pixmap added means we're not null
        QIcon icon(prefix + "nonExistentFile.svg");
        icon.addFile(prefix + "image.png", QSize(32,32));
        QVERIFY(!icon.isNull());
    }

}

void tst_QIcon_Svg::sizeInPercent()
{
    QIcon icon(prefix + "rect_size_100percent.svg");
    QCOMPARE(icon.actualSize(QSize(16, 8)), QSize(16, 8));
    QCOMPARE(icon.pixmap(QSize(16, 8)).size(), QSize(16, 8));

    QCOMPARE(icon.actualSize(QSize(8, 8)), QSize(8, 4));
    QCOMPARE(icon.pixmap(QSize(8, 8)).size(), QSize(8, 4));
}


QTEST_MAIN(tst_QIcon_Svg)
#include "tst_qicon_svg.moc"
