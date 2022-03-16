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

#include "qwebenginecookiestore.h"
#include "qwebenginecookiestore_p.h"

#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

#include "net/cookie_monster_delegate_qt.h"

#include <QByteArray>
#include <QUrl>


namespace {

inline GURL toGurl(const QUrl& url)
{
    return GURL(url.toString().toStdString());
}

}

QT_BEGIN_NAMESPACE

using namespace QtWebEngineCore;

QWebEngineCookieStorePrivate::QWebEngineCookieStorePrivate(QWebEngineCookieStore *q)
    : q_ptr(q)
    , m_nextCallbackId(CallbackDirectory::ReservedCallbackIdsEnd)
    , m_deleteSessionCookiesPending(false)
    , m_deleteAllCookiesPending(false)
    , m_getAllCookiesPending(false)
    , delegate(0)
{
}

void QWebEngineCookieStorePrivate::processPendingUserCookies()
{
    Q_ASSERT(delegate);
    Q_ASSERT(delegate->hasCookieMonster());

    if (m_getAllCookiesPending) {
        m_getAllCookiesPending = false;
        delegate->getAllCookies(CallbackDirectory::GetAllCookiesCallbackId);
    }

    if (m_deleteAllCookiesPending) {
        m_deleteAllCookiesPending = false;
        delegate->deleteAllCookies(CallbackDirectory::DeleteAllCookiesCallbackId);
    }

    if (m_deleteSessionCookiesPending) {
        m_deleteSessionCookiesPending = false;
        delegate->deleteSessionCookies(CallbackDirectory::DeleteSessionCookiesCallbackId);
    }

    if (m_pendingUserCookies.isEmpty())
        return;

    for (const CookieData &cookieData : qAsConst(m_pendingUserCookies)) {
        if (cookieData.callbackId == CallbackDirectory::DeleteCookieCallbackId)
            delegate->deleteCookie(cookieData.cookie, cookieData.origin);
        else
            delegate->setCookie(cookieData.callbackId, cookieData.cookie, cookieData.origin);
    }

    m_pendingUserCookies.clear();
}

void QWebEngineCookieStorePrivate::rejectPendingUserCookies()
{
    m_getAllCookiesPending = false;
    m_deleteAllCookiesPending = false;
    m_deleteSessionCookiesPending = false;
    m_pendingUserCookies.clear();
}

void QWebEngineCookieStorePrivate::setCookie(const QWebEngineCallback<bool> &callback, const QNetworkCookie &cookie, const QUrl &origin)
{
    const quint64 currentCallbackId = callback ? m_nextCallbackId++ : static_cast<quint64>(CallbackDirectory::NoCallbackId);

    if (currentCallbackId != CallbackDirectory::NoCallbackId)
        callbackDirectory.registerCallback(currentCallbackId, callback);

    if (!delegate || !delegate->hasCookieMonster()) {
        m_pendingUserCookies.append(CookieData{ currentCallbackId, cookie, origin });
        return;
    }

    delegate->setCookie(currentCallbackId, cookie, origin);
}

void QWebEngineCookieStorePrivate::deleteCookie(const QNetworkCookie &cookie, const QUrl &url)
{
    if (!delegate || !delegate->hasCookieMonster()) {
        m_pendingUserCookies.append(CookieData{ CallbackDirectory::DeleteCookieCallbackId, cookie, url });
        return;
    }

    delegate->deleteCookie(cookie, url);
}

void QWebEngineCookieStorePrivate::deleteSessionCookies()
{
    if (!delegate || !delegate->hasCookieMonster()) {
        m_deleteSessionCookiesPending = true;
        return;
    }

    delegate->deleteSessionCookies(CallbackDirectory::DeleteSessionCookiesCallbackId);
}

void QWebEngineCookieStorePrivate::deleteAllCookies()
{
    if (!delegate || !delegate->hasCookieMonster()) {
        m_deleteAllCookiesPending = true;
        m_deleteSessionCookiesPending = false;
        return;
    }

    delegate->deleteAllCookies(CallbackDirectory::DeleteAllCookiesCallbackId);
}

void QWebEngineCookieStorePrivate::getAllCookies()
{
    if (!delegate || !delegate->hasCookieMonster()) {
        m_getAllCookiesPending = true;
        return;
    }

    delegate->getAllCookies(CallbackDirectory::GetAllCookiesCallbackId);
}

void QWebEngineCookieStorePrivate::onGetAllCallbackResult(qint64 callbackId, const QByteArray &cookieList)
{
    callbackDirectory.invoke(callbackId, cookieList);
}
void QWebEngineCookieStorePrivate::onSetCallbackResult(qint64 callbackId, bool success)
{
    callbackDirectory.invoke(callbackId, success);
}

void QWebEngineCookieStorePrivate::onDeleteCallbackResult(qint64 callbackId, int numCookies)
{
    callbackDirectory.invoke(callbackId, numCookies);
}

void QWebEngineCookieStorePrivate::onCookieChanged(const QNetworkCookie &cookie, bool removed)
{
    if (removed)
        Q_EMIT q_ptr->cookieRemoved(cookie);
    else
        Q_EMIT q_ptr->cookieAdded(cookie);
}

bool QWebEngineCookieStorePrivate::canAccessCookies(const QUrl &firstPartyUrl, const QUrl &url) const
{
    if (!filterCallback)
        return true;

    // Empty first-party URL indicates a first-party request (see net/base/static_cookie_policy.cc)
    bool thirdParty = !firstPartyUrl.isEmpty() &&
            !net::registry_controlled_domains::SameDomainOrHost(toGurl(url),
                                                                toGurl(firstPartyUrl),
                                                                net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

    QWebEngineCookieStore::FilterRequest request = { firstPartyUrl, url, thirdParty, false, 0};
    return filterCallback(request);
}

/*!
    \class QWebEngineCookieStore
    \inmodule QtWebEngineCore
    \since 5.6
    \brief The QWebEngineCookieStore class provides access to Chromium's cookies.

    The class allows to access HTTP cookies of Chromium for a specific profile.
    It can be used to synchronize cookies of Chromium and the QNetworkAccessManager, as well as
    to set, delete, and intercept cookies during navigation.
    Because cookie operations are asynchronous, the user can choose to provide a callback function
    to get notified about the success of the operation.
    The signal handlers for removal and addition should not be used to execute heavy tasks,
    because they might block the IO thread in case of a blocking connection.

    Use QWebEngineProfile::cookieStore() and QQuickWebEngineProfile::cookieStore()
    to access the cookie store object for a specific profile.
*/

/*!
    \fn void QWebEngineCookieStore::cookieAdded(const QNetworkCookie &cookie)

    This signal is emitted whenever a new \a cookie is added to the cookie store.
*/

/*!
    \fn void QWebEngineCookieStore::cookieRemoved(const QNetworkCookie &cookie)

    This signal is emitted whenever a \a cookie is deleted from the cookie store.
*/

/*!
    Creates a new QWebEngineCookieStore object with \a parent.
*/

QWebEngineCookieStore::QWebEngineCookieStore(QObject *parent)
    : QObject(parent)
    , d_ptr(new QWebEngineCookieStorePrivate(this))
{
}

/*!
    Destroys this QWebEngineCookieStore object.
*/

QWebEngineCookieStore::~QWebEngineCookieStore()
{

}

/*!
    Adds \a cookie to the cookie store.
    \note If \a cookie specifies a QNetworkCookie::domain() that does not start with a dot,
    a dot is automatically prepended. To limit the cookie to the exact server,
    omit QNetworkCookie::domain() and set \a origin instead.

    The provided URL should also include the scheme.

    \note This operation is asynchronous.
*/

void QWebEngineCookieStore::setCookie(const QNetworkCookie &cookie, const QUrl &origin)
{
    //TODO: use callbacks or delete dummy ones
    d_ptr->setCookie(QWebEngineCallback<bool>(), cookie, origin);
}

/*!
    Deletes \a cookie from the cookie store.
    It is possible to provide an optional \a origin URL argument to limit the scope of the
    cookie to be deleted.

    \note This operation is asynchronous.
*/

void QWebEngineCookieStore::deleteCookie(const QNetworkCookie &cookie, const QUrl &origin)
{
    d_ptr->deleteCookie(cookie, origin);
}

/*!
    Loads all the cookies into the cookie store. The cookieAdded() signal is emitted on every
    loaded cookie. Cookies are loaded automatically when the store gets initialized, which
    in most cases happens on loading the first URL. However, calling this function is useful
    if cookies should be listed before entering the web content.

    \note This operation is asynchronous.
*/

void QWebEngineCookieStore::loadAllCookies()
{
    //TODO: use callbacks or delete dummy ones
    if (d_ptr->m_getAllCookiesPending)
        return;
    d_ptr->callbackDirectory.registerCallback(CallbackDirectory::GetAllCookiesCallbackId, QWebEngineCallback<const QByteArray&>());
    //this will trigger cookieAdded signal
    d_ptr->getAllCookies();
}

/*!
    Deletes all the session cookies in the cookie store. Session cookies do not have an
    expiration date assigned to them.

    \note This operation is asynchronous.
    \sa loadAllCookies()
*/

void QWebEngineCookieStore::deleteSessionCookies()
{
    //TODO: use callbacks or delete dummy ones
    if (d_ptr->m_deleteAllCookiesPending || d_ptr->m_deleteSessionCookiesPending)
        return;
    d_ptr->callbackDirectory.registerCallback(CallbackDirectory::DeleteSessionCookiesCallbackId, QWebEngineCallback<int>());
    d_ptr->deleteSessionCookies();
}

/*!
    Deletes all the cookies in the cookie store.
    \note This operation is asynchronous.
    \sa loadAllCookies()
*/

void QWebEngineCookieStore::deleteAllCookies()
{
    //TODO: use callbacks or delete dummy ones
    if (d_ptr->m_deleteAllCookiesPending)
        return;
    d_ptr->callbackDirectory.registerCallback(CallbackDirectory::DeleteAllCookiesCallbackId, QWebEngineCallback<int>());
    d_ptr->deleteAllCookies();
}

/*!
    \since 5.11

    Installs a cookie filter that can prevent sites and resources from using cookies.
    The \a filterCallback must be a lambda or functor taking a FilterRequest structure. If the
    cookie access is to be accepted, the filter function should return \c true; otherwise
    it should return \c false.

    The following code snippet illustrates how to set a cookie filter:

    \code
    profile->cookieStore()->setCookieFilter(
        [&allowThirdPartyCookies](const QWebEngineCookieStore::FilterRequest &request)
        { return !request.thirdParty || allowThirdPartyCookies; }
    );
    \endcode

    You can unset the filter with a \c nullptr argument.

    The callback should not be used to execute heavy tasks since it is running on the
    IO thread and therefore blocks the Chromium networking.

    \note The cookie filter also controls other features with tracking capabilities similar to
    those of cookies; including IndexedDB, DOM storage, filesystem API, service workers,
    and AppCache.

    \sa deleteAllCookies(), loadAllCookies()
*/
void QWebEngineCookieStore::setCookieFilter(const std::function<bool(const FilterRequest &)> &filterCallback)
{
    d_ptr->filterCallback = filterCallback;
}

/*!
    \since 5.11
    \overload
*/
void QWebEngineCookieStore::setCookieFilter(std::function<bool(const FilterRequest &)> &&filterCallback)
{
    d_ptr->filterCallback = std::move(filterCallback);
}

/*!
    \class QWebEngineCookieStore::FilterRequest
    \inmodule QtWebEngineCore
    \since 5.11

    \brief The QWebEngineCookieStore::FilterRequest struct is used in conjunction with QWebEngineCookieStore::setCookieFilter() and is
    the type \a filterCallback operates on.

    \sa QWebEngineCookieStore::setCookieFilter()
*/

/*!
    \variable QWebEngineCookieStore::FilterRequest::firstPartyUrl
    \brief The URL that was navigated to.

    The site that would be showing in the location bar if the application has one.

    Can be used to white-list or black-list cookie access or third-party cookie access
    for specific sites visited.

    \sa origin, thirdParty
*/

/*!
    \variable QWebEngineCookieStore::FilterRequest::_reservedFlag
    \internal
*/

/*!
    \variable QWebEngineCookieStore::FilterRequest::_reservedType
    \internal
*/

/*!
    \variable QWebEngineCookieStore::FilterRequest::origin
    \brief The URL of the script or content accessing a cookie.

    Can be used to white-list or black-list third-party cookie access
    for specific services.

    \sa firstPartyUrl, thirdParty
*/

/*!
    \variable QWebEngineCookieStore::FilterRequest::thirdParty
    \brief Whether this is considered a third-party access.

    This is calculated by comparing FilterRequest::origin and FilterRequest::firstPartyUrl and
    checking if they share a common origin that is not a top-domain (like .com or .co.uk),
    or a known hosting site with independently owned subdomains.

    \sa firstPartyUrl, origin
*/

QT_END_NAMESPACE
