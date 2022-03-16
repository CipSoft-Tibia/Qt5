/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWaylandCompositor module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "qwaylandcompositorextension.h"
#include "qwaylandcompositorextension_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

#include <wayland-server.h>

QT_BEGIN_NAMESPACE

QWaylandCompositorExtension::QWaylandCompositorExtension()
    : QWaylandObject(*new QWaylandCompositorExtensionPrivate())
{
}

QWaylandCompositorExtension::QWaylandCompositorExtension(QWaylandObject *container)
    : QWaylandObject(*new QWaylandCompositorExtensionPrivate())
{
    d_func()->extension_container = container;
    QCoreApplication::postEvent(this, new QEvent(QEvent::Polish));
}

QWaylandCompositorExtension::QWaylandCompositorExtension(QWaylandCompositorExtensionPrivate &dd)
    : QWaylandObject(dd)
{
}

QWaylandCompositorExtension::QWaylandCompositorExtension(QWaylandObject *container, QWaylandCompositorExtensionPrivate &dd)
    : QWaylandObject(dd)
{
    d_func()->extension_container = container;
    QCoreApplication::postEvent(this, new QEvent(QEvent::Polish));
}

QWaylandCompositorExtension::~QWaylandCompositorExtension()
{
    Q_D(QWaylandCompositorExtension);
    if (d->extension_container)
        d->extension_container->removeExtension(this);
}

QWaylandObject *QWaylandCompositorExtension::extensionContainer() const
{
    Q_D(const QWaylandCompositorExtension);
    return d->extension_container;
}

void QWaylandCompositorExtension::setExtensionContainer(QWaylandObject *container)
{
    Q_D(QWaylandCompositorExtension);
    d->extension_container = container;
}

void QWaylandCompositorExtension::initialize()
{
    Q_D(QWaylandCompositorExtension);
    if (d->initialized) {
        qWarning() << "QWaylandCompositorExtension:" << extensionInterface()->name << "is already initialized";
        return;
    }

    if (!d->extension_container && parent()) {
        QWaylandObject *parentObj = qobject_cast<QWaylandObject*>(parent());
        if (parentObj)
            setExtensionContainer(parentObj);
    }

    if (!d->extension_container) {
        qWarning() << "QWaylandCompositorExtension:" << extensionInterface()->name << "requests to initialize with no extension container set";
        return;
    }

    d->extension_container->addExtension(this);
    d->initialized = true;
}

bool QWaylandCompositorExtension::isInitialized() const
{
    Q_D(const QWaylandCompositorExtension);
    return d->initialized;
}

bool QWaylandCompositorExtension::event(QEvent *event)
{
    switch(event->type()) {
    case QEvent::Polish:
        if (!isInitialized())
            initialize();
        break;
    default:
        break;
    }
    return QWaylandObject::event(event);
}

QWaylandObject::QWaylandObject(QObject *parent)
    :QObject(parent)
{
}

QWaylandObject::QWaylandObject(QObjectPrivate &d, QObject *parent)
    :QObject(d, parent)
{
}


QWaylandObject::~QWaylandObject()
{
    foreach (QWaylandCompositorExtension *extension, extension_vector)
        QWaylandCompositorExtensionPrivate::get(extension)->extension_container = nullptr;
}

QWaylandCompositorExtension *QWaylandObject::extension(const QByteArray &name)
{
    for (int i = 0; i < extension_vector.size(); i++) {
        if (extension_vector.at(i)->extensionInterface()->name == name)
            return extension_vector.at(i);
    }
    return nullptr;
}

QWaylandCompositorExtension *QWaylandObject::extension(const wl_interface *interface)
{
    for (int i = 0; i < extension_vector.size(); i++) {
        if (extension_vector.at(i)->extensionInterface() == interface)
            return extension_vector.at(i);
    }
    return nullptr;
}

QList<QWaylandCompositorExtension *> QWaylandObject::extensions() const
{
    return extension_vector;
}

void QWaylandObject::addExtension(QWaylandCompositorExtension *extension)
{
    Q_ASSERT(!extension_vector.contains(extension));
    extension_vector.append(extension);
}

void QWaylandObject::removeExtension(QWaylandCompositorExtension *extension)
{
    Q_ASSERT(extension_vector.contains(extension));
    extension_vector.removeOne(extension);
}

QT_END_NAMESPACE
