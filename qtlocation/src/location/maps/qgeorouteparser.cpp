/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qgeorouteparser_p.h"
#include "qgeorouteparser_p_p.h"
#include "qgeoroutesegment.h"
#include "qgeomaneuver.h"

#include <QtCore/private/qobject_p.h>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtPositioning/private/qlocationutils_p.h>

QT_BEGIN_NAMESPACE

/*
    Private class implementations
*/

QGeoRouteParserPrivate::QGeoRouteParserPrivate() : QObjectPrivate(), trafficSide(QGeoRouteParser::RightHandTraffic)
{
}

QGeoRouteParserPrivate::~QGeoRouteParserPrivate()
{
}

/*
    Public class implementations
*/

QGeoRouteParser::~QGeoRouteParser()
{

}

QGeoRouteParser::QGeoRouteParser(QGeoRouteParserPrivate &dd, QObject *parent) : QObject(dd, parent)
{

}

QGeoRouteReply::Error QGeoRouteParser::parseReply(QList<QGeoRoute> &routes, QString &errorString, const QByteArray &reply) const
{
    Q_D(const QGeoRouteParser);
    return d->parseReply(routes, errorString, reply);
}

QUrl QGeoRouteParser::requestUrl(const QGeoRouteRequest &request, const QString &prefix) const
{
    Q_D(const QGeoRouteParser);
    return d->requestUrl(request, prefix);
}

QGeoRouteParser::TrafficSide QGeoRouteParser::trafficSide() const
{
    Q_D(const QGeoRouteParser);
    return d->trafficSide;
}

void QGeoRouteParser::setTrafficSide(QGeoRouteParser::TrafficSide trafficSide)
{
    Q_D(QGeoRouteParser);
    if (d->trafficSide == trafficSide) return;
    d->trafficSide = trafficSide;
    Q_EMIT trafficSideChanged(trafficSide);
}

QT_END_NAMESPACE


