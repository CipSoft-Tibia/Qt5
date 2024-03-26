// Copyright (C) 2015 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qgeotiledmap_p.h"
#include "qgeotiledmap_p_p.h"
#include "qgeotiledmappingmanagerengine_p.h"
#include "qabstractgeotilecache_p.h"
#include "qgeotilespec_p.h"
#include "qgeoprojection_p.h"

#include "qgeocameratiles_p.h"
#include "qgeotilerequestmanager_p.h"
#include "qgeotiledmapscene_p.h"
#include "qgeocameracapabilities_p.h"
#include <cmath>

QT_BEGIN_NAMESPACE
#define PREFETCH_FRUSTUM_SCALE 2.0

QGeoTiledMap::QGeoTiledMap(QGeoTiledMappingManagerEngine *engine, QObject *parent)
    : QGeoMap(*new QGeoTiledMapPrivate(engine), parent)
{
    Q_D(QGeoTiledMap);

    d->m_tileRequests = new QGeoTileRequestManager(this, engine);

    QObject::connect(engine,&QGeoTiledMappingManagerEngine::tileVersionChanged,
                     this,&QGeoTiledMap::handleTileVersionChanged);
    QObject::connect(this, &QGeoMap::cameraCapabilitiesChanged,
                     [d](const QGeoCameraCapabilities &oldCameraCapabilities) {
                       d->onCameraCapabilitiesChanged(oldCameraCapabilities);
                     });
}

QGeoTiledMap::QGeoTiledMap(QGeoTiledMapPrivate &dd, QGeoTiledMappingManagerEngine *engine, QObject *parent)
    : QGeoMap(dd, parent)
{
    Q_D(QGeoTiledMap);

    d->m_tileRequests = new QGeoTileRequestManager(this, engine);

    QObject::connect(engine,&QGeoTiledMappingManagerEngine::tileVersionChanged,
                     this,&QGeoTiledMap::handleTileVersionChanged);
    QObject::connect(this, &QGeoMap::cameraCapabilitiesChanged,
                     [d](const QGeoCameraCapabilities &oldCameraCapabilities) {
                       d->onCameraCapabilitiesChanged(oldCameraCapabilities);
                     });
}

QGeoTiledMap::~QGeoTiledMap()
{
    Q_D(QGeoTiledMap);
    delete d->m_tileRequests;
    d->m_tileRequests = nullptr;

    if (!d->m_engine.isNull()) {
        QGeoTiledMappingManagerEngine *engine = qobject_cast<QGeoTiledMappingManagerEngine*>(d->m_engine);
        Q_ASSERT(engine);
        engine->releaseMap(this);
    }
}

QGeoTileRequestManager *QGeoTiledMap::requestManager()
{
    Q_D(QGeoTiledMap);
    return d->m_tileRequests;
}

void QGeoTiledMap::updateTile(const QGeoTileSpec &spec)
{
    Q_D(QGeoTiledMap);
    d->updateTile(spec);
}

void QGeoTiledMap::setPrefetchStyle(QGeoTiledMap::PrefetchStyle style)
{
    Q_D(QGeoTiledMap);
    d->m_prefetchStyle = style;
}

QAbstractGeoTileCache *QGeoTiledMap::tileCache()
{
    Q_D(QGeoTiledMap);
    return d->m_cache;
}

QSGNode *QGeoTiledMap::updateSceneGraph(QSGNode *oldNode, QQuickWindow *window)
{
    Q_D(QGeoTiledMap);
    return d->updateSceneGraph(oldNode, window);
}

void QGeoTiledMap::prefetchData()
{
    Q_D(QGeoTiledMap);
    d->prefetchTiles();
}

void QGeoTiledMap::clearData()
{
    Q_D(QGeoTiledMap);
    d->m_cache->clearAll();
    d->m_mapScene->clearTexturedTiles();
    d->updateScene();
    sgNodeChanged();
}

QGeoMap::Capabilities QGeoTiledMap::capabilities() const
{
    return Capabilities(SupportsVisibleRegion
                        | SupportsSetBearing
                        | SupportsAnchoringCoordinate
                        | SupportsVisibleArea);
}

void QGeoTiledMap::setCopyrightVisible(bool visible)
{
    Q_D(QGeoTiledMap);
    if (visible == d->m_copyrightVisible)
        return;

    QGeoMap::setCopyrightVisible(visible);
    if (visible)
        evaluateCopyrights(d->m_mapScene->visibleTiles());
}

void QGeoTiledMap::clearScene(int mapId)
{
    Q_D(QGeoTiledMap);
    if (activeMapType().mapId() == mapId)
        d->clearScene();
}

void QGeoTiledMap::handleTileVersionChanged()
{
    Q_D(QGeoTiledMap);
    if (!d->m_engine.isNull()) {
        QGeoTiledMappingManagerEngine* engine = qobject_cast<QGeoTiledMappingManagerEngine*>(d->m_engine);
        Q_ASSERT(engine);
        d->changeTileVersion(engine->tileVersion());
    }
}

void QGeoTiledMap::evaluateCopyrights(const QSet<QGeoTileSpec> &visibleTiles)
{
    Q_UNUSED(visibleTiles);
}

QGeoTiledMapPrivate::QGeoTiledMapPrivate(QGeoTiledMappingManagerEngine *engine)
    : QGeoMapPrivate(engine, new QGeoProjectionWebMercator),
      m_cache(engine->tileCache()),
      m_visibleTiles(new QGeoCameraTiles()),
      m_prefetchTiles(new QGeoCameraTiles()),
      m_mapScene(new QGeoTiledMapScene()),
      m_tileRequests(nullptr),
      m_maxZoomLevel(static_cast<int>(std::ceil(m_cameraCapabilities.maximumZoomLevel()))),
      m_minZoomLevel(static_cast<int>(std::ceil(m_cameraCapabilities.minimumZoomLevel()))),
      m_prefetchStyle(QGeoTiledMap::PrefetchTwoNeighbourLayers)
{
    int tileSize = m_cameraCapabilities.tileSize();
    QString pluginString(engine->managerName() + QLatin1Char('_') + QString::number(engine->managerVersion()));
    m_visibleTiles->setTileSize(tileSize);
    m_prefetchTiles->setTileSize(tileSize);
    m_visibleTiles->setPluginString(pluginString);
    m_prefetchTiles->setPluginString(pluginString);
    m_mapScene->setTileSize(tileSize);
}

QGeoTiledMapPrivate::~QGeoTiledMapPrivate()
{
    // controller_ is a child of map_, don't need to delete it here

    delete m_mapScene;
    delete m_visibleTiles;
    delete m_prefetchTiles;

    // TODO map items are not deallocated!
    // However: how to ensure this is done in rendering thread?
}

void QGeoTiledMapPrivate::prefetchTiles()
{
    if (m_tileRequests && m_prefetchStyle != QGeoTiledMap::NoPrefetching) {

        QSet<QGeoTileSpec> tiles;
        QGeoCameraData camera = m_visibleTiles->cameraData();
        int currentIntZoom = static_cast<int>(std::floor(camera.zoomLevel()));

        m_prefetchTiles->setCameraData(camera);
        m_prefetchTiles->setViewExpansion(PREFETCH_FRUSTUM_SCALE);
        tiles = m_prefetchTiles->createTiles();

        switch (m_prefetchStyle) {

        case QGeoTiledMap::PrefetchNeighbourLayer: {
            double zoomFraction = camera.zoomLevel() - currentIntZoom;
            int nearestNeighbourLayer = zoomFraction > 0.5 ? currentIntZoom + 1 : currentIntZoom - 1;
            if (nearestNeighbourLayer <= m_maxZoomLevel && nearestNeighbourLayer >= m_minZoomLevel) {
                camera.setZoomLevel(nearestNeighbourLayer);
                // Approx heuristic, keeping total # prefetched tiles roughly independent of the
                // fractional zoom level.
                double neighbourScale = (1.0 + zoomFraction)/2.0;
                m_prefetchTiles->setCameraData(camera);
                m_prefetchTiles->setViewExpansion(PREFETCH_FRUSTUM_SCALE * neighbourScale);
                tiles += m_prefetchTiles->createTiles();
            }
        }
            break;

        case QGeoTiledMap::PrefetchTwoNeighbourLayers: {
            // This is a simpler strategy, we just prefetch from layer above and below
            // for the layer below we only use half the size as this fills the screen
            if (currentIntZoom > m_minZoomLevel) {
                camera.setZoomLevel(currentIntZoom - 1);
                m_prefetchTiles->setCameraData(camera);
                m_prefetchTiles->setViewExpansion(0.5);
                tiles += m_prefetchTiles->createTiles();
            }

            if (currentIntZoom < m_maxZoomLevel) {
                camera.setZoomLevel(currentIntZoom + 1);
                m_prefetchTiles->setCameraData(camera);
                m_prefetchTiles->setViewExpansion(1.0);
                tiles += m_prefetchTiles->createTiles();
            }
        }
            break;

        default:
            break;
        }

        m_tileRequests->requestTiles(tiles - m_mapScene->texturedTiles());
    }
}

QGeoMapType QGeoTiledMapPrivate::activeMapType() const
{
    return m_visibleTiles->activeMapType();
}

// Called before changeCameraData
void QGeoTiledMapPrivate::onCameraCapabilitiesChanged(const QGeoCameraCapabilities &oldCameraCapabilities)
{
    // Handle varying min/maxZoomLevel
    if (oldCameraCapabilities.minimumZoomLevel() != m_cameraCapabilities.minimumZoomLevel())
        m_minZoomLevel = static_cast<int>(std::ceil(m_cameraCapabilities.minimumZoomLevel()));
    if (oldCameraCapabilities.maximumZoomLevel() != m_cameraCapabilities.maximumZoomLevel())
        m_maxZoomLevel = static_cast<int>(std::ceil(m_cameraCapabilities.maximumZoomLevel()));

    // Handle varying tile size
    if (oldCameraCapabilities.tileSize() != m_cameraCapabilities.tileSize()) {
        m_visibleTiles->setTileSize(oldCameraCapabilities.tileSize());
        m_prefetchTiles->setTileSize(oldCameraCapabilities.tileSize());
        m_mapScene->setTileSize(oldCameraCapabilities.tileSize());
    }
}

void QGeoTiledMapPrivate::changeCameraData(const QGeoCameraData &cameraData)
{
    Q_Q(QGeoTiledMap);

    QGeoCameraData cam = cameraData;

    // The incoming zoom level is intended for a tileSize of 256.
    // Adapt it to the current tileSize
    double zoomLevel = cameraData.zoomLevel();
    if (m_visibleTiles->tileSize() != 256)
        zoomLevel = std::log(std::pow(2.0, zoomLevel) * 256.0 / m_visibleTiles->tileSize()) * (1.0 / std::log(2.0));
    cam.setZoomLevel(zoomLevel);

    // For zoomlevel, "snap" 0.01 either side of a whole number.
    // This is so that when we turn off bilinear scaling, we're
    // snapped to the exact pixel size of the tiles
    int izl = static_cast<int>(std::floor(cam.zoomLevel()));
    float delta = cam.zoomLevel() - izl;

    if (delta > 0.5) {
        izl++;
        delta -= 1.0;
    }

    // TODO: Don't do this if there's tilt or bearing.
    if (qAbs(delta) < 0.01) {
        cam.setZoomLevel(izl);
    }

    m_visibleTiles->setCameraData(cam);
    m_mapScene->setCameraData(cam);

    updateScene();
    q->sgNodeChanged(); // ToDo: explain why emitting twice
}

void QGeoTiledMapPrivate::updateScene()
{
    Q_Q(QGeoTiledMap);
    // detect if new tiles introduced
    const QSet<QGeoTileSpec>& tiles = m_visibleTiles->createTiles();
    bool newTilesIntroduced = !m_mapScene->visibleTiles().contains(tiles);
    m_mapScene->setVisibleTiles(tiles);

    if (newTilesIntroduced && m_copyrightVisible)
        q->evaluateCopyrights(tiles);

    // don't request tiles that are already built and textured
    QMap<QGeoTileSpec, QSharedPointer<QGeoTileTexture> > cachedTiles =
            m_tileRequests->requestTiles(m_visibleTiles->createTiles() - m_mapScene->texturedTiles());

    for (auto it = cachedTiles.cbegin(); it != cachedTiles.cend(); ++it)
        m_mapScene->addTile(it.key(), it.value());

    if (!cachedTiles.isEmpty())
        emit q->sgNodeChanged();
}

void QGeoTiledMapPrivate::setVisibleArea(const QRectF &visibleArea)
{
    Q_Q(QGeoTiledMap);
    const QRectF va = clampVisibleArea(visibleArea);
    if (va == m_visibleArea)
        return;

    m_visibleArea = va;
    m_geoProjection->setVisibleArea(va);

    m_visibleTiles->setVisibleArea(va);
    m_prefetchTiles->setVisibleArea(va);
    m_mapScene->setVisibleArea(va);

     if (m_copyrightVisible)
        q->evaluateCopyrights(m_mapScene->visibleTiles());
    updateScene();
    q->sgNodeChanged(); // ToDo: explain why emitting twice
}

QRectF QGeoTiledMapPrivate::visibleArea() const
{
    return m_visibleArea;
}

void QGeoTiledMapPrivate::changeActiveMapType(const QGeoMapType &mapType)
{
    m_visibleTiles->setTileSize(m_cameraCapabilities.tileSize());
    m_prefetchTiles->setTileSize(m_cameraCapabilities.tileSize());
    m_mapScene->setTileSize(m_cameraCapabilities.tileSize());
    m_visibleTiles->setMapType(mapType);
    m_prefetchTiles->setMapType(mapType);
    changeCameraData(m_cameraData); // Updates the zoom level to the possibly new tile size
    // updateScene called in changeCameraData()
}

void QGeoTiledMapPrivate::changeTileVersion(int version)
{
    m_visibleTiles->setMapVersion(version);
    m_prefetchTiles->setMapVersion(version);
    updateScene();
}

void QGeoTiledMapPrivate::clearScene()
{
    m_mapScene->clearTexturedTiles();
    m_mapScene->setVisibleTiles(QSet<QGeoTileSpec>());
    updateScene();
}

void QGeoTiledMapPrivate::changeViewportSize(const QSize& size)
{
    Q_Q(QGeoTiledMap);

    m_visibleTiles->setScreenSize(size);
    m_prefetchTiles->setScreenSize(size);
    m_mapScene->setScreenSize(size);


    if (!size.isEmpty() && m_cache) {
        // absolute minimum size: one tile each side of display, 32-bit colour
        int texCacheSize = (size.width() + m_visibleTiles->tileSize() * 2) *
                (size.height() + m_visibleTiles->tileSize() * 2) * 4;

        // multiply by 3 so the 'recent' list in the cache is big enough for
        // an entire display of tiles
        texCacheSize *= 3;
        // TODO: move this reasoning into the tilecache

        int newSize = qMax(m_cache->minTextureUsage(), texCacheSize);
        m_cache->setMinTextureUsage(newSize);
    }

    if (m_copyrightVisible)
        q->evaluateCopyrights(m_mapScene->visibleTiles());
    updateScene();
}

void QGeoTiledMapPrivate::updateTile(const QGeoTileSpec &spec)
{
     Q_Q(QGeoTiledMap);
    // Only promote the texture up to GPU if it is visible
    if (m_visibleTiles->createTiles().contains(spec)){
        QSharedPointer<QGeoTileTexture> tex = m_tileRequests->tileTexture(spec);
        if (!tex.isNull() && !tex->image.isNull()) {
            m_mapScene->addTile(spec, tex);
            emit q->sgNodeChanged();
        }
    }
}

QSGNode *QGeoTiledMapPrivate::updateSceneGraph(QSGNode *oldNode, QQuickWindow *window)
{
    return m_mapScene->updateSceneGraph(oldNode, window);
}

QT_END_NAMESPACE
