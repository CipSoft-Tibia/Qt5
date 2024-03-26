// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QTest>
#include <Qt3DAnimation/private/bezierevaluator_p.h>
#include <Qt3DAnimation/private/keyframe_p.h>
#include <QtCore/qlist.h>

#include <cmath>

Q_DECLARE_METATYPE(Qt3DAnimation::Animation::Keyframe)

using namespace Qt3DAnimation;
using namespace Qt3DAnimation::Animation;

static const QList<float> globalTimes = { 0.0f,      1.00375f, 2.48f,
                                          4.37625f,  6.64f,    9.21875f,
                                         12.06f,    15.11125f, 18.32f,
                                         21.63375f, 25.0f,     28.36625f,
                                         31.68f,    34.88875f, 37.94f,
                                         40.78125f, 43.36f,    45.62375f,
                                         47.52f,    48.99625f, 50.0f };

static const QList<float> globalValues = { 0.0f,     0.03625f, 0.14f,
                                           0.30375f, 0.52f,    0.78125f,
                                           1.08f,    1.40875f, 1.76f,
                                           2.12625f, 2.5f,     2.87375f,
                                           3.24f,    3.59125f, 3.92f,
                                           4.21875f, 4.48f,    4.69625f,
                                           4.86f,    4.96375f, 5.0f };


class tst_BezierEvaluator : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void checkFindCubicRoots_data()
    {
        // Test data verified on Wolfram Alpha with snippets such as:
        // Plot[x^3-5x^2+x+3,{x,-3,6}]
        // Solve[x^3-5x^2+x+3,x]
        // If you need more, try these at https://www.wolframalpha.com/

        QTest::addColumn<float>("a");
        QTest::addColumn<float>("b");
        QTest::addColumn<float>("c");
        QTest::addColumn<float>("d");
        QTest::addColumn<int>("rootCount");
        QTest::addColumn<QList<float>>("roots");

        float a = 1.0f;
        float b = 0.0f;
        float c = 0.0f;
        float d = 0.0f;
        int rootCount = 1;
        QVector<float> roots = { 0.0f };
        QTest::newRow("a=1, b=0, c=0, d=0") << a << b << c << d << rootCount << roots;
        roots.clear();

        a = 1.0f;
        b = -1.0f;
        c = 1.0f;
        d = -1.0f;
        rootCount = 1;
        roots.resize(1);
        roots[0] = 1.0f;
        QTest::newRow("a=1, b=-1, c=1, d=-1") << a << b << c << d << rootCount << roots;
        roots.clear();

        a = 1.0f;
        b = -2.0f;
        c = 1.0f;
        d = -1.0f;
        rootCount = 1;
        roots.resize(1);
        roots[0] = 1.7548776f;
        QTest::newRow("a=1, b=-2, c=1, d=-1") << a << b << c << d << rootCount << roots;
        roots.clear();

        a = 1.0f;
        b = -5.0f;
        c = 1.0f;
        d = 3.0f;
        rootCount = 3;
        roots.resize(3);
        roots[0] = 2.0f + std::sqrt(7.0f);
        roots[1] = 1.0f;
        roots[2] = 2.0f - std::sqrt(7.0f);
        QTest::newRow("a=1, b=-5, c=1, d=3") << a << b << c << d << rootCount << roots;
        roots.clear();

        // quadratic equation
        a = 0.0f;
        b = 9.0f;
        c = 11.0f;
        d = 3.0f;
        roots.resize(2);
        roots[0] = -11.0f/18.0f + std::sqrt(13.0f) / 18.0f;
        roots[1] = -11.0f/18.0f - std::sqrt(13.0f) / 18.0f;
        QTest::newRow("a=0, b=9, c=11, d=3") << a << b << c << d << roots.size() << roots;
        roots.clear();

        // quadratic equation with discriminant = 0
        a = 0.0f;
        b = 1.0f;
        c = 2.0f;
        d = 1.0f;
        roots.resize(1);
        roots[0] = -1.f;
        QTest::newRow("a=0, b=1, c=2, d=1") << a << b << c << d << roots.size() << roots;
        roots.clear();

        // quadratic equation with discriminant < 0
        a = 0.0f;
        b = 1.0f;
        c = 4.0f;
        d = 8.0f;
        roots.resize(0);
        QTest::newRow("a=0, b=1, c=4, d=8") << a << b << c << d << roots.size() << roots;
        roots.clear();

        // linear equation
        a = 0.0f;
        b = 0.0f;
        c = 2.0f;
        d = 1.0f;
        roots.resize(1);
        roots[0] = -0.5f;
        QTest::newRow("a=0, b=0, c=2, d=1") << a << b << c << d << roots.size() << roots;
        roots.clear();

        // linear equation
        a = 0.0f;
        b = 0.0f;
        c = 8.0f;
        d = -5.0f;
        roots.resize(1);
        roots[0] = -d/c;
        QTest::newRow("a=0, b=0, c=8, d=-5") << a << b << c << d << roots.size() << roots;
        roots.clear();

        // invalid equation
        a = 0.0f;
        b = 0.0f;
        c = 0.0f;
        d = -5.0f;
        roots.resize(0);
        QTest::newRow("a=0, b=0, c=0, d=-5") << a << b << c << d << roots.size() << roots;
        roots.clear();

        // Invalid equation
        a = 0.0f;
        b = 0.0f;
        c = 0.0f;
        d = 42.0f;
        roots.resize(0);
        QTest::newRow("a=0, b=0, c=0, d=42") << a << b << c << d << roots.size() << roots;
        roots.clear();

        // almost linear equation
        a = 1.90735e-06f;
        b = -2.86102e-06f;
        c = 5.0;
        d = 0.0;
        roots.resize(1);
        roots[0] = -d/c;
        QTest::newRow("a=~0, b=~0, c=5, d=0") << a << b << c << d << roots.size() << roots;
        roots.clear();

        // case that produces a result just below zero, that should be evaluated as zero
        a = -0.75f;
        b = 0.75f;
        c = 2.5;
        d = 0.0;
        roots.resize(3);
        roots[0] = 2.39297f;
        roots[1] = 0.f;
        roots[2] = -1.39297f;
        QTest::newRow("a=-0.75, b=0.75, c=2.5, d=0") << a << b << c << d << roots.size() << roots;
        roots.clear();

        // Case that produces a discriminant that is close enough to zero that it should be
        // evaluated as zero.
        // Expected roots = 0.0, ~1.5
        a = -3.998f;
        b = 5.997f;
        c = 0.0f;
        d = 0.0f;
        roots.resize(2);
        roots[0] = 1.5f;
        roots[1] = 0.0f;
        QTest::newRow("a=-3.998, b=5.997, c=0, d=0") << a << b << c << d << roots.size() << roots;
        roots.clear();
    }

    void checkFindCubicRoots()
    {
        QFETCH(float, a);
        QFETCH(float, b);
        QFETCH(float, c);
        QFETCH(float, d);
        QFETCH(int, rootCount);
        QFETCH(QList<float>, roots);

        float coeffs[4];
        coeffs[0] = d;
        coeffs[1] = c;
        coeffs[2] = b;
        coeffs[3] = a;

        float results[3];
        const int foundRootCount = BezierEvaluator::findCubicRoots(coeffs, results);

        QCOMPARE(foundRootCount, rootCount);
        for (int i = 0; i < rootCount; ++i)
            QCOMPARE(results[i], roots[i]);
    }

    void checkParameterForTime_data()
    {
        QTest::addColumn<float>("t0");
        QTest::addColumn<Keyframe>("kf0");
        QTest::addColumn<float>("t1");
        QTest::addColumn<Keyframe>("kf1");
        QTest::addColumn<QList<float>>("times");
        QTest::addColumn<QList<float>>("bezierParamters");

        {
            float t0 = 0.0f;
            Keyframe kf0{0.0f, {-5.0f, 0.0f}, {5.0f, 0.0f}, QKeyFrame::BezierInterpolation};
            float t1 = 50.0f;
            Keyframe kf1{5.0f, {45.0f, 5.0f}, {55.0f, 5.0f}, QKeyFrame::BezierInterpolation};
            const int count = 21;

            QList<float> bezierParameters;
            float deltaU = 1.0f / float(count - 1);
            for (int i = 0; i < count; ++i)
                bezierParameters.push_back(float(i) * deltaU);

            QTest::newRow("t=0 to t=50, default easing") << t0 << kf0
                                                         << t1 << kf1
                                                         << globalTimes << bezierParameters;
        }
        {
            // This test creates a case where the coefficients for finding
            // the cubic roots will be a = 0, b = 0, c ~= 6.28557 d ~= -6.28557
            // Because c ~= d, the answer should be one root = 1, but
            // because of numerical imprecision, it will be slightly larger.
            // We have a fuzzy check in parameterForTime that takes care of this.
            float t0 = 3.71443009f;
            Keyframe kf0{150.0f, {0.0f, 0.0f}, {5.80961999f, 150.0f}, QKeyFrame::BezierInterpolation};
            float t1 = 10.0f;
            Keyframe kf1{-150.0f, {7.904809959f, 150.0f}, {0.f, 0.f}, QKeyFrame::BezierInterpolation};
            QList<float> times = { 10.f };
            QList<float> results = { 1.0f };
            QTest::newRow("t=0 to t=10, regression") << t0 << kf0
                                                     << t1 << kf1
                                                     << times << results;
        }
    }

    void checkParameterForTime()
    {
        // GIVEN
        QFETCH(float, t0);
        QFETCH(Keyframe, kf0);
        QFETCH(float, t1);
        QFETCH(Keyframe, kf1);
        QFETCH(QList<float>, times);
        QFETCH(QList<float>, bezierParamters);

        // WHEN
        BezierEvaluator bezier(t0, kf0, t1, kf1);

        // THEN
        for (int i = 0; i < times.size(); ++i) {
            const float time = times[i];
            const float u = bezier.parameterForTime(time);
            QCOMPARE(u, bezierParamters[i]);
        }
    }

    void checkValueForTime_data()
    {
        QTest::addColumn<float>("t0");
        QTest::addColumn<Keyframe>("kf0");
        QTest::addColumn<float>("t1");
        QTest::addColumn<Keyframe>("kf1");
        QTest::addColumn<QList<float>>("times");
        QTest::addColumn<QList<float>>("values");

        float t0 = 0.0f;
        Keyframe kf0{0.0f, {-5.0f, 0.0f}, {5.0f, 0.0f}, QKeyFrame::BezierInterpolation};
        float t1 = 50.0f;
        Keyframe kf1{5.0f, {45.0f, 5.0f}, {55.0f, 5.0f}, QKeyFrame::BezierInterpolation};

        QTest::newRow("t=0, value=0 to t=50, value=5, default easing") << t0 << kf0
                                                     << t1 << kf1
                                                     << globalTimes << globalValues;
    }

    void checkValueForTime()
    {
        // GIVEN
        QFETCH(float, t0);
        QFETCH(Keyframe, kf0);
        QFETCH(float, t1);
        QFETCH(Keyframe, kf1);
        QFETCH(QList<float>, times);
        QFETCH(QList<float>, values);

        // WHEN
        BezierEvaluator bezier(t0, kf0, t1, kf1);

        // THEN
        for (int i = 0; i < times.size(); ++i) {
            const float time = times[i];
            const float value = bezier.valueForTime(time);
            QCOMPARE(value, values[i]);
        }
    }
};

QTEST_APPLESS_MAIN(tst_BezierEvaluator)

#include "tst_bezierevaluator.moc"
