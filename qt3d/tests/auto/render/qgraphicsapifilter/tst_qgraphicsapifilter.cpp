// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QTest>
#include <QObject>
#include <Qt3DRender/qgraphicsapifilter.h>
#include <qopenglcontext.h>
#include <QtCore/qsharedpointer.h>
#include <QSignalSpy>

class tst_QGraphicsApiFilter : public QObject
{
    Q_OBJECT
public:
    tst_QGraphicsApiFilter() : QObject()
    {
        qRegisterMetaType<Qt3DRender::QGraphicsApiFilter::Api>();
        qRegisterMetaType<Qt3DRender::QGraphicsApiFilter::OpenGLProfile>();
    }

private Q_SLOTS:
    void defaultConstruction()
    {
        // WHEN
        Qt3DRender::QGraphicsApiFilter apiFilter;

        // THEN
        const auto api = (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL)
            ? Qt3DRender::QGraphicsApiFilter::OpenGL
            : Qt3DRender::QGraphicsApiFilter::OpenGLES;
        QCOMPARE(apiFilter.api(), api);
        QCOMPARE(apiFilter.profile(), Qt3DRender::QGraphicsApiFilter::NoProfile);
        QCOMPARE(apiFilter.majorVersion(), 0);
        QCOMPARE(apiFilter.minorVersion(), 0);
        QCOMPARE(apiFilter.extensions(), QStringList());
        QCOMPARE(apiFilter.vendor(), QString());
    }

    void properties()
    {
        // GIVEN
        Qt3DRender::QGraphicsApiFilter apiFilter;

        {
            // WHEN
            QSignalSpy spy(&apiFilter, SIGNAL(apiChanged(Qt3DRender::QGraphicsApiFilter::Api)));
            apiFilter.setApi(Qt3DRender::QGraphicsApiFilter::OpenGL);

            // THEN
            QCOMPARE(apiFilter.api(), Qt3DRender::QGraphicsApiFilter::OpenGL);
            bool shouldEmitSignal = (QOpenGLContext::openGLModuleType() != QOpenGLContext::LibGL);
            QCOMPARE(spy.isEmpty(), !shouldEmitSignal);
        }

        {
            // WHEN
            QSignalSpy spy(&apiFilter, SIGNAL(profileChanged(Qt3DRender::QGraphicsApiFilter::OpenGLProfile)));
            apiFilter.setProfile(Qt3DRender::QGraphicsApiFilter::CoreProfile);

            // THEN
            QCOMPARE(apiFilter.profile(), Qt3DRender::QGraphicsApiFilter::CoreProfile);
            QCOMPARE(spy.size(), 1);
        }

        {
            // WHEN
            QSignalSpy spy(&apiFilter, SIGNAL(majorVersionChanged(int)));
            apiFilter.setMajorVersion(4);

            // THEN
            QCOMPARE(apiFilter.majorVersion(), 4);
            QCOMPARE(spy.size(), 1);
        }

        {
            // WHEN
            QSignalSpy spy(&apiFilter, SIGNAL(minorVersionChanged(int)));
            apiFilter.setMinorVersion(5);

            // THEN
            QCOMPARE(apiFilter.minorVersion(), 5);
            QCOMPARE(spy.size(), 1);
        }

        {
            // WHEN
            QSignalSpy spy(&apiFilter, SIGNAL(extensionsChanged(QStringList)));
            const auto extensions = (QStringList() << QLatin1String("extension1") << QLatin1String("extension2"));
            apiFilter.setExtensions(extensions);

            // THEN
            QCOMPARE(apiFilter.extensions(), extensions);
            QCOMPARE(spy.size(), 1);
        }

        {
            // WHEN
            QSignalSpy spy(&apiFilter, SIGNAL(vendorChanged(QString)));
            const QLatin1String vendor("Triangles McTriangleFace");
            apiFilter.setVendor(vendor);

            // THEN
            QCOMPARE(apiFilter.vendor(), vendor);
            QCOMPARE(spy.size(), 1);
        }
    }

    void shouldDetermineCompatibility_data()
    {
        QTest::addColumn<QSharedPointer<Qt3DRender::QGraphicsApiFilter>>("required");
        QTest::addColumn<QSharedPointer<Qt3DRender::QGraphicsApiFilter>>("actual");
        QTest::addColumn<bool>("expected");

        auto required = QSharedPointer<Qt3DRender::QGraphicsApiFilter>::create();
        required->setApi(Qt3DRender::QGraphicsApiFilter::OpenGL);
        required->setProfile(Qt3DRender::QGraphicsApiFilter::CoreProfile);
        required->setMajorVersion(4);
        required->setMinorVersion(5);
        auto actual = QSharedPointer<Qt3DRender::QGraphicsApiFilter>::create();
        actual->setApi(Qt3DRender::QGraphicsApiFilter::OpenGL);
        actual->setProfile(Qt3DRender::QGraphicsApiFilter::CoreProfile);
        actual->setMajorVersion(4);
        actual->setMinorVersion(5);
        bool expected = true;
        QTest::newRow("exact_match") << required << actual << expected;

        required = QSharedPointer<Qt3DRender::QGraphicsApiFilter>::create();
        required->setApi(Qt3DRender::QGraphicsApiFilter::OpenGL);
        required->setProfile(Qt3DRender::QGraphicsApiFilter::CoreProfile);
        required->setMajorVersion(3);
        required->setMinorVersion(2);
        actual = QSharedPointer<Qt3DRender::QGraphicsApiFilter>::create();
        actual->setApi(Qt3DRender::QGraphicsApiFilter::OpenGL);
        actual->setProfile(Qt3DRender::QGraphicsApiFilter::CoreProfile);
        actual->setMajorVersion(4);
        actual->setMinorVersion(5);
        expected = true;
        QTest::newRow("actual_is_higher_version") << required << actual << expected;

        required = QSharedPointer<Qt3DRender::QGraphicsApiFilter>::create();
        required->setApi(Qt3DRender::QGraphicsApiFilter::OpenGL);
        required->setProfile(Qt3DRender::QGraphicsApiFilter::CoreProfile);
        required->setMajorVersion(4);
        required->setMinorVersion(5);
        actual = QSharedPointer<Qt3DRender::QGraphicsApiFilter>::create();
        actual->setApi(Qt3DRender::QGraphicsApiFilter::OpenGL);
        actual->setProfile(Qt3DRender::QGraphicsApiFilter::CoreProfile);
        actual->setMajorVersion(3);
        actual->setMinorVersion(2);
        expected = false;
        QTest::newRow("actual_is_lower_version") << required << actual << expected;

        required = QSharedPointer<Qt3DRender::QGraphicsApiFilter>::create();
        required->setApi(Qt3DRender::QGraphicsApiFilter::OpenGL);
        required->setProfile(Qt3DRender::QGraphicsApiFilter::CompatibilityProfile);
        required->setMajorVersion(4);
        required->setMinorVersion(5);
        actual = QSharedPointer<Qt3DRender::QGraphicsApiFilter>::create();
        actual->setApi(Qt3DRender::QGraphicsApiFilter::OpenGL);
        actual->setProfile(Qt3DRender::QGraphicsApiFilter::CoreProfile);
        actual->setMajorVersion(4);
        actual->setMinorVersion(5);
        expected = false;
        QTest::newRow("wrong_profile") << required << actual << expected;

        required = QSharedPointer<Qt3DRender::QGraphicsApiFilter>::create();
        required->setApi(Qt3DRender::QGraphicsApiFilter::OpenGL);
        required->setProfile(Qt3DRender::QGraphicsApiFilter::CoreProfile);
        required->setMajorVersion(3);
        required->setMinorVersion(2);
        actual = QSharedPointer<Qt3DRender::QGraphicsApiFilter>::create();
        actual->setApi(Qt3DRender::QGraphicsApiFilter::OpenGLES);
        actual->setProfile(Qt3DRender::QGraphicsApiFilter::NoProfile);
        actual->setMajorVersion(3);
        actual->setMinorVersion(2);
        expected = false;
        QTest::newRow("wrong_api") << required << actual << expected;

        required = QSharedPointer<Qt3DRender::QGraphicsApiFilter>::create();
        required->setApi(Qt3DRender::QGraphicsApiFilter::OpenGL);
        required->setProfile(Qt3DRender::QGraphicsApiFilter::NoProfile);
        required->setMajorVersion(2);
        required->setMinorVersion(0);
        actual = QSharedPointer<Qt3DRender::QGraphicsApiFilter>::create();
        actual->setApi(Qt3DRender::QGraphicsApiFilter::OpenGL);
        actual->setProfile(Qt3DRender::QGraphicsApiFilter::CompatibilityProfile);
        actual->setMajorVersion(3);
        actual->setMinorVersion(2);
        expected = true;
        QTest::newRow("gl_3_2_compatibility_can_use_gl_2_0") << required << actual << expected;

        required = QSharedPointer<Qt3DRender::QGraphicsApiFilter>::create();
        required->setApi(Qt3DRender::QGraphicsApiFilter::OpenGL);
        required->setProfile(Qt3DRender::QGraphicsApiFilter::NoProfile);
        required->setMajorVersion(2);
        required->setMinorVersion(0);
        actual = QSharedPointer<Qt3DRender::QGraphicsApiFilter>::create();
        actual->setApi(Qt3DRender::QGraphicsApiFilter::OpenGL);
        actual->setProfile(Qt3DRender::QGraphicsApiFilter::CoreProfile);
        actual->setMajorVersion(3);
        actual->setMinorVersion(2);
        expected = false;
        QTest::newRow("gl_3_2_core_cant_use_gl_2_0") << required << actual << expected;
    }

    void shouldDetermineCompatibility()
    {
        // GIVEN
        QFETCH(QSharedPointer<Qt3DRender::QGraphicsApiFilter>, required);
        QFETCH(QSharedPointer<Qt3DRender::QGraphicsApiFilter>, actual);
        QFETCH(bool, expected);

        // WHEN
        const auto isCompatible = (*actual == *required);

        // THEN
        QCOMPARE(isCompatible, expected);
    }

    void checkEnumTypeValues()
    {
        // THEN
        QCOMPARE(int(Qt3DRender::QGraphicsApiFilter::OpenGL), 1);
        QCOMPARE(int(Qt3DRender::QGraphicsApiFilter::OpenGLES), 2);
        QCOMPARE(int(Qt3DRender::QGraphicsApiFilter::Vulkan), 3);
        QCOMPARE(int(Qt3DRender::QGraphicsApiFilter::DirectX), 4);
    }

    // TODO: Add equality test in 5.8 when we can add new api to
    // test for compatibility and properly use operator == to really
    // test for equality.
};


QTEST_MAIN(tst_QGraphicsApiFilter)

#include "tst_qgraphicsapifilter.moc"
