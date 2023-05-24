// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "private/bmlayer_p.h"
#include "private/bmellipse_p.h"

class tst_BMEllipse: public QObject
{
    Q_OBJECT

public:
    tst_BMEllipse();
    ~tst_BMEllipse();

private:

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testStaticInitialX();
    void testStaticInitialY();
    void testStaticInitialWidth();
    void testStaticInitialHeight();
    void testStaticUpdatedX();
    void testStaticUpdatedY();
    void testStaticUpdatedWidth();
    void testStaticUpdatedHeight();

    void testAnimatedInitialX();
    void testAnimatedInitialY();
    void testAnimatedInitialWidth();
    void testAnimatedInitialHeight();
    void testAnimatedUpdatedX();
    void testAnimatedUpdatedY();
    void testAnimatedUpdatedWidth();
    void testAnimatedUpdatedHeight();

    void testName();
    void testType();
    void testActive();
    void testHidden();
    void testDirection();

private:
    void loadTestData(const QByteArray &filename);
    void updateProperty(int frame);

    BMEllipse *m_ellipse = nullptr;
};

tst_BMEllipse::tst_BMEllipse()
{

}

tst_BMEllipse::~tst_BMEllipse()
{

}

void tst_BMEllipse::initTestCase()
{
}

void tst_BMEllipse::cleanupTestCase()
{
    if (m_ellipse)
        delete m_ellipse;
}

void tst_BMEllipse::testStaticInitialX()
{
    loadTestData("ellipse_static_100x80.json");

    QVERIFY(qFuzzyCompare(m_ellipse->position().x(), 0.0));
}

void tst_BMEllipse:: testStaticInitialY()
{
    loadTestData("ellipse_static_100x80.json");

    QVERIFY(qFuzzyCompare(m_ellipse->position().y(), 0.0));
}

void tst_BMEllipse:: testStaticInitialWidth()
{
    loadTestData("ellipse_static_100x80.json");

    QVERIFY(qFuzzyCompare(m_ellipse->size().width(), 100));
}

void tst_BMEllipse:: testStaticInitialHeight()
{
    loadTestData("ellipse_static_100x80.json");

    QVERIFY(qFuzzyCompare(m_ellipse->size().height(), 80));
}

void tst_BMEllipse::testStaticUpdatedX()
{
    loadTestData("ellipse_static_100x80.json");
    updateProperty(179);

    QVERIFY(qFuzzyCompare(m_ellipse->position().x(), 0.0));
}

void tst_BMEllipse:: testStaticUpdatedY()
{
    loadTestData("ellipse_static_100x80.json");
    updateProperty(179);

    QVERIFY(qFuzzyCompare(m_ellipse->position().y(), 0.0));
}

void tst_BMEllipse:: testStaticUpdatedWidth()
{
    loadTestData("ellipse_static_100x80.json");
    updateProperty(179);

    QVERIFY(qFuzzyCompare(m_ellipse->size().width(), 100));
}

void tst_BMEllipse:: testStaticUpdatedHeight()
{
    loadTestData("ellipse_static_100x80.json");
    updateProperty(179);

    QVERIFY(qFuzzyCompare(m_ellipse->size().height(), 80));
}

void tst_BMEllipse::testAnimatedInitialX()
{
    loadTestData("ellipse_animated_100x80at00to200x40at50100.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_ellipse->position().x(), 0.0));
}

void tst_BMEllipse::testAnimatedInitialY()
{
    loadTestData("ellipse_animated_100x80at00to200x40at50100.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_ellipse->position().y(), 0.0));
}

void tst_BMEllipse::testAnimatedInitialWidth()
{
    loadTestData("ellipse_animated_100x80at00to200x40at50100.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_ellipse->size().width(), 100.0));
}

void tst_BMEllipse::testAnimatedInitialHeight()
{
    loadTestData("ellipse_animated_100x80at00to200x40at50100.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_ellipse->size().height(), 80.0));
}

void tst_BMEllipse::testAnimatedUpdatedX()
{
    loadTestData("ellipse_animated_100x80at00to200x40at50100.json");
    updateProperty(179);

    QVERIFY(qFuzzyCompare(m_ellipse->position().x(), 50.0));
}

void tst_BMEllipse::testAnimatedUpdatedY()
{
    loadTestData("ellipse_animated_100x80at00to200x40at50100.json");
    updateProperty(179);

    QVERIFY(qFuzzyCompare(m_ellipse->position().y(), 100.0));
}

void tst_BMEllipse::testAnimatedUpdatedWidth()
{
    loadTestData("ellipse_animated_100x80at00to200x40at50100.json");
    updateProperty(179);

    QVERIFY(qFuzzyCompare(m_ellipse->size().width(), 200.0));
}

void tst_BMEllipse::testAnimatedUpdatedHeight()
{
    loadTestData("ellipse_animated_100x80at00to200x40at50100.json");
    updateProperty(179);

    QVERIFY(qFuzzyCompare(m_ellipse->size().height(), 40.0));
}

void tst_BMEllipse::testName()
{
    loadTestData("ellipse_static_100x80.json");
    QVERIFY(m_ellipse->name() == QString("Ellipse Path 1"));
}

void tst_BMEllipse::testType()
{
    loadTestData("ellipse_static_100x80.json");
    QVERIFY(m_ellipse->type() == BM_SHAPE_ELLIPSE_IX);
}

void tst_BMEllipse::testActive()
{
    loadTestData("ellipse_static_100x80.json");
    QVERIFY(m_ellipse->active(100) == true);

    loadTestData("ellipse_hidden.json");
    QVERIFY(m_ellipse->active(100) == false);
}

void tst_BMEllipse::testHidden()
{
    loadTestData("ellipse_hidden.json");
    QVERIFY(m_ellipse->hidden() == true);
}

void tst_BMEllipse::testDirection()
{
    loadTestData("ellipse_hidden.json");
    QVERIFY(m_ellipse->direction() == 0);
    loadTestData("ellipse_direction.json");
    QVERIFY(m_ellipse->direction() == 0);
}

void tst_BMEllipse::loadTestData(const QByteArray &filename)
{
    if (m_ellipse) {
        delete m_ellipse;
        m_ellipse = nullptr;
    }

    QFile sourceFile(QFINDTESTDATA(QLatin1String("data/") + filename));
    if (!sourceFile.exists())
        QFAIL("File does not exist");
    if (!sourceFile.open(QIODevice::ReadOnly))
        QFAIL("Cannot read test file");

    QByteArray json = sourceFile.readAll();

    sourceFile.close();

    QJsonDocument doc = QJsonDocument::fromJson(json);
    QJsonObject rootObj = doc.object();
    if (rootObj.empty())
        QFAIL("Cannot parse test file");

    QStringList vs = rootObj.value(QLatin1String("v")).toString().split(u'.');
    QList<int> vi;
    foreach (QString v, vs)
        vi.append(v.toInt());
    QVersionNumber version = QVersionNumber(vi);

    QJsonArray layers = rootObj.value(QLatin1String("layers")).toArray();
    QJsonObject layerObj = layers[0].toObject();
    int type = layerObj.value(QLatin1String("ty")).toInt();
    if (type != 4)
        QFAIL("It's not shape layer");

    QJsonArray shapes = layerObj.value(QLatin1String("shapes")).toArray();
    QJsonArray::const_iterator shapesIt = shapes.constBegin();
    BMShape* shape = nullptr;
    while (shapesIt != shapes.end()) {
        QJsonObject childObj = (*shapesIt).toObject();
        shape = BMShape::construct(childObj, version);
        QVERIFY(shape != nullptr);
        if (shape->type() == BM_SHAPE_ELLIPSE_IX)
            break;
        shapesIt++;
    }

    m_ellipse = static_cast<BMEllipse*>(shape);

    QVERIFY(m_ellipse != nullptr);
}

void tst_BMEllipse::updateProperty(int frame)
{
    m_ellipse->updateProperties(frame);
}

QTEST_MAIN(tst_BMEllipse)
#include "tst_bmellipse.moc"
