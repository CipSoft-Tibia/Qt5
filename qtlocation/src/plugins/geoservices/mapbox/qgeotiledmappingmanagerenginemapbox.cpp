// Copyright (C) 2014 Canonical Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgeotiledmappingmanagerenginemapbox.h"
#include "qgeotilefetchermapbox.h"

#include <QtLocation/private/qgeocameracapabilities_p.h>
#include <QtLocation/private/qgeomaptype_p.h>
#include <QtLocation/private/qgeotiledmap_p.h>
#include "qgeofiletilecachemapbox.h"
typedef QGeoTiledMap Map;

QT_BEGIN_NAMESPACE

QGeoTiledMappingManagerEngineMapbox::QGeoTiledMappingManagerEngineMapbox(const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString)
:   QGeoTiledMappingManagerEngine()
{
    QGeoCameraCapabilities cameraCaps;
    cameraCaps.setMinimumZoomLevel(0.0);
    cameraCaps.setMaximumZoomLevel(19.0);
    cameraCaps.setSupportsBearing(true);
    cameraCaps.setSupportsTilting(true);
    cameraCaps.setMinimumTilt(0);
    cameraCaps.setMaximumTilt(80);
    cameraCaps.setMinimumFieldOfView(20.0);
    cameraCaps.setMaximumFieldOfView(120.0);
    cameraCaps.setOverzoomEnabled(true);
    setCameraCapabilities(cameraCaps);

    setTileSize(QSize(256, 256));

    const QByteArray pluginName = "mapbox";
    QList<QGeoMapType> mapTypes;
    // as index 0 to retain compatibility with the current API, that expects the passed map_id to be on by default.
    if (parameters.contains(QStringLiteral("mapbox.mapping.map_id"))) {
        const QString name = parameters.value(QStringLiteral("mapbox.mapping.map_id")).toString();
        mapTypes << QGeoMapType(QGeoMapType::CustomMap, name, name, false, false, mapTypes.size() + 1, pluginName, cameraCaps);
    } else if (parameters.contains(QStringLiteral("mapbox.map_id"))) { //deprecated
        const QString name = parameters.value(QStringLiteral("mapbox.map_id")).toString();
        mapTypes << QGeoMapType(QGeoMapType::CustomMap, name, name, false, false, mapTypes.size() + 1, pluginName, cameraCaps);
    }

    // As of 2016.06.15, valid mapbox map_ids are documented at https://www.mapbox.com/api-documentation/#maps
    //: Noun describing map type 'Street map'
    mapTypes << QGeoMapType(QGeoMapType::StreetMap, QStringLiteral("mapbox.streets"), tr("Street"), false, false, mapTypes.size() + 1, pluginName, cameraCaps);
    //: Noun describing type of a map using light colors (weak contrast)
    mapTypes << QGeoMapType(QGeoMapType::StreetMap, QStringLiteral("mapbox.light"), tr("Light"), false, false, mapTypes.size() + 1, pluginName, cameraCaps);
    //: Noun describing type of a map using dark colors
    mapTypes << QGeoMapType(QGeoMapType::StreetMap, QStringLiteral("mapbox.dark"), tr("Dark"), false, true, mapTypes.size() + 1, pluginName, cameraCaps);
    //: Noun describing type of a map created by satellite
    mapTypes << QGeoMapType(QGeoMapType::SatelliteMapDay, QStringLiteral("mapbox.satellite"), tr("Satellite"), false, false, mapTypes.size() + 1, pluginName, cameraCaps);
    //: Noun describing type of a street map created by satellite
    mapTypes << QGeoMapType(QGeoMapType::HybridMap, QStringLiteral("mapbox.streets-satellite"), tr("Streets Satellite"), false, false, mapTypes.size() + 1, pluginName, cameraCaps);
    //: Noun describing type of a map using wheat paste colors
    mapTypes << QGeoMapType(QGeoMapType::CustomMap, QStringLiteral("mapbox.wheatpaste"), tr("Wheatpaste"), false, false, mapTypes.size() + 1, pluginName, cameraCaps);
    //: Noun describing type of a basic street map
    mapTypes << QGeoMapType(QGeoMapType::StreetMap, QStringLiteral("mapbox.streets-basic"), tr("Streets Basic"), false, false, mapTypes.size() + 1, pluginName, cameraCaps);
    //: Noun describing type of a map using cartoon-style fonts
    mapTypes << QGeoMapType(QGeoMapType::CustomMap, QStringLiteral("mapbox.comic"), tr("Comic"), false, false, mapTypes.size() + 1, pluginName, cameraCaps);
    //: Noun describing type of a map for outdoor activities
    mapTypes << QGeoMapType(QGeoMapType::PedestrianMap, QStringLiteral("mapbox.outdoors"), tr("Outdoors"), false, false, mapTypes.size() + 1, pluginName, cameraCaps);
    //: Noun describing type of a map for sports
    mapTypes << QGeoMapType(QGeoMapType::CycleMap, QStringLiteral("mapbox.run-bike-hike"), tr("Run Bike Hike"), false, false, mapTypes.size() + 1, pluginName, cameraCaps);
    //: Noun describing type of a map drawn by pencil
    mapTypes << QGeoMapType(QGeoMapType::CustomMap, QStringLiteral("mapbox.pencil"), tr("Pencil"), false, false, mapTypes.size() + 1, pluginName, cameraCaps);
    //: Noun describing type of a treasure map with pirate boat watermark
    mapTypes << QGeoMapType(QGeoMapType::CustomMap, QStringLiteral("mapbox.pirates"), tr("Pirates"), false, false, mapTypes.size() + 1, pluginName, cameraCaps);
    //: Noun describing type of a map using emerald colors
    mapTypes << QGeoMapType(QGeoMapType::CustomMap, QStringLiteral("mapbox.emerald"), tr("Emerald"), false, false, mapTypes.size() + 1, pluginName, cameraCaps);
    //: Noun describing type of a map with high contrast
    mapTypes << QGeoMapType(QGeoMapType::CustomMap, QStringLiteral("mapbox.high-contrast"), tr("High Contrast"), false, false, mapTypes.size() + 1, pluginName, cameraCaps);

    // New way to specify multiple customized map_ids via additional_map_ids
    if (parameters.contains(QStringLiteral("mapbox.mapping.additional_map_ids"))) {
        const QString ids = parameters.value(QStringLiteral("mapbox.mapping.additional_map_ids")).toString();
        const QStringList idList = ids.split(',', Qt::SkipEmptyParts);

        for (const QString &name: idList) {
            if (!name.isEmpty())
                mapTypes << QGeoMapType(QGeoMapType::CustomMap, name, name, false, false, mapTypes.size() + 1, pluginName, cameraCaps);
        }
    }

    QList<QString> mapIds;
    for (const auto &mapType : std::as_const(mapTypes))
         mapIds.push_back(mapType.name());

    setSupportedMapTypes(mapTypes);

    int scaleFactor = 1;
    if (parameters.contains(QStringLiteral("mapbox.mapping.highdpi_tiles"))) {
        const QString param = parameters.value(QStringLiteral("mapbox.mapping.highdpi_tiles")).toString().toLower();
        if (param == "true")
            scaleFactor = 2;
    }

    QGeoTileFetcherMapbox *tileFetcher = new QGeoTileFetcherMapbox(scaleFactor, this);
    tileFetcher->setMapIds(mapIds);

    if (parameters.contains(QStringLiteral("useragent"))) {
        const QByteArray ua = parameters.value(QStringLiteral("useragent")).toString().toLatin1();
        tileFetcher->setUserAgent(ua);
    }
    if (parameters.contains(QStringLiteral("mapbox.mapping.format"))) {
        const QString format = parameters.value(QStringLiteral("mapbox.mapping.format")).toString();
        tileFetcher->setFormat(format);
    } else if (parameters.contains(QStringLiteral("mapbox.format"))) { //deprecated
        const QString format = parameters.value(QStringLiteral("mapbox.format")).toString();
        tileFetcher->setFormat(format);
    }
    if (parameters.contains(QStringLiteral("mapbox.access_token"))) {
        const QString token = parameters.value(QStringLiteral("mapbox.access_token")).toString();
        tileFetcher->setAccessToken(token);
    }

    setTileFetcher(tileFetcher);

    // TODO: do this in a plugin-neutral way so that other tiled map plugins
    //       don't need this boilerplate or hardcode plugin name

    if (parameters.contains(QStringLiteral("mapbox.mapping.cache.directory"))) {
        m_cacheDirectory = parameters.value(QStringLiteral("mapbox.mapping.cache.directory")).toString();
    } else {
        // managerName() is not yet set, we have to hardcode the plugin name below
        m_cacheDirectory = QAbstractGeoTileCache::baseLocationCacheDirectory() + QLatin1String(pluginName);
    }

    QGeoFileTileCache *tileCache = new QGeoFileTileCacheMapbox(mapTypes, scaleFactor, m_cacheDirectory);

    /*
     * Disk cache setup -- defaults to Unitary since:
     *
     * The Mapbox free plan allows for 6000 tiles to be stored for offline uses,
     * As of 2016.06.15, according to https://www.mapbox.com/help/mobile-offline/ .
     * Thus defaulting to Unitary strategy, and setting 6000 tiles as default cache disk size
     */
    if (parameters.contains(QStringLiteral("mapbox.mapping.cache.disk.cost_strategy"))) {
        QString cacheStrategy = parameters.value(QStringLiteral("mapbox.mapping.cache.disk.cost_strategy")).toString().toLower();
        if (cacheStrategy == QLatin1String("bytesize"))
            tileCache->setCostStrategyDisk(QGeoFileTileCache::ByteSize);
        else
            tileCache->setCostStrategyDisk(QGeoFileTileCache::Unitary);
    } else {
        tileCache->setCostStrategyDisk(QGeoFileTileCache::Unitary);
    }
    if (parameters.contains(QStringLiteral("mapbox.mapping.cache.disk.size"))) {
        bool ok = false;
        int cacheSize = parameters.value(QStringLiteral("mapbox.mapping.cache.disk.size")).toString().toInt(&ok);
        if (ok)
            tileCache->setMaxDiskUsage(cacheSize);
    } else {
        if (tileCache->costStrategyDisk() == QGeoFileTileCache::Unitary)
            tileCache->setMaxDiskUsage(6000); // The maximum allowed with the free tier
    }

    /*
     * Memory cache setup -- defaults to ByteSize (old behavior)
     */
    if (parameters.contains(QStringLiteral("mapbox.mapping.cache.memory.cost_strategy"))) {
        QString cacheStrategy = parameters.value(QStringLiteral("mapbox.mapping.cache.memory.cost_strategy")).toString().toLower();
        if (cacheStrategy == QLatin1String("bytesize"))
            tileCache->setCostStrategyMemory(QGeoFileTileCache::ByteSize);
        else
            tileCache->setCostStrategyMemory(QGeoFileTileCache::Unitary);
    } else {
        tileCache->setCostStrategyMemory(QGeoFileTileCache::ByteSize);
    }
    if (parameters.contains(QStringLiteral("mapbox.mapping.cache.memory.size"))) {
        bool ok = false;
        int cacheSize = parameters.value(QStringLiteral("mapbox.mapping.cache.memory.size")).toString().toInt(&ok);
        if (ok)
            tileCache->setMaxMemoryUsage(cacheSize);
    }

    /*
     * Texture cache setup -- defaults to ByteSize (old behavior)
     */
    if (parameters.contains(QStringLiteral("mapbox.mapping.cache.texture.cost_strategy"))) {
        QString cacheStrategy = parameters.value(QStringLiteral("mapbox.mapping.cache.texture.cost_strategy")).toString().toLower();
        if (cacheStrategy == QLatin1String("bytesize"))
            tileCache->setCostStrategyTexture(QGeoFileTileCache::ByteSize);
        else
            tileCache->setCostStrategyTexture(QGeoFileTileCache::Unitary);
    } else {
        tileCache->setCostStrategyTexture(QGeoFileTileCache::ByteSize);
    }
    if (parameters.contains(QStringLiteral("mapbox.mapping.cache.texture.size"))) {
        bool ok = false;
        int cacheSize = parameters.value(QStringLiteral("mapbox.mapping.cache.texture.size")).toString().toInt(&ok);
        if (ok)
            tileCache->setExtraTextureUsage(cacheSize);
    }

    /* PREFETCHING */
    if (parameters.contains(QStringLiteral("mapbox.mapping.prefetching_style"))) {
        const QString prefetchingMode = parameters.value(QStringLiteral("mapbox.mapping.prefetching_style")).toString();
        if (prefetchingMode == QStringLiteral("TwoNeighbourLayers"))
            m_prefetchStyle = QGeoTiledMap::PrefetchTwoNeighbourLayers;
        else if (prefetchingMode == QStringLiteral("OneNeighbourLayer"))
            m_prefetchStyle = QGeoTiledMap::PrefetchNeighbourLayer;
        else if (prefetchingMode == QStringLiteral("NoPrefetching"))
            m_prefetchStyle = QGeoTiledMap::NoPrefetching;
    }

    setTileCache(tileCache);

    *error = QGeoServiceProvider::NoError;
    errorString->clear();
}

QGeoTiledMappingManagerEngineMapbox::~QGeoTiledMappingManagerEngineMapbox()
{
}

QGeoMap *QGeoTiledMappingManagerEngineMapbox::createMap()
{
    QGeoTiledMap *map = new Map(this, 0);
    map->setPrefetchStyle(m_prefetchStyle);
    return map;
}

QT_END_NAMESPACE
