// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QT_NO_HTTP

#include <qoauth2authorizationcodeflow.h>
#include <private/qoauth2authorizationcodeflow_p.h>

#include <qmap.h>
#include <qurl.h>
#include <qvariant.h>
#include <qurlquery.h>
#include <qjsonobject.h>
#include <qjsondocument.h>
#include <qauthenticator.h>
#include <qoauthhttpserverreplyhandler.h>

#include <functional>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QOAuth2AuthorizationCodeFlow
    \inmodule QtNetworkAuth
    \ingroup oauth
    \brief The QOAuth2AuthorizationCodeFlow class provides an
    implementation of the
    \l {https://tools.ietf.org/html/rfc6749#section-4.1}
    {Authorization Code Grant} flow.
    \since 5.8

    This class implements the
    \l {https://tools.ietf.org/html/rfc6749#section-4.1}
    {Authorization Code Grant} flow, which is used both to obtain and
    to refresh access tokens. It is a redirection-based flow so the
    user will need access to a web browser.
*/

/*!
    \property QOAuth2AuthorizationCodeFlow::accessTokenUrl
    \brief This property holds the URL used to convert the temporary
    code received during the authorization response.

    \b {See also}:
    \l {https://tools.ietf.org/html/rfc6749#section-4.1.3}{Access
    Token Request}
*/

QOAuth2AuthorizationCodeFlowPrivate::QOAuth2AuthorizationCodeFlowPrivate(
        const QUrl &authorizationUrl, const QUrl &accessTokenUrl, const QString &clientIdentifier,
        QNetworkAccessManager *manager) :
    QAbstractOAuth2Private(qMakePair(clientIdentifier, QString()), authorizationUrl, manager),
    accessTokenUrl(accessTokenUrl)
{
    responseType = QStringLiteral("code");
}

void QOAuth2AuthorizationCodeFlowPrivate::_q_handleCallback(const QVariantMap &data)
{
    Q_Q(QOAuth2AuthorizationCodeFlow);
    using Key = QAbstractOAuth2Private::OAuth2KeyString;

    if (status != QAbstractOAuth::Status::NotAuthenticated) {
        qCWarning(loggingCategory) << "Authorization stage: callback in unexpected status:"
                                   << static_cast<int>(status) << ", ignoring the callback";
        return;
    }

    Q_ASSERT(!state.isEmpty());

    const QString error = data.value(Key::error).toString();
    const QString code = data.value(Key::code).toString();
    const QString receivedState = data.value(Key::state).toString();
    if (error.size()) {
        // RFC 6749, Section 5.2 Error Response
        const QString uri = data.value(Key::errorUri).toString();
        const QString description = data.value(Key::errorDescription).toString();
        qCWarning(loggingCategory, "Authorization stage: AuthenticationError: %s(%s): %s",
                  qPrintable(error), qPrintable(uri), qPrintable(description));
        Q_EMIT q->error(error, description, uri);
        // Emit also requestFailed() so that it is a signal for all errors
        emit q->requestFailed(QAbstractOAuth::Error::ServerError);
        return;
    }

    if (code.isEmpty()) {
        qCWarning(loggingCategory, "Authorization stage: Code not received");
        emit q->requestFailed(QAbstractOAuth::Error::OAuthTokenNotFoundError);
        return;
    }
    if (receivedState.isEmpty()) {
        qCWarning(loggingCategory, "Authorization stage: State not received");
        emit q->requestFailed(QAbstractOAuth::Error::ServerError);
        return;
    }
    if (state != receivedState) {
        qCWarning(loggingCategory) << "Authorization stage: State mismatch";
        emit q->requestFailed(QAbstractOAuth::Error::ServerError);
        return;
    }

    setStatus(QAbstractOAuth::Status::TemporaryCredentialsReceived);

    QVariantMap copy(data);
    copy.remove(Key::code);
    extraTokens = copy;
    q->requestAccessToken(code);
}

void QOAuth2AuthorizationCodeFlowPrivate::_q_accessTokenRequestFinished(const QVariantMap &values)
{
    Q_Q(QOAuth2AuthorizationCodeFlow);
    using Key = QAbstractOAuth2Private::OAuth2KeyString;

    if (values.contains(Key::error)) {
        _q_accessTokenRequestFailed(QAbstractOAuth::Error::ServerError,
                                    values.value(Key::error).toString());
        return;
    }

    bool ok;
    const QString accessToken = values.value(Key::accessToken).toString();
    tokenType = values.value(Key::tokenType).toString();
    int expiresIn = values.value(Key::expiresIn).toInt(&ok);
    if (!ok)
        expiresIn = -1;
    if (values.value(Key::refreshToken).isValid())
        q->setRefreshToken(values.value(Key::refreshToken).toString());
    scope = values.value(Key::scope).toString();
    if (accessToken.isEmpty()) {
        _q_accessTokenRequestFailed(QAbstractOAuth::Error::OAuthTokenNotFoundError,
                                    "Access token not received"_L1);
        return;
    }
    q->setToken(accessToken);

    const QDateTime currentDateTime = QDateTime::currentDateTime();
    if (expiresIn > 0 && currentDateTime.secsTo(expiresAt) != expiresIn) {
        expiresAt = currentDateTime.addSecs(expiresIn);
        Q_EMIT q->expirationAtChanged(expiresAt);
    }

    QVariantMap copy(values);
    copy.remove(Key::accessToken);
    copy.remove(Key::expiresIn);
    copy.remove(Key::refreshToken);
    copy.remove(Key::scope);
    copy.remove(Key::tokenType);
    extraTokens.insert(copy);

    setStatus(QAbstractOAuth::Status::Granted);
}

void QOAuth2AuthorizationCodeFlowPrivate::_q_accessTokenRequestFailed(QAbstractOAuth::Error error,
                                                                      const QString& errorString)
{
    Q_Q(QOAuth2AuthorizationCodeFlow);
    qCWarning(loggingCategory) << "Token request failed:" << errorString;
    // If we were refreshing, reset status to Granted if we have an access token.
    // The access token might still be valid, and even if it wouldn't be,
    // refreshing can be attempted again.
    if (q->status() == QAbstractOAuth::Status::RefreshingToken) {
        if (!q->token().isEmpty())
            setStatus(QAbstractOAuth::Status::Granted);
        else
            setStatus(QAbstractOAuth::Status::NotAuthenticated);
    }
    emit q->requestFailed(error);
}

void QOAuth2AuthorizationCodeFlowPrivate::_q_authenticate(QNetworkReply *reply,
                                                          QAuthenticator *authenticator)
{
    if (reply == currentReply){
        const auto url = reply->url();
        if (url == accessTokenUrl) {
            authenticator->setUser(clientIdentifier);
            authenticator->setPassword(QString());
        }
    }
}

/*!
    Constructs a QOAuth2AuthorizationCodeFlow object with parent
    object \a parent.
*/
QOAuth2AuthorizationCodeFlow::QOAuth2AuthorizationCodeFlow(QObject *parent) :
    QOAuth2AuthorizationCodeFlow(nullptr,
                                 parent)
{}

/*!
    Constructs a QOAuth2AuthorizationCodeFlow object using \a parent
    as parent and sets \a manager as the network access manager.
*/
QOAuth2AuthorizationCodeFlow::QOAuth2AuthorizationCodeFlow(QNetworkAccessManager *manager,
                                                           QObject *parent) :
    QOAuth2AuthorizationCodeFlow(QString(),
                                 manager,
                                 parent)
{}

/*!
    Constructs a QOAuth2AuthorizationCodeFlow object using \a parent
    as parent and sets \a manager as the network access manager. The
    client identifier is set to \a clientIdentifier.
*/
QOAuth2AuthorizationCodeFlow::QOAuth2AuthorizationCodeFlow(const QString &clientIdentifier,
                                                           QNetworkAccessManager *manager,
                                                           QObject *parent) :
    QAbstractOAuth2(*new QOAuth2AuthorizationCodeFlowPrivate(QUrl(), QUrl(), clientIdentifier,
                                                             manager),
                    parent)
{}

/*!
    Constructs a QOAuth2AuthorizationCodeFlow object using \a parent
    as parent and sets \a manager as the network access manager. The
    authenticate URL is set to \a authenticateUrl and the access
    token URL is set to \a accessTokenUrl.
*/
QOAuth2AuthorizationCodeFlow::QOAuth2AuthorizationCodeFlow(const QUrl &authenticateUrl,
                                                           const QUrl &accessTokenUrl,
                                                           QNetworkAccessManager *manager,
                                                           QObject *parent) :
    QAbstractOAuth2(*new QOAuth2AuthorizationCodeFlowPrivate(authenticateUrl, accessTokenUrl,
                                                             QString(), manager),
                    parent)
{}

/*!
    Constructs a QOAuth2AuthorizationCodeFlow object using \a parent
    as parent and sets \a manager as the network access manager. The
    client identifier is set to \a clientIdentifier the authenticate
    URL is set to \a authenticateUrl and the access token URL is set
    to \a accessTokenUrl.
*/
QOAuth2AuthorizationCodeFlow::QOAuth2AuthorizationCodeFlow(const QString &clientIdentifier,
                                                           const QUrl &authenticateUrl,
                                                           const QUrl &accessTokenUrl,
                                                           QNetworkAccessManager *manager,
                                                           QObject *parent) :
    QAbstractOAuth2(*new QOAuth2AuthorizationCodeFlowPrivate(authenticateUrl, accessTokenUrl,
                                                             clientIdentifier, manager),
                    parent)
{}

/*!
    Destroys the QOAuth2AuthorizationCodeFlow instance.
*/
QOAuth2AuthorizationCodeFlow::~QOAuth2AuthorizationCodeFlow()
{}

/*!
    Returns the URL used to request the access token.
    \sa setAccessTokenUrl()
*/
QUrl QOAuth2AuthorizationCodeFlow::accessTokenUrl() const
{
    Q_D(const QOAuth2AuthorizationCodeFlow);
    return d->accessTokenUrl;
}

/*!
    Sets the URL used to request the access token to
    \a accessTokenUrl.
*/
void QOAuth2AuthorizationCodeFlow::setAccessTokenUrl(const QUrl &accessTokenUrl)
{
    Q_D(QOAuth2AuthorizationCodeFlow);
    if (d->accessTokenUrl != accessTokenUrl) {
        d->accessTokenUrl = accessTokenUrl;
        Q_EMIT accessTokenUrlChanged(accessTokenUrl);
    }
}

/*!
    Starts the authentication flow as described in
    \l {https://tools.ietf.org/html/rfc6749#section-4.1}{The OAuth
    2.0 Authorization Framework}
*/
void QOAuth2AuthorizationCodeFlow::grant()
{
    Q_D(QOAuth2AuthorizationCodeFlow);
    if (d->authorizationUrl.isEmpty()) {
        qCWarning(d->loggingCategory, "No authenticate Url set");
        return;
    }
    if (d->accessTokenUrl.isEmpty()) {
        qCWarning(d->loggingCategory, "No request access token Url set");
        return;
    }

    resourceOwnerAuthorization(d->authorizationUrl);
}

/*!
    Call this function to refresh the token. Access tokens are not
    permanent. After a time specified along with the access token
    when it was obtained, the access token will become invalid.

    If refreshing the token fails and an access token exists, the status is
    set to QAbstractOAuth::Status::Granted, and to
    QAbstractOAuth::Status::NotAuthenticated otherwise.

    \sa QAbstractOAuth::requestFailed()
    \sa {https://tools.ietf.org/html/rfc6749#section-1.5}{Refresh
    Token}
*/
void QOAuth2AuthorizationCodeFlow::refreshAccessToken()
{
    Q_D(QOAuth2AuthorizationCodeFlow);

    if (d->refreshToken.isEmpty()) {
        qCWarning(d->loggingCategory, "Cannot refresh access token. Empty refresh token");
        return;
    }
    if (d->status == Status::RefreshingToken) {
        qCWarning(d->loggingCategory, "Cannot refresh access token. "
                                      "Refresh Access Token is already in progress");
        return;
    }

    using Key = QAbstractOAuth2Private::OAuth2KeyString;

    QMultiMap<QString, QVariant> parameters;
    QNetworkRequest request(d->accessTokenUrl);
#ifndef QT_NO_SSL
    if (d->sslConfiguration && !d->sslConfiguration->isNull())
        request.setSslConfiguration(*d->sslConfiguration);
#endif
    QUrlQuery query;
    parameters.insert(Key::grantType, QStringLiteral("refresh_token"));
    parameters.insert(Key::refreshToken, d->refreshToken);
    parameters.insert(Key::redirectUri, QUrl::toPercentEncoding(callback()));
    parameters.insert(Key::clientIdentifier, d->clientIdentifier);
    parameters.insert(Key::clientSharedSecret, d->clientIdentifierSharedKey);
    if (d->modifyParametersFunction)
        d->modifyParametersFunction(Stage::RefreshingAccessToken, &parameters);
    query = QAbstractOAuthPrivate::createQuery(parameters);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/x-www-form-urlencoded"));

    const QString data = query.toString(QUrl::FullyEncoded);
    d->currentReply = d->networkAccessManager()->post(request, data.toUtf8());
    setStatus(Status::RefreshingToken);

    QNetworkReply *reply = d->currentReply.data();
    QAbstractOAuthReplyHandler *handler = replyHandler();
    connect(reply, &QNetworkReply::finished,
            [handler, reply]() { handler->networkReplyFinished(reply); });
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    QObjectPrivate::connect(d->replyHandler.data(), &QAbstractOAuthReplyHandler::tokensReceived, d,
                            &QOAuth2AuthorizationCodeFlowPrivate::_q_accessTokenRequestFinished,
                            Qt::UniqueConnection);
    QObjectPrivate::connect(d->networkAccessManager(),
                            &QNetworkAccessManager::authenticationRequired,
                            d, &QOAuth2AuthorizationCodeFlowPrivate::_q_authenticate,
                            Qt::UniqueConnection);
    QObjectPrivate::connect(d->replyHandler.data(),
                            &QAbstractOAuthReplyHandler::tokenRequestErrorOccurred,
                            d, &QOAuth2AuthorizationCodeFlowPrivate::_q_accessTokenRequestFailed,
                            Qt::UniqueConnection);
}

/*!
    Generates an authentication URL to be used in the
    \l {https://tools.ietf.org/html/rfc6749#section-4.1.1}
    {Authorization Request} using \a parameters.
*/
QUrl QOAuth2AuthorizationCodeFlow::buildAuthenticateUrl(const QMultiMap<QString, QVariant> &parameters)
{
    Q_D(QOAuth2AuthorizationCodeFlow);
    using Key = QAbstractOAuth2Private::OAuth2KeyString;

    if (d->state.isEmpty())
        setState(QAbstractOAuth2Private::generateRandomState());
    Q_ASSERT(!d->state.isEmpty());
    const QString state = d->state;

    QMultiMap<QString, QVariant> p(parameters);
    QUrl url(d->authorizationUrl);
    p.insert(Key::responseType, responseType());
    p.insert(Key::clientIdentifier, d->clientIdentifier);
    p.insert(Key::redirectUri, callback());
    p.insert(Key::scope, d->scope);
    p.insert(Key::state, state);
    if (d->modifyParametersFunction)
        d->modifyParametersFunction(Stage::RequestingAuthorization, &p);
    url.setQuery(d->createQuery(p));
    connect(d->replyHandler.data(), &QAbstractOAuthReplyHandler::callbackReceived, this,
            &QOAuth2AuthorizationCodeFlow::authorizationCallbackReceived, Qt::UniqueConnection);
    setStatus(QAbstractOAuth::Status::NotAuthenticated);
    qCDebug(d->loggingCategory, "Generated URL: %s", qPrintable(url.toString()));
    return url;
}

/*!
    Requests an access token from the received \a code. The \a code
    is received as a response when the user completes a successful
    authentication in the browser.
*/
void QOAuth2AuthorizationCodeFlow::requestAccessToken(const QString &code)
{
    Q_D(QOAuth2AuthorizationCodeFlow);
    using Key = QAbstractOAuth2Private::OAuth2KeyString;

    QMultiMap<QString, QVariant> parameters;
    QNetworkRequest request(d->accessTokenUrl);
#ifndef QT_NO_SSL
    if (d->sslConfiguration && !d->sslConfiguration->isNull())
        request.setSslConfiguration(*d->sslConfiguration);
#endif
    QUrlQuery query;
    parameters.insert(Key::grantType, QStringLiteral("authorization_code"));
    parameters.insert(Key::code, QUrl::toPercentEncoding(code));
    parameters.insert(Key::redirectUri, QUrl::toPercentEncoding(callback()));
    parameters.insert(Key::clientIdentifier, QUrl::toPercentEncoding(d->clientIdentifier));
    if (!d->clientIdentifierSharedKey.isEmpty())
        parameters.insert(Key::clientSharedSecret, d->clientIdentifierSharedKey);
    if (d->modifyParametersFunction)
        d->modifyParametersFunction(Stage::RequestingAccessToken, &parameters);
    query = QAbstractOAuthPrivate::createQuery(parameters);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/x-www-form-urlencoded"));

    const QString data = query.toString(QUrl::FullyEncoded);
    QNetworkReply *reply = d->networkAccessManager()->post(request, data.toUtf8());
    d->currentReply = reply;
    QAbstractOAuthReplyHandler *handler = replyHandler();
    QObject::connect(reply, &QNetworkReply::finished,
                     [handler, reply] { handler->networkReplyFinished(reply); });
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    QObjectPrivate::connect(d->replyHandler.data(), &QAbstractOAuthReplyHandler::tokensReceived, d,
                            &QOAuth2AuthorizationCodeFlowPrivate::_q_accessTokenRequestFinished,
                            Qt::UniqueConnection);
    QObjectPrivate::connect(d->networkAccessManager(),
                            &QNetworkAccessManager::authenticationRequired,
                            d, &QOAuth2AuthorizationCodeFlowPrivate::_q_authenticate,
                            Qt::UniqueConnection);
    QObjectPrivate::connect(d->replyHandler.data(),
                            &QAbstractOAuthReplyHandler::tokenRequestErrorOccurred,
                            d, &QOAuth2AuthorizationCodeFlowPrivate::_q_accessTokenRequestFailed,
                            Qt::UniqueConnection);
}

/*!
    Builds an authentication URL using \a url and \a parameters. This
    function emits an authorizeWithBrowser() signal to require user
    interaction.
*/
void QOAuth2AuthorizationCodeFlow::resourceOwnerAuthorization(const QUrl &url,
                                                              const QMultiMap<QString, QVariant> &parameters)
{
    Q_D(QOAuth2AuthorizationCodeFlow);
    if (Q_UNLIKELY(url != d->authorizationUrl)) {
        qCWarning(d->loggingCategory, "Invalid URL: %s", qPrintable(url.toString()));
        return;
    }
    const QUrl u = buildAuthenticateUrl(parameters);
    QObjectPrivate::connect(this, &QOAuth2AuthorizationCodeFlow::authorizationCallbackReceived, d,
                            &QOAuth2AuthorizationCodeFlowPrivate::_q_handleCallback,
                            Qt::UniqueConnection);
    Q_EMIT authorizeWithBrowser(u);
}

QT_END_NAMESPACE

#include "moc_qoauth2authorizationcodeflow.cpp"

#endif // QT_NO_HTTP
