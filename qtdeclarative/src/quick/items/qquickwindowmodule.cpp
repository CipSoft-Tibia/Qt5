/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#include "qquickwindowmodule_p.h"
#include "qquickwindowattached_p.h"
#include "qquickrendercontrol.h"
#include "qquickscreen_p.h"
#include "qquickview_p.h"
#include <QtQuick/QQuickWindow>
#include <QtCore/QCoreApplication>
#include <QtQml/QQmlEngine>

#include <private/qguiapplication_p.h>
#include <private/qqmlengine_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qqmlglobal_p.h>
#include <qpa/qplatformintegration.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcTransient)

class QQuickWindowQmlImplPrivate : public QQuickWindowPrivate
{
public:
    QQuickWindowQmlImplPrivate()
        : complete(false)
        , visible(false)
        , visibility(QQuickWindow::AutomaticVisibility)
    {
    }

    bool complete;
    bool visible;
    QQuickWindow::Visibility visibility;
    QV4::PersistentValue rootItemMarker;
};

QQuickWindowQmlImpl::QQuickWindowQmlImpl(QWindow *parent)
    : QQuickWindow(*(new QQuickWindowQmlImplPrivate), parent)
{
    connect(this, &QWindow::visibleChanged, this, &QQuickWindowQmlImpl::visibleChanged);
    connect(this, &QWindow::visibilityChanged, this, &QQuickWindowQmlImpl::visibilityChanged);
    connect(this, &QWindow::screenChanged, this, &QQuickWindowQmlImpl::screenChanged);
}

void QQuickWindowQmlImpl::setVisible(bool visible)
{
    Q_D(QQuickWindowQmlImpl);
    d->visible = visible;
    if (d->complete && (!transientParent() || transientParentVisible()))
        QQuickWindow::setVisible(visible);
}

void QQuickWindowQmlImpl::setVisibility(Visibility visibility)
{
    Q_D(QQuickWindowQmlImpl);
    d->visibility = visibility;
    if (d->complete)
        QQuickWindow::setVisibility(visibility);
}

QQuickWindowAttached *QQuickWindowQmlImpl::qmlAttachedProperties(QObject *object)
{
    return new QQuickWindowAttached(object);
}

void QQuickWindowQmlImpl::classBegin()
{
    Q_D(QQuickWindowQmlImpl);
    QQmlEngine* e = qmlEngine(this);

    QQmlEngine::setContextForObject(contentItem(), e->rootContext());

    //Give QQuickView behavior when created from QML with QQmlApplicationEngine
    if (QCoreApplication::instance()->property("__qml_using_qqmlapplicationengine") == QVariant(true)) {
        if (e && !e->incubationController())
            e->setIncubationController(incubationController());
    }
    {
        // The content item has CppOwnership policy (set in QQuickWindow). Ensure the presence of a JS
        // wrapper so that the garbage collector can see the policy.
        QV4::ExecutionEngine *v4 = e->handle();
        QV4::QObjectWrapper::wrap(v4, d->contentItem);
    }
}

void QQuickWindowQmlImpl::componentComplete()
{
    Q_D(QQuickWindowQmlImpl);
    d->complete = true;
    QQuickItem *itemParent = qmlobject_cast<QQuickItem *>(QObject::parent());
    const bool transientParentAlreadySet = QQuickWindowPrivate::get(this)->transientParentPropertySet;
    if (!transientParentAlreadySet && itemParent && !itemParent->window()) {
        qCDebug(lcTransient) << "window" << title() << "has invisible Item parent" << itemParent << "transientParent"
                             << transientParent() << "declared visibility" << d->visibility << "; delaying show";
        connect(itemParent, &QQuickItem::windowChanged, this,
                &QQuickWindowQmlImpl::setWindowVisibility, Qt::QueuedConnection);
    } else if (transientParent() && !transientParent()->isVisible()) {
        connect(transientParent(), &QQuickWindow::visibleChanged, this,
                &QQuickWindowQmlImpl::setWindowVisibility, Qt::QueuedConnection);
    } else {
        setWindowVisibility();
    }
}

void QQuickWindowQmlImpl::setWindowVisibility()
{
    Q_D(QQuickWindowQmlImpl);
    if (transientParent() && !transientParentVisible())
        return;

    if (QQuickItem *senderItem = qmlobject_cast<QQuickItem *>(sender())) {
        disconnect(senderItem, &QQuickItem::windowChanged, this, &QQuickWindowQmlImpl::setWindowVisibility);
    } else if (sender()) {
        disconnect(transientParent(), &QWindow::visibleChanged, this, &QQuickWindowQmlImpl::setWindowVisibility);
    }

    // We have deferred window creation until we have the full picture of what
    // the user wanted in terms of window state, geometry, visibility, etc.

    if ((d->visibility == Hidden && d->visible) || (d->visibility > AutomaticVisibility && !d->visible)) {
        QQmlData *data = QQmlData::get(this);
        Q_ASSERT(data && data->context);

        QQmlError error;
        error.setObject(this);

        const QQmlContextData* urlContext = data->context;
        while (urlContext && urlContext->url().isEmpty())
            urlContext = urlContext->parent;
        error.setUrl(urlContext ? urlContext->url() : QUrl());

        QString objectId = data->context->findObjectId(this);
        if (!objectId.isEmpty())
            error.setDescription(QCoreApplication::translate("QQuickWindowQmlImpl",
                "Conflicting properties 'visible' and 'visibility' for Window '%1'").arg(objectId));
        else
            error.setDescription(QCoreApplication::translate("QQuickWindowQmlImpl",
                "Conflicting properties 'visible' and 'visibility'"));

        QQmlEnginePrivate::get(data->context->engine)->warning(error);
    }

    if (d->visibility == AutomaticVisibility) {
        setWindowState(QGuiApplicationPrivate::platformIntegration()->defaultWindowState(flags()));
        setVisible(d->visible);
    } else {
        setVisibility(d->visibility);
    }
}

QObject *QQuickWindowQmlImpl::screen() const
{
    return new QQuickScreenInfo(const_cast<QQuickWindowQmlImpl *>(this), QWindow::screen());
}

void QQuickWindowQmlImpl::setScreen(QObject *screen)
{
    QQuickScreenInfo *screenWrapper = qobject_cast<QQuickScreenInfo *>(screen);
    QWindow::setScreen(screenWrapper ? screenWrapper->wrappedScreen() : nullptr);
}

bool QQuickWindowQmlImpl::transientParentVisible()
{
   Q_ASSERT(transientParent());
   if (!transientParent()->isVisible()) {
       // handle case where transient parent is offscreen window
       QWindow *rw = QQuickRenderControl::renderWindowFor(qobject_cast<QQuickWindow*>(transientParent()));
       return rw && rw->isVisible();
   }
   return true;
}

QT_END_NAMESPACE

#include "moc_qquickwindowmodule_p.cpp"
