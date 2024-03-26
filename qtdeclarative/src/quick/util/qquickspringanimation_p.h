// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSPRINGANIMATION_H
#define QQUICKSPRINGANIMATION_H

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

#include <qqml.h>
#include "qquickanimation_p.h"

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QQuickSpringAnimationPrivate;
class Q_QUICK_PRIVATE_EXPORT QQuickSpringAnimation : public QQuickNumberAnimation
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QQuickSpringAnimation)
    Q_DECLARE_PRIVATE(QQuickSpringAnimation)
    Q_INTERFACES(QQmlPropertyValueSource)

    Q_PROPERTY(qreal velocity READ velocity WRITE setVelocity FINAL)
    Q_PROPERTY(qreal spring READ spring WRITE setSpring FINAL)
    Q_PROPERTY(qreal damping READ damping WRITE setDamping FINAL)
    Q_PROPERTY(qreal epsilon READ epsilon WRITE setEpsilon FINAL)
    Q_PROPERTY(qreal modulus READ modulus WRITE setModulus NOTIFY modulusChanged FINAL)
    Q_PROPERTY(qreal mass READ mass WRITE setMass NOTIFY massChanged FINAL)
    QML_NAMED_ELEMENT(SpringAnimation)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickSpringAnimation(QObject *parent=nullptr);
    ~QQuickSpringAnimation();

    qreal velocity() const;
    void setVelocity(qreal velocity);

    qreal spring() const;
    void setSpring(qreal spring);

    qreal damping() const;
    void setDamping(qreal damping);

    qreal epsilon() const;
    void setEpsilon(qreal epsilon);

    qreal mass() const;
    void setMass(qreal modulus);

    qreal modulus() const;
    void setModulus(qreal modulus);

    QAbstractAnimationJob* transition(QQuickStateActions &actions,
                            QQmlProperties &modified,
                            TransitionDirection direction,
                            QObject *defaultTarget = nullptr) override;

Q_SIGNALS:
    void modulusChanged();
    void massChanged();
    void syncChanged();
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickSpringAnimation)

#endif // QQUICKSPRINGANIMATION_H
