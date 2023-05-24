// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "private/bmlayer_p.h"
#include "private/bmfill_p.h"

class tst_BMFill: public QObject
{
    Q_OBJECT

public:
    tst_BMFill();
    ~tst_BMFill();

private:

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testStaticInitialColor();
    void testStaticeInitialOpacity();
    void testStaticUpdatedColor();
    void testStaticeUpdatedOpacity();

    void testAnimatedInitialColor();
    void testAnimatedInitialOpacity();
    void testAnimatedUpdatedColor();
    void testAnimatedUpdatedOpacity();

    void testName();
    void testType();
    void testActive();
    void testHidden();

private:
    void loadTestData(const QByteArray &filename);
    void updateProperty(int frame);

    BMFill *m_fill = nullptr;
};

tst_BMFill::tst_BMFill()
{

}

tst_BMFill::~tst_BMFill()
{

}

void tst_BMFill::initTestCase()
{
}

void tst_BMFill::cleanupTestCase()
{
    if (m_fill)
        delete m_fill;
}

void tst_BMFill::testStaticInitialColor()
{
    loadTestData("fill_static_red_100.json");

    QVERIFY(m_fill->color() == QColor(Qt::red));
}

void tst_BMFill:: testStaticeInitialOpacity()
{
    loadTestData("fill_static_red_100.json");

    QVERIFY(qFuzzyCompare(m_fill->opacity(), 100.0));
}

void tst_BMFill::testStaticUpdatedColor()
{
    loadTestData("fill_static_red_100.json");
    updateProperty(179);

    QVERIFY(m_fill->color() == QColor(Qt::red));
}

void tst_BMFill:: testStaticeUpdatedOpacity()
{
    loadTestData("fill_static_red_100.json");
    updateProperty(179);

    QVERIFY(qFuzzyCompare(m_fill->opacity(), 100.0));
}

void tst_BMFill::testAnimatedInitialColor()
{
    loadTestData("fill_animated_red100_green0.json");
    updateProperty(0);

    QVERIFY(m_fill->color() == QColor(Qt::red));
}

void tst_BMFill::testAnimatedInitialOpacity()
{
    loadTestData("fill_animated_red100_green0.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_fill->opacity(), 100.0));
}

void tst_BMFill::testAnimatedUpdatedColor()
{
    loadTestData("fill_animated_red100_green0.json");
    updateProperty(179);

    QVERIFY(m_fill->color() == QColor(Qt::green));
}

void tst_BMFill::testAnimatedUpdatedOpacity()
{
    loadTestData("fill_animated_red100_green0.json");
    updateProperty(179);

    QVERIFY(qFuzzyCompare(m_fill->opacity(), 0.0));
}

void tst_BMFill::testName()
{
    loadTestData("fill_static_red_100.json");
    QVERIFY(m_fill->name() == QString("Fill 1"));
}

void tst_BMFill::testType()
{
    loadTestData("fill_static_red_100.json");
    QVERIFY(m_fill->type() == BM_SHAPE_FILL_IX);
}

void tst_BMFill::testActive()
{
    loadTestData("fill_static_red_100.json");
    QVERIFY(m_fill->active(100) == true);

    loadTestData("fill_hidden.json");
    QVERIFY(m_fill->active(100) == false);
}

void tst_BMFill::testHidden()
{
    loadTestData("fill_hidden.json");
    QVERIFY(m_fill->hidden() == true);
}

void tst_BMFill::loadTestData(const QByteArray &filename)
{
    if (m_fill) {
        delete m_fill;
        m_fill = nullptr;
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
        if (shape->type() == BM_SHAPE_FILL_IX)
            break;
        shapesIt++;
    }

    m_fill = static_cast<BMFill*>(shape);

    QVERIFY(m_fill != nullptr);
}

void tst_BMFill::updateProperty(int frame)
{
    m_fill->updateProperties(frame);
}

QTEST_MAIN(tst_BMFill)
#include "tst_bmfill.moc"
