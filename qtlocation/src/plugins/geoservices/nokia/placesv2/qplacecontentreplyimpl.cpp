/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtLocation module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "jsonparserhelpers.h"
#include "qplacecontentreplyimpl.h"
#include "../qplacemanagerengine_nokiav2.h"
#include "../qgeoerror_messages.h"

#include <QCoreApplication>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

QT_BEGIN_NAMESPACE

QPlaceContentReplyImpl::QPlaceContentReplyImpl(const QPlaceContentRequest &request,
                                               QNetworkReply *reply,
                                               QPlaceManagerEngineNokiaV2 *engine)
    :   QPlaceContentReply(engine), m_engine(engine)
{
    Q_ASSERT(engine);
    if (!reply) {
        setError(UnknownError, QStringLiteral("Null reply"));
        return;
    }
    setRequest(request);

    connect(reply, SIGNAL(finished()), this, SLOT(replyFinished()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(replyError(QNetworkReply::NetworkError)));
    connect(this, &QPlaceReply::aborted, reply, &QNetworkReply::abort);
    connect(this, &QObject::destroyed, reply, &QObject::deleteLater);
}

QPlaceContentReplyImpl::~QPlaceContentReplyImpl()
{
}

void QPlaceContentReplyImpl::setError(QPlaceReply::Error error_, const QString &errorString)
{
    QPlaceContentReply::setError(error_, errorString);
    emit error(error_, errorString);
    setFinished(true);
    emit finished();
}

void QPlaceContentReplyImpl::replyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
        return;

    QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
    if (!document.isObject()) {
        setError(ParseError, QCoreApplication::translate(NOKIA_PLUGIN_CONTEXT_NAME, PARSE_ERROR));
        return;
    }

    QJsonObject object = document.object();

    QPlaceContent::Collection collection;
    int totalCount;
    QPlaceContentRequest previous;
    QPlaceContentRequest next;

    parseCollection(request().contentType(), object, &collection, &totalCount,
                    &previous, &next, m_engine);

    setTotalCount(totalCount);
    setContent(collection);
    setPreviousPageRequest(previous);
    setNextPageRequest(next);

    setFinished(true);
    emit finished();
}

void QPlaceContentReplyImpl::replyError(QNetworkReply::NetworkError error)
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if (error == QNetworkReply::OperationCanceledError)
        setError(QPlaceReply::CancelError, QStringLiteral("Request cancelled"));
    else
        setError(QPlaceReply::CommunicationError, reply->errorString());
}

QT_END_NAMESPACE
