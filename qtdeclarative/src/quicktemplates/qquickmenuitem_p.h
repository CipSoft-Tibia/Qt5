// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKMENUITEM_P_H
#define QQUICKMENUITEM_P_H

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

#include <QtQuickTemplates2/private/qquickabstractbutton_p.h>

QT_BEGIN_NAMESPACE

class QQuickMenu;
class QQuickMenuItemPrivate;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickMenuItem : public QQuickAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(bool highlighted READ isHighlighted WRITE setHighlighted NOTIFY highlightedChanged FINAL)
    // 2.3 (Qt 5.10)
    Q_PROPERTY(QQuickItem *arrow READ arrow WRITE setArrow NOTIFY arrowChanged FINAL REVISION(2, 3))
    Q_PROPERTY(QQuickMenu *menu READ menu NOTIFY menuChanged FINAL REVISION(2, 3))
    Q_PROPERTY(QQuickMenu *subMenu READ subMenu NOTIFY subMenuChanged FINAL REVISION(2, 3))
    Q_CLASSINFO("DeferredPropertyNames", "arrow,background,contentItem,indicator")
    QML_NAMED_ELEMENT(MenuItem)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickMenuItem(QQuickItem *parent = nullptr);

    bool isHighlighted() const;
    void setHighlighted(bool highlighted);

    // 2.3 (Qt 5.10)
    QQuickItem *arrow() const;
    void setArrow(QQuickItem *arrow);

    QQuickMenu *menu() const;
    QQuickMenu *subMenu() const;

Q_SIGNALS:
    void triggered();
    void highlightedChanged();
    // 2.3 (Qt 5.10)
    Q_REVISION(2, 3) void arrowChanged();
    Q_REVISION(2, 3) void menuChanged();
    Q_REVISION(2, 3) void subMenuChanged();

protected:
    void componentComplete() override;

    QFont defaultFont() const override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickMenuItem)
    Q_DECLARE_PRIVATE(QQuickMenuItem)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickMenuItem)

#endif // QQUICKMENUITEM_P_H
