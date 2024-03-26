// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKPATHINTERPOLATOR_P_H
#define QQUICKPATHINTERPOLATOR_P_H

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

QT_REQUIRE_CONFIG(quick_path);

#include <qqml.h>
#include <QObject>

QT_BEGIN_NAMESPACE

class QQuickPath;
class Q_QUICK_PRIVATE_EXPORT QQuickPathInterpolator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickPath *path READ path WRITE setPath NOTIFY pathChanged FINAL)
    Q_PROPERTY(qreal progress READ progress WRITE setProgress NOTIFY progressChanged FINAL)
    Q_PROPERTY(qreal x READ x NOTIFY xChanged FINAL)
    Q_PROPERTY(qreal y READ y NOTIFY yChanged FINAL)
    Q_PROPERTY(qreal angle READ angle NOTIFY angleChanged FINAL)
    QML_NAMED_ELEMENT(PathInterpolator)
    QML_ADDED_IN_VERSION(2, 0)
public:
    explicit QQuickPathInterpolator(QObject *parent = nullptr);

    QQuickPath *path() const;
    void setPath(QQuickPath *path);

    qreal progress() const;
    void setProgress(qreal progress);

    qreal x() const;
    qreal y() const;
    qreal angle() const;

Q_SIGNALS:
    void pathChanged();
    void progressChanged();
    void xChanged();
    void yChanged();
    void angleChanged();

private Q_SLOTS:
    void _q_pathUpdated();

private:
    QQuickPath *_path;
    qreal _x;
    qreal _y;
    qreal _angle;
    qreal _progress;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickPathInterpolator)

#endif // QQUICKPATHINTERPOLATOR_P_H
