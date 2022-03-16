/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Wayland module
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef CUSTOMEXTENSION_H
#define CUSTOMEXTENSION_H

#include <QtWaylandClient/QWaylandClientExtension>
#include <QtGui/QWindow>
#include "qwayland-custom.h"

QT_BEGIN_NAMESPACE

class CustomExtensionObject;

class CustomExtension : public QWaylandClientExtensionTemplate<CustomExtension>
        , public QtWayland::qt_example_extension
{
    Q_OBJECT
public:
    CustomExtension();
    Q_INVOKABLE void registerWindow(QWindow *window);

    CustomExtensionObject *createCustomObject(const QString &color, const QString &text);

public slots:
    void sendBounce(QWindow *window, uint ms);
    void sendSpin(QWindow *window, uint ms);

signals:
    void eventReceived(const QString &text, uint value);
    void fontSize(QWindow *window, uint pixelSize);
    void showDecorations(bool);

private slots:
    void handleExtensionActive();

private:
    void example_extension_close(wl_surface *surface) override;
    void example_extension_set_font_size(wl_surface *surface, uint32_t pixel_size) override;
    void example_extension_set_window_decoration(uint32_t state) override;

    bool eventFilter(QObject *object, QEvent *event) override;

    QWindow *windowForSurface(struct ::wl_surface *);
    void sendWindowRegistration(QWindow *);

    QList<QWindow *> m_windows;
    bool m_activated = false;
};

class CustomExtensionObject : public QWaylandClientExtensionTemplate<CustomExtensionObject>
        , public QtWayland::qt_example_local_object
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
public:
    CustomExtensionObject(struct ::qt_example_local_object *wl_object, const QString &text);

    QString text() const
    {
        return m_text;
    }

protected:
    void example_local_object_clicked() override;

public slots:
    void setText(const QString &text);


signals:
    void textChanged(const QString &text);
    void clicked();

private:
    QString m_text;
};



QT_END_NAMESPACE

#endif // CUSTOMEXTENSION_H
