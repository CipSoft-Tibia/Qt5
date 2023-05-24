// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "private/bmlayer_p.h"
#include "private/bmbasictransform_p.h"

class tst_BMBasicTransform: public QObject
{
    Q_OBJECT

public:
    tst_BMBasicTransform();
    ~tst_BMBasicTransform();

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
    void testStaticUpdatedAnchorX();
    void testStaticUpdatedAnchorY();
    void testStaticUpdatedPositionX();
    void testStaticUpdatedPositionY();
    void testStaticUpdatedScaleX();
    void testStaticUpdatedScaleY();
    void testStaticUpdatedRotation();
    void testStaticUpdatedOpacity();

    void testAnimatedInitialAnchorX();
    void testAnimatedInitialAnchorY();
    void testAnimatedInitialPositionX();
    void testAnimatedInitialPositionY();
    void testAnimatedInitialScaleX();
    void testAnimatedInitialScaleY();
    void testAnimatedInitialRotation();
    void testAnimatedInitialOpacity();
    void testAnimatedUpdatedAnchorX();
    void testAnimatedUpdatedAnchorY();
    void testAnimatedUpdatedPositionX();
    void testAnimatedUpdatedPositionY();
    void testAnimatedUpdatedScaleX();
    void testAnimatedUpdatedScaleY();
    void testAnimatedUpdatedRotation();
    void testAnimatedUpdatedOpacity();

    void testActive();

private:
    void loadTestData(const QByteArray &filename);
    void updateProperty(int frame);

    BMBasicTransform *m_transform = nullptr;
};

tst_BMBasicTransform::tst_BMBasicTransform()
{

}

tst_BMBasicTransform::~tst_BMBasicTransform()
{

}

void tst_BMBasicTransform::initTestCase()
{
}

void tst_BMBasicTransform::cleanupTestCase()
{
    if (m_transform)
        delete m_transform;
}

void tst_BMBasicTransform::testStaticInitialAnchorX()
{
    loadTestData("transform_static.json");

    QVERIFY(qFuzzyCompare(m_transform->anchorPoint().x(), 50.0));
}

void tst_BMBasicTransform::testStaticInitialAnchorY()
{
    loadTestData("transform_static.json");

    QVERIFY(qFuzzyCompare(m_transform->anchorPoint().y(), 50.0));
}

void tst_BMBasicTransform::testStaticInitialPositionX()
{
    loadTestData("transform_static.json");

    QVERIFY(qFuzzyCompare(m_transform->position().x(), 50.0));
}

void tst_BMBasicTransform::testStaticInitialPositionY()
{
    loadTestData("transform_static.json");

    QVERIFY(qFuzzyCompare(m_transform->position().y(), 50.0));
}

void tst_BMBasicTransform::testStaticInitialScaleX()
{
    loadTestData("transform_static.json");

    QVERIFY(qFuzzyCompare(m_transform->scale().x(), 0.5));
}

void tst_BMBasicTransform::testStaticInitialScaleY()
{
    loadTestData("transform_static.json");

    QVERIFY(qFuzzyCompare(m_transform->scale().y(), 0.5));
}

void tst_BMBasicTransform::testStaticInitialRotation()
{
    loadTestData("transform_static.json");

    QVERIFY(qFuzzyCompare(m_transform->rotation(), 0.0));
}

void tst_BMBasicTransform::testStaticInitialOpacity()
{
    loadTestData("transform_static.json");

    QVERIFY(qFuzzyCompare(m_transform->opacity(), 1.0));
}

void tst_BMBasicTransform::testStaticUpdatedAnchorX()
{
    loadTestData("transform_static.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->anchorPoint().x(), 50.0));
}

void tst_BMBasicTransform::testStaticUpdatedAnchorY()
{
    loadTestData("transform_static.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->anchorPoint().y(), 50.0));
}

void tst_BMBasicTransform::testStaticUpdatedPositionX()
{
    loadTestData("transform_static.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->position().x(), 50.0));
}

void tst_BMBasicTransform::testStaticUpdatedPositionY()
{
    loadTestData("transform_static.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->position().y(), 50.0));
}

void tst_BMBasicTransform::testStaticUpdatedScaleX()
{
    loadTestData("transform_static.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->scale().x(), 0.5));
}

void tst_BMBasicTransform::testStaticUpdatedScaleY()
{
    loadTestData("transform_static.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->scale().y(), 0.5));
}

void tst_BMBasicTransform::testStaticUpdatedRotation()
{
    loadTestData("transform_static.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->rotation(), 0.0));
}

void tst_BMBasicTransform::testStaticUpdatedOpacity()
{
    loadTestData("transform_static.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->opacity(), 1.0));
}

void tst_BMBasicTransform::testAnimatedInitialAnchorX()
{
    loadTestData("transform_animated.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_transform->anchorPoint().x(), 50.0));
}

void tst_BMBasicTransform::testAnimatedInitialAnchorY()
{
    loadTestData("transform_animated.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_transform->anchorPoint().y(), 50.0));
}

void tst_BMBasicTransform::testAnimatedInitialPositionX()
{
    loadTestData("transform_animated.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_transform->position().x(), 50.0));
}

void tst_BMBasicTransform::testAnimatedInitialPositionY()
{
    loadTestData("transform_animated.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_transform->position().y(), 50.0));
}

void tst_BMBasicTransform::testAnimatedInitialScaleX()
{
    loadTestData("transform_animated.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_transform->scale().x(), 0.5));
}

void tst_BMBasicTransform::testAnimatedInitialScaleY()
{
    loadTestData("transform_animated.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_transform->scale().y(), 0.5));
}

void tst_BMBasicTransform::testAnimatedInitialRotation()
{
    loadTestData("transform_animated.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_transform->rotation(), 0.0));
}

void tst_BMBasicTransform::testAnimatedInitialOpacity()
{
    loadTestData("transform_animated.json");
    updateProperty(0);

    QVERIFY(qFuzzyCompare(m_transform->opacity(), 1.0));
}

void tst_BMBasicTransform::testAnimatedUpdatedAnchorX()
{
    loadTestData("transform_animated.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->anchorPoint().x(), 25.0));
}

void tst_BMBasicTransform::testAnimatedUpdatedAnchorY()
{
    loadTestData("transform_animated.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->anchorPoint().y(), 25.0));
}

void tst_BMBasicTransform::testAnimatedUpdatedPositionX()
{
    loadTestData("transform_animated.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->position().x(), 75.0));
}

void tst_BMBasicTransform::testAnimatedUpdatedPositionY()
{
    loadTestData("transform_animated.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->position().y(), 75.0));
}

void tst_BMBasicTransform::testAnimatedUpdatedScaleX()
{
    loadTestData("transform_animated.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->scale().x(), 1.0));
}

void tst_BMBasicTransform::testAnimatedUpdatedScaleY()
{
    loadTestData("transform_animated.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->scale().y(), 1.0));
}

void tst_BMBasicTransform::testAnimatedUpdatedRotation()
{
    loadTestData("transform_animated.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->rotation(), (3 * 360 + 30.0)));
}

void tst_BMBasicTransform::testAnimatedUpdatedOpacity()
{
    loadTestData("transform_animated.json");
    updateProperty(180);

    QVERIFY(qFuzzyCompare(m_transform->opacity(), 0.25));
}

void tst_BMBasicTransform::testActive()
{
    loadTestData("transform_static.json");
    QVERIFY(m_transform->active(100) == true);
}

void tst_BMBasicTransform::loadTestData(const QByteArray &filename)
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

    QJsonObject transformObj = layerObj.value(QLatin1String("ks")).toObject();
    m_transform = new BMBasicTransform(transformObj, version);

    QVERIFY(m_transform != nullptr);
}

void tst_BMBasicTransform::updateProperty(int frame)
{
    m_transform->updateProperties(frame);
}

QTEST_MAIN(tst_BMBasicTransform)
#include "tst_bmbasictransform.moc"
