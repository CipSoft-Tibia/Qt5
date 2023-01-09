/****************************************************************************
**
** Copyright (C) 2017 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWaylandCompositor module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWAYLANDSHELL_H
#define QWAYLANDSHELL_H

#include <QtWaylandCompositor/QWaylandCompositorExtension>

QT_BEGIN_NAMESPACE

class QWaylandShellPrivate;

class Q_WAYLAND_COMPOSITOR_EXPORT QWaylandShell : public QWaylandCompositorExtension
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QWaylandShell)
    Q_PROPERTY(FocusPolicy focusPolicy READ focusPolicy WRITE setFocusPolicy NOTIFY focusPolicyChanged)
public:
    enum FocusPolicy {
        AutomaticFocus,
        ManualFocus
    };
    Q_ENUM(FocusPolicy)

    QWaylandShell();
    QWaylandShell(QWaylandObject *waylandObject);

    FocusPolicy focusPolicy() const;
    void setFocusPolicy(FocusPolicy focusPolicy);

Q_SIGNALS:
    void focusPolicyChanged();

protected:
    explicit QWaylandShell(QWaylandShellPrivate &dd);
    explicit QWaylandShell(QWaylandObject *container, QWaylandShellPrivate &dd);

    //Qt 6: remove
    Q_DECL_DEPRECATED QWaylandShell(QWaylandCompositorExtensionPrivate &dd);
    Q_DECL_DEPRECATED QWaylandShell(QWaylandObject *container, QWaylandCompositorExtensionPrivate &dd);
};

template <typename T>
class Q_WAYLAND_COMPOSITOR_EXPORT QWaylandShellTemplate : public QWaylandShell
{
public:
    QWaylandShellTemplate()
        : QWaylandShell()
    { }

    QWaylandShellTemplate(QWaylandObject *container)
        : QWaylandShell(container)
    { }

    const struct wl_interface *extensionInterface() const override
    {
        return T::interface();
    }

    static T *findIn(QWaylandObject *container)
    {
        if (!container) return nullptr;
        return qobject_cast<T *>(container->extension(T::interfaceName()));
    }

protected:
    QWaylandShellTemplate(QWaylandShellPrivate &dd)
        : QWaylandShell(dd)
    { }

    QWaylandShellTemplate(QWaylandObject *container, QWaylandShellPrivate &dd)
        : QWaylandShell(container,dd)
    { }
};

QT_END_NAMESPACE

#endif // QWAYLANDSHELL_H
