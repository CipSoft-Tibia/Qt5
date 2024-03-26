// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKTRANSLATE_P_H
#define QQUICKTRANSLATE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qtquickglobal_p.h>

#include <QtQuick/qquickitem.h>

#include <QtGui/qmatrix4x4.h>

QT_BEGIN_NAMESPACE

class QQuickTranslatePrivate;
class Q_QUICK_PRIVATE_EXPORT QQuickTranslate : public QQuickTransform
{
    Q_OBJECT

    Q_PROPERTY(qreal x READ x WRITE setX NOTIFY xChanged FINAL)
    Q_PROPERTY(qreal y READ y WRITE setY NOTIFY yChanged FINAL)
    QML_NAMED_ELEMENT(Translate)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickTranslate(QObject *parent = nullptr);

    qreal x() const;
    void setX(qreal);

    qreal y() const;
    void setY(qreal);

    void applyTo(QMatrix4x4 *matrix) const override;

Q_SIGNALS:
    void xChanged();
    void yChanged();

private:
    Q_DECLARE_PRIVATE(QQuickTranslate)
};

class QQuickScalePrivate;
class Q_QUICK_PRIVATE_EXPORT QQuickScale : public QQuickTransform
{
    Q_OBJECT

    Q_PROPERTY(QVector3D origin READ origin WRITE setOrigin NOTIFY originChanged FINAL)
    Q_PROPERTY(qreal xScale READ xScale WRITE setXScale NOTIFY xScaleChanged FINAL)
    Q_PROPERTY(qreal yScale READ yScale WRITE setYScale NOTIFY yScaleChanged FINAL)
    Q_PROPERTY(qreal zScale READ zScale WRITE setZScale NOTIFY zScaleChanged FINAL)
    QML_NAMED_ELEMENT(Scale)
    QML_ADDED_IN_VERSION(2, 0)
public:
    QQuickScale(QObject *parent = nullptr);

    QVector3D origin() const;
    void setOrigin(const QVector3D &point);

    qreal xScale() const;
    void setXScale(qreal);

    qreal yScale() const;
    void setYScale(qreal);

    qreal zScale() const;
    void setZScale(qreal);

    void applyTo(QMatrix4x4 *matrix) const override;

Q_SIGNALS:
    void originChanged();
    void xScaleChanged();
    void yScaleChanged();
    void zScaleChanged();
    void scaleChanged();

private:
    Q_DECLARE_PRIVATE(QQuickScale)
};

class QQuickRotationPrivate;
class Q_QUICK_PRIVATE_EXPORT QQuickRotation : public QQuickTransform
{
    Q_OBJECT

    Q_PROPERTY(QVector3D origin READ origin WRITE setOrigin NOTIFY originChanged FINAL)
    Q_PROPERTY(qreal angle READ angle WRITE setAngle NOTIFY angleChanged FINAL)
    Q_PROPERTY(QVector3D axis READ axis WRITE setAxis NOTIFY axisChanged FINAL)
    QML_NAMED_ELEMENT(Rotation)
    QML_ADDED_IN_VERSION(2, 0)
public:
    QQuickRotation(QObject *parent = nullptr);

    QVector3D origin() const;
    void setOrigin(const QVector3D &point);

    qreal angle() const;
    void setAngle(qreal);

    QVector3D axis() const;
    void setAxis(const QVector3D &axis);
    void setAxis(Qt::Axis axis);

    void applyTo(QMatrix4x4 *matrix) const override;

Q_SIGNALS:
    void originChanged();
    void angleChanged();
    void axisChanged();

private:
    Q_DECLARE_PRIVATE(QQuickRotation)
};

class QQuickMatrix4x4Private;
class Q_QUICK_PRIVATE_EXPORT QQuickMatrix4x4 : public QQuickTransform
{
    Q_OBJECT

    Q_PROPERTY(QMatrix4x4 matrix READ matrix WRITE setMatrix NOTIFY matrixChanged FINAL)
    QML_NAMED_ELEMENT(Matrix4x4)
    QML_ADDED_IN_VERSION(2, 3)
public:
    QQuickMatrix4x4(QObject *parent = nullptr);

    QMatrix4x4 matrix() const;
    void setMatrix(const QMatrix4x4& matrix);

    void applyTo(QMatrix4x4 *matrix) const override;

Q_SIGNALS:
    void matrixChanged();

private:
    Q_DECLARE_PRIVATE(QQuickMatrix4x4)
};


QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickTranslate)

#endif
