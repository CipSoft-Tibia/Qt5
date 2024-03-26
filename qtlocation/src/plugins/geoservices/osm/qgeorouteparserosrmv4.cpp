// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgeorouteparserosrmv4_p.h"
#include "qgeoroutesegment.h"
#include "qgeomaneuver.h"

#include <QtLocation/private/qgeorouteparser_p_p.h>
#include <QtCore/private/qobject_p.h>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QUrlQuery>

QT_BEGIN_NAMESPACE

static QList<QGeoCoordinate> parsePolyline(const QByteArray &data)
{
    QList<QGeoCoordinate> path;

    bool parsingLatitude = true;

    int shift = 0;
    int value = 0;

    QGeoCoordinate coord(0, 0);

    for (int i = 0; i < data.length(); ++i) {
        unsigned char c = data.at(i) - 63;

        value |= (c & 0x1f) << shift;
        shift += 5;

        // another chunk
        if (c & 0x20)
            continue;

        int diff = (value & 1) ? ~(value >> 1) : (value >> 1);

        if (parsingLatitude) {
            coord.setLatitude(coord.latitude() + (double)diff/1e6);
        } else {
            coord.setLongitude(coord.longitude() + (double)diff/1e6);
            path.append(coord);
        }

        parsingLatitude = !parsingLatitude;

        value = 0;
        shift = 0;
    }

    return path;
}

static QGeoManeuver::InstructionDirection osrmInstructionDirection(const QString &instructionCode, QGeoRouteParser::TrafficSide trafficSide)
{
    if (instructionCode == QLatin1String("0"))
        return QGeoManeuver::NoDirection;
    else if (instructionCode == QLatin1String("1"))
        return QGeoManeuver::DirectionForward;
    else if (instructionCode == QLatin1String("2"))
        return QGeoManeuver::DirectionBearRight;
    else if (instructionCode == QLatin1String("3"))
        return QGeoManeuver::DirectionRight;
    else if (instructionCode == QLatin1String("4"))
        return QGeoManeuver::DirectionHardRight;
    else if (instructionCode == QLatin1String("5")) {
        switch (trafficSide) {
        case QGeoRouteParser::RightHandTraffic:
            return QGeoManeuver::DirectionUTurnLeft;
        case QGeoRouteParser::LeftHandTraffic:
            return QGeoManeuver::DirectionUTurnRight;
        }
        return QGeoManeuver::DirectionUTurnLeft;
    } else if (instructionCode == QLatin1String("6"))
        return QGeoManeuver::DirectionHardLeft;
    else if (instructionCode == QLatin1String("7"))
        return QGeoManeuver::DirectionLeft;
    else if (instructionCode == QLatin1String("8"))
        return QGeoManeuver::DirectionBearLeft;
    else if (instructionCode == QLatin1String("9"))
        return QGeoManeuver::NoDirection;
    else if (instructionCode == QLatin1String("10"))
        return QGeoManeuver::DirectionForward;
    else if (instructionCode == QLatin1String("11"))
        return QGeoManeuver::NoDirection;
    else if (instructionCode == QLatin1String("12"))
        return QGeoManeuver::NoDirection;
    else if (instructionCode == QLatin1String("13"))
        return QGeoManeuver::NoDirection;
    else if (instructionCode == QLatin1String("14"))
        return QGeoManeuver::NoDirection;
    else if (instructionCode == QLatin1String("15"))
        return QGeoManeuver::NoDirection;
    else
        return QGeoManeuver::NoDirection;
}

static QString osrmInstructionText(const QString &instructionCode, const QString &wayname)
{
    if (instructionCode == QLatin1String("0")) {
        return QString();
    } else if (instructionCode == QLatin1String("1")) {
        if (wayname.isEmpty())
            return QGeoRouteParserOsrmV4::tr("Go straight.");
        else
            return QGeoRouteParserOsrmV4::tr("Go straight onto %1.").arg(wayname);
    } else if (instructionCode == QLatin1String("2")) {
        if (wayname.isEmpty())
            return QGeoRouteParserOsrmV4::tr("Turn slightly right.");
        else
            return QGeoRouteParserOsrmV4::tr("Turn slightly right onto %1.").arg(wayname);
    } else if (instructionCode == QLatin1String("3")) {
        if (wayname.isEmpty())
            return QGeoRouteParserOsrmV4::tr("Turn right.");
        else
            return QGeoRouteParserOsrmV4::tr("Turn right onto %1.").arg(wayname);
    } else if (instructionCode == QLatin1String("4")) {
        if (wayname.isEmpty())
            return QGeoRouteParserOsrmV4::tr("Make a sharp right.");
        else
            return QGeoRouteParserOsrmV4::tr("Make a sharp right onto %1.").arg(wayname);
    }
    else if (instructionCode == QLatin1String("5")) {
        return QGeoRouteParserOsrmV4::tr("When it is safe to do so, perform a U-turn.");
    } else if (instructionCode == QLatin1String("6")) {
        if (wayname.isEmpty())
            return QGeoRouteParserOsrmV4::tr("Make a sharp left.");
        else
            return QGeoRouteParserOsrmV4::tr("Make a sharp left onto %1.").arg(wayname);
    } else if (instructionCode == QLatin1String("7")) {
        if (wayname.isEmpty())
            return QGeoRouteParserOsrmV4::tr("Turn left.");
        else
            return QGeoRouteParserOsrmV4::tr("Turn left onto %1.").arg(wayname);
    } else if (instructionCode == QLatin1String("8")) {
        if (wayname.isEmpty())
            return QGeoRouteParserOsrmV4::tr("Turn slightly left.");
        else
            return QGeoRouteParserOsrmV4::tr("Turn slightly left onto %1.").arg(wayname);
    } else if (instructionCode == QLatin1String("9")) {
        return QGeoRouteParserOsrmV4::tr("Reached waypoint.");
    } else if (instructionCode == QLatin1String("10")) {
        if (wayname.isEmpty())
            return QGeoRouteParserOsrmV4::tr("Head on.");
        else
            return QGeoRouteParserOsrmV4::tr("Head onto %1.").arg(wayname);
    } else if (instructionCode == QLatin1String("11")) {
        return QGeoRouteParserOsrmV4::tr("Enter the roundabout.");
    } else if (instructionCode == QLatin1String("11-1")) {
        if (wayname.isEmpty())
            return QGeoRouteParserOsrmV4::tr("At the roundabout take the first exit.");
        else
            return QGeoRouteParserOsrmV4::tr("At the roundabout take the first exit onto %1.").arg(wayname);
    } else if (instructionCode == QLatin1String("11-2")) {
        if (wayname.isEmpty())
            return QGeoRouteParserOsrmV4::tr("At the roundabout take the second exit.");
        else
            return QGeoRouteParserOsrmV4::tr("At the roundabout take the second exit onto %1.").arg(wayname);
    } else if (instructionCode == QLatin1String("11-3")) {
        if (wayname.isEmpty())
            return QGeoRouteParserOsrmV4::tr("At the roundabout take the third exit.");
        else
            return QGeoRouteParserOsrmV4::tr("At the roundabout take the third exit onto %1.").arg(wayname);
    } else if (instructionCode == QLatin1String("11-4")) {
        if (wayname.isEmpty())
            return QGeoRouteParserOsrmV4::tr("At the roundabout take the fourth exit.");
        else
            return QGeoRouteParserOsrmV4::tr("At the roundabout take the fourth exit onto %1.").arg(wayname);
    } else if (instructionCode == QLatin1String("11-5")) {
        if (wayname.isEmpty())
            return QGeoRouteParserOsrmV4::tr("At the roundabout take the fifth exit.");
        else
            return QGeoRouteParserOsrmV4::tr("At the roundabout take the fifth exit onto %1.").arg(wayname);
    } else if (instructionCode == QLatin1String("11-6")) {
        if (wayname.isEmpty())
            return QGeoRouteParserOsrmV4::tr("At the roundabout take the sixth exit.");
        else
            return QGeoRouteParserOsrmV4::tr("At the roundabout take the sixth exit onto %1.").arg(wayname);
    } else if (instructionCode == QLatin1String("11-7")) {
        if (wayname.isEmpty())
            return QGeoRouteParserOsrmV4::tr("At the roundabout take the seventh exit.");
        else
            return QGeoRouteParserOsrmV4::tr("At the roundabout take the seventh exit onto %1.").arg(wayname);
    } else if (instructionCode == QLatin1String("11-8")) {
        if (wayname.isEmpty())
            return QGeoRouteParserOsrmV4::tr("At the roundabout take the eighth exit.");
        else
            return QGeoRouteParserOsrmV4::tr("At the roundabout take the eighth exit onto %1.").arg(wayname);
    } else if (instructionCode == QLatin1String("11-9")) {
        if (wayname.isEmpty())
            return QGeoRouteParserOsrmV4::tr("At the roundabout take the ninth exit.");
        else
            return QGeoRouteParserOsrmV4::tr("At the roundabout take the ninth exit onto %1.").arg(wayname);
    } else if (instructionCode == QLatin1String("12")) {
        if (wayname.isEmpty())
            return QGeoRouteParserOsrmV4::tr("Leave the roundabout.");
        else
            return QGeoRouteParserOsrmV4::tr("Leave the roundabout onto %1.").arg(wayname);
    } else if (instructionCode == QLatin1String("13")) {
        return QGeoRouteParserOsrmV4::tr("Stay on the roundabout.");
    } else if (instructionCode == QLatin1String("14")) {
        if (wayname.isEmpty())
            return QGeoRouteParserOsrmV4::tr("Start at the end of the street.");
        else
            return QGeoRouteParserOsrmV4::tr("Start at the end of %1.").arg(wayname);
    } else if (instructionCode == QLatin1String("15")) {
        return QGeoRouteParserOsrmV4::tr("You have reached your destination.");
    } else {
        return QGeoRouteParserOsrmV4::tr("Don't know what to say for '%1'").arg(instructionCode);
    }
}

static QGeoRoute constructRoute(const QByteArray &geometry, const QJsonArray &instructions,
                                const QJsonObject &summary, QGeoRouteParser::TrafficSide trafficSide)
{
    QGeoRoute route;

    const QList<QGeoCoordinate> path = parsePolyline(geometry);

    QGeoRouteSegment firstSegment;
    int firstPosition = -1;

    for (qsizetype i = instructions.count() - 1; i >= 0; --i) {
        const QJsonArray instruction = instructions.at(i).toArray();

        if (instruction.count() < 8) {
            qWarning("Instruction does not contain enough fields.");
            continue;
        }

        const QString instructionCode = instruction.at(0).toString();
        const QString wayname = instruction.at(1).toString();
        double segmentLength = instruction.at(2).toDouble();
        int position = instruction.at(3).toDouble();
        int time = instruction.at(4).toDouble();
        //const QString segmentLengthString = instruction.at(5).toString();
        //const QString direction = instruction.at(6).toString();
        //double azimuth = instruction.at(7).toDouble();

        QGeoRouteSegment segment;
        segment.setDistance(segmentLength);

        QGeoManeuver maneuver;
        maneuver.setDirection(osrmInstructionDirection(instructionCode, trafficSide));
        maneuver.setDistanceToNextInstruction(segmentLength);
        maneuver.setInstructionText(osrmInstructionText(instructionCode, wayname));
        maneuver.setPosition(path.at(position));
        maneuver.setTimeToNextInstruction(time);

        segment.setManeuver(maneuver);

        if (firstPosition == -1)
            segment.setPath(path.mid(position));
        else
            segment.setPath(path.mid(position, firstPosition - position));

        segment.setTravelTime(time);

        segment.setNextRouteSegment(firstSegment);

        firstSegment = segment;
        firstPosition = position;
    }

    route.setDistance(summary.value(QStringLiteral("total_distance")).toDouble());
    route.setTravelTime(summary.value(QStringLiteral("total_time")).toDouble());
    route.setFirstRouteSegment(firstSegment);
    route.setPath(path);

    return route;
}

class QGeoRouteParserOsrmV4Private :  public QGeoRouteParserPrivate
{
    Q_DECLARE_PUBLIC(QGeoRouteParserOsrmV4)
public:
    QGeoRouteParserOsrmV4Private();
    virtual ~QGeoRouteParserOsrmV4Private();

    QGeoRouteReply::Error parseReply(QList<QGeoRoute> &routes, QString &errorString, const QByteArray &reply) const override;
    QUrl requestUrl(const QGeoRouteRequest &request, const QString &prefix) const override;
};

QGeoRouteParserOsrmV4Private::QGeoRouteParserOsrmV4Private() : QGeoRouteParserPrivate()
{
}

QGeoRouteParserOsrmV4Private::~QGeoRouteParserOsrmV4Private()
{
}

QGeoRouteReply::Error QGeoRouteParserOsrmV4Private::parseReply(QList<QGeoRoute> &routes, QString &errorString, const QByteArray &reply) const
{
    // OSRM v4 specs: https://github.com/Project-OSRM/osrm-backend/wiki/Server-API---v4,-old
    QJsonDocument document = QJsonDocument::fromJson(reply);

    if (document.isObject()) {
        QJsonObject object = document.object();

        //double version = object.value(QStringLiteral("version")).toDouble();
        int status = object.value(QStringLiteral("status")).toDouble();
        QString statusMessage = object.value(QStringLiteral("status_message")).toString();

        // status code 0 or 200 are case of success
        // status code is 207 if no route was found
        // an error occurred when trying to find a route
        if (0 != status && 200 != status) {
            errorString = statusMessage;
            return QGeoRouteReply::UnknownError;
        }

        QJsonObject routeSummary = object.value(QStringLiteral("route_summary")).toObject();

        QByteArray routeGeometry =
            object.value(QStringLiteral("route_geometry")).toString().toLatin1();

        QJsonArray routeInstructions = object.value(QStringLiteral("route_instructions")).toArray();

        QGeoRoute route = constructRoute(routeGeometry, routeInstructions, routeSummary, trafficSide);

        routes.append(route);

        const QJsonArray alternativeSummaries =
            object.value(QStringLiteral("alternative_summaries")).toArray();
        const QJsonArray alternativeGeometries =
            object.value(QStringLiteral("alternative_geometries")).toArray();
        const QJsonArray alternativeInstructions =
            object.value(QStringLiteral("alternative_instructions")).toArray();

        if (alternativeSummaries.count() == alternativeGeometries.count() &&
            alternativeSummaries.count() == alternativeInstructions.count()) {
            for (qsizetype i = 0; i < alternativeSummaries.count(); ++i) {
                route = constructRoute(alternativeGeometries.at(i).toString().toLatin1(),
                                       alternativeInstructions.at(i).toArray(),
                                       alternativeSummaries.at(i).toObject(),
                                       trafficSide);
                //routes.append(route);
            }
        }

        return QGeoRouteReply::NoError;
    } else {
        errorString = QStringLiteral("Couldn't parse json.");
        return QGeoRouteReply::ParseError;
    }
}

QUrl QGeoRouteParserOsrmV4Private::requestUrl(const QGeoRouteRequest &request, const QString &prefix) const
{
    QUrl url(prefix);
    QUrlQuery query;

    query.addQueryItem(QStringLiteral("instructions"), QStringLiteral("true"));

    for (const QGeoCoordinate &c : request.waypoints()) {
        query.addQueryItem(QStringLiteral("loc"), QString::number(c.latitude()) + QLatin1Char(',') +
                                                 QString::number(c.longitude()));
    }

    url.setQuery(query);
    return url;
}

QGeoRouteParserOsrmV4::QGeoRouteParserOsrmV4(QObject *parent)
    : QGeoRouteParser(*new QGeoRouteParserOsrmV4Private(), parent)
{
}

QGeoRouteParserOsrmV4::~QGeoRouteParserOsrmV4()
{
}

QT_END_NAMESPACE
