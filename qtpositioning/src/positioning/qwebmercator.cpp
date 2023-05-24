// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qwebmercator_p.h"

#include "qgeocoordinate.h"

#include <qnumeric.h>
#include <qmath.h>

#include "qdoublevector2d_p.h"

QT_BEGIN_NAMESPACE

// y-Coordinates of maps aproach +-infinity when latitudes approach +-90 and
// they have to be limited to avoid numerical problems. Here the limit is
// chosen such that only points below/above +-89.9999999999 deg latituide are
// cut off, which corresponds to accuracy of qFuzzyCompare
const static double yCutOff = 4.0;

QDoubleVector2D QWebMercator::coordToMercator(const QGeoCoordinate &coord)
{
    const double pi = M_PI;

    double lon = coord.longitude() / 360.0 + 0.5;

    double lat = coord.latitude();
    lat = 0.5 - (std::log(std::tan((pi / 4.0) + (pi / 2.0) * lat / 180.0)) / pi) / 2.0;
    lat = qBound(-yCutOff, lat, 1.0 + yCutOff);

    return QDoubleVector2D(lon, lat);
}

double QWebMercator::realmod(const double a, const double b)
{
    quint64 div = static_cast<quint64>(a / b);
    return a - static_cast<double>(div) * b;
}

QGeoCoordinate QWebMercator::mercatorToCoord(const QDoubleVector2D &mercator)
{
    const double pi = M_PI;

    const double fx = mercator.x();
    const double fy = mercator.y();

    double lat;

    if (fy <= -yCutOff)
        lat = 90.0;
    else if (fy >= 1.0 + yCutOff)
        lat = -90.0;
    else
        lat = (180.0 / pi) * (2.0 * std::atan(std::exp(pi * (1.0 - 2.0 * fy))) - (pi / 2.0));

    double lng;
    if (fx >= 0) {
        lng = realmod(fx, 1.0);
    } else {
        lng = realmod(1.0 - realmod(-1.0 * fx, 1.0), 1.0);
    }

    lng = lng * 360.0 - 180.0;

    return QGeoCoordinate(lat, lng, 0.0);
}

QGeoCoordinate QWebMercator::coordinateInterpolation(const QGeoCoordinate &from, const QGeoCoordinate &to, qreal progress)
{
    QDoubleVector2D s = QWebMercator::coordToMercator(from);
    QDoubleVector2D e = QWebMercator::coordToMercator(to);

    double x;

    if (0.5 < qAbs(e.x() - s.x())) {
        // handle dateline crossing
        double ex = e.x();
        double sx = s.x();
        if (ex < sx)
            sx -= 1.0;
        else if (sx < ex)
            ex -= 1.0;

        x = (1.0 - progress) * sx + progress * ex;

        if (!qFuzzyIsNull(x) && (x < 0.0))
            x += 1.0;

    } else {
        x = (1.0 - progress) * s.x() + progress * e.x();
    }

    double y = (1.0 - progress) * s.y() + progress * e.y();

    QGeoCoordinate result = QWebMercator::mercatorToCoord(QDoubleVector2D(x, y));
    result.setAltitude((1.0 - progress) * from.altitude() + progress * to.altitude());

    return result;
}

QT_END_NAMESPACE
