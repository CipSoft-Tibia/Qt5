// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Mapbox, Inc.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgeomappingmanagerenginemapboxgl.h"
#include "qgeomapmapboxgl.h"

#include <QtCore/qstandardpaths.h>
#include <QtLocation/private/qabstractgeotilecache_p.h>
#include <QtLocation/private/qgeocameracapabilities_p.h>
#include <QtLocation/private/qgeomaptype_p.h>

#include <QDir>

QT_BEGIN_NAMESPACE

QGeoMappingManagerEngineMapboxGL::QGeoMappingManagerEngineMapboxGL(const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString)
:   QGeoMappingManagerEngine()
{
    *error = QGeoServiceProvider::NoError;
    errorString->clear();

    QGeoCameraCapabilities cameraCaps;
    cameraCaps.setMinimumZoomLevel(0.0);
    cameraCaps.setMaximumZoomLevel(20.0);
    cameraCaps.setTileSize(512);
    cameraCaps.setSupportsBearing(true);
    cameraCaps.setSupportsTilting(true);
    cameraCaps.setMinimumTilt(0);
    cameraCaps.setMaximumTilt(60);
    cameraCaps.setMinimumFieldOfView(36.87);
    cameraCaps.setMaximumFieldOfView(36.87);
    setCameraCapabilities(cameraCaps);

    QList<QGeoMapType> mapTypes;
    int mapId = 0;
    const QByteArray pluginName = "mapboxgl";

    if (parameters.contains(QStringLiteral("mapboxgl.china"))) {
        m_useChinaEndpoint = parameters.value(QStringLiteral("mapboxgl.china")).toBool();
    }

    if (parameters.contains(QStringLiteral("mapboxgl.api_base_url"))) {
        const QString apiBaseUrl = parameters.value(QStringLiteral("mapboxgl.api_base_url")).toString();
        m_settings.setApiBaseUrl(apiBaseUrl);
    }

    QVariantMap metadata;
    metadata["isHTTPS"] = true;

    if (m_useChinaEndpoint) {
        m_settings.setApiBaseUrl(QStringLiteral("https://api.mapbox.cn"));

        mapTypes << QGeoMapType(QGeoMapType::StreetMap, QStringLiteral("mapbox://styles/mapbox/streets-zh-v1"),
                tr("China Streets"), false, false, ++mapId, pluginName, cameraCaps, metadata);
        mapTypes << QGeoMapType(QGeoMapType::GrayStreetMap, QStringLiteral("mapbox://styles/mapbox/light-zh-v1"),
                tr("China Light"), false, false, ++mapId, pluginName, cameraCaps, metadata);
        mapTypes << QGeoMapType(QGeoMapType::GrayStreetMap, QStringLiteral("mapbox://styles/mapbox/dark-zh-v1"),
                tr("China Dark"), false, false, ++mapId, pluginName, cameraCaps, metadata);
    } else {
        mapTypes << QGeoMapType(QGeoMapType::StreetMap, QStringLiteral("mapbox://styles/mapbox/streets-v10"),
                tr("Streets"), false, false, ++mapId, pluginName, cameraCaps, metadata);
        mapTypes << QGeoMapType(QGeoMapType::StreetMap, QStringLiteral("mapbox://styles/mapbox/basic-v9"),
                tr("Basic"), false, false, ++mapId, pluginName, cameraCaps, metadata);
        mapTypes << QGeoMapType(QGeoMapType::StreetMap, QStringLiteral("mapbox://styles/mapbox/bright-v9"),
                tr("Bright"), false, false, ++mapId, pluginName, cameraCaps, metadata);
        mapTypes << QGeoMapType(QGeoMapType::TerrainMap, QStringLiteral("mapbox://styles/mapbox/outdoors-v10"),
                tr("Outdoors"), false, false, ++mapId, pluginName, cameraCaps, metadata);
        mapTypes << QGeoMapType(QGeoMapType::SatelliteMapDay, QStringLiteral("mapbox://styles/mapbox/satellite-v9"),
                tr("Satellite"), false, false, ++mapId, pluginName, cameraCaps, metadata);
        mapTypes << QGeoMapType(QGeoMapType::HybridMap, QStringLiteral("mapbox://styles/mapbox/satellite-streets-v10"),
                tr("Satellite Streets"), false, false, ++mapId, pluginName, cameraCaps, metadata);
        mapTypes << QGeoMapType(QGeoMapType::GrayStreetMap, QStringLiteral("mapbox://styles/mapbox/light-v9"),
                tr("Light"), false, false, ++mapId, pluginName, cameraCaps, metadata);
        mapTypes << QGeoMapType(QGeoMapType::GrayStreetMap, QStringLiteral("mapbox://styles/mapbox/dark-v9"),
                tr("Dark"), false, false, ++mapId, pluginName, cameraCaps, metadata);
        mapTypes << QGeoMapType(QGeoMapType::TransitMap, QStringLiteral("mapbox://styles/mapbox/navigation-preview-day-v2"),
                tr("Navigation Preview Day"), false, false, ++mapId, pluginName, cameraCaps, metadata);
        mapTypes << QGeoMapType(QGeoMapType::TransitMap, QStringLiteral("mapbox://styles/mapbox/navigation-preview-night-v2"),
                tr("Navigation Preview Night"), false, true, ++mapId, pluginName, cameraCaps, metadata);
        mapTypes << QGeoMapType(QGeoMapType::CarNavigationMap, QStringLiteral("mapbox://styles/mapbox/navigation-guidance-day-v2"),
                tr("Navigation Guidance Day"), false, false, ++mapId, pluginName, cameraCaps, metadata);
        mapTypes << QGeoMapType(QGeoMapType::CarNavigationMap, QStringLiteral("mapbox://styles/mapbox/navigation-guidance-night-v2"),
                tr("Navigation Guidance Night"), false, true, ++mapId, pluginName, cameraCaps, metadata);
    }

    if (parameters.contains(QStringLiteral("mapboxgl.mapping.additional_style_urls"))) {
        const QString ids = parameters.value(QStringLiteral("mapboxgl.mapping.additional_style_urls")).toString();
        const QStringList idList = ids.split(',', Qt::SkipEmptyParts);

        for (auto it = idList.crbegin(), end = idList.crend(); it != end; ++it) {
            if ((*it).isEmpty())
                continue;
            if ((*it).startsWith(QStringLiteral("http:")))
                metadata["isHTTPS"] = false;
            else
                metadata["isHTTPS"] = true;

            mapTypes.prepend(QGeoMapType(QGeoMapType::CustomMap, *it,
                    tr("User provided style"), false, false, ++mapId, pluginName, cameraCaps, metadata));
        }
    }

    setSupportedMapTypes(mapTypes);

    if (parameters.contains(QStringLiteral("mapboxgl.access_token"))) {
        m_settings.setAccessToken(parameters.value(QStringLiteral("mapboxgl.access_token")).toString());
    }

    bool memoryCache = false;
    if (parameters.contains(QStringLiteral("mapboxgl.mapping.cache.memory"))) {
        memoryCache = parameters.value(QStringLiteral("mapboxgl.mapping.cache.memory")).toBool();
        m_settings.setCacheDatabasePath(QStringLiteral(":memory:"));
    }

    QString cacheDirectory;
    if (parameters.contains(QStringLiteral("mapboxgl.mapping.cache.directory"))) {
        cacheDirectory = parameters.value(QStringLiteral("mapboxgl.mapping.cache.directory")).toString();
    } else {
        cacheDirectory = QAbstractGeoTileCache::baseLocationCacheDirectory() + QStringLiteral("mapboxgl/");
    }

    if (!memoryCache && QDir::root().mkpath(cacheDirectory)) {
        m_settings.setCacheDatabasePath(cacheDirectory + "/mapboxgl.db");
    }

    if (parameters.contains(QStringLiteral("mapboxgl.mapping.cache.size"))) {
        bool ok = false;
        int cacheSize = parameters.value(QStringLiteral("mapboxgl.mapping.cache.size")).toString().toInt(&ok);

        if (ok)
            m_settings.setCacheDatabaseMaximumSize(cacheSize);
    }

    if (parameters.contains(QStringLiteral("mapboxgl.mapping.use_fbo"))) {
        m_useFBO = parameters.value(QStringLiteral("mapboxgl.mapping.use_fbo")).toBool();
    }

    if (parameters.contains(QStringLiteral("mapboxgl.mapping.items.insert_before"))) {
        m_mapItemsBefore = parameters.value(QStringLiteral("mapboxgl.mapping.items.insert_before")).toString();
    }

    engineInitialized();
}

QGeoMappingManagerEngineMapboxGL::~QGeoMappingManagerEngineMapboxGL()
{
}

QGeoMap *QGeoMappingManagerEngineMapboxGL::createMap()
{
    QGeoMapMapboxGL* map = new QGeoMapMapboxGL(this, 0);
    map->setMapboxGLSettings(m_settings, m_useChinaEndpoint);
    map->setUseFBO(m_useFBO);
    map->setMapItemsBefore(m_mapItemsBefore);

    return map;
}

QT_END_NAMESPACE
