// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKHEADERVIEW_P_H
#define QQUICKHEADERVIEW_P_H

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

#include <private/qquicktableview_p.h>
#include <private/qtquicktemplates2global_p.h>

QT_BEGIN_NAMESPACE

class QQuickHeaderViewBase;
class QQuickHeaderViewBasePrivate;
class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickHeaderViewBase : public QQuickTableView
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickHeaderViewBase)
    Q_PROPERTY(QString textRole READ textRole WRITE setTextRole NOTIFY textRoleChanged FINAL)

public:
    explicit QQuickHeaderViewBase(Qt::Orientation orient, QQuickItem *parent = nullptr);
    ~QQuickHeaderViewBase();

    QString textRole() const;
    void setTextRole(const QString &role);

protected:
    QQuickHeaderViewBase(QQuickHeaderViewBasePrivate &dd, QQuickItem *parent);

Q_SIGNALS:
    void textRoleChanged();

private:
    Q_DISABLE_COPY(QQuickHeaderViewBase)
    friend class QQuickHorizontalHeaderView;
    friend class QQuickVerticalHeaderView;
};

class QQuickHorizontalHeaderViewPrivate;
class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickHorizontalHeaderView : public QQuickHeaderViewBase
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickHorizontalHeaderView)
    QML_NAMED_ELEMENT(HorizontalHeaderView)
    QML_ADDED_IN_VERSION(2, 15)

public:
    QQuickHorizontalHeaderView(QQuickItem *parent = nullptr);
    ~QQuickHorizontalHeaderView() override;

protected:
    QQuickHorizontalHeaderView(QQuickHorizontalHeaderViewPrivate &dd, QQuickItem *parent);

private:
    Q_DISABLE_COPY(QQuickHorizontalHeaderView)
};

class QQuickVerticalHeaderViewPrivate;
class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickVerticalHeaderView : public QQuickHeaderViewBase
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickVerticalHeaderView)
    QML_NAMED_ELEMENT(VerticalHeaderView)
    QML_ADDED_IN_VERSION(2, 15)

public:
    QQuickVerticalHeaderView(QQuickItem *parent = nullptr);
    ~QQuickVerticalHeaderView() override;

protected:
    QQuickVerticalHeaderView(QQuickVerticalHeaderViewPrivate &dd, QQuickItem *parent);

private:
    Q_DISABLE_COPY(QQuickVerticalHeaderView)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickHorizontalHeaderView)
QML_DECLARE_TYPE(QQuickVerticalHeaderView)

#endif // QQUICKHEADERVIEW_P_H
