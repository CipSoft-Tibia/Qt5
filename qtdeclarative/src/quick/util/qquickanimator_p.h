// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKANIMATOR_P_H
#define QQUICKANIMATOR_P_H

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

#include "qquickanimation_p.h"

QT_BEGIN_NAMESPACE

class QQuickItem;

class QQuickAnimatorJob;
class QQuickAnimatorPrivate;
class Q_QUICK_PRIVATE_EXPORT QQuickAnimator : public QQuickAbstractAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickAnimator)
    Q_PROPERTY(QQuickItem *target READ targetItem WRITE setTargetItem NOTIFY targetItemChanged FINAL)
    Q_PROPERTY(QEasingCurve easing READ easing WRITE setEasing NOTIFY easingChanged FINAL)
    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged FINAL)
    Q_PROPERTY(qreal to READ to WRITE setTo NOTIFY toChanged FINAL)
    Q_PROPERTY(qreal from READ from WRITE setFrom NOTIFY fromChanged FINAL)

    QML_NAMED_ELEMENT(Animator)
    QML_ADDED_IN_VERSION(2, 2)
    QML_UNCREATABLE("Animator is an abstract class")

public:
    QQuickItem *targetItem() const;
    void setTargetItem(QQuickItem *target);

    int duration() const;
    void setDuration(int duration);

    QEasingCurve easing() const;
    void setEasing(const QEasingCurve & easing);

    qreal to() const;
    void setTo(qreal to);

    qreal from() const;
    void setFrom(qreal from);

protected:
    ThreadingModel threadingModel() const override { return RenderThread; }
    virtual QQuickAnimatorJob *createJob() const = 0;
    virtual QString propertyName() const = 0;
    QAbstractAnimationJob *transition(QQuickStateActions &actions,
                                      QQmlProperties &modified,
                                      TransitionDirection,
                                      QObject *) override;

    QQuickAnimator(QQuickAnimatorPrivate &dd, QObject *parent = nullptr);
    QQuickAnimator(QObject *parent = nullptr);

Q_SIGNALS:
    void targetItemChanged(QQuickItem *);
    void durationChanged(int duration);
    void easingChanged(const QEasingCurve &curve);
    void toChanged(qreal to);
    void fromChanged(qreal from);
};

class Q_QUICK_PRIVATE_EXPORT QQuickScaleAnimator : public QQuickAnimator
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ScaleAnimator)
    QML_ADDED_IN_VERSION(2, 2)
public:
    QQuickScaleAnimator(QObject *parent = nullptr);
protected:
    QQuickAnimatorJob *createJob() const override;
    QString propertyName() const override { return QStringLiteral("scale"); }
};

class Q_QUICK_PRIVATE_EXPORT QQuickXAnimator : public QQuickAnimator
{
    Q_OBJECT
    QML_NAMED_ELEMENT(XAnimator)
    QML_ADDED_IN_VERSION(2, 2)
public:
    QQuickXAnimator(QObject *parent = nullptr);
protected:
    QQuickAnimatorJob *createJob() const override;
    QString propertyName() const override { return QStringLiteral("x"); }
};

class Q_QUICK_PRIVATE_EXPORT QQuickYAnimator : public QQuickAnimator
{
    Q_OBJECT
    QML_NAMED_ELEMENT(YAnimator)
    QML_ADDED_IN_VERSION(2, 2)
public:
    QQuickYAnimator(QObject *parent = nullptr);
protected:
    QQuickAnimatorJob *createJob() const override;
    QString propertyName() const override { return QStringLiteral("y"); }
};

class Q_QUICK_PRIVATE_EXPORT QQuickOpacityAnimator : public QQuickAnimator
{
    Q_OBJECT
    QML_NAMED_ELEMENT(OpacityAnimator)
    QML_ADDED_IN_VERSION(2, 2)
public:
    QQuickOpacityAnimator(QObject *parent = nullptr);
protected:
    QQuickAnimatorJob *createJob() const override;
    QString propertyName() const override { return QStringLiteral("opacity"); }
};

class QQuickRotationAnimatorPrivate;
class Q_QUICK_PRIVATE_EXPORT QQuickRotationAnimator : public QQuickAnimator
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickRotationAnimator)
    Q_PROPERTY(RotationDirection direction READ direction WRITE setDirection NOTIFY directionChanged FINAL)
    QML_NAMED_ELEMENT(RotationAnimator)
    QML_ADDED_IN_VERSION(2, 2)

public:
    enum RotationDirection { Numerical, Shortest, Clockwise, Counterclockwise };
    Q_ENUM(RotationDirection)

    QQuickRotationAnimator(QObject *parent = nullptr);

    void setDirection(RotationDirection dir);
    RotationDirection direction() const;

Q_SIGNALS:
    void directionChanged(RotationDirection dir);

protected:
    QQuickAnimatorJob *createJob() const override;
    QString propertyName() const override { return QStringLiteral("rotation"); }
};

#if QT_CONFIG(quick_shadereffect)
class QQuickUniformAnimatorPrivate;
class Q_QUICK_PRIVATE_EXPORT QQuickUniformAnimator : public QQuickAnimator
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickUniformAnimator)
    Q_PROPERTY(QString uniform READ uniform WRITE setUniform NOTIFY uniformChanged FINAL)
    QML_NAMED_ELEMENT(UniformAnimator)
    QML_ADDED_IN_VERSION(2, 2)

public:
    QQuickUniformAnimator(QObject *parent = nullptr);

    QString uniform() const;
    void setUniform(const QString &);

Q_SIGNALS:
    void uniformChanged(const QString &);

protected:
    QQuickAnimatorJob *createJob() const override;
    QString propertyName() const override;
};
#endif

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickAnimator)
QML_DECLARE_TYPE(QQuickXAnimator)
QML_DECLARE_TYPE(QQuickYAnimator)
QML_DECLARE_TYPE(QQuickScaleAnimator)
QML_DECLARE_TYPE(QQuickRotationAnimator)
QML_DECLARE_TYPE(QQuickOpacityAnimator)
#if QT_CONFIG(quick_shadereffect)
QML_DECLARE_TYPE(QQuickUniformAnimator)
#endif
#endif // QQUICKANIMATOR_P_H
