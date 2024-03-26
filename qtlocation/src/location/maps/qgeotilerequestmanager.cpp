// Copyright (C) 2015 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qgeotilerequestmanager_p.h"
#include "qgeotilespec_p.h"
#include "qgeotiledmap_p.h"
#include "qgeotiledmappingmanagerengine_p.h"
#include "qabstractgeotilecache_p.h"

#include <QtCore/QPointer>
#include <QtCore/QTimer>

QT_BEGIN_NAMESPACE

class RetryFuture;

class QGeoTileRequestManagerPrivate
{
public:
    explicit QGeoTileRequestManagerPrivate(QGeoTiledMap *map, QGeoTiledMappingManagerEngine *engine);
    ~QGeoTileRequestManagerPrivate();

    QGeoTiledMap *m_map;
    QPointer<QGeoTiledMappingManagerEngine> m_engine;

    QMap<QGeoTileSpec, QSharedPointer<QGeoTileTexture> > requestTiles(const QSet<QGeoTileSpec> &tiles);
    void tileError(const QGeoTileSpec &tile, const QString &errorString);

    QHash<QGeoTileSpec, int> m_retries;
    QHash<QGeoTileSpec, QSharedPointer<RetryFuture> > m_futures;
    QSet<QGeoTileSpec> m_requested;

    void tileFetched(const QGeoTileSpec &spec);
};

QGeoTileRequestManager::QGeoTileRequestManager(QGeoTiledMap *map, QGeoTiledMappingManagerEngine *engine)
    : d_ptr(new QGeoTileRequestManagerPrivate(map, engine))
{

}

QGeoTileRequestManager::~QGeoTileRequestManager()
{

}

QMap<QGeoTileSpec, QSharedPointer<QGeoTileTexture> > QGeoTileRequestManager::requestTiles(const QSet<QGeoTileSpec> &tiles)
{
    return d_ptr->requestTiles(tiles);
}

void QGeoTileRequestManager::tileFetched(const QGeoTileSpec &spec)
{
    d_ptr->tileFetched(spec);
}

QSharedPointer<QGeoTileTexture> QGeoTileRequestManager::tileTexture(const QGeoTileSpec &spec)
{
    if (d_ptr->m_engine)
        return d_ptr->m_engine->getTileTexture(spec);
    else
        return QSharedPointer<QGeoTileTexture>();
}

void QGeoTileRequestManager::tileError(const QGeoTileSpec &tile, const QString &errorString)
{
    d_ptr->tileError(tile, errorString);
}

QGeoTileRequestManagerPrivate::QGeoTileRequestManagerPrivate(QGeoTiledMap *map,QGeoTiledMappingManagerEngine *engine)
    : m_map(map),
      m_engine(engine)
{
}

QGeoTileRequestManagerPrivate::~QGeoTileRequestManagerPrivate()
{
}

QMap<QGeoTileSpec, QSharedPointer<QGeoTileTexture> > QGeoTileRequestManagerPrivate::requestTiles(const QSet<QGeoTileSpec> &tiles)
{
    QSet<QGeoTileSpec> cancelTiles = m_requested - tiles;
    QSet<QGeoTileSpec> requestTiles = tiles - m_requested;
    QSet<QGeoTileSpec> cached;
//    int tileSize = tiles.size();
//    int newTiles = requestTiles.size();

    typedef QSet<QGeoTileSpec>::const_iterator iter;

    QMap<QGeoTileSpec, QSharedPointer<QGeoTileTexture> > cachedTex;

    // remove tiles in cache from request tiles
    if (!m_engine.isNull()) {
        iter i = requestTiles.constBegin();
        iter end = requestTiles.constEnd();
        for (; i != end; ++i) {
            QGeoTileSpec tile = *i;
            QSharedPointer<QGeoTileTexture> tex = m_engine->getTileTexture(tile);
            if (tex) {
                if (!tex->image.isNull())
                    cachedTex.insert(tile, tex);
                cached.insert(tile);
            } else {
                // Try to use textures from lower zoom levels, but still request the proper tile
                QGeoTileSpec spec = tile;
                const int endRange = qMax(0, tile.zoom() - 4); // Using up to 4 zoom levels up. 4 is arbitrary.
                for (int z = tile.zoom() - 1; z >= endRange; z--) {
                    int denominator = 1 << (tile.zoom() - z);
                    spec.setZoom(z);
                    spec.setX(tile.x() / denominator);
                    spec.setY(tile.y() / denominator);
                    QSharedPointer<QGeoTileTexture> t = m_engine->getTileTexture(spec);
                    if (t && !t->image.isNull()) {
                        cachedTex.insert(tile, t);
                        break;
                    }
                }
            }
        }
    }

    requestTiles -= cached;

    m_requested -= cancelTiles;
    m_requested += requestTiles;

//    qDebug() << "required # tiles: " << tileSize << ", new tiles: " << newTiles << ", total server requests: " << requested_.size();

    if (!requestTiles.isEmpty() || !cancelTiles.isEmpty()) {
        if (!m_engine.isNull()) {
//            qDebug() << "new server requests: " << requestTiles.size() << ", server cancels: " << cancelTiles.size();
            m_engine->updateTileRequests(m_map, requestTiles, cancelTiles);

            // Remove any cancelled tiles from the error retry hash to avoid
            // re-using the numbers for a totally different request cycle.
            iter i = cancelTiles.constBegin();
            iter end = cancelTiles.constEnd();
            for (; i != end; ++i) {
                m_retries.remove(*i);
                m_futures.remove(*i);
            }
        }
    }

    return cachedTex;
}

void QGeoTileRequestManagerPrivate::tileFetched(const QGeoTileSpec &spec)
{
    m_map->updateTile(spec);
    m_requested.remove(spec);
    m_retries.remove(spec);
    m_futures.remove(spec);
}

// Represents a tile that needs to be retried after a certain period of time
class RetryFuture : public QObject
{
    Q_OBJECT
public:
    RetryFuture(const QGeoTileSpec &tile, QGeoTiledMap *map, QGeoTiledMappingManagerEngine* engine, QObject *parent = nullptr);

public Q_SLOTS:
    void retry();

private:
    QGeoTileSpec m_tile;
    QGeoTiledMap *m_map;
    QPointer<QGeoTiledMappingManagerEngine> m_engine;
};

RetryFuture::RetryFuture(const QGeoTileSpec &tile, QGeoTiledMap *map, QGeoTiledMappingManagerEngine* engine, QObject *parent)
    : QObject(parent), m_tile(tile), m_map(map), m_engine(engine)
{}

void RetryFuture::retry()
{
    QSet<QGeoTileSpec> requestTiles;
    QSet<QGeoTileSpec> cancelTiles;
    requestTiles.insert(m_tile);
    if (!m_engine.isNull())
        m_engine->updateTileRequests(m_map, requestTiles, cancelTiles);
}

void QGeoTileRequestManagerPrivate::tileError(const QGeoTileSpec &tile, const QString &errorString)
{
    if (m_requested.contains(tile)) {
        int count = m_retries.value(tile, 0);
        m_retries.insert(tile, count + 1);

        if (count >= 5) {
            qWarning("QGeoTileRequestManager: Failed to fetch tile (%d,%d,%d) 5 times, giving up. "
                     "Last error message was: '%s'",
                     tile.x(), tile.y(), tile.zoom(), qPrintable(errorString));
            m_requested.remove(tile);
            m_retries.remove(tile);
            m_futures.remove(tile);

        } else {
            // Exponential time backoff when retrying
            int delay = (1 << count) * 500;

            QSharedPointer<RetryFuture> future(new RetryFuture(tile,m_map,m_engine));
            m_futures.insert(tile, future);

            QTimer::singleShot(delay, future.data(), &RetryFuture::retry);
            // Passing .data() to singleShot is ok -- Qt will clean up the
            // connection if the target qobject is deleted
        }
    }
}

QT_END_NAMESPACE

#include "qgeotilerequestmanager.moc"
