// Copyright (C) 2016 Aaron McCarthy <mccarthy.aaron@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplacesearchreplyosm.h"
#include "qplacemanagerengineosm.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkReply>
#include <QtPositioning/QGeoCircle>
#include <QtPositioning/QGeoRectangle>
#include <QtPositioning/QGeoLocation>
#include <QtPositioning/QGeoAddress>
#include <QtLocation/QPlace>
#include <QtLocation/QPlaceAttribute>
#include <QtLocation/QPlaceIcon>
#include <QtLocation/QPlaceResult>
#include <QtLocation/QPlaceCategory>
#include <QtLocation/QPlaceSearchRequest>
#include <QtLocation/private/qplacesearchrequest_p.h>

QT_BEGIN_NAMESPACE

QPlaceSearchReplyOsm::QPlaceSearchReplyOsm(const QPlaceSearchRequest &request,
                                             QNetworkReply *reply, QPlaceManagerEngineOsm *parent)
:   QPlaceSearchReply(parent)
{
    Q_ASSERT(parent);
    if (!reply) {
        setError(UnknownError, QStringLiteral("Null reply"));
        return;
    }
    setRequest(request);

    connect(reply, &QNetworkReply::finished,
            this, &QPlaceSearchReplyOsm::replyFinished);
    connect(reply, &QNetworkReply::errorOccurred,
            this, &QPlaceSearchReplyOsm::networkError);
    connect(this, &QPlaceReply::aborted, reply, &QNetworkReply::abort);
    connect(this, &QObject::destroyed, reply, &QObject::deleteLater);
}

QPlaceSearchReplyOsm::~QPlaceSearchReplyOsm()
{
}

void QPlaceSearchReplyOsm::setError(QPlaceReply::Error errorCode, const QString &errorString)
{
    QPlaceReply::setError(errorCode, errorString);
    emit errorOccurred(errorCode, errorString);
    setFinished(true);
    emit finished();
}

static QGeoRectangle parseBoundingBox(const QJsonArray &coordinates)
{
    if (coordinates.count() != 4)
        return QGeoRectangle();

    double bottom = coordinates.at(0).toString().toDouble();
    double top = coordinates.at(1).toString().toDouble();
    double left = coordinates.at(2).toString().toDouble();
    double right = coordinates.at(3).toString().toDouble();

    return QGeoRectangle(QGeoCoordinate(top, left), QGeoCoordinate(bottom, right));
}

void QPlaceSearchReplyOsm::replyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
        return;

    QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
    if (!document.isArray()) {
        setError(ParseError, tr("Response parse error"));
        return;
    }

    QJsonArray resultsArray = document.array();

    QGeoCoordinate searchCenter = request().searchArea().center();

    QStringList placeIds;

    QList<QPlaceSearchResult> results;
    for (int i = 0; i < resultsArray.count(); ++i) {
        QJsonObject item = resultsArray.at(i).toObject();
        QPlaceResult pr = parsePlaceResult(item);
        pr.setDistance(searchCenter.distanceTo(pr.place().location().coordinate()));
        placeIds.append(pr.place().placeId());
        results.append(pr);
    }

    QVariantMap searchContext = request().searchContext().toMap();
    QStringList excludePlaceIds =
        searchContext.value(QStringLiteral("ExcludePlaceIds")).toStringList();

    if (!excludePlaceIds.isEmpty()) {
        QPlaceSearchRequest r = request();
        QVariantMap parameters = searchContext;

        QStringList epi = excludePlaceIds;
        epi.removeLast();

        parameters.insert(QStringLiteral("ExcludePlaceIds"), epi);
        r.setSearchContext(parameters);
        QPlaceSearchRequestPrivate *rpimpl = QPlaceSearchRequestPrivate::get(r);
        rpimpl->related = true;
        rpimpl->page--;
        setPreviousPageRequest(r);
    }

    if (!placeIds.isEmpty()) {
        QPlaceSearchRequest r = request();
        QVariantMap parameters = searchContext;

        QStringList epi = excludePlaceIds;
        epi.append(placeIds.join(QLatin1Char(',')));

        parameters.insert(QStringLiteral("ExcludePlaceIds"), epi);
        r.setSearchContext(parameters);
        QPlaceSearchRequestPrivate *rpimpl = QPlaceSearchRequestPrivate::get(r);
        rpimpl->related = true;
        rpimpl->page++;
        setNextPageRequest(r);
    }

    setResults(results);

    setFinished(true);
    emit finished();
}

void QPlaceSearchReplyOsm::networkError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error);
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    setError(QPlaceReply::CommunicationError, reply->errorString());
}

QPlaceResult QPlaceSearchReplyOsm::parsePlaceResult(const QJsonObject &item) const
{
    QPlace place;

    QGeoCoordinate coordinate = QGeoCoordinate(item.value(QStringLiteral("lat")).toString().toDouble(),
                                               item.value(QStringLiteral("lon")).toString().toDouble());

    //const QString placeRank = item.value(QStringLiteral("place_rank")).toString();
    const QString categoryName = item.value(QStringLiteral("category")).toString();
    const QString type = item.value(QStringLiteral("type")).toString();
    //double importance = item.value(QStringLiteral("importance")).toDouble();

    place.setAttribution(item.value(QStringLiteral("licence")).toString());
    place.setPlaceId(QString::number(item.value(QStringLiteral("place_id")).toInt()));

    QVariantMap iconParameters;
    iconParameters.insert(QPlaceIcon::SingleUrl,
                          QUrl(item.value(QStringLiteral("icon")).toString()));
    QPlaceIcon icon;
    icon.setParameters(iconParameters);
    place.setIcon(icon);

    QJsonObject addressDetails = item.value(QStringLiteral("address")).toObject();
    const QString title = addressDetails.value(categoryName).toString();

    place.setName(title);

    if (!requestUrl.isEmpty()) {
        QPlaceAttribute attribute;
        attribute.setLabel("requestUrl");
        attribute.setText(requestUrl);
        place.setExtendedAttribute("requestUrl", attribute);
    }

    QGeoAddress address;
    address.setCity(addressDetails.value(QStringLiteral("city")).toString());
    address.setCountry(addressDetails.value(QStringLiteral("country")).toString());
    // FIXME: country_code is alpha-2 setCountryCode takes alpha-3
    //address.setCountryCode(addressDetails.value(QStringLiteral("country_code")).toString());
    address.setPostalCode(addressDetails.value(QStringLiteral("postcode")).toString());
    address.setStreet(addressDetails.value(QStringLiteral("road")).toString());
    address.setStreetNumber(addressDetails.value(QStringLiteral("house_number")).toString());
    address.setState(addressDetails.value(QStringLiteral("state")).toString());
    address.setDistrict(addressDetails.value(QStringLiteral("suburb")).toString());

    QGeoLocation location;
    location.setCoordinate(coordinate);
    location.setAddress(address);
    location.setBoundingShape(parseBoundingBox(item.value(QStringLiteral("boundingbox")).toArray()));
    place.setLocation(location);

    QPlaceCategory category;
    category.setName(categoryName + "=" + type);
    category.setCategoryId(categoryName + "=" + type);
    place.setCategory(category);

    QPlaceResult result;
    result.setIcon(icon);
    result.setPlace(place);
    result.setTitle(title);

    return result;
}

QT_END_NAMESPACE
