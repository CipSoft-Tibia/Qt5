// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>
#include <QtQuickShapesDesignHelpers/private/qquickrectangleshape_p.h>
#include <QtQuickTest/quicktest.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

using namespace QQuickVisualTestUtils;

class tst_QQuickRectangleShape : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickRectangleShape();

private slots:
    void initTestCase() override;

    void changeSignals_data();
    void changeSignals();

private:
    // If we load the QML from scratch every test, each row takes ~350ms.
    // However, we still want it to be data-driven (so that e.g. you can
    // run individual rows), so we store it here and create it once at startup
    // rather than loop through a list in changeSignals.
    // TODO: clean this up when we have a callback that is run after all rows
    // have been run for a test function: QTBUG-134198
    std::unique_ptr<QQuickApplicationHelper> changeSignalTestAppHelper;
};

tst_QQuickRectangleShape::tst_QQuickRectangleShape()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQuickRectangleShape::initTestCase()
{
    QQmlDataTest::initTestCase();
    changeSignalTestAppHelper.reset(new QQuickApplicationHelper(this, "default.qml"));
}

void tst_QQuickRectangleShape::changeSignals_data()
{
    QTest::addColumn<QVariant>("propertyValue");
    QTest::addColumn<QMetaMethod>("changeSignal");

    QTest::newRow("radius") << QVariant::fromValue(1)
        << QMetaMethod::fromSignal(&QQuickRectangleShape::radiusChanged);
    QTest::newRow("topLeftRadius") << QVariant::fromValue(1)
        << QMetaMethod::fromSignal(&QQuickRectangleShape::topLeftRadiusChanged);
    QTest::newRow("topRightRadius") << QVariant::fromValue(2)
        << QMetaMethod::fromSignal(&QQuickRectangleShape::topRightRadiusChanged);
    QTest::newRow("bottomLeftRadius") << QVariant::fromValue(3)
        << QMetaMethod::fromSignal(&QQuickRectangleShape::bottomLeftRadiusChanged);
    QTest::newRow("bottomRightRadius") << QVariant::fromValue(4)
        << QMetaMethod::fromSignal(&QQuickRectangleShape::bottomRightRadiusChanged);
    QTest::newRow("bevel") << QVariant::fromValue(true)
        << QMetaMethod::fromSignal(&QQuickRectangleShape::bevelChanged);
    QTest::newRow("topLeftBevel") << QVariant::fromValue(true)
        << QMetaMethod::fromSignal(&QQuickRectangleShape::topLeftBevelChanged);
    QTest::newRow("topRightBevel") << QVariant::fromValue(true)
        << QMetaMethod::fromSignal(&QQuickRectangleShape::topRightBevelChanged);
    QTest::newRow("bottomLeftBevel") << QVariant::fromValue(true)
        << QMetaMethod::fromSignal(&QQuickRectangleShape::bottomLeftBevelChanged);
    QTest::newRow("bottomRightBevel") << QVariant::fromValue(true)
        << QMetaMethod::fromSignal(&QQuickRectangleShape::bottomRightBevelChanged);
    QTest::newRow("strokeColor") << QVariant::fromValue(QColorConstants::Svg::purple)
        << QMetaMethod::fromSignal(&QQuickRectangleShape::strokeColorChanged);
    QTest::newRow("strokeWidth") << QVariant::fromValue(0)
        << QMetaMethod::fromSignal(&QQuickRectangleShape::strokeWidthChanged);
    QTest::newRow("fillColor") << QVariant::fromValue(QColorConstants::Svg::purple)
        << QMetaMethod::fromSignal(&QQuickRectangleShape::fillColorChanged);
    QTest::newRow("joinStyle") << QVariant::fromValue(QQuickShapePath::RoundJoin)
        << QMetaMethod::fromSignal(&QQuickRectangleShape::joinStyleChanged);
    QTest::newRow("capStyle") << QVariant::fromValue(QQuickShapePath::RoundCap)
        << QMetaMethod::fromSignal(&QQuickRectangleShape::capStyleChanged);
    QTest::newRow("strokeStyle") << QVariant::fromValue(QQuickShapePath::DashLine)
        << QMetaMethod::fromSignal(&QQuickRectangleShape::strokeStyleChanged);
    QTest::newRow("dashOffset") << QVariant::fromValue(4)
        << QMetaMethod::fromSignal(&QQuickRectangleShape::dashOffsetChanged);
    QTest::newRow("dashPattern") << QVariant::fromValue(QList<qreal>{1, 2})
        << QMetaMethod::fromSignal(&QQuickRectangleShape::dashPatternChanged);
    QTest::newRow("borderMode") << QVariant::fromValue(QQuickRectangleShape::Outside)
        << QMetaMethod::fromSignal(&QQuickRectangleShape::borderModeChanged);
}

void tst_QQuickRectangleShape::changeSignals()
{
    QFETCH(const QVariant, propertyValue);
    QFETCH(const QMetaMethod, changeSignal);

    QVERIFY2(changeSignalTestAppHelper->ready, changeSignalTestAppHelper->failureMessage());
    QQuickWindow *window = changeSignalTestAppHelper->window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *rectangleShape = window->property("rectangleShape").value<QQuickRectangleShape *>();
    QVERIFY(rectangleShape);
    const QSignalSpy signalSpy(rectangleShape, changeSignal);
    QVERIFY(signalSpy.isValid());
    QVERIFY(rectangleShape->setProperty(QTest::currentDataTag(), propertyValue));
    QCOMPARE(signalSpy.count(), 1);
}

QTEST_MAIN(tst_QQuickRectangleShape)

#include "tst_qquickrectangleshape.moc"
