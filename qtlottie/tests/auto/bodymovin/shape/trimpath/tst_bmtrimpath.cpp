// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "private/bmlayer_p.h"
#include "private/bmtrimpath_p.h"

class tst_BMTrimPath: public QObject
{
    Q_OBJECT

public:
    tst_BMTrimPath();
    ~tst_BMTrimPath();

private:

    //    void testParseStaticRect();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testStaticInitialStart();
    void testStaticInitialEnd();
    void testStaticInitialOffset();
    void testStaticInitialSimultaneous();
    void testStaticUpdatedStart();
    void testStaticUpdatedEnd();
    void testStaticUpdatedOffset();
    void testStaticUpdatedSimultaneous();

    void testAnimatedInitialStart();
    void testAnimatedInitialEnd();
    void testAnimatedInitialOffset();
    void testAnimatedInitialSimultaneous();
    void testAnimatedUpdatedStart();
    void testAnimatedUpdatedEnd();
    void testAnimatedUpdatedOffset();
    void testAnimatedUpdatedSimultaneous();

    void testName();
    void testType();
    void testHidden();
    void testActive();

private:
    void loadTestData(const QByteArray &filename);
    void updateProperty(int frame);

    BMTrimPath *m_trimpath = nullptr;
};

tst_BMTrimPath::tst_BMTrimPath()
{

}

tst_BMTrimPath::~tst_BMTrimPath()
{

}

void tst_BMTrimPath::initTestCase()
{
}

void tst_BMTrimPath::cleanupTestCase()
{
    if (m_trimpath)
        delete m_trimpath;
}

void tst_BMTrimPath::testStaticInitialStart()
{
    loadTestData("trimpath_static_20to80.json");

    QVERIFY(qFuzzyCompare(m_trimpath->start(), 20.0));
}

void tst_BMTrimPath::testStaticInitialEnd()
{
    loadTestData("trimpath_static_20to80.json");

    QVERIFY(qFuzzyCompare(m_trimpath->end(), 80.0));
}

void tst_BMTrimPath::testStaticInitialOffset()
{
    loadTestData("trimpath_static_20to80.json");

    QVERIFY(qFuzzyCompare(m_trimpath->offset(), 0.0));
}

void tst_BMTrimPath::testStaticInitialSimultaneous()
{
    loadTestData("trimpath_static_20to80.json");

    QVERIFY(m_trimpath->simultaneous() == true);
}

void tst_BMTrimPath::testStaticUpdatedStart()
{
    loadTestData("trimpath_static_20to80.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_trimpath->start(), 20.0));
}

void tst_BMTrimPath::testStaticUpdatedEnd()
{
    loadTestData("trimpath_static_20to80.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_trimpath->end(), 80.0));
}

void tst_BMTrimPath::testStaticUpdatedOffset()
{
    loadTestData("trimpath_static_20to80.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_trimpath->offset(), 0.0));
}

void tst_BMTrimPath::testStaticUpdatedSimultaneous()
{
    loadTestData("trimpath_static_20to80.json");
    updateProperty(180);

    QVERIFY(m_trimpath->simultaneous() == true);
}

void tst_BMTrimPath::testAnimatedInitialStart()
{
    loadTestData("trimpath_animated_2080_0_to_0060_3x30.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_trimpath->start(), 20.0));
}

void tst_BMTrimPath::testAnimatedInitialEnd()
{
    loadTestData("trimpath_animated_2080_0_to_0060_3x30.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_trimpath->end(), 80.0));
}

void tst_BMTrimPath::testAnimatedInitialOffset()
{
    loadTestData("trimpath_animated_2080_0_to_0060_3x30.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_trimpath->offset(), 0.0));
}

void tst_BMTrimPath::testAnimatedInitialSimultaneous()
{
    loadTestData("trimpath_animated_2080_0_to_0060_3x30.json");
    updateProperty(0);

    QVERIFY(m_trimpath->simultaneous() == false);
}

void tst_BMTrimPath::testAnimatedUpdatedStart()
{
    loadTestData("trimpath_animated_2080_0_to_0060_3x30.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_trimpath->start(), 0.0));
}

void tst_BMTrimPath::testAnimatedUpdatedEnd()
{
    loadTestData("trimpath_animated_2080_0_to_0060_3x30.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_trimpath->end(), 60.0));
}

void tst_BMTrimPath::testAnimatedUpdatedOffset()
{
    loadTestData("trimpath_animated_2080_0_to_0060_3x30.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_trimpath->offset(), (360 * 3 + 30.0)));
}

void tst_BMTrimPath::testAnimatedUpdatedSimultaneous()
{
    loadTestData("trimpath_animated_2080_0_to_0060_3x30.json");
    updateProperty(180);

    QVERIFY(m_trimpath->simultaneous() == false);
}

void tst_BMTrimPath::testName()
{
    loadTestData("trimpath_hidden.json");

    QVERIFY(m_trimpath->name() == QString("Trim Paths 1"));
}

void tst_BMTrimPath::testType()
{
    loadTestData("trimpath_hidden.json");

    QVERIFY(m_trimpath->type() == BM_SHAPE_TRIM_IX);
}

void tst_BMTrimPath::testHidden()
{
    loadTestData("trimpath_hidden.json");

    QVERIFY(m_trimpath->hidden() == true);
}

void tst_BMTrimPath::testActive()
{
    loadTestData("trimpath_animated_2080_0_to_0060_3x30.json");
    QVERIFY(m_trimpath->active(100) == true);

    loadTestData("trimpath_hidden.json");
    QVERIFY(m_trimpath->active(100) == false);
}

void tst_BMTrimPath::loadTestData(const QByteArray &filename)
{
    if (m_trimpath) {
        delete m_trimpath;
        m_trimpath = nullptr;
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
        if (shape->type() == BM_SHAPE_TRIM_IX)
            break;
        shapesIt++;
    }

    m_trimpath = static_cast<BMTrimPath*>(shape);

    QVERIFY(m_trimpath != nullptr);
}

void tst_BMTrimPath::updateProperty(int frame)
{
    m_trimpath->updateProperties(frame);
}

QTEST_MAIN(tst_BMTrimPath)
#include "tst_bmtrimpath.moc"
