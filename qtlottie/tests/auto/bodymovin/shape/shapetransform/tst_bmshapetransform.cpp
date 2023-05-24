// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "private/bmlayer_p.h"
#include "private/bmgroup_p.h"
#include "private/bmshapetransform_p.h"

class tst_BMShapeTransform: public QObject
{
    Q_OBJECT

public:
    tst_BMShapeTransform();
    ~tst_BMShapeTransform();

private:

    //    void testParseStaticRect();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testStaticInitialAnchorX();
    void testStaticInitialAnchorY();
    void testStaticInitialPositionX();
    void testStaticInitialPositionY();
    void testStaticInitialScaleX();
    void testStaticInitialScaleY();
    void testStaticInitialRotation();
    void testStaticInitialOpacity();
    void testStaticInitialSkew();
    void testStaticInitialSkewAxis();
    void testStaticUpdatedAnchorX();
    void testStaticUpdatedAnchorY();
    void testStaticUpdatedPositionX();
    void testStaticUpdatedPositionY();
    void testStaticUpdatedScaleX();
    void testStaticUpdatedScaleY();
    void testStaticUpdatedRotation();
    void testStaticUpdatedOpacity();
    void testStaticUpdatedSkew();
    void testStaticUpdatedSkewAxis();


    void testAnimatedInitialAnchorX();
    void testAnimatedInitialAnchorY();
    void testAnimatedInitialPositionX();
    void testAnimatedInitialPositionY();
    void testAnimatedInitialScaleX();
    void testAnimatedInitialScaleY();
    void testAnimatedInitialRotation();
    void testAnimatedInitialOpacity();
    void testAnimatedInitialSkew();
    void testAnimatedInitialSkewAxis();
    void testAnimatedUpdatedAnchorX();
    void testAnimatedUpdatedAnchorY();
    void testAnimatedUpdatedPositionX();
    void testAnimatedUpdatedPositionY();
    void testAnimatedUpdatedScaleX();
    void testAnimatedUpdatedScaleY();
    void testAnimatedUpdatedRotation();
    void testAnimatedUpdatedOpacity();
    void testAnimatedUpdatedSkew();
    void testAnimatedUpdatedSkewAxis();

    void testName();
    void testType();
    void testActive();

private:
    void loadTestData(const QByteArray &filename);
    void updateProperty(int frame);

    BMShapeTransform *m_transform = nullptr;
};

tst_BMShapeTransform::tst_BMShapeTransform()
{

}

tst_BMShapeTransform::~tst_BMShapeTransform()
{

}

void tst_BMShapeTransform::initTestCase()
{
}

void tst_BMShapeTransform::cleanupTestCase()
{
    if (m_transform)
        delete m_transform;
}

void tst_BMShapeTransform::testStaticInitialAnchorX()
{
    loadTestData("shapetransform_static.json");

    QVERIFY(qFuzzyCompare(m_transform->anchorPoint().x(), 50.0));
}

void tst_BMShapeTransform::testStaticInitialAnchorY()
{
    loadTestData("shapetransform_static.json");

    QVERIFY(qFuzzyCompare(m_transform->anchorPoint().y(), 50.0));
}

void tst_BMShapeTransform::testStaticInitialPositionX()
{
    loadTestData("shapetransform_static.json");

    QVERIFY(qFuzzyCompare(m_transform->position().x(), 50.0));
}

void tst_BMShapeTransform::testStaticInitialPositionY()
{
    loadTestData("shapetransform_static.json");

    QVERIFY(qFuzzyCompare(m_transform->position().y(), 50.0));
}

void tst_BMShapeTransform::testStaticInitialScaleX()
{
    loadTestData("shapetransform_static.json");

    QVERIFY(qFuzzyCompare(m_transform->scale().x(), 0.5));
}

void tst_BMShapeTransform::testStaticInitialScaleY()
{
    loadTestData("shapetransform_static.json");

    QVERIFY(qFuzzyCompare(m_transform->scale().y(), 0.5));
}

void tst_BMShapeTransform::testStaticInitialRotation()
{
    loadTestData("shapetransform_static.json");

    QVERIFY(qFuzzyCompare(m_transform->rotation(), 0.0));
}

void tst_BMShapeTransform::testStaticInitialOpacity()
{
    loadTestData("shapetransform_static.json");

    QVERIFY(qFuzzyCompare(m_transform->opacity(), 1.0));
}

void tst_BMShapeTransform::testStaticInitialSkew()
{
    loadTestData("shapetransform_static.json");

    QVERIFY(qFuzzyCompare(m_transform->skew(), 0.0));
}

void tst_BMShapeTransform::testStaticInitialSkewAxis()
{
    loadTestData("shapetransform_static.json");

    QVERIFY(qFuzzyCompare(m_transform->skewAxis(), 0.0));
}

void tst_BMShapeTransform::testStaticUpdatedAnchorX()
{
    loadTestData("shapetransform_static.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->anchorPoint().x(), 50.0));
}

void tst_BMShapeTransform::testStaticUpdatedAnchorY()
{
    loadTestData("shapetransform_static.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->anchorPoint().y(), 50.0));
}

void tst_BMShapeTransform::testStaticUpdatedPositionX()
{
    loadTestData("shapetransform_static.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->position().x(), 50.0));
}

void tst_BMShapeTransform::testStaticUpdatedPositionY()
{
    loadTestData("shapetransform_static.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->position().y(), 50.0));
}

void tst_BMShapeTransform::testStaticUpdatedScaleX()
{
    loadTestData("shapetransform_static.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->scale().x(), 0.5));
}

void tst_BMShapeTransform::testStaticUpdatedScaleY()
{
    loadTestData("shapetransform_static.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->scale().y(), 0.5));
}

void tst_BMShapeTransform::testStaticUpdatedRotation()
{
    loadTestData("shapetransform_static.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->rotation(), 0.0));
}

void tst_BMShapeTransform::testStaticUpdatedOpacity()
{
    loadTestData("shapetransform_static.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->opacity(), 1.0));
}

void tst_BMShapeTransform::testStaticUpdatedSkew()
{
    loadTestData("shapetransform_static.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->skew(), 0.0));
}

void tst_BMShapeTransform::testStaticUpdatedSkewAxis()
{
    loadTestData("shapetransform_static.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->skewAxis(), 0.0));
}

void tst_BMShapeTransform::testAnimatedInitialAnchorX()
{
    loadTestData("shapetransform_animated.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_transform->anchorPoint().x(), 50.0));
}

void tst_BMShapeTransform::testAnimatedInitialAnchorY()
{
    loadTestData("shapetransform_animated.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_transform->anchorPoint().y(), 50.0));
}

void tst_BMShapeTransform::testAnimatedInitialPositionX()
{
    loadTestData("shapetransform_animated.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_transform->position().x(), 50.0));
}

void tst_BMShapeTransform::testAnimatedInitialPositionY()
{
    loadTestData("shapetransform_animated.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_transform->position().y(), 50.0));
}

void tst_BMShapeTransform::testAnimatedInitialScaleX()
{
    loadTestData("shapetransform_animated.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_transform->scale().x(), 0.5));
}

void tst_BMShapeTransform::testAnimatedInitialScaleY()
{
    loadTestData("shapetransform_animated.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_transform->scale().y(), 0.5));
}

void tst_BMShapeTransform::testAnimatedInitialRotation()
{
    loadTestData("shapetransform_animated.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_transform->rotation(), 0.0));
}

void tst_BMShapeTransform::testAnimatedInitialOpacity()
{
    loadTestData("shapetransform_animated.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_transform->opacity(), 1.0));
}

void tst_BMShapeTransform::testAnimatedInitialSkew()
{
    loadTestData("shapetransform_animated.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_transform->skew(), 0.0));
}

void tst_BMShapeTransform::testAnimatedInitialSkewAxis()
{
    loadTestData("shapetransform_animated.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_transform->skewAxis(), 0.0));
}

void tst_BMShapeTransform::testAnimatedUpdatedAnchorX()
{
    loadTestData("shapetransform_animated.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->anchorPoint().x(), 25.0));
}

void tst_BMShapeTransform::testAnimatedUpdatedAnchorY()
{
    loadTestData("shapetransform_animated.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->anchorPoint().y(), 25.0));
}

void tst_BMShapeTransform::testAnimatedUpdatedPositionX()
{
    loadTestData("shapetransform_animated.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->position().x(), 75.0));
}

void tst_BMShapeTransform::testAnimatedUpdatedPositionY()
{
    loadTestData("shapetransform_animated.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->position().y(), 75.0));
}

void tst_BMShapeTransform::testAnimatedUpdatedScaleX()
{
    loadTestData("shapetransform_animated.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->scale().x(), 1.0));
}

void tst_BMShapeTransform::testAnimatedUpdatedScaleY()
{
    loadTestData("shapetransform_animated.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->scale().y(), 1.0));
}

void tst_BMShapeTransform::testAnimatedUpdatedRotation()
{
    loadTestData("shapetransform_animated.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->rotation(), (3 * 360 + 30.0)));
}

void tst_BMShapeTransform::testAnimatedUpdatedOpacity()
{
    loadTestData("shapetransform_animated.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->opacity(), 0.25));
}

void tst_BMShapeTransform::testAnimatedUpdatedSkew()
{
    loadTestData("shapetransform_animated.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->skew(), 4.0));
}

void tst_BMShapeTransform::testAnimatedUpdatedSkewAxis()
{
    loadTestData("shapetransform_animated.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->skewAxis(), (2 * 360 + 25.0)));
}

void tst_BMShapeTransform::testName()
{
    loadTestData("shapetransform_static.json");
    QVERIFY(m_transform->name() == QString("Transform"));
}

void tst_BMShapeTransform::testType()
{
    loadTestData("shapetransform_static.json");
    QVERIFY(m_transform->type() == BM_SHAPE_TRANS_IX);
}

void tst_BMShapeTransform::testActive()
{
    loadTestData("shapetransform_static.json");
    QVERIFY(m_transform->active(100) == true);
}

void tst_BMShapeTransform::loadTestData(const QByteArray &filename)
{
    if (m_transform) {
        delete m_transform;
        m_transform = nullptr;
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

    QJsonArray layers = rootObj.value(QLatin1String("layers")).toArray();
    QJsonObject layerObj = layers[0].toObject();
    int type = layerObj.value(QLatin1String("ty")).toInt();
    if (type != 4)
        QFAIL("It's not shape layer");

    QStringList vs = rootObj.value(QLatin1String("v")).toString().split(u'.');
    QList<int> vi;
    foreach (QString v, vs)
        vi.append(v.toInt());
    QVersionNumber version = QVersionNumber(vi);

    QJsonArray shapes = layerObj.value(QLatin1String("shapes")).toArray();
    QJsonArray::const_iterator shapesIt = shapes.constBegin();
    BMGroup* group = nullptr;
    while (shapesIt != shapes.end()) {
        QJsonObject childObj = (*shapesIt).toObject();
        BMShape *shape = BMShape::construct(childObj, version);
        QVERIFY(shape != nullptr);
        if (shape->type() == BM_SHAPE_GROUP_IX) {
            group = static_cast<BMGroup *>(shape);
            break;
        }
        shapesIt++;
    }
    QVERIFY(group != nullptr);

    m_transform = static_cast<BMShapeTransform*>(group->findChild("Transform"));

    QVERIFY(m_transform != nullptr);
}

void tst_BMShapeTransform::updateProperty(int frame)
{
    m_transform->updateProperties(frame);
}

QTEST_MAIN(tst_BMShapeTransform)
#include "tst_bmshapetransform.moc"
