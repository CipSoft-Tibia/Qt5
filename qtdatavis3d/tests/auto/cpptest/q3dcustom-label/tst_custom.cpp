// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <QtDataVisualization/QCustom3DLabel>

#include "cpptestutil.h"

class tst_custom: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void construct();

    void initialProperties();
    void initializeProperties();
    void invalidProperties();

private:
    QCustom3DLabel *m_custom;
};

void tst_custom::initTestCase()
{
    if (!CpptestUtil::isOpenGLSupported())
        QSKIP("OpenGL not supported on this platform");
}

void tst_custom::cleanupTestCase()
{
}

void tst_custom::init()
{
    m_custom = new QCustom3DLabel();
}

void tst_custom::cleanup()
{
    delete m_custom;
}

void tst_custom::construct()
{
    QCustom3DLabel *custom = new QCustom3DLabel();
    QVERIFY(custom);
    delete custom;

    custom = new QCustom3DLabel("label", QFont("Times New Roman", 10.0), QVector3D(1.0f, 1.0f, 1.0f),
                                QVector3D(1.0f, 1.0f, 1.0f), QQuaternion(1.0f, 1.0f, 10.0f, 100.0f));
    QVERIFY(custom);
    QCOMPARE(custom->backgroundColor(), QColor(Qt::gray));
    QCOMPARE(custom->isBackgroundEnabled(), true);
    QCOMPARE(custom->isBorderEnabled(), true);
    QCOMPARE(custom->isFacingCamera(), false);
    QCOMPARE(custom->font(), QFont("Times New Roman", 10));
    QCOMPARE(custom->text(), QString("label"));
    QCOMPARE(custom->textColor(), QColor(Qt::white));
    QCOMPARE(custom->meshFile(), QString(":/defaultMeshes/plane"));
    QCOMPARE(custom->position(), QVector3D(1.0f, 1.0f, 1.0f));
    QCOMPARE(custom->isPositionAbsolute(), false);
    QCOMPARE(custom->rotation(), QQuaternion(1.0f, 1.0f, 10.0f, 100.0f));
    QCOMPARE(custom->scaling(), QVector3D(1.0f, 1.0f, 1.0f));
    QCOMPARE(custom->isScalingAbsolute(), true);
    QCOMPARE(custom->isShadowCasting(), false);
    QCOMPARE(custom->textureFile(), QString());
    QCOMPARE(custom->isVisible(), true);
    delete custom;
}

void tst_custom::initialProperties()
{
    QVERIFY(m_custom);

    QCOMPARE(m_custom->backgroundColor(), QColor(Qt::gray));
    QCOMPARE(m_custom->isBackgroundEnabled(), true);
    QCOMPARE(m_custom->isBorderEnabled(), true);
    QCOMPARE(m_custom->isFacingCamera(), false);
    QCOMPARE(m_custom->font(), QFont("Arial", 20));
    QCOMPARE(m_custom->text(), QString());
    QCOMPARE(m_custom->textColor(), QColor(Qt::white));

    // Common (from QCustom3DItem)
    QCOMPARE(m_custom->meshFile(), QString(":/defaultMeshes/plane"));
    QCOMPARE(m_custom->position(), QVector3D());
    QCOMPARE(m_custom->isPositionAbsolute(), false);
    QCOMPARE(m_custom->rotation(), QQuaternion());
    QCOMPARE(m_custom->scaling(), QVector3D(0.1f, 0.1f, 0.1f));
    QCOMPARE(m_custom->isScalingAbsolute(), true);
    QCOMPARE(m_custom->isShadowCasting(), false);
    QCOMPARE(m_custom->textureFile(), QString());
    QCOMPARE(m_custom->isVisible(), true);
}

void tst_custom::initializeProperties()
{
    QVERIFY(m_custom);

    m_custom->setBackgroundColor(QColor(Qt::red));
    m_custom->setBackgroundEnabled(false);
    m_custom->setBorderEnabled(false);
    m_custom->setFacingCamera(true);
    m_custom->setFont(QFont("Times New Roman", 10));
    m_custom->setText(QString("This is a Custom Label"));
    m_custom->setTextColor(QColor(Qt::blue));

    QCOMPARE(m_custom->backgroundColor(), QColor(Qt::red));
    QCOMPARE(m_custom->isBackgroundEnabled(), false);
    QCOMPARE(m_custom->isBorderEnabled(), false);
    QCOMPARE(m_custom->isFacingCamera(), true);
    QCOMPARE(m_custom->font(), QFont("Times New Roman", 10));
    QCOMPARE(m_custom->text(), QString("This is a Custom Label"));
    QCOMPARE(m_custom->textColor(), QColor(Qt::blue));

    // Common (from QCustom3DItem)
    m_custom->setPosition(QVector3D(1.0f, 1.0f, 1.0f));
    m_custom->setPositionAbsolute(true);
    m_custom->setRotation(QQuaternion(1.0f, 1.0f, 10.0f, 100.0f));
    m_custom->setScaling(QVector3D(1.0f, 1.0f, 1.0f));
    m_custom->setShadowCasting(true);
    m_custom->setVisible(false);

    QCOMPARE(m_custom->position(), QVector3D(1.0f, 1.0f, 1.0f));
    QCOMPARE(m_custom->isPositionAbsolute(), true);
    QCOMPARE(m_custom->rotation(), QQuaternion(1.0f, 1.0f, 10.0f, 100.0f));
    QCOMPARE(m_custom->scaling(), QVector3D(1.0f, 1.0f, 1.0f));
    QCOMPARE(m_custom->isShadowCasting(), true);
    QCOMPARE(m_custom->isVisible(), false);
}

void tst_custom::invalidProperties()
{
    m_custom->setScalingAbsolute(false);
    QCOMPARE(m_custom->isScalingAbsolute(), true);
}

QTEST_MAIN(tst_custom)
#include "tst_custom.moc"
