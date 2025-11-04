// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtQuick>
#include <QtQuick/private/qquicktranslate_p.h>

QT_BEGIN_NAMESPACE

class tst_qquicktransform : public QObject
{
    Q_OBJECT
private slots:
    void shearTransform();
};

void tst_qquicktransform::shearTransform()
{
    const QRect rect(10, 20, 100, 200);

    {
        QQuickShear shear;
        shear.setXFactor(1.0);
        shear.setOrigin(QVector3D(rect.topLeft()));

        QMatrix4x4 matrix;
        shear.applyTo(&matrix);

        QRect mappedRect = matrix.mapRect(rect);

        QCOMPARE(mappedRect.topLeft(), rect.topLeft());
        QCOMPARE(mappedRect.bottomRight(), rect.bottomRight() + QPoint(rect.height(), 0.0));
    }

    {
        QQuickShear shear;
        shear.setOrigin(QVector3D(rect.topLeft()));
        shear.setYFactor(0.5);

        QMatrix4x4 matrix;
        shear.applyTo(&matrix);

        QRect mappedRect = matrix.mapRect(rect);

        QCOMPARE(mappedRect.topLeft(), rect.topLeft());
        QCOMPARE(mappedRect.bottomRight(), rect.bottomRight() + QPoint(0.0, rect.width() * 0.5));
    }

    {
        QQuickShear shear;
        shear.setOrigin(QVector3D(rect.topLeft()));
        shear.setXFactor(1.0);
        shear.setYFactor(0.5);

        QMatrix4x4 matrix;
        shear.applyTo(&matrix);

        QRect mappedRect = matrix.mapRect(rect);

        QCOMPARE(mappedRect.topLeft(), rect.topLeft());
        QCOMPARE(mappedRect.bottomRight(), rect.bottomRight() + QPoint(rect.height(), rect.width() * 0.5));
    }

    {
        QQuickShear shear;
        shear.setOrigin(QVector3D(rect.topLeft()));
        shear.setXFactor(0.5);
        shear.setXAngle(qAtan(0.5) * 180.0 / M_PI);

        QMatrix4x4 matrix;
        shear.applyTo(&matrix);

        QRect mappedRect = matrix.mapRect(rect);

        QCOMPARE(mappedRect.topLeft(), rect.topLeft());
        QCOMPARE(mappedRect.bottomRight(), rect.bottomRight() + QPoint(rect.height(), 0.0));
    }

    {
        QQuickShear shear;
        shear.setOrigin(QVector3D(rect.topLeft()));
        shear.setYFactor(0.25);
        shear.setYAngle(qAtan(0.25) * 180.0 / M_PI);

        QMatrix4x4 matrix;
        shear.applyTo(&matrix);

        QRect mappedRect = matrix.mapRect(rect);

        QCOMPARE(mappedRect.topLeft(), rect.topLeft());
        QCOMPARE(mappedRect.bottomRight(), rect.bottomRight() + QPoint(0.0, rect.width() * 0.5));
    }
}

QT_END_NAMESPACE

QTEST_MAIN(tst_qquicktransform)

#include "tst_qquicktransform.moc"
