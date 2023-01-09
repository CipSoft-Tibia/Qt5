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

#ifndef QGEOFILETILECACHEOSM_H
#define QGEOFILETILECACHEOSM_H

#include "qgeotileproviderosm.h"
#include <QtLocation/private/qgeofiletilecache_p.h>
#include <QHash>
#include <QtConcurrent>
#include <qatomic.h>

QT_BEGIN_NAMESPACE

class QGeoFileTileCacheOsm : public QGeoFileTileCache
{
    Q_OBJECT
public:
    QGeoFileTileCacheOsm(const QVector<QGeoTileProviderOsm *> &providers,
                         const QString &offlineDirectory = QString(),
                         const QString &directory = QString(),
                         QObject *parent = 0);
    ~QGeoFileTileCacheOsm();

    QSharedPointer<QGeoTileTexture> get(const QGeoTileSpec &spec) override;

Q_SIGNALS:
    void mapDataUpdated(int mapId);

protected Q_SLOTS:
    void onProviderResolutionFinished(const QGeoTileProviderOsm *provider);
    void onProviderResolutionError(const QGeoTileProviderOsm *provider, QNetworkReply::NetworkError error);

protected:
    void init() override;
    inline QString tileSpecToFilename(const QGeoTileSpec &spec, const QString &format, int providerId) const;
    QString tileSpecToFilename(const QGeoTileSpec &spec, const QString &format, const QString &directory) const override;
    QGeoTileSpec filenameToTileSpec(const QString &filename) const override;
    QSharedPointer<QGeoTileTexture> getFromOfflineStorage(const QGeoTileSpec &spec);
    void dropTiles(int mapId);
    void loadTiles(int mapId);

    void clearObsoleteTiles(const QGeoTileProviderOsm *p);

    QDir m_offlineDirectory;
    bool m_offlineData;
    QVector<QGeoTileProviderOsm *> m_providers;
    QVector<bool> m_highDpi;
    QVector<QDateTime> m_maxMapIdTimestamps;
};

QT_END_NAMESPACE

#endif // QGEOFILETILECACHEOSM_H
