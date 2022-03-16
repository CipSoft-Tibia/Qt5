/****************************************************************************
**
** Copyright (C) 2018 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtPositioning module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgeopositioninfosource_geoclue2_p.h"
#include "qgeopositioninfosourcefactory_geoclue2.h"

#include <QtCore/QLoggingCategory>

Q_LOGGING_CATEGORY(lcPositioningGeoclue2, "qt.positioning.geoclue2")

QT_BEGIN_NAMESPACE

QGeoPositionInfoSource *QGeoPositionInfoSourceFactoryGeoclue2::positionInfoSource(QObject *parent)
{
    return new QGeoPositionInfoSourceGeoclue2(parent);
}

QGeoSatelliteInfoSource *QGeoPositionInfoSourceFactoryGeoclue2::satelliteInfoSource(QObject *parent)
{
    Q_UNUSED(parent)
    return nullptr;
}

QGeoAreaMonitorSource *QGeoPositionInfoSourceFactoryGeoclue2::areaMonitor(QObject *parent)
{
    Q_UNUSED(parent)
    return nullptr;
}

QT_END_NAMESPACE
