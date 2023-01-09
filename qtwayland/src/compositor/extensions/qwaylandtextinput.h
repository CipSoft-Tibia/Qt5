/****************************************************************************
**
** Copyright (C) 2017-2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
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

#ifndef QWAYLANDTEXTINPUT_H
#define QWAYLANDTEXTINPUT_H

#include <QtWaylandCompositor/QWaylandCompositorExtension>

struct wl_client;

QT_BEGIN_NAMESPACE

class QWaylandTextInputPrivate;

class QInputMethodEvent;
class QKeyEvent;
class QWaylandSurface;

class QWaylandTextInput : public QWaylandCompositorExtensionTemplate<QWaylandTextInput>
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QWaylandTextInput)
public:
    explicit QWaylandTextInput(QWaylandObject *container, QWaylandCompositor *compositor);
    ~QWaylandTextInput() override;

    void sendInputMethodEvent(QInputMethodEvent *event);
    void sendKeyEvent(QKeyEvent *event);

    QVariant inputMethodQuery(Qt::InputMethodQuery property, QVariant argument) const;

    QWaylandSurface *focus() const;
    void setFocus(QWaylandSurface *surface);

    bool isSurfaceEnabled(QWaylandSurface *surface) const;

    void add(::wl_client *client, uint32_t id, int version);
    static const struct wl_interface *interface();
    static QByteArray interfaceName();

Q_SIGNALS:
    void updateInputMethod(Qt::InputMethodQueries queries);
    void surfaceEnabled(QWaylandSurface *surface);
    void surfaceDisabled(QWaylandSurface *surface);

private:
    void focusSurfaceDestroyed(void *);
    void sendInputPanelState();
    void sendTextDirection();
    void sendLocale();
};

QT_END_NAMESPACE

#endif // QWAYLANDTEXTINPUT_H
