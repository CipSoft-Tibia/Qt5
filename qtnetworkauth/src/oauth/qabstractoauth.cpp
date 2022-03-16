/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Network Auth module of the Qt Toolkit.
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

#ifndef QT_NO_HTTP

#include <qabstractoauth.h>
#include <qabstractoauthreplyhandler.h>

#include <private/qabstractoauth_p.h>

#include <QtCore/qurl.h>
#include <QtCore/qpair.h>
#include <QtCore/qstring.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qurlquery.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qmessageauthenticationcode.h>

#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkaccessmanager.h>

#include <random>

Q_DECLARE_METATYPE(QAbstractOAuth::Error)

QT_BEGIN_NAMESPACE

/*!
    \class QAbstractOAuth
    \inmodule QtNetworkAuth
    \ingroup oauth
    \brief The QAbstractOAuth class is the base of all
    implementations of OAuth authentication methods.
    \since 5.8

    The class defines the basic interface of the OAuth
    authentication classes. By inheriting this class, you
    can create custom authentication methods for different web
    services.

    It also contains some functions to ease the process of
    implementing different authentication flows.
*/

/*!
    \enum QAbstractOAuth::Status

    Indicates the current authentication status.

    \value NotAuthenticated                 No token has been
    retrieved.

    \value TemporaryCredentialsReceived     Temporary credentials
    have been received, this status is used in some OAuth
    authetication methods.

    \value Granted                          Token credentials have
    been received and authenticated calls are allowed.

    \value RefreshingToken                  New token credentials
    have been requested.
*/

/*!
    \enum QAbstractOAuth::Stage

    Identifies an authentication stage.  It's passed to a
    ModifyParametersFunction so that it can make different changes to
    parameters at each call to it during the process of
    authentication.

    \value RequestingTemporaryCredentials   Preparing the temporary
    credentials request.

    \value RequestingAuthorization          Preparing the
    authorization grant URL.

    \value RequestingAccessToken            Preparing the token
    request.

    \value RefreshingAccessToken            Preparing the access
    token refresh.
*/

/*!
    \enum QAbstractOAuth::Error

    Indicates the latest received error.

    \value NoError                          No error has ocurred.

    \value NetworkError                     Failed to connect to the server.

    \value ServerError                      The server answered the
    request with an error.

    \value OAuthTokenNotFoundError          The server's response to
    a token request provided no token identifier.

    \value OAuthTokenSecretNotFoundError    The server's response to
    a token request provided no token secret.

    \value OAuthCallbackNotVerified         The authorization server
    has not verified the supplied callback URI in the request. This
    usually happens when the provided callback does not match with
    the callback supplied during client registration.
*/

/*!
    \enum QAbstractOAuth::ContentType

    Indicates the MIME Content-Type of the POST methods in
    authenticated calls.

    \value WwwFormUrlEncoded                Uses
    application/x-www-form-urlencoded format.

    \value Json                             Uses
    application/json format.
*/

/*!
    \property QAbstractOAuth::status
    \brief This property holds the current authentication status.
*/

/*!
    \property QAbstractOAuth::extraTokens
    \brief This property holds the extra tokens received from the
    server.
*/

/*!
    \property QAbstractOAuth::authorizationUrl
    \brief This property holds the URL used to request the Resource
    Owner Authorization as described in:
    \l{https://tools.ietf.org/html/rfc5849#section-2.2}{The OAuth
    1.0 Protocol: Resource Owner Authorization}
*/

/*!
    \property QAbstractOAuth::contentType
    \brief The Content-Type to use when sending authorization
    parameters.

    This property controls how parameters are formatted when sent
    with a POST request.  A suitable header is also added.
*/

/*!
    \fn void QAbstractOAuth::authorizeWithBrowser(const QUrl &url)

    This signal is emitted when the \a url generated by
    resourceOwnerAuthorization() is ready to be used in the web
    browser to allow the application to impersonate the user.
    \sa resourceOwnerAuthorization()
*/

/*!
    \fn void QAbstractOAuth::granted()

    This signal is emitted when the authorization flow finishes
    successfully.
*/

/*!
    \fn QNetworkReply *QAbstractOAuth::head(const QUrl &url, const QVariantMap &parameters)

    Sends an authenticated HEAD request and returns a new
    QNetworkReply. The \a url and \a parameters are used to create
    the request.

    \b {See also}: \l {https://tools.ietf.org/html/rfc2616#section-9.4}
    {Hypertext Transfer Protocol -- HTTP/1.1: HEAD}
*/

/*!
    \fn QNetworkReply *QAbstractOAuth::get(const QUrl &url, const QVariantMap &parameters)

    Sends an authenticated GET request and returns a new
    QNetworkReply. The \a url and \a parameters are used to create
    the request.

    \b {See also}: \l {https://tools.ietf.org/html/rfc2616#section-9.3}
    {Hypertext Transfer Protocol -- HTTP/1.1: GET}
*/

/*!
    \fn QNetworkReply *QAbstractOAuth::post(const QUrl &url, const QVariantMap &parameters)

    Sends an authenticated POST request and returns a new
    QNetworkReply. The \a url and \a parameters are used to create
    the request.

    \b {See also}: \l {https://tools.ietf.org/html/rfc2616#section-9.5}
    {Hypertext Transfer Protocol -- HTTP/1.1: POST}
*/

/*!
    \fn QNetworkReply *QAbstractOAuth::put(const QUrl &url, const QVariantMap &parameters)

    Sends an authenticated PUT request and returns a new
    QNetworkReply. The \a url and \a parameters are used to create
    the request.

    \b {See also}: \l {https://tools.ietf.org/html/rfc2616#section-9.6}
    {Hypertext Transfer Protocol -- HTTP/1.1: PUT}
*/

/*!
    \fn QNetworkReply *QAbstractOAuth::deleteResource(const QUrl &url, const QVariantMap &parameters)

    Sends an authenticated DELETE request and returns a new
    QNetworkReply. The \a url and \a parameters are used to create
    the request.

    \b {See also}: \l {https://tools.ietf.org/html/rfc2616#section-9.7}
    {Hypertext Transfer Protocol -- HTTP/1.1: DELETE}
*/

/*!
    \fn void QAbstractOAuth::grant()

    Override this function to implement the corresponding
    authentication flow in the subclasses. Client code calls this
    function to start the authentication workflow. This may require
    user interaction: for example, asking the user's authorization
    via a web browser. When the authentication succeeds, it should
    emit granted(); this gives notice that credentials are ready to
    be used in authenticated calls.
*/

QAbstractOAuthPrivate::QAbstractOAuthPrivate(const char *loggingCategory,
                                             const QUrl &authorizationUrl,
                                             const QString &clientIdentifier,
                                             QNetworkAccessManager *manager) :
    loggingCategory(loggingCategory),
    clientIdentifier(clientIdentifier),
    authorizationUrl(authorizationUrl),
    defaultReplyHandler(new QOAuthOobReplyHandler),
    networkAccessManagerPointer(manager)
{}

QAbstractOAuthPrivate::~QAbstractOAuthPrivate()
{}

QNetworkAccessManager *QAbstractOAuthPrivate::networkAccessManager()
{
    Q_Q(QAbstractOAuth);
    if (!networkAccessManagerPointer)
        networkAccessManagerPointer = new QNetworkAccessManager(q);
    return networkAccessManagerPointer.data();
}

void QAbstractOAuthPrivate::setStatus(QAbstractOAuth::Status newStatus)
{
    Q_Q(QAbstractOAuth);
    if (status != newStatus) {
        status = newStatus;
        Q_EMIT q->statusChanged(status);
        if (status == QAbstractOAuth::Status::Granted)
            Q_EMIT q->granted();
    }
}

QByteArray QAbstractOAuthPrivate::generateRandomString(quint8 length)
{
    const char characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    static std::mt19937 randomEngine(QDateTime::currentDateTime().toMSecsSinceEpoch());
    std::uniform_int_distribution<int> distribution(0, sizeof(characters) - 2);
    QByteArray data;
    data.reserve(length);
    for (quint8 i = 0; i < length; ++i)
        data.append(characters[distribution(randomEngine)]);
    return data;
}

QByteArray QAbstractOAuthPrivate::convertParameters(const QVariantMap &parameters)
{
    QByteArray data;
    switch (contentType) {
    case QAbstractOAuth::ContentType::Json:
        data = QJsonDocument::fromVariant(QVariant(parameters)).toJson();
        break;
    case QAbstractOAuth::ContentType::WwwFormUrlEncoded: {
        QUrlQuery query;
        for (auto it = parameters.begin(), end = parameters.end(); it != end; ++it)
            query.addQueryItem(it.key(), it->toString());
        data = query.toString(QUrl::FullyEncoded).toUtf8();
        break;
    }
    }
    return data;
}

void QAbstractOAuthPrivate::addContentTypeHeaders(QNetworkRequest *request)
{
    Q_ASSERT(request);

    switch (contentType) {
    case QAbstractOAuth::ContentType::WwwFormUrlEncoded:
        request->setHeader(QNetworkRequest::ContentTypeHeader,
                           QStringLiteral("application/x-www-form-urlencoded"));
        break;
    case QAbstractOAuth::ContentType::Json:
        request->setHeader(QNetworkRequest::ContentTypeHeader,
                           QStringLiteral("application/json"));
        break;
    }
}

QUrlQuery QAbstractOAuthPrivate::createQuery(const QVariantMap &parameters)
{
    QUrlQuery query;
    for (auto it = parameters.begin(), end = parameters.end(); it != end; ++it)
        query.addQueryItem(it.key(), it.value().toString());
    return query;
}

QAbstractOAuth::QAbstractOAuth(QAbstractOAuthPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
    qRegisterMetaType<QAbstractOAuth::Error>();
}

/*!
    Destroys the abstract OAuth.
*/
QAbstractOAuth::~QAbstractOAuth()
{}

/*!
    Returns the current client identifier used in the authentication
    process.

    \sa setClientIdentifier()
*/
QString QAbstractOAuth::clientIdentifier() const
{
    Q_D(const QAbstractOAuth);
    return d->clientIdentifier;
}

/*!
    Sets the current client identifier to \a clientIdentifier.

    \sa clientIdentifier()
*/
void QAbstractOAuth::setClientIdentifier(const QString &clientIdentifier)
{
    Q_D(QAbstractOAuth);
    if (d->clientIdentifier != clientIdentifier) {
        d->clientIdentifier = clientIdentifier;
        Q_EMIT clientIdentifierChanged(clientIdentifier);
    }
}

/*!
    Returns the token used to sign the authenticated requests.

    \sa setToken()
*/
QString QAbstractOAuth::token() const
{
    Q_D(const QAbstractOAuth);
    return d->token;
}

/*!
    Sets the token used to sign authenticated requests to \a token.

    \sa token()
*/
void QAbstractOAuth::setToken(const QString &token)
{
    Q_D(QAbstractOAuth);
    if (d->token != token) {
        d->token = token;
        Q_EMIT tokenChanged(token);
    }
}

/*!
    Returns the current network access manager used to send the
    requests to the server during authentication flows or to make
    authentication calls.

    \sa setNetworkAccessManager(), QNetworkAccessManager
*/
QNetworkAccessManager *QAbstractOAuth::networkAccessManager() const
{
    Q_D(const QAbstractOAuth);
    return d->networkAccessManagerPointer.data();
}

/*!
    Sets the network manager to \a networkAccessManager.
    QAbstractOAuth does not take ownership of
    \a networkAccessManager. If no custom network access manager is
    set, an internal network access manager is used.
    This network access manager will be used
    to make the request to the authentication server and the
    authenticated request to the web service.

    \sa networkAccessManager(), QNetworkAccessManager
*/
void QAbstractOAuth::setNetworkAccessManager(QNetworkAccessManager *networkAccessManager)
{
    Q_D(QAbstractOAuth);
    if (networkAccessManager != d->networkAccessManagerPointer) {
        if (d->networkAccessManagerPointer && d->networkAccessManagerPointer->parent() == this)
            delete d->networkAccessManagerPointer.data();
        d->networkAccessManagerPointer = networkAccessManager;
    }
}

/*!
    Returns the current authentication status.
    \sa Status
*/
QAbstractOAuth::Status QAbstractOAuth::status() const
{
    Q_D(const QAbstractOAuth);
    return d->status;
}

/*!
    Returns the authorization request URL.
    \sa setAuthorizationUrl()
*/
QUrl QAbstractOAuth::authorizationUrl() const
{
    Q_D(const QAbstractOAuth);
    return d->authorizationUrl;
}

/*!
    Sets the authorization request URL to \a url. This address
    will be used to allow the user to grant the application the
    ability to make authenticated calls on behalf of the user.
    \sa authorizationUrl()
*/
void QAbstractOAuth::setAuthorizationUrl(const QUrl &url)
{
    Q_D(QAbstractOAuth);
    if (d->authorizationUrl != url) {
        d->authorizationUrl = url;
        Q_EMIT authorizationUrlChanged(url);
    }
}

/*!
    Sets the current status to \a status. This method is for use
    by classes based on QAbstractOAuth.
    \sa status()
*/
void QAbstractOAuth::setStatus(QAbstractOAuth::Status status)
{
    Q_D(QAbstractOAuth);
    if (status != d->status) {
        d->status = status;
        Q_EMIT statusChanged(status);
    }
}

/*!
    Returns the reply handler currently in use.
    \sa setReplyHandler(), QAbstractOAuthReplyHandler
*/
QAbstractOAuthReplyHandler *QAbstractOAuth::replyHandler() const
{
    Q_D(const QAbstractOAuth);
    return d->replyHandler ? d->replyHandler.data() : d->defaultReplyHandler.data();
}

/*!
    Sets the current reply handler to \a handler.
    \note Does not take ownership of \a handler.
*/
void QAbstractOAuth::setReplyHandler(QAbstractOAuthReplyHandler *handler)
{
    Q_D(QAbstractOAuth);
    d->replyHandler = handler;
}

/*!
    Returns the current parameter-modification function.
    \sa ModifyParametersFunction, setModifyParametersFunction(), Stage
*/
QAbstractOAuth::ModifyParametersFunction QAbstractOAuth::modifyParametersFunction() const
{
    Q_D(const QAbstractOAuth);
    return d->modifyParametersFunction;
}

/*!
    Sets the parameter-modification function \a modifyParametersFunction.
    This function is used to customize the parameters sent to the server
    during a specified authorization stage. The number of calls to this
    function depends on the flow used during the authentication.
    \sa modifyParametersFunction(), ModifyParametersFunction, Stage
*/
void QAbstractOAuth::setModifyParametersFunction(
        const QAbstractOAuth::ModifyParametersFunction &modifyParametersFunction)
{
    Q_D(QAbstractOAuth);
    d->modifyParametersFunction = modifyParametersFunction;
}

/*!
    Returns the current Content-Type used in authenticated calls.
    \sa setContentType(), post()
*/
QAbstractOAuth::ContentType QAbstractOAuth::contentType() const
{
    Q_D(const QAbstractOAuth);
    return d->contentType;
}

/*!
    Sets the current Content-Type to \a contentType.
*/
void QAbstractOAuth::setContentType(QAbstractOAuth::ContentType contentType)
{
    Q_D(QAbstractOAuth);
    if (d->contentType != contentType) {
        d->contentType = contentType;
        Q_EMIT contentTypeChanged(contentType);
    }
}

/*!
    Returns the extra tokens received from the server during
    authentication.
    \sa extraTokensChanged()
*/
QVariantMap QAbstractOAuth::extraTokens() const
{
    Q_D(const QAbstractOAuth);
    return d->extraTokens;
}

/*!
    Returns the current callback string corresponding to the
    current reply handler. The returned string is the string
    sent to the server to specify the callback URI, or the word
    identifying the alternative method in headless devices.
    \sa replyHandler(), setReplyHandler()
*/
QString QAbstractOAuth::callback() const
{
    Q_D(const QAbstractOAuth);
    return d->replyHandler ? d->replyHandler->callback()
                           : d->defaultReplyHandler->callback();
}

/*!
    Builds the resource owner authorization URL to be used in the web
    browser: \a url is used as the base URL and the query is created
    using \a parameters. When the URL is ready, the
    authorizeWithBrowser() signal will be emitted with the generated
    URL.
    \sa authorizeWithBrowser()
*/
void QAbstractOAuth::resourceOwnerAuthorization(const QUrl &url, const QVariantMap &parameters)
{
    QUrl u = url;
    u.setQuery(QAbstractOAuthPrivate::createQuery(parameters));
    Q_EMIT authorizeWithBrowser(u);
}

/*!
    Generates a random string which could be used as state or nonce.
    The parameter \a length determines the size of the generated
    string.

    \b {See also}: \l {https://tools.ietf.org/html/rfc5849#section-3.3}{The
    OAuth 1.0 Protocol: Nonce and Timestamp}.
*/
QByteArray QAbstractOAuth::generateRandomString(quint8 length)
{
    return QAbstractOAuthPrivate::generateRandomString(length);
}

QT_END_NAMESPACE

#endif // QT_NO_HTTP
