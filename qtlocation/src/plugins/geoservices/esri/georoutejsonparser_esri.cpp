// Copyright (C) 2013-2018 Esri <contracts@esri.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "georoutejsonparser_esri.h"

#include <QJsonArray>
#include <QGeoRectangle>
#include <QGeoManeuver>
#include <QGeoRouteSegment>

QT_BEGIN_NAMESPACE

// JSON reference:
// https://resources.arcgis.com/en/help/arcgis-rest-api/#/Route_service_with_synchronous_execution/02r300000036000000/

static const QString kErrorMessage(QStringLiteral("Error %1: %2."));
static const QString kErrorJson(QStringLiteral("Error: invalide JSON document."));

static const QString kErrorKey(QStringLiteral("error"));
static const QString kErrorCodeKey(QStringLiteral("code"));
static const QString kErrorMessageKey(QStringLiteral("message"));
static const QString kErrorDetailsKey(QStringLiteral("details"));
static const QString kDirectionsKey(QStringLiteral("directions"));
static const QString kRoutesKey(QStringLiteral("routes"));
static const QString kBarriersKey(QStringLiteral("barriers"));
static const QString kMessagesKey(QStringLiteral("messages"));
static const QString kDirectionsRouteIdKey(QStringLiteral("routeId"));
static const QString kDirectionsRouteNameKey(QStringLiteral("routeName"));
static const QString kDirectionsSummaryKey(QStringLiteral("summary"));
static const QString kDirectionsTotalLengthKey(QStringLiteral("totalLength"));
static const QString kDirectionsTotalTimeKey(QStringLiteral("totalTime"));
static const QString kDirectionsTotalDriveTimeKey(QStringLiteral("totalDriveTime"));
static const QString kDirectionsEnvelopeKey(QStringLiteral("envelope"));
static const QString kDirectionsEnvelopeXminKey(QStringLiteral("xmin"));
static const QString kDirectionsEnvelopeYminKey(QStringLiteral("ymin"));
static const QString kDirectionsEnvelopeXmaxKey(QStringLiteral("xmax"));
static const QString kDirectionsEnvelopeYmaxKey(QStringLiteral("ymax"));
static const QString kDirectionsFeaturesKey(QStringLiteral("features"));
static const QString kDirectionsFeaturesAttributesKey(QStringLiteral("attributes"));
static const QString kDirectionsFeaturesCompressedGeometryKey(QStringLiteral("compressedGeometry"));
static const QString kDirectionsFeaturesAttributesLengthKey(QStringLiteral("length"));
static const QString kDirectionsFeaturesAttributesTimeKey(QStringLiteral("time"));
static const QString kDirectionsFeaturesAttributesTextKey(QStringLiteral("text"));
static const QString kDirectionsFeaturesAttributesEtaKey(QStringLiteral("ETA"));
static const QString kDirectionsFeaturesAttributesManeuverTypeKey(QStringLiteral("maneuverType"));
static const QString kRoutesFeaturesKey(QStringLiteral("features"));
static const QString kRoutesFeaturesAttributesKey(QStringLiteral("attributes"));
static const QString kRoutesFeaturesObjectIdKey(QStringLiteral("ObjectID"));
static const QString kRoutesFeaturesGeometryKey(QStringLiteral("geometry"));
static const QString kRoutesFeaturesGeometryPathsKey(QStringLiteral("paths"));

GeoRouteJsonParserEsri::GeoRouteJsonParserEsri(const QJsonDocument &document)
{
    if (!document.isObject())
    {
        m_error = kErrorJson;
        return;
    }

    m_json = document.object();
    if (m_json.contains(kErrorKey))
    {
        QJsonObject error = m_json.value(kErrorKey).toObject();
        int code = error.value(kErrorCodeKey).toInt();
        QString message = error.value(kErrorMessageKey).toString();

        m_error = kErrorMessage.arg(code).arg(message);
        return;
    }

    parseDirections();
    parseRoutes();
}

QList<QGeoRoute> GeoRouteJsonParserEsri::routes() const
{
    return m_routes.values();
}

bool GeoRouteJsonParserEsri::isValid() const
{
    return (m_error.isEmpty());
}

QString GeoRouteJsonParserEsri::errorString() const
{
    return m_error;
}

void GeoRouteJsonParserEsri::parseDirections()
{
    QJsonArray directions = m_json.value(kDirectionsKey).toArray();
    for (const QJsonValueRef direction : directions)
        parseDirection(direction.toObject());
}

void GeoRouteJsonParserEsri::parseDirection(const QJsonObject &direction)
{
    QGeoRoute &geoRoute = m_routes[direction.value(kDirectionsRouteIdKey).toInt()];

    // parse summary
    geoRoute.setRouteId(direction.value(kDirectionsRouteNameKey).toString());

    QJsonObject summary = direction.value(kDirectionsSummaryKey).toObject();
    geoRoute.setDistance(summary.value(kDirectionsTotalLengthKey).toDouble());

    geoRoute.setTravelTime(summary.value(kDirectionsTotalTimeKey).toDouble() * 60);
        // default units is minutes, see directionsTimeAttributeName param

    geoRoute.setTravelMode(QGeoRouteRequest::CarTravel);
        // default request is time for car, see directionsTimeAttributeName param

    QJsonObject enveloppe = summary.value(kDirectionsEnvelopeKey).toObject();

    QGeoCoordinate topLeft(enveloppe.value(kDirectionsEnvelopeXminKey).toDouble(),
                   enveloppe.value(kDirectionsEnvelopeYmaxKey).toDouble());
    QGeoCoordinate bottomRight(enveloppe.value(kDirectionsEnvelopeXmaxKey).toDouble(),
                   enveloppe.value(kDirectionsEnvelopeYminKey).toDouble());
    geoRoute.setBounds(QGeoRectangle(topLeft, bottomRight));

    // parse features
    QJsonArray features = direction.value(kDirectionsFeaturesKey).toArray();

    static const QMap<QString, QGeoManeuver::InstructionDirection> esriDirectionsManeuverTypes
    {
        { QStringLiteral("esriDMTUnknown"), QGeoManeuver::NoDirection },
        { QStringLiteral("esriDMTStop"), QGeoManeuver::NoDirection },
        { QStringLiteral("esriDMTStraight"), QGeoManeuver::DirectionForward },
        { QStringLiteral("esriDMTBearLeft"), QGeoManeuver::DirectionBearLeft },
        { QStringLiteral("esriDMTBearRight"), QGeoManeuver::DirectionBearRight },
        { QStringLiteral("esriDMTTurnLeft"), QGeoManeuver::DirectionLeft },
        { QStringLiteral("esriDMTTurnRight"), QGeoManeuver::DirectionRight },
        { QStringLiteral("esriDMTSharpLeft"), QGeoManeuver::DirectionLightLeft },
        { QStringLiteral("esriDMTSharpRight"), QGeoManeuver::DirectionLightRight },
        { QStringLiteral("esriDMTUTurn"), QGeoManeuver::DirectionUTurnRight },
        { QStringLiteral("esriDMTFerry"), QGeoManeuver::NoDirection },
        { QStringLiteral("esriDMTRoundabout"), QGeoManeuver::NoDirection },
        { QStringLiteral("esriDMTHighwayMerge"), QGeoManeuver::NoDirection },
        { QStringLiteral("esriDMTHighwayExit"), QGeoManeuver::NoDirection },
        { QStringLiteral("esriDMTHighwayChange"), QGeoManeuver::NoDirection },
        { QStringLiteral("esriDMTForkCenter"), QGeoManeuver::NoDirection },
        { QStringLiteral("esriDMTForkLeft"), QGeoManeuver::NoDirection },
        { QStringLiteral("esriDMTForkRight"), QGeoManeuver::NoDirection },
        { QStringLiteral("esriDMTDepart"), QGeoManeuver::NoDirection },
        { QStringLiteral("esriDMTTripItem"), QGeoManeuver::NoDirection },
        { QStringLiteral("esriDMTEndOfFerry"), QGeoManeuver::NoDirection }
    };

    QGeoRouteSegment firstSegment;
    for (qsizetype i = features.size() - 1; i >= 0; --i)
    {
        QJsonObject feature = features.at(i).toObject();
        QJsonObject attributes = feature.value(kDirectionsFeaturesAttributesKey).toObject();

        QGeoRouteSegment segment;
        double length = attributes.value(kDirectionsFeaturesAttributesLengthKey).toDouble();
        segment.setDistance(length);

        double time = attributes.value(kDirectionsFeaturesAttributesTimeKey).toDouble() * 60;
            // default units is minutes, see directionsTimeAttributeName param
        segment.setTravelTime(time);

        QGeoManeuver maneuver;
        QString type = attributes.value(kDirectionsFeaturesAttributesManeuverTypeKey).toString();
        maneuver.setDirection(esriDirectionsManeuverTypes.value(type));

        maneuver.setInstructionText(attributes.value(kDirectionsFeaturesAttributesTextKey).toString() + ".");
        maneuver.setDistanceToNextInstruction(length);
        maneuver.setTimeToNextInstruction(time);

        segment.setManeuver(maneuver);

        segment.setNextRouteSegment(firstSegment);
        firstSegment = segment;
    }
    geoRoute.setFirstRouteSegment(firstSegment);
}

void GeoRouteJsonParserEsri::parseRoutes()
{
    QJsonObject routes = m_json.value(kRoutesKey).toObject();
    QJsonArray features = routes.value(kRoutesFeaturesKey).toArray();
    for (const QJsonValueRef feature : features)
        parseRoute(feature.toObject());
}

void GeoRouteJsonParserEsri::parseRoute(const QJsonObject &route)
{
    QJsonObject attributes = route.value(kRoutesFeaturesAttributesKey).toObject();
    QGeoRoute &geoRoute = m_routes[attributes.value(kRoutesFeaturesObjectIdKey).toInt()];

    QJsonObject geometry = route.value(kRoutesFeaturesGeometryKey).toObject();
    QJsonArray paths = geometry.value(kRoutesFeaturesGeometryPathsKey).toArray();

    if (!paths.isEmpty())
    {
        QList<QGeoCoordinate> geoCoordinates;
        for (const QJsonValueRef value : paths.first().toArray()) // only first polyline?
        {
            QJsonArray geoCoordinate = value.toArray();
            if (geoCoordinate.size() == 2)  // ignore 3rd coordinate
            {
                geoCoordinates.append(QGeoCoordinate(geoCoordinate[1].toDouble(),
                                      geoCoordinate[0].toDouble()));
            }
        }
        geoRoute.setPath(geoCoordinates);
    }
}

QT_END_NAMESPACE
