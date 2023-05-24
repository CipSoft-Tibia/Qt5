// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef JNIPOSITIONING_H
#define JNIPOSITIONING_H

#include <QGeoPositionInfoSource>
#include <QGeoSatelliteInfoSource>

namespace AndroidPositioning
{
    int registerPositionInfoSource(QObject *obj);
    void unregisterPositionInfoSource(int key);

    QGeoPositionInfoSource::PositioningMethods availableProviders();
    QGeoPositionInfo lastKnownPosition(bool fromSatellitePositioningMethodsOnly);

    QGeoPositionInfoSource::Error startUpdates(int androidClassKey);
    void stopUpdates(int androidClassKey);
    QGeoPositionInfoSource::Error requestUpdate(int androidClassKey, int timeout);

    QGeoSatelliteInfoSource::Error startSatelliteUpdates(int androidClassKey,
                                                         bool isSingleRequest,
                                                         int updateRequestTimeout);
    bool hasPositioningPermissions();
}

#endif // JNIPOSITIONING_H
