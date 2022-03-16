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

#include "qplacesearchreplyhere.h"
#include "jsonparserhelpers.h"
#include "../qplacemanagerengine_nokiav2.h"
#include "../qgeoerror_messages.h"

#include <QCoreApplication>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtLocation/QPlaceIcon>
#include <QtLocation/QPlaceResult>
#include <QtLocation/QPlaceProposedSearchResult>
#include <QtLocation/private/qplacesearchrequest_p.h>

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

QPlaceSearchReplyHere::QPlaceSearchReplyHere(const QPlaceSearchRequest &request,
                                             QNetworkReply *reply,
                                             QPlaceManagerEngineNokiaV2 *parent)
    :   QPlaceSearchReply(parent), m_engine(parent)
{
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

QPlaceSearchReplyHere::~QPlaceSearchReplyHere()
{
}

void QPlaceSearchReplyHere::setError(QPlaceReply::Error error_, const QString &errorString)
{
    QPlaceReply::setError(error_, errorString);
    emit error(error_, errorString);
    setFinished(true);
    emit finished();
}

void QPlaceSearchReplyHere::replyFinished()
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

    QJsonObject resultsObject = document.object();

    if (resultsObject.contains(QStringLiteral("results")))
        resultsObject = resultsObject.value(QStringLiteral("results")).toObject();

    QJsonArray items = resultsObject.value(QStringLiteral("items")).toArray();

    QList<QPlaceSearchResult> results;
    for (int i = 0; i < items.count(); ++i) {
        QJsonObject item = items.at(i).toObject();

        const QString type = item.value(QStringLiteral("type")).toString();
        if (type == QStringLiteral("urn:nlp-types:place"))
            results.append(parsePlaceResult(item));
        else if (type == QStringLiteral("urn:nlp-types:search"))
            results.append(parseSearchResult(item));
    }

    QPlaceSearchRequest r_orig = request();
    QPlaceSearchRequestPrivate *rpimpl_orig = QPlaceSearchRequestPrivate::get(r_orig);

    if (resultsObject.contains(QStringLiteral("next"))) {
        QPlaceSearchRequest request;
        request.setSearchContext(QUrl(resultsObject.value(QStringLiteral("next")).toString()));
        QPlaceSearchRequestPrivate *rpimpl = QPlaceSearchRequestPrivate::get(request);
        rpimpl->related = true;
        rpimpl->page = rpimpl_orig->page + 1;
        setNextPageRequest(request);
    }

    if (resultsObject.contains(QStringLiteral("previous"))) {
        QPlaceSearchRequest request;
        request.setSearchContext(QUrl(resultsObject.value(QStringLiteral("previous")).toString()));
        QPlaceSearchRequestPrivate *rpimpl = QPlaceSearchRequestPrivate::get(request);
        rpimpl->related = true;
        rpimpl->page = rpimpl_orig->page - 1;
        setPreviousPageRequest(request);
    }

    setResults(results);

    setFinished(true);
    emit finished();
}

QPlaceResult QPlaceSearchReplyHere::parsePlaceResult(const QJsonObject &item) const
{
    QPlaceResult result;

    if (item.contains(QStringLiteral("distance")))
        result.setDistance(item.value(QStringLiteral("distance")).toDouble());

    QPlace place;

    QGeoLocation location;

    location.setCoordinate(parseCoordinate(item.value(QStringLiteral("position")).toArray()));

    const QString vicinity = item.value(QStringLiteral("vicinity")).toString();
    QGeoAddress address;
    address.setText(vicinity);
    location.setAddress(address);

    if (item.contains(QStringLiteral("bbox"))) {
        QJsonArray bbox = item.value(QStringLiteral("bbox")).toArray();
        QGeoRectangle box(QGeoCoordinate(bbox.at(3).toDouble(), bbox.at(0).toDouble()),
                            QGeoCoordinate(bbox.at(1).toDouble(), bbox.at(2).toDouble()));
        location.setBoundingBox(box);
    }

    place.setLocation(location);

    QPlaceRatings ratings;
    ratings.setAverage(item.value(QStringLiteral("averageRating")).toDouble());
    ratings.setMaximum(5.0);
    place.setRatings(ratings);

    const QString title = item.value(QStringLiteral("title")).toString();
    place.setName(title);
    result.setTitle(title);

    QPlaceIcon icon = m_engine->icon(item.value(QStringLiteral("icon")).toString());
    place.setIcon(icon);
    result.setIcon(icon);

    place.setCategory(parseCategory(item.value(QStringLiteral("category")).toObject(),
                                    m_engine));

    //QJsonArray having = item.value(QStringLiteral("having")).toArray();

    result.setSponsored(item.value(QStringLiteral("sponsored")).toBool());

    QUrl href = item.value(QStringLiteral("href")).toString();
    //QUrl type = item.value(QStringLiteral("type")).toString();

    place.setPlaceId(href.path().mid(18, 41));

    QPlaceAttribute provider;
    provider.setText(QStringLiteral("here"));
    place.setExtendedAttribute(QPlaceAttribute::Provider, provider);
    place.setVisibility(QLocation::PublicVisibility);

    result.setPlace(place);

    return result;
}

QPlaceProposedSearchResult QPlaceSearchReplyHere::parseSearchResult(const QJsonObject &item) const
{
    QPlaceProposedSearchResult result;

    result.setTitle(item.value(QStringLiteral("title")).toString());

    QPlaceIcon icon = m_engine->icon(item.value(QStringLiteral("icon")).toString());
    result.setIcon(icon);

    QPlaceSearchRequest request;
    request.setSearchContext(QUrl(item.value("href").toString()));

    result.setSearchRequest(request);

    return result;
}

void QPlaceSearchReplyHere::replyError(QNetworkReply::NetworkError error)
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if (error == QNetworkReply::OperationCanceledError) {
        setError(QPlaceReply::CancelError, QCoreApplication::translate(NOKIA_PLUGIN_CONTEXT_NAME, CANCEL_ERROR));
    } else if (error == QNetworkReply::ContentNotFoundError) {
        setError(QPlaceReply::PlaceDoesNotExistError,
                 QString::fromLatin1("The id, %1, does not reference an existing place")
                 .arg(request().recommendationId()));
    } else {
        setError(QPlaceReply::CommunicationError, QCoreApplication::translate(NOKIA_PLUGIN_CONTEXT_NAME, NETWORK_ERROR));
    }
}

QT_END_NAMESPACE
