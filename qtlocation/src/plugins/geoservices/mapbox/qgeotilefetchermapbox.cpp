// Copyright (C) 2014 Canonical Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgeotilefetchermapbox.h"
#include "qgeomapreplymapbox.h"
#include "qmapboxcommon.h"

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtLocation/private/qgeotilefetcher_p.h>
#include <QtLocation/private/qgeotiledmappingmanagerengine_p.h>
#include <QtLocation/private/qgeotilespec_p.h>
#include <QDebug>

QT_BEGIN_NAMESPACE

QGeoTileFetcherMapbox::QGeoTileFetcherMapbox(int scaleFactor, QGeoTiledMappingManagerEngine *parent)
:   QGeoTileFetcher(parent), m_networkManager(new QNetworkAccessManager(this)),
    m_userAgent(mapboxDefaultUserAgent),
    m_format("png"),
    m_replyFormat("png"),
    m_accessToken("")
{
    m_scaleFactor = qBound(1, scaleFactor, 2);
}

void QGeoTileFetcherMapbox::setUserAgent(const QByteArray &userAgent)
{
    m_userAgent = userAgent;
}

void QGeoTileFetcherMapbox::setMapIds(const QList<QString> &mapIds)
{
    m_mapIds = mapIds;
}

void QGeoTileFetcherMapbox::setFormat(const QString &format)
{
    m_format = format;

    if (m_format == "png" || m_format == "png32" || m_format == "png64" || m_format == "png128" || m_format == "png256")
        m_replyFormat = "png";
    else if (m_format == "jpg70" || m_format == "jpg80" || m_format == "jpg90")
        m_replyFormat = "jpg";
    else
        qWarning() << "Unknown map format " << m_format;
}

void QGeoTileFetcherMapbox::setAccessToken(const QString &accessToken)
{
    m_accessToken = accessToken;
}

QGeoTiledMapReply *QGeoTileFetcherMapbox::getTileImage(const QGeoTileSpec &spec)
{
    QNetworkRequest request;
    request.setRawHeader("User-Agent", m_userAgent);

    request.setUrl(QUrl(mapboxTilesApiPath +
                        ((spec.mapId() >= m_mapIds.size()) ? QStringLiteral("mapbox.streets") : m_mapIds[spec.mapId() - 1]) + QLatin1Char('/') +
                        QString::number(spec.zoom()) + QLatin1Char('/') +
                        QString::number(spec.x()) + QLatin1Char('/') +
                        QString::number(spec.y()) +
                        ((m_scaleFactor > 1) ? (QLatin1Char('@') + QString::number(m_scaleFactor) + QLatin1String("x.")) : QLatin1String(".")) +
                        m_format + QLatin1Char('?') +
                        QStringLiteral("access_token=") + m_accessToken));

    QNetworkReply *reply = m_networkManager->get(request);

    return new QGeoMapReplyMapbox(reply, spec, m_replyFormat);
}

QT_END_NAMESPACE
