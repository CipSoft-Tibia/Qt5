// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKITEMANIMATION_H
#define QQUICKITEMANIMATION_H

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

#include "qquickitem.h"

#include <QtQuick/private/qquickanimation_p.h>

QT_BEGIN_NAMESPACE

class QQuickParentAnimationPrivate;
class Q_QUICK_PRIVATE_EXPORT QQuickParentAnimation : public QQuickAnimationGroup
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickParentAnimation)

    Q_PROPERTY(QQuickItem *target READ target WRITE setTargetObject NOTIFY targetChanged FINAL)
    Q_PROPERTY(QQuickItem *newParent READ newParent WRITE setNewParent NOTIFY newParentChanged FINAL)
    Q_PROPERTY(QQuickItem *via READ via WRITE setVia NOTIFY viaChanged FINAL)
    QML_NAMED_ELEMENT(ParentAnimation)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickParentAnimation(QObject *parent=nullptr);

    QQuickItem *target() const;
    void setTargetObject(QQuickItem *);

    QQuickItem *newParent() const;
    void setNewParent(QQuickItem *);

    QQuickItem *via() const;
    void setVia(QQuickItem *);

Q_SIGNALS:
    void targetChanged();
    void newParentChanged();
    void viaChanged();

protected:
    QAbstractAnimationJob* transition(QQuickStateActions &actions,
                            QQmlProperties &modified,
                            TransitionDirection direction,
                            QObject *defaultTarget = nullptr) override;
};

class QQuickAnchorAnimationPrivate;
class Q_QUICK_PRIVATE_EXPORT QQuickAnchorAnimation : public QQuickAbstractAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickAnchorAnimation)
    Q_PROPERTY(QQmlListProperty<QQuickItem> targets READ targets FINAL)
    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged FINAL)
    Q_PROPERTY(QEasingCurve easing READ easing WRITE setEasing NOTIFY easingChanged FINAL)
    QML_NAMED_ELEMENT(AnchorAnimation)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickAnchorAnimation(QObject *parent=nullptr);

    QQmlListProperty<QQuickItem> targets();

    int duration() const;
    void setDuration(int);

    QEasingCurve easing() const;
    void setEasing(const QEasingCurve &);

Q_SIGNALS:
    void durationChanged(int);
    void easingChanged(const QEasingCurve&);

protected:
    QAbstractAnimationJob* transition(QQuickStateActions &actions,
                            QQmlProperties &modified,
                            TransitionDirection direction,
                            QObject *defaultTarget = nullptr) override;
};

#if QT_CONFIG(quick_path)

class QQuickItem;
class QQuickPath;
class QQuickPathAnimationPrivate;
class Q_QUICK_PRIVATE_EXPORT QQuickPathAnimation : public QQuickAbstractAnimation
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QQuickPathAnimation)
    Q_DECLARE_PRIVATE(QQuickPathAnimation)

    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged FINAL)
    Q_PROPERTY(QEasingCurve easing READ easing WRITE setEasing NOTIFY easingChanged FINAL)
    Q_PROPERTY(QQuickPath *path READ path WRITE setPath NOTIFY pathChanged FINAL)
    Q_PROPERTY(QQuickItem *target READ target WRITE setTargetObject NOTIFY targetChanged FINAL)
    Q_PROPERTY(Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged FINAL)
    Q_PROPERTY(QPointF anchorPoint READ anchorPoint WRITE setAnchorPoint NOTIFY anchorPointChanged FINAL)
    Q_PROPERTY(int orientationEntryDuration READ orientationEntryDuration WRITE setOrientationEntryDuration NOTIFY orientationEntryDurationChanged FINAL)
    Q_PROPERTY(int orientationExitDuration READ orientationExitDuration WRITE setOrientationExitDuration NOTIFY orientationExitDurationChanged FINAL)
    Q_PROPERTY(qreal endRotation READ endRotation WRITE setEndRotation NOTIFY endRotationChanged FINAL)
    QML_NAMED_ELEMENT(PathAnimation)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickPathAnimation(QObject *parent=nullptr);
    virtual ~QQuickPathAnimation();

    enum Orientation {
        Fixed,
        RightFirst,
        LeftFirst,
        BottomFirst,
        TopFirst
    };
    Q_ENUM(Orientation)

    int duration() const;
    void setDuration(int);

    QEasingCurve easing() const;
    void setEasing(const QEasingCurve &);

    QQuickPath *path() const;
    void setPath(QQuickPath *);

    QQuickItem *target() const;
    void setTargetObject(QQuickItem *);

    Orientation orientation() const;
    void setOrientation(Orientation orientation);

    QPointF anchorPoint() const;
    void setAnchorPoint(const QPointF &point);

    int orientationEntryDuration() const;
    void setOrientationEntryDuration(int);

    int orientationExitDuration() const;
    void setOrientationExitDuration(int);

    qreal endRotation() const;
    void setEndRotation(qreal);

protected:
    QAbstractAnimationJob* transition(QQuickStateActions &actions,
                            QQmlProperties &modified,
                            TransitionDirection direction,
                            QObject *defaultTarget = nullptr) override;
Q_SIGNALS:
    void durationChanged(int);
    void easingChanged(const QEasingCurve &);
    void pathChanged();
    void targetChanged();
    void orientationChanged(Orientation);
    void anchorPointChanged(const QPointF &);
    void orientationEntryDurationChanged(qreal);
    void orientationExitDurationChanged(qreal);
    void endRotationChanged(qreal);
};

#endif

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickParentAnimation)
QML_DECLARE_TYPE(QQuickAnchorAnimation)
#if QT_CONFIG(quick_path)
QML_DECLARE_TYPE(QQuickPathAnimation)
#endif

#endif // QQUICKITEMANIMATION_H
