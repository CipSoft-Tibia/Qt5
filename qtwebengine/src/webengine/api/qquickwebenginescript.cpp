/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWebEngine module of the Qt Toolkit.
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

#include "qquickwebenginescript.h"
#include "qquickwebenginescript_p.h"

#include <QQmlFile>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QTimerEvent>
#include "renderer_host/user_resource_controller_host.h"

using QtWebEngineCore::UserScript;

QT_BEGIN_NAMESPACE

/*!
    \class QQuickWebEngineScript
    \brief Enables the injection of scripts in the JavaScript engine.
    \inmodule QtWebEngine
    \since 5.9

    The QQuickWebEngineScript type enables the programmatic injection of so called \e {user scripts} in
    the JavaScript engine at different points, determined by injectionPoint, during the loading of
    web content.

    Scripts can be executed either in the main JavaScript \e world, along with the rest of the
    JavaScript coming from the web contents, or in their own isolated world. While the DOM of the
    page can be accessed from any world, JavaScript variables of a function defined in one world are
    not accessible from a different one. The worldId property provides some predefined IDs for this
    purpose.
*/

/*!
    \enum QQuickWebEngineScript::InjectionPoint

    The point in the loading process at which the script will be executed.

    \value DocumentCreation
           The script will be executed as soon as the document is created. This is not suitable for
           any DOM operation.
    \value DocumentReady
           The script will run as soon as the DOM is ready. This is equivalent to the
           \c DOMContentLoaded event firing in JavaScript.
    \value Deferred
           The script will run when the page load finishes, or 500 ms after the document is ready,
           whichever comes first.
*/

/*!
    \enum QQuickWebEngineScript::ScriptWorldId

    The world ID defining which isolated world the script is executed in. Besides these predefined
    IDs custom IDs can be used, but must be integers between \c 0 and \c 256.

    \value MainWorld
           The world used by the page's web contents. It can be useful in order to expose custom
           functionality to web contents in certain scenarios.
    \value ApplicationWorld
           The default isolated world used for application level functionality implemented in
           JavaScript.
    \value UserWorld
           The first isolated world to be used by scripts set by users if the application is not
           making use of more worlds. As a rule of thumb, if that functionality is exposed to the
           application users, each individual script should probably get its own isolated world.
*/

/*!
    \qmltype WebEngineScript
    \instantiates QQuickWebEngineScript
    \inqmlmodule QtWebEngine
    \since QtWebEngine 1.1
    \brief Enables the programmatic injection of scripts in the JavaScript engine.

    The WebEngineScript type enables the programmatic injection of so called \e {user scripts} in
    the JavaScript engine at different points, determined by injectionPoint, during the loading of
    web content.

    Scripts can be executed either in the main JavaScript \e world, along with the rest of the
    JavaScript coming from the web contents, or in their own isolated world. While the DOM of the
    page can be accessed from any world, JavaScript variables of a function defined in one world are
    not accessible from a different one. The worldId property provides some predefined IDs for this
    purpose.

    The following \l Greasemonkey  attributes are supported since Qt 5.8:
    \c @exclude, \c @include, \c @name, \c @match, and \c @run-at.

    Use \l{WebEngineView::userScripts}{WebEngineView.userScripts} to access a list of scripts
    attached to the web view.
*/

/*!
    Constructs a new QQuickWebEngineScript with the parent \a parent.
*/
QQuickWebEngineScript::QQuickWebEngineScript(QObject *parent)
    : QObject(parent)
    , d_ptr(new QQuickWebEngineScriptPrivate)
{
    d_ptr->q_ptr = this;
}

/*!
   \internal
*/
QQuickWebEngineScript::~QQuickWebEngineScript()
{
}

/*!
    Returns the script object as string.
*/
QString QQuickWebEngineScript::toString() const
{
    Q_D(const QQuickWebEngineScript);
    if (d->coreScript.isNull())
        return QStringLiteral("QWebEngineScript()");
    QString ret = QStringLiteral("QWebEngineScript(") % d->coreScript.name() % QStringLiteral(", ");
    switch (d->coreScript.injectionPoint()) {
    case UserScript::DocumentElementCreation:
        ret.append(QStringLiteral("WebEngineScript::DocumentCreation, "));
        break;
    case UserScript::DocumentLoadFinished:
        ret.append(QStringLiteral("WebEngineScript::DocumentReady, "));
        break;
    case UserScript::AfterLoad:
        ret.append(QStringLiteral("WebEngineScript::Deferred, "));
        break;
    }
    ret.append(QString::number(d->coreScript.worldId()) % QStringLiteral(", ")
               % (d->coreScript.runsOnSubFrames() ? QStringLiteral("true") : QStringLiteral("false"))
               % QStringLiteral(", ") % d->coreScript.sourceCode() % QLatin1Char(')'));
    return ret;
}

/*!
    \property QQuickWebEngineScript::name
    \brief The name of the script.

    Can be useful to retrieve a particular script from
    QQuickWebEngineProfile::userScripts.
*/

/*!
    \qmlproperty string WebEngineScript::name

    The name of the script. Can be useful to retrieve a particular script from
    \l{WebEngineView::userScripts}{WebEngineView.userScripts}.
*/
QString QQuickWebEngineScript::name() const
{
    Q_D(const QQuickWebEngineScript);
    return d->coreScript.name();
}

/*!
    \property QQuickWebEngineScript::sourceUrl
    \brief The remote source location of the user script (if any).

    Unlike \l sourceCode, this property allows referring to user scripts that
    are not already loaded in memory, for instance, when stored on disk.

    Setting this property will change the \l sourceCode of the script.

    \note At present, only file-based sources are supported.

    \sa QQuickWebEngineScript::sourceCode
*/

/*!
    \qmlproperty url WebEngineScript::sourceUrl

    This property holds the remote source location of the user script (if any).

    Unlike \l sourceCode, this property allows referring to user scripts that
    are not already loaded in memory, for instance, when stored on disk.

    Setting this property will change the \l sourceCode of the script.

    \note At present, only file-based sources are supported.

    \sa sourceCode
*/
QUrl QQuickWebEngineScript::sourceUrl() const
{
    Q_D(const QQuickWebEngineScript);
    return d->m_sourceUrl;
}

/*!
    \property QQuickWebEngineScript::sourceCode
    \brief The JavaScript source code of the user script.

    \sa QQuickWebEngineScript::sourceUrl
*/

/*!
    \qmlproperty string WebEngineScript::sourceCode

    This property holds the JavaScript source code of the user script.

    \sa sourceUrl
*/
QString QQuickWebEngineScript::sourceCode() const
{
    Q_D(const QQuickWebEngineScript);
    return d->coreScript.sourceCode();
}

ASSERT_ENUMS_MATCH(QQuickWebEngineScript::Deferred, UserScript::AfterLoad)
ASSERT_ENUMS_MATCH(QQuickWebEngineScript::DocumentReady, UserScript::DocumentLoadFinished)
ASSERT_ENUMS_MATCH(QQuickWebEngineScript::DocumentCreation, UserScript::DocumentElementCreation)

/*!
    \property QQuickWebEngineScript::injectionPoint
    \brief  The point in the loading process at which the script will be executed.

    The default value is \c Deferred.
*/

/*!
    \qmlproperty enumeration WebEngineScript::injectionPoint

    The point in the loading process at which the script will be executed.
    The default value is \c Deferred.

    \value WebEngineScript.DocumentCreation
           The script will be executed as soon as the document is created. This is not suitable for
           any DOM operation.
    \value WebEngineScript.DocumentReady
           The script will run as soon as the DOM is ready. This is equivalent to the
           \c DOMContentLoaded event firing in JavaScript.
    \value WebEngineScript.Deferred
           The script will run when the page load finishes, or 500 ms after the document is ready,
           whichever comes first.
*/
QQuickWebEngineScript::InjectionPoint QQuickWebEngineScript::injectionPoint() const
{
    Q_D(const QQuickWebEngineScript);
    return static_cast<QQuickWebEngineScript::InjectionPoint>(d->coreScript.injectionPoint());
}

/*!
    \property QQuickWebEngineScript::worldId
    \brief The world ID defining which isolated world the script is executed in.
*/

/*!
    \qmlproperty enumeration WebEngineScript::worldId

    The world ID defining which isolated world the script is executed in.

    \value WebEngineScript.MainWorld
           The world used by the page's web contents. It can be useful in order to expose custom
           functionality to web contents in certain scenarios.
    \value WebEngineScript.ApplicationWorld
           The default isolated world used for application level functionality implemented in
           JavaScript.
    \value WebEngineScript.UserWorld
           The first isolated world to be used by scripts set by users if the application is not
           making use of more worlds. As a rule of thumb, if that functionality is exposed to the
           application users, each individual script should probably get its own isolated world.
*/
QQuickWebEngineScript::ScriptWorldId QQuickWebEngineScript::worldId() const
{
    Q_D(const QQuickWebEngineScript);
    return static_cast<QQuickWebEngineScript::ScriptWorldId>(d->coreScript.worldId());
}

/*!
    \property QQuickWebEngineScript::runOnSubframes
    \brief Whether the script is executed on every frame or only on the main frame.

    Set this property to \c true if the script is executed on every frame in the page, or \c false
    if it is only run for the main frame.
    The default value is \c{false}.
*/

/*!
    \qmlproperty int WebEngineScript::runOnSubframes

    Set this property to \c true if the script is executed on every frame in the page, or \c false
    if it is only ran for the main frame.
    The default value is \c{false}.
 */
bool QQuickWebEngineScript::runOnSubframes() const
{
    Q_D(const QQuickWebEngineScript);
    return d->coreScript.runsOnSubFrames();
}

void QQuickWebEngineScript::setName(const QString &name)
{
    Q_D(QQuickWebEngineScript);
    if (name == QQuickWebEngineScript::name())
        return;
    d->aboutToUpdateUnderlyingScript();
    d->coreScript.setName(name);
    Q_EMIT nameChanged(name);
}

void QQuickWebEngineScript::setSourceCode(const QString &code)
{
    Q_D(QQuickWebEngineScript);
    if (code == sourceCode())
        return;

    // setting the source directly resets the sourceUrl
    if (d->m_sourceUrl != QUrl()) {
        d->m_sourceUrl = QUrl();
        Q_EMIT sourceUrlChanged(d->m_sourceUrl);
    }

    d->aboutToUpdateUnderlyingScript();
    d->coreScript.setSourceCode(code);
    Q_EMIT sourceCodeChanged(code);
}

void QQuickWebEngineScript::setSourceUrl(const QUrl &url)
{
    Q_D(QQuickWebEngineScript);
    if (url == sourceUrl())
        return;

    d->m_sourceUrl = url;
    Q_EMIT sourceUrlChanged(d->m_sourceUrl);

    QFile f(QQmlFile::urlToLocalFileOrQrc(url));
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "Can't open user script " << url;
        return;
    }

    d->aboutToUpdateUnderlyingScript();
    QString source = QString::fromUtf8(f.readAll());
    d->coreScript.setSourceCode(source);
    Q_EMIT sourceCodeChanged(source);
}

void QQuickWebEngineScript::setInjectionPoint(QQuickWebEngineScript::InjectionPoint injectionPoint)
{
    Q_D(QQuickWebEngineScript);
    if (injectionPoint == QQuickWebEngineScript::injectionPoint())
        return;
    d->aboutToUpdateUnderlyingScript();
    d->coreScript.setInjectionPoint(static_cast<UserScript::InjectionPoint>(injectionPoint));
    Q_EMIT injectionPointChanged(injectionPoint);
}


void QQuickWebEngineScript::setWorldId(QQuickWebEngineScript::ScriptWorldId scriptWorldId)
{
    Q_D(QQuickWebEngineScript);
    if (scriptWorldId == worldId())
        return;
    d->aboutToUpdateUnderlyingScript();
    d->coreScript.setWorldId(scriptWorldId);
    Q_EMIT worldIdChanged(scriptWorldId);
}


void QQuickWebEngineScript::setRunOnSubframes(bool on)
{
    Q_D(QQuickWebEngineScript);
    if (on == runOnSubframes())
        return;
    d->aboutToUpdateUnderlyingScript();
    d->coreScript.setRunsOnSubFrames(on);
    Q_EMIT runOnSubframesChanged(on);
}

/*!
    \internal
*/
void QQuickWebEngineScript::timerEvent(QTimerEvent *e)
{
    Q_D(QQuickWebEngineScript);
    if (e->timerId() != d->m_basicTimer.timerId()) {
        QObject::timerEvent(e);
        return;
    }
    if (!d->m_controllerHost)
        return;
    d->m_basicTimer.stop();
    d->m_controllerHost->addUserScript(d->coreScript, d->m_adapter);
}

void QQuickWebEngineScriptPrivate::bind(QtWebEngineCore::UserResourceControllerHost *resourceController, QtWebEngineCore::WebContentsAdapter *adapter)
{
    aboutToUpdateUnderlyingScript();
    m_adapter = adapter;
    m_controllerHost = resourceController;
}

QQuickWebEngineScriptPrivate::QQuickWebEngineScriptPrivate()
    :m_controllerHost(0)
    , m_adapter(0)

{
}

void QQuickWebEngineScriptPrivate::aboutToUpdateUnderlyingScript()
{
    Q_Q(QQuickWebEngineScript);
    if (m_controllerHost)
        m_controllerHost->removeUserScript(coreScript, m_adapter);
   // Defer updates to the next event loop
   m_basicTimer.start(0, q);
}

QT_END_NAMESPACE
