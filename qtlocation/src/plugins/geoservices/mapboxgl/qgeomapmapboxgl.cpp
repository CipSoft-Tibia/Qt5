// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Mapbox, Inc.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgeomapmapboxgl.h"
#include "qgeomapmapboxgl_p.h"
#include "qsgmapboxglnode.h"
#include "qmapboxglstylechange_p.h"

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtGui/QOpenGLContext>
#include <QtOpenGL/QOpenGLFramebufferObject>
#include <QtLocation/private/qdeclarativecirclemapitem_p.h>
#include <QtLocation/private/qdeclarativegeomapitembase_p.h>
#include <QtLocation/private/qdeclarativepolygonmapitem_p.h>
#include <QtLocation/private/qdeclarativepolylinemapitem_p.h>
#include <QtLocation/private/qdeclarativerectanglemapitem_p.h>
#include <QtLocation/private/qgeoprojection_p.h>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGImageNode>
#include <QtQuick/private/qsgcontext_p.h> // for debugging the context name

#include <QMapboxGL>

#include <cmath>

// FIXME: Expose from Mapbox GL constants
#define MBGL_TILE_SIZE 512.0

namespace {

// WARNING! The development token is subject to Mapbox Terms of Services
// and must not be used in production.
static char developmentToken[] =
    "pk.eyJ1IjoicXRzZGsiLCJhIjoiY2l5azV5MHh5MDAwdTMybzBybjUzZnhxYSJ9.9rfbeqPjX2BusLRDXHCOBA";

static const double invLog2 = 1.0 / std::log(2.0);

static double zoomLevelFrom256(double zoomLevelFor256, double tileSize)
{
    return std::log(std::pow(2.0, zoomLevelFor256) * 256.0 / tileSize) * invLog2;
}

} // namespace

QGeoMapMapboxGLPrivate::QGeoMapMapboxGLPrivate(QGeoMappingManagerEngineMapboxGL *engine)
    : QGeoMapPrivate(engine, new QGeoProjectionWebMercator)
{
}

QGeoMapMapboxGLPrivate::~QGeoMapMapboxGLPrivate()
{
}

QSGNode *QGeoMapMapboxGLPrivate::updateSceneGraph(QSGNode *node, QQuickWindow *window)
{
    Q_Q(QGeoMapMapboxGL);

    if (m_viewportSize.isEmpty()) {
        delete node;
        return nullptr;
    }

    QMapboxGL *map = nullptr;
    if (!node) {
        QOpenGLContext *currentCtx = QOpenGLContext::currentContext();
        if (!currentCtx) {
            qWarning("QOpenGLContext is NULL!");
            qWarning() << "You are running on QSG backend " << QSGContext::backend();
            qWarning("The MapboxGL plugin works with both Desktop and ES 2.0+ OpenGL versions.");
            qWarning("Verify that your Qt is built with OpenGL, and what kind of OpenGL.");
            qWarning("To force using a specific OpenGL version, check QSurfaceFormat::setRenderableType and QSurfaceFormat::setDefaultFormat");

            return node;
        }
        if (m_useFBO) {
            QSGMapboxGLTextureNode *mbglNode = new QSGMapboxGLTextureNode(m_settings, m_viewportSize, window->devicePixelRatio(), q);
            QObject::connect(mbglNode->map(), &QMapboxGL::mapChanged, q, &QGeoMapMapboxGL::onMapChanged);
            m_syncState = MapTypeSync | CameraDataSync | ViewportSync | VisibleAreaSync;
            node = mbglNode;
        } else {
            QSGMapboxGLRenderNode *mbglNode = new QSGMapboxGLRenderNode(m_settings, m_viewportSize, window->devicePixelRatio(), q);
            QObject::connect(mbglNode->map(), &QMapboxGL::mapChanged, q, &QGeoMapMapboxGL::onMapChanged);
            m_syncState = MapTypeSync | CameraDataSync | ViewportSync | VisibleAreaSync;
            node = mbglNode;
        }
    }
    map = (m_useFBO) ? static_cast<QSGMapboxGLTextureNode *>(node)->map()
                     : static_cast<QSGMapboxGLRenderNode *>(node)->map();

    if (m_syncState & MapTypeSync) {
        m_developmentMode = m_activeMapType.name().startsWith("mapbox://")
            && m_settings.accessToken() == developmentToken;

        map->setStyleUrl(m_activeMapType.name());
    }

    if (m_syncState & VisibleAreaSync) {
        if (m_visibleArea.isEmpty()) {
            map->setMargins(QMargins());
        } else {
            QMargins margins(m_visibleArea.x(),                                                     // left
                             m_visibleArea.y(),                                                     // top
                             m_viewportSize.width() - m_visibleArea.width() - m_visibleArea.x(),    // right
                             m_viewportSize.height() - m_visibleArea.height() - m_visibleArea.y()); // bottom
            map->setMargins(margins);
        }
    }

    if (m_syncState & CameraDataSync || m_syncState & VisibleAreaSync) {
        map->setZoom(zoomLevelFrom256(m_cameraData.zoomLevel() , MBGL_TILE_SIZE));
        map->setBearing(m_cameraData.bearing());
        map->setPitch(m_cameraData.tilt());

        QGeoCoordinate coordinate = m_cameraData.center();
        map->setCoordinate(QMapbox::Coordinate(coordinate.latitude(), coordinate.longitude()));
    }

    if (m_syncState & ViewportSync) {
        if (m_useFBO) {
            static_cast<QSGMapboxGLTextureNode *>(node)->resize(m_viewportSize, window->devicePixelRatio());
        } else {
            map->resize(m_viewportSize);
        }
    }

    if (m_styleLoaded) {
        syncStyleChanges(map);
    }

    if (m_useFBO) {
        static_cast<QSGMapboxGLTextureNode *>(node)->render(window);
    }

    threadedRenderingHack(window, map);

    m_syncState = NoSync;

    return node;
}

QGeoMap::ItemTypes QGeoMapMapboxGLPrivate::supportedMapItemTypes() const
{
    return QGeoMap::MapRectangle | QGeoMap::MapCircle | QGeoMap::MapPolygon | QGeoMap::MapPolyline;
}

void QGeoMapMapboxGLPrivate::addMapItem(QDeclarativeGeoMapItemBase *item)
{
    Q_Q(QGeoMapMapboxGL);

    switch (item->itemType()) {
    case QGeoMap::NoItem:
    case QGeoMap::MapQuickItem:
    case QGeoMap::CustomMapItem:
        return;
    case QGeoMap::MapRectangle: {
        QDeclarativeRectangleMapItem *mapItem = static_cast<QDeclarativeRectangleMapItem *>(item);
        QObject::connect(mapItem, &QQuickItem::visibleChanged, q, &QGeoMapMapboxGL::onMapItemPropertyChanged);
        QObject::connect(mapItem, &QDeclarativeGeoMapItemBase::mapItemOpacityChanged, q, &QGeoMapMapboxGL::onMapItemPropertyChanged);
        QObject::connect(mapItem, &QDeclarativeRectangleMapItem::bottomRightChanged, q, &QGeoMapMapboxGL::onMapItemGeometryChanged);
        QObject::connect(mapItem, &QDeclarativeRectangleMapItem::topLeftChanged, q, &QGeoMapMapboxGL::onMapItemGeometryChanged);
        QObject::connect(mapItem, &QDeclarativeRectangleMapItem::colorChanged, q, &QGeoMapMapboxGL::onMapItemPropertyChanged);
        QObject::connect(mapItem->border(), &QDeclarativeMapLineProperties::colorChanged, q, &QGeoMapMapboxGL::onMapItemSubPropertyChanged);
        QObject::connect(mapItem->border(), &QDeclarativeMapLineProperties::widthChanged, q, &QGeoMapMapboxGL::onMapItemUnsupportedPropertyChanged);
    } break;
    case QGeoMap::MapCircle: {
        QDeclarativeCircleMapItem *mapItem = static_cast<QDeclarativeCircleMapItem *>(item);
        QObject::connect(mapItem, &QQuickItem::visibleChanged, q, &QGeoMapMapboxGL::onMapItemPropertyChanged);
        QObject::connect(mapItem, &QDeclarativeGeoMapItemBase::mapItemOpacityChanged, q, &QGeoMapMapboxGL::onMapItemPropertyChanged);
        QObject::connect(mapItem, &QDeclarativeCircleMapItem::centerChanged, q, &QGeoMapMapboxGL::onMapItemGeometryChanged);
        QObject::connect(mapItem, &QDeclarativeCircleMapItem::radiusChanged, q, &QGeoMapMapboxGL::onMapItemGeometryChanged);
        QObject::connect(mapItem, &QDeclarativeCircleMapItem::colorChanged, q, &QGeoMapMapboxGL::onMapItemPropertyChanged);
        QObject::connect(mapItem->border(), &QDeclarativeMapLineProperties::colorChanged, q, &QGeoMapMapboxGL::onMapItemSubPropertyChanged);
        QObject::connect(mapItem->border(), &QDeclarativeMapLineProperties::widthChanged, q, &QGeoMapMapboxGL::onMapItemUnsupportedPropertyChanged);
    } break;
    case QGeoMap::MapPolygon: {
        QDeclarativePolygonMapItem *mapItem = static_cast<QDeclarativePolygonMapItem *>(item);
        QObject::connect(mapItem, &QQuickItem::visibleChanged, q, &QGeoMapMapboxGL::onMapItemPropertyChanged);
        QObject::connect(mapItem, &QDeclarativeGeoMapItemBase::mapItemOpacityChanged, q, &QGeoMapMapboxGL::onMapItemPropertyChanged);
        QObject::connect(mapItem, &QDeclarativePolygonMapItem::pathChanged, q, &QGeoMapMapboxGL::onMapItemGeometryChanged);
        QObject::connect(mapItem, &QDeclarativePolygonMapItem::colorChanged, q, &QGeoMapMapboxGL::onMapItemPropertyChanged);
        QObject::connect(mapItem->border(), &QDeclarativeMapLineProperties::colorChanged, q, &QGeoMapMapboxGL::onMapItemSubPropertyChanged);
        QObject::connect(mapItem->border(), &QDeclarativeMapLineProperties::widthChanged, q, &QGeoMapMapboxGL::onMapItemUnsupportedPropertyChanged);
    } break;
    case QGeoMap::MapPolyline: {
        QDeclarativePolylineMapItem *mapItem = static_cast<QDeclarativePolylineMapItem *>(item);
        QObject::connect(mapItem, &QQuickItem::visibleChanged, q, &QGeoMapMapboxGL::onMapItemPropertyChanged);
        QObject::connect(mapItem, &QDeclarativeGeoMapItemBase::mapItemOpacityChanged, q, &QGeoMapMapboxGL::onMapItemPropertyChanged);
        QObject::connect(mapItem, &QDeclarativePolylineMapItem::pathChanged, q, &QGeoMapMapboxGL::onMapItemGeometryChanged);
        QObject::connect(mapItem->line(), &QDeclarativeMapLineProperties::colorChanged, q, &QGeoMapMapboxGL::onMapItemSubPropertyChanged);
        QObject::connect(mapItem->line(), &QDeclarativeMapLineProperties::widthChanged, q, &QGeoMapMapboxGL::onMapItemSubPropertyChanged);
    } break;
    }

    QObject::connect(item, &QDeclarativeGeoMapItemBase::mapItemOpacityChanged, q, &QGeoMapMapboxGL::onMapItemPropertyChanged);

    m_styleChanges << QMapboxGLStyleChange::addMapItem(item, m_mapItemsBefore);

    emit q->sgNodeChanged();
}

void QGeoMapMapboxGLPrivate::removeMapItem(QDeclarativeGeoMapItemBase *item)
{
    Q_Q(QGeoMapMapboxGL);

    switch (item->itemType()) {
    case QGeoMap::NoItem:
    case QGeoMap::MapQuickItem:
    case QGeoMap::CustomMapItem:
        return;
    case QGeoMap::MapRectangle:
        q->disconnect(static_cast<QDeclarativeRectangleMapItem *>(item)->border());
        break;
    case QGeoMap::MapCircle:
        q->disconnect(static_cast<QDeclarativeCircleMapItem *>(item)->border());
        break;
    case QGeoMap::MapPolygon:
        q->disconnect(static_cast<QDeclarativePolygonMapItem *>(item)->border());
        break;
    case QGeoMap::MapPolyline:
        q->disconnect(static_cast<QDeclarativePolylineMapItem *>(item)->line());
        break;
    }

    q->disconnect(item);

    m_styleChanges << QMapboxGLStyleChange::removeMapItem(item);

    emit q->sgNodeChanged();
}

void QGeoMapMapboxGLPrivate::changeViewportSize(const QSize &)
{
    Q_Q(QGeoMapMapboxGL);

    m_syncState = m_syncState | ViewportSync;
    emit q->sgNodeChanged();
}

void QGeoMapMapboxGLPrivate::changeCameraData(const QGeoCameraData &)
{
    Q_Q(QGeoMapMapboxGL);

    m_syncState = m_syncState | CameraDataSync;
    emit q->sgNodeChanged();
}

void QGeoMapMapboxGLPrivate::changeActiveMapType(const QGeoMapType)
{
    Q_Q(QGeoMapMapboxGL);

    m_syncState = m_syncState | MapTypeSync;
    emit q->sgNodeChanged();
}

void QGeoMapMapboxGLPrivate::setVisibleArea(const QRectF &visibleArea)
{
    Q_Q(QGeoMapMapboxGL);
    const QRectF va = clampVisibleArea(visibleArea);
    if (va == m_visibleArea)
        return;

    m_visibleArea = va;
    m_geoProjection->setVisibleArea(va);

    m_syncState = m_syncState | VisibleAreaSync;
    emit q->sgNodeChanged();
}

QRectF QGeoMapMapboxGLPrivate::visibleArea() const
{
    return m_visibleArea;
}

void QGeoMapMapboxGLPrivate::syncStyleChanges(QMapboxGL *map)
{
    for (const auto& change : m_styleChanges) {
        change->apply(map);
    }

    m_styleChanges.clear();
}

void QGeoMapMapboxGLPrivate::threadedRenderingHack(QQuickWindow *window, QMapboxGL *map)
{
    // FIXME: Optimal support for threaded rendering needs core changes
    // in Mapbox GL Native. Meanwhile we need to set a timer to update
    // the map until all the resources are loaded, which is not exactly
    // battery friendly, because might trigger more paints than we need.
    if (!m_warned) {
        m_threadedRendering = window->openglContext()->thread() != QCoreApplication::instance()->thread();

        if (m_threadedRendering) {
            qWarning() << "Threaded rendering is not optimal in the Mapbox GL plugin.";
        }

        m_warned = true;
    }

    if (m_threadedRendering) {
        if (!map->isFullyLoaded()) {
            QMetaObject::invokeMethod(&m_refresh, "start", Qt::QueuedConnection);
        } else {
            QMetaObject::invokeMethod(&m_refresh, "stop", Qt::QueuedConnection);
        }
    }
}

/*
 * QGeoMapMapboxGL implementation
 */

QGeoMapMapboxGL::QGeoMapMapboxGL(QGeoMappingManagerEngineMapboxGL *engine, QObject *parent)
    :   QGeoMap(*new QGeoMapMapboxGLPrivate(engine), parent), m_engine(engine)
{
    Q_D(QGeoMapMapboxGL);

    connect(&d->m_refresh, &QTimer::timeout, this, &QGeoMap::sgNodeChanged);
    d->m_refresh.setInterval(250);
}

QGeoMapMapboxGL::~QGeoMapMapboxGL()
{
}

QString QGeoMapMapboxGL::copyrightsStyleSheet() const
{
    return QStringLiteral("* { vertical-align: middle; font-weight: normal }");
}

void QGeoMapMapboxGL::setMapboxGLSettings(const QMapboxGLSettings& settings, bool useChinaEndpoint)
{
    Q_D(QGeoMapMapboxGL);

    d->m_settings = settings;

    // If the access token is not set, use the development access token.
    // This will only affect mapbox:// styles.
    // Mapbox China requires a China-specific access token.
    if (d->m_settings.accessToken().isEmpty()) {
        if (useChinaEndpoint) {
            qWarning("Mapbox China requires an access token: https://www.mapbox.com/contact/sales");
        } else {
            d->m_settings.setAccessToken(developmentToken);
        }
    }
}

void QGeoMapMapboxGL::setUseFBO(bool useFBO)
{
    Q_D(QGeoMapMapboxGL);
    d->m_useFBO = useFBO;
}

void QGeoMapMapboxGL::setMapItemsBefore(const QString &before)
{
    Q_D(QGeoMapMapboxGL);
    d->m_mapItemsBefore = before;
}

QGeoMap::Capabilities QGeoMapMapboxGL::capabilities() const
{
    return Capabilities(SupportsVisibleRegion
                        | SupportsSetBearing
                        | SupportsAnchoringCoordinate
                        | SupportsVisibleArea );
}

QSGNode *QGeoMapMapboxGL::updateSceneGraph(QSGNode *oldNode, QQuickWindow *window)
{
    Q_D(QGeoMapMapboxGL);
    return d->updateSceneGraph(oldNode, window);
}

void QGeoMapMapboxGL::onMapChanged(QMapboxGL::MapChange change)
{
    Q_D(QGeoMapMapboxGL);

    if (change == QMapboxGL::MapChangeDidFinishLoadingStyle || change == QMapboxGL::MapChangeDidFailLoadingMap) {
        d->m_styleLoaded = true;
    } else if (change == QMapboxGL::MapChangeWillStartLoadingMap) {
        d->m_styleLoaded = false;
        d->m_styleChanges.clear();

        for (QDeclarativeGeoMapItemBase *item : d->m_mapItems)
            d->m_styleChanges << QMapboxGLStyleChange::addMapItem(item, d->m_mapItemsBefore);
    }
}

void QGeoMapMapboxGL::onMapItemPropertyChanged()
{
    Q_D(QGeoMapMapboxGL);

    QDeclarativeGeoMapItemBase *item = static_cast<QDeclarativeGeoMapItemBase *>(sender());
    d->m_styleChanges << QMapboxGLStyleSetPaintProperty::fromMapItem(item);
    d->m_styleChanges << QMapboxGLStyleSetLayoutProperty::fromMapItem(item);

    emit sgNodeChanged();
}

void QGeoMapMapboxGL::onMapItemSubPropertyChanged()
{
    Q_D(QGeoMapMapboxGL);

    QDeclarativeGeoMapItemBase *item = static_cast<QDeclarativeGeoMapItemBase *>(sender()->parent());
    d->m_styleChanges << QMapboxGLStyleSetPaintProperty::fromMapItem(item);

    emit sgNodeChanged();
}

void QGeoMapMapboxGL::onMapItemUnsupportedPropertyChanged()
{
    // TODO https://bugreports.qt.io/browse/QTBUG-58872
    qWarning() << "Unsupported property for managed Map item";
}

void QGeoMapMapboxGL::onMapItemGeometryChanged()
{
    Q_D(QGeoMapMapboxGL);

    QDeclarativeGeoMapItemBase *item = static_cast<QDeclarativeGeoMapItemBase *>(sender());
    d->m_styleChanges << QMapboxGLStyleAddSource::fromMapItem(item);

    emit sgNodeChanged();
}

void QGeoMapMapboxGL::copyrightsChanged(const QString &copyrightsHtml)
{
    Q_D(QGeoMapMapboxGL);

    QString copyrightsHtmlFinal = copyrightsHtml;

    if (d->m_developmentMode) {
        copyrightsHtmlFinal.prepend("<a href='https://www.mapbox.com/pricing'>"
            + tr("Development access token, do not use in production.") + "</a> - ");
    }

    if (d->m_activeMapType.name().startsWith("mapbox://")) {
        copyrightsHtmlFinal = "<table><tr><th><img src='qrc:/mapboxgl/logo.png'/></th><th>"
            + copyrightsHtmlFinal + "</th></tr></table>";
    }

    QGeoMap::copyrightsChanged(copyrightsHtmlFinal);
}
