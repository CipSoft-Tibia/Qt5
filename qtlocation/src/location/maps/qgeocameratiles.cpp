// Copyright (C) 2015 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qgeocameratiles_p.h"
#include "qgeocameratiles_p_p.h"
#include "qgeocameradata_p.h"
#include "qgeotilespec_p.h"
#include "qgeomaptype_p.h"

#include <QtGui/QMatrix4x4>
#include <QList>
#include <QMap>
#include <QPair>
#include <QSet>
#include <QSize>

#include <QtPositioning/private/qwebmercator_p.h>
#include <QtPositioning/private/qdoublevector2d_p.h>
#include <QtPositioning/private/qdoublevector3d_p.h>
#include <QtPositioning/private/qlocationutils_p.h>

#include <cmath>
#include <limits>

static QVector3D toVector3D(const QDoubleVector3D& in)
{
    return QVector3D(in.x(), in.y(), in.z());
}

static QDoubleVector3D toDoubleVector3D(const QVector3D& in)
{
    return QDoubleVector3D(in.x(), in.y(), in.z());
}

QT_BEGIN_NAMESPACE

QGeoCameraTiles::QGeoCameraTiles()
    : d_ptr(new QGeoCameraTilesPrivate()) {}

QGeoCameraTiles::~QGeoCameraTiles()
{
}

void QGeoCameraTiles::setCameraData(const QGeoCameraData &camera)
{
    if (d_ptr->m_camera == camera)
        return;

    d_ptr->m_dirtyGeometry = true;
    d_ptr->m_camera = camera;
    d_ptr->m_intZoomLevel = static_cast<int>(std::floor(d_ptr->m_camera.zoomLevel()));
    d_ptr->m_sideLength = 1 << d_ptr->m_intZoomLevel;
}

QGeoCameraData QGeoCameraTiles::cameraData() const
{
    return d_ptr->m_camera;
}

void QGeoCameraTiles::setVisibleArea(const QRectF &visibleArea)
{
    if (d_ptr->m_visibleArea == visibleArea)
        return;

    d_ptr->m_visibleArea = visibleArea;
    d_ptr->m_dirtyGeometry = true;
}

void QGeoCameraTiles::setScreenSize(const QSize &size)
{
    if (d_ptr->m_screenSize == size)
        return;

    d_ptr->m_dirtyGeometry = true;
    d_ptr->m_screenSize = size;
}

void QGeoCameraTiles::setPluginString(const QString &pluginString)
{
    if (d_ptr->m_pluginString == pluginString)
        return;

    d_ptr->m_dirtyMetadata = true;
    d_ptr->m_pluginString = pluginString;
}

void QGeoCameraTiles::setMapType(const QGeoMapType &mapType)
{
    if (d_ptr->m_mapType == mapType)
        return;

    d_ptr->m_dirtyMetadata = true;
    d_ptr->m_mapType = mapType;
}

QGeoMapType QGeoCameraTiles::activeMapType() const
{
    return d_ptr->m_mapType;
}

void QGeoCameraTiles::setMapVersion(int mapVersion)
{
    if (d_ptr->m_mapVersion == mapVersion)
        return;

    d_ptr->m_dirtyMetadata = true;
    d_ptr->m_mapVersion = mapVersion;
}

void QGeoCameraTiles::setTileSize(int tileSize)
{
    if (d_ptr->m_tileSize == tileSize)
        return;

    d_ptr->m_dirtyGeometry = true;
    d_ptr->m_tileSize = tileSize;
}

void QGeoCameraTiles::setViewExpansion(double viewExpansion)
{
    d_ptr->m_viewExpansion = viewExpansion;
    d_ptr->m_dirtyGeometry = true;
}

int QGeoCameraTiles::tileSize() const
{
    return d_ptr->m_tileSize;
}

const QSet<QGeoTileSpec>& QGeoCameraTiles::createTiles()
{
    if (d_ptr->m_dirtyGeometry) {
        d_ptr->m_tiles.clear();
        d_ptr->updateGeometry();
        d_ptr->m_dirtyGeometry = false;
    }

    if (d_ptr->m_dirtyMetadata) {
        d_ptr->updateMetadata();
        d_ptr->m_dirtyMetadata = false;
    }

    return d_ptr->m_tiles;
}


void QGeoCameraTilesPrivate::updateMetadata()
{
    typedef QSet<QGeoTileSpec>::const_iterator iter;

    QSet<QGeoTileSpec> newTiles;

    iter i = m_tiles.constBegin();
    iter end = m_tiles.constEnd();

    for (; i != end; ++i) {
        QGeoTileSpec tile = *i;
        newTiles.insert(QGeoTileSpec(m_pluginString, m_mapType.mapId(), tile.zoom(), tile.x(), tile.y(), m_mapVersion));
    }

    m_tiles = newTiles;
}

void QGeoCameraTilesPrivate::updateGeometry()
{
    // Find the frustum from the camera / screen / viewport information
    // The larger frustum when stationary is a form of prefetching
    Frustum f = createFrustum(m_viewExpansion);
#ifdef QT_LOCATION_DEBUG
    m_frustum = f;
#endif

    // Find the polygon where the frustum intersects the plane of the map
    PolygonVector footprint = frustumFootprint(f);
#ifdef QT_LOCATION_DEBUG
    m_frustumFootprint = footprint;
#endif

    // Clip the polygon to the map, split it up if it cross the dateline
    ClippedFootprint polygons = clipFootprintToMap(footprint);
#ifdef QT_LOCATION_DEBUG
    m_clippedFootprint = polygons;
#endif


    if (!polygons.left.isEmpty()) {
        QSet<QGeoTileSpec> tilesLeft = tilesFromPolygon(polygons.left);
        m_tiles.unite(tilesLeft);
    }

    if (!polygons.right.isEmpty()) {
        QSet<QGeoTileSpec> tilesRight = tilesFromPolygon(polygons.right);
        m_tiles.unite(tilesRight);
    }

    if (!polygons.mid.isEmpty()) {
        QSet<QGeoTileSpec> tilesRight = tilesFromPolygon(polygons.mid);
        m_tiles.unite(tilesRight);
    }
}

Frustum QGeoCameraTilesPrivate::createFrustum(double viewExpansion) const
{
    double apertureSize = 1.0;
    if (m_camera.fieldOfView() != 90.0) //aperture(90 / 2) = 1
        apertureSize = tan(QLocationUtils::radians(m_camera.fieldOfView()) * 0.5);
    QDoubleVector3D center = m_sideLength * QWebMercator::coordToMercator(m_camera.center());
#ifdef QT_LOCATION_DEBUG
    m_createFrustum_center = center;
#endif


    double f = m_screenSize.height();

    double z = std::pow(2.0, m_camera.zoomLevel() - m_intZoomLevel) * m_tileSize; // between 1 and 2 * m_tileSize

    double altitude = (f / (2.0 * z)) / apertureSize;
    QDoubleVector3D eye = center;
    eye.setZ(altitude);

    QDoubleVector3D view = eye - center;
    QDoubleVector3D side = QDoubleVector3D::normal(view, QDoubleVector3D(0.0, 1.0, 0.0));
    QDoubleVector3D up = QDoubleVector3D::normal(side, view);

    QMatrix4x4 mBearing;
    // The rotation direction here is the opposite of QGeoTiledMapScene::setupCamera,
    // as this is basically rotating the map against a fixed view frustum.
    mBearing.rotate(1.0 * m_camera.bearing(), toVector3D(view));
    up = toDoubleVector3D(mBearing.map(toVector3D(up)));

    // same for tilting
    QDoubleVector3D side2 = QDoubleVector3D::normal(up, view);
    QMatrix4x4 mTilt;
    mTilt.rotate(-1.0 * m_camera.tilt(), toVector3D(side2));
    eye = toDoubleVector3D((mTilt.map(toVector3D(view))) + toVector3D(center));

    view = eye - center;
    side = QDoubleVector3D::normal(view, QDoubleVector3D(0.0, 1.0, 0.0));
    up = QDoubleVector3D::normal(view, side2);

    double nearPlane =  1.0 / 32.0; // The denominator used to be (4.0 * m_tileSize ), which produces an extremely narrow and tiny near plane.
    // farPlane plays a role on how much gets clipped when the map gets tilted. It used to be altitude + 1.0
    // The value of 8.0 has been chosen as an acceptable compromise.
    // TODO: use m_camera.clipDistance(); when this will be introduced
    double farPlane = altitude + 8.0;

    double aspectRatio = 1.0 * m_screenSize.width() / m_screenSize.height();

    // Half values. Half width near, far, height near, far.
    double hhn,hwn,hhf,hwf = 0.0;

    // This used to fix the (half) field of view at 45 degrees
    // half because this assumed that viewSize = 2*nearPlane x 2*nearPlane
    viewExpansion *= apertureSize;

    hhn = viewExpansion * nearPlane;
    hwn = hhn * aspectRatio;

    hhf = viewExpansion * farPlane;
    hwf = hhf * aspectRatio;

    QDoubleVector3D d = center - eye;
    d.normalize();
    up.normalize();
    QDoubleVector3D right = QDoubleVector3D::normal(d, up);

    QDoubleVector3D cf = eye + d * farPlane;
    QDoubleVector3D cn = eye + d * nearPlane;

    Frustum frustum;

    frustum.apex = eye;
#ifdef QT_LOCATION_DEBUG
    m_createFrustum_eye = eye;
#endif

    QRectF va = m_visibleArea;
    if (va.isNull())
        va = QRectF(0, 0, m_screenSize.width(), m_screenSize.height());
    QRectF screen = QRectF(QPointF(0,0),m_screenSize);
    QPointF vaCenter = va.center();
    QPointF screenCenter = screen.center();
    QPointF diff = screenCenter - vaCenter;
    double xdiffpct = diff.x() / m_screenSize.width();
    double ydiffpct = -(diff.y() / m_screenSize.height());

    double wn = (2 * hwn) * xdiffpct;
    double hn = (2 * hhn) * ydiffpct;
    double wf = (2 * hwf) * xdiffpct;
    double hf = (2 * hhf) * ydiffpct;

    // TODO: fix eye

    frustum.topLeftFar = cf - (up * (hhf + hf)) - (right * (hwf + wf));
    frustum.topRightFar = cf - (up * (hhf + hf)) + (right * (hwf + wf));
    frustum.bottomLeftFar = cf + (up * (hhf + hf)) - (right * (hwf + wf));
    frustum.bottomRightFar = cf + (up * (hhf + hf)) + (right * (hwf + wf));

    frustum.topLeftNear = cn - (up * (hhn + hn)) - (right * (hwn + wn));
    frustum.topRightNear = cn - (up * (hhn + hn)) + (right * (hwn + wn));
    frustum.bottomLeftNear = cn + (up * (hhn + hn)) - (right * (hwn + wn));
    frustum.bottomRightNear = cn + (up * (hhn + hn)) + (right * (hwn + wn));

    return frustum;
}

static bool appendZIntersects(const QDoubleVector3D &start, const QDoubleVector3D &end, double z,
                              QList<QDoubleVector3D> &results)
{
    if (start.z() == end.z()) {
        return false;
    } else {
        double f = (start.z() - z) / (start.z() - end.z());
        if ((f >= 0) && (f <= 1.0)) {
            results.append((1 - f) * start + f * end);
            return true;
        }
    }
    return false;
}

// Returns the intersection of the plane of the map and the camera frustum as a right handed polygon
PolygonVector QGeoCameraTilesPrivate::frustumFootprint(const Frustum &frustum) const
{
    PolygonVector points;
    points.reserve(4);

    // The camera is always upright. Tilting angle never reach 90degrees.
    // Meaning: bottom frustum edges always intersect the map plane, top ones may not.

    // Top Right
    if (!appendZIntersects(frustum.apex, frustum.topRightFar, 0.0, points))
        appendZIntersects(frustum.topRightFar, frustum.bottomRightFar, 0.0, points);

    // Bottom Right
    appendZIntersects(frustum.apex, frustum.bottomRightFar, 0.0, points);

    // Bottom Left
    appendZIntersects(frustum.apex, frustum.bottomLeftFar, 0.0, points);

    // Top Left
    if (!appendZIntersects(frustum.apex, frustum.topLeftFar, 0.0, points))
        appendZIntersects(frustum.topLeftFar, frustum.bottomLeftFar, 0.0, points);

    return points;
}

QPair<PolygonVector, PolygonVector> QGeoCameraTilesPrivate::splitPolygonAtAxisValue(const PolygonVector &polygon, int axis, double value) const
{
    PolygonVector polygonBelow;
    PolygonVector polygonAbove;

    const qsizetype size = polygon.size();

    if (size == 0)
        return QPair<PolygonVector, PolygonVector>(polygonBelow, polygonAbove);

    QList<int> comparisons(polygon.size());

    for (qsizetype i = 0; i < size; ++i) {
        const double v = polygon.at(i).get(axis);
        if (qFuzzyCompare(v - value + 1.0, 1.0)) {
            comparisons[i] = 0;
        } else {
            if (v < value) {
                comparisons[i] = -1;
            } else if (value < v) {
                comparisons[i] = 1;
            }
        }
    }

    for (qsizetype index = 0; index < size; ++index) {
        qsizetype prevIndex = index - 1;
        if (prevIndex < 0)
            prevIndex += size;
        const qsizetype nextIndex = (index + 1) % size;

        int prevComp = comparisons[prevIndex];
        int comp = comparisons[index];
        int nextComp = comparisons[nextIndex];

         if (comp == 0) {
            if (prevComp == -1) {
                polygonBelow.append(polygon.at(index));
                if (nextComp == 1) {
                    polygonAbove.append(polygon.at(index));
                }
            } else if (prevComp == 1) {
                polygonAbove.append(polygon.at(index));
                if (nextComp == -1) {
                    polygonBelow.append(polygon.at(index));
                }
            } else if (prevComp == 0) {
                if (nextComp == -1) {
                    polygonBelow.append(polygon.at(index));
                } else if (nextComp == 1) {
                    polygonAbove.append(polygon.at(index));
                } else if (nextComp == 0) {
                    // do nothing
                }
            }
        } else {
             if (comp == -1) {
                 polygonBelow.append(polygon.at(index));
             } else if (comp == 1) {
                 polygonAbove.append(polygon.at(index));
             }

             // there is a point between this and the next point
             // on the polygon that lies on the splitting line
             // and should be added to both the below and above
             // polygons
             if ((nextComp != 0) && (nextComp != comp)) {
                 QDoubleVector3D p1 = polygon.at(index);
                 QDoubleVector3D p2 = polygon.at(nextIndex);

                 double p1v = p1.get(axis);
                 double p2v = p2.get(axis);

                 double f = (p1v - value) / (p1v - p2v);

                 if (((0 <= f) && (f <= 1.0))
                         || qFuzzyCompare(f + 1.0, 1.0)
                         || qFuzzyCompare(f + 1.0, 2.0) ) {
                     QDoubleVector3D midPoint = (1.0 - f) * p1 + f * p2;
                     polygonBelow.append(midPoint);
                     polygonAbove.append(midPoint);
                 }
             }
        }
    }

    return QPair<PolygonVector, PolygonVector>(polygonBelow, polygonAbove);
}

static void addXOffset(PolygonVector &footprint, double xoff)
{
    for (QDoubleVector3D &v: footprint)
        v.setX(v.x() + xoff);
}

QGeoCameraTilesPrivate::ClippedFootprint QGeoCameraTilesPrivate::clipFootprintToMap(const PolygonVector &footprint) const
{
    bool clipX0 = false;
    bool clipX1 = false;
    bool clipY0 = false;
    bool clipY1 = false;

    double side = 1.0 * m_sideLength;
    double minX = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();

    for (const QDoubleVector3D &p: footprint) {
        if (p.y() < 0.0)
            clipY0 = true;
        if (p.y() > side)
            clipY1 = true;
    }

    PolygonVector results = footprint;

    if (clipY0)
        results = splitPolygonAtAxisValue(results, 1, 0.0).second;

    if (clipY1)
        results = splitPolygonAtAxisValue(results, 1, side).first;

    for (const QDoubleVector3D &p : std::as_const(results)) {
        if ((p.x() < 0.0) || (qFuzzyIsNull(p.x())))
            clipX0 = true;
        if ((p.x() > side) || (qFuzzyCompare(side, p.x())))
            clipX1 = true;
    }

    for (const QDoubleVector3D &v : std::as_const(results)) {
        minX = qMin(v.x(), minX);
        maxX = qMax(v.x(), maxX);
    }

    double footprintWidth = maxX - minX;

    if (clipX0) {
        if (clipX1) {
            if (footprintWidth > side) {
                PolygonVector rightPart = splitPolygonAtAxisValue(results, 0, side).second;
                addXOffset(rightPart,  -side);
                rightPart = splitPolygonAtAxisValue(rightPart, 0, side).first; // clip it again, should it tend to infinite or so

                PolygonVector leftPart = splitPolygonAtAxisValue(results, 0, 0).first;
                addXOffset(leftPart,  side);
                leftPart = splitPolygonAtAxisValue(leftPart, 0, 0).second; // same here

                results = splitPolygonAtAxisValue(results, 0, 0.0).second;
                results = splitPolygonAtAxisValue(results, 0, side).first;
                return ClippedFootprint(leftPart, results, rightPart);
            } else { // fitting the WebMercator square exactly?
                results = splitPolygonAtAxisValue(results, 0, 0.0).second;
                results = splitPolygonAtAxisValue(results, 0, side).first;
                return ClippedFootprint(PolygonVector(), results, PolygonVector());
            }
        } else {
            QPair<PolygonVector, PolygonVector> pair = splitPolygonAtAxisValue(results, 0, 0.0);
            if (pair.first.isEmpty()) {
                // if we touched the line but didn't cross it...
                for (const auto &v : std::as_const(pair.second)) {
                    if (qFuzzyIsNull(v.x()))
                        pair.first.append(v);
                }
                if (pair.first.size() == 2) {
                    double y0 = pair.first[0].y();
                    double y1 = pair.first[1].y();
                    pair.first.clear();
                    pair.first.append(QDoubleVector3D(side, y0, 0.0));
                    pair.first.append(QDoubleVector3D(side - 0.001, y0, 0.0));
                    pair.first.append(QDoubleVector3D(side - 0.001, y1, 0.0));
                    pair.first.append(QDoubleVector3D(side, y1, 0.0));
                } else if (pair.first.size() == 1) {
                    // FIXME this is trickier
                    // - touching at one point on the tile boundary
                    // - probably need to build a triangular polygon across the edge
                    // - don't want to add another y tile if we can help it
                    //   - initial version doesn't care
                    double y = pair.first.at(0).y();
                    pair.first.clear();
                    pair.first.append(QDoubleVector3D(side - 0.001, y, 0.0));
                    pair.first.append(QDoubleVector3D(side, y + 0.001, 0.0));
                    pair.first.append(QDoubleVector3D(side, y - 0.001, 0.0));
                }
            } else {
                addXOffset(pair.first, side);
                if (footprintWidth > side)
                    pair.first = splitPolygonAtAxisValue(pair.first, 0, 0).second;
            }
            return ClippedFootprint(pair.first, pair.second, PolygonVector());
        }
    } else {
        if (clipX1) {
            QPair<PolygonVector, PolygonVector> pair = splitPolygonAtAxisValue(results, 0, side);
            if (pair.second.isEmpty()) {
                // if we touched the line but didn't cross it...
                for (const auto &v : std::as_const(pair.first)) {
                    if (qFuzzyCompare(side, v.x()))
                        pair.second.append(v);
                }
                if (pair.second.size() == 2) {
                    double y0 = pair.second[0].y();
                    double y1 = pair.second[1].y();
                    pair.second.clear();
                    pair.second.append(QDoubleVector3D(0, y0, 0.0));
                    pair.second.append(QDoubleVector3D(0.001, y0, 0.0));
                    pair.second.append(QDoubleVector3D(0.001, y1, 0.0));
                    pair.second.append(QDoubleVector3D(0, y1, 0.0));
                } else if (pair.second.size() == 1) {
                    // FIXME this is trickier
                    // - touching at one point on the tile boundary
                    // - probably need to build a triangular polygon across the edge
                    // - don't want to add another y tile if we can help it
                    //   - initial version doesn't care
                    double y = pair.second.at(0).y();
                    pair.second.clear();
                    pair.second.append(QDoubleVector3D(0.001, y, 0.0));
                    pair.second.append(QDoubleVector3D(0.0, y - 0.001, 0.0));
                    pair.second.append(QDoubleVector3D(0.0, y + 0.001, 0.0));
                }
            } else {
                addXOffset(pair.second, -side);
                if (footprintWidth > side)
                    pair.second = splitPolygonAtAxisValue(pair.second, 0, side).first;
            }
            return ClippedFootprint(PolygonVector(), pair.first, pair.second);
        } else {
            return ClippedFootprint(PolygonVector(), results, PolygonVector());
        }
    }

}

QList<QPair<double, int>> QGeoCameraTilesPrivate::tileIntersections(double p1, int t1, double p2, int t2) const
{
    if (t1 == t2) {
        QList<QPair<double, int>> results = QList<QPair<double, int>>();
        results.append(QPair<double, int>(0.0, t1));
        return results;
    }

    int step = 1;
    if (t1 > t2)
        step = -1;

    qsizetype size = 1 + ((t2 - t1) / step);

    QList<QPair<double, int>> results = { QPair<double, int>(0.0, t1) };

    if (step == 1) {
        for (qsizetype i = 1; i < size; ++i) {
            double f = (t1 + i - p1) / (p2 - p1);
            results.append(QPair<double, int>(f, t1 + i));
        }
    } else {
        for (qsizetype i = 1; i < size; ++i) {
            double f = (t1 - i + 1 - p1) / (p2 - p1);
            results.append(QPair<double, int>(f, t1 - i));
        }
    }

    return results;
}

QSet<QGeoTileSpec> QGeoCameraTilesPrivate::tilesFromPolygon(const PolygonVector &polygon) const
{
    const qsizetype numPoints = polygon.size();

    if (numPoints == 0)
        return QSet<QGeoTileSpec>();

    QList<int> tilesX(polygon.size());
    QList<int> tilesY(polygon.size());

    // grab tiles at the corners of the polygon
    for (qsizetype i = 0; i < numPoints; ++i) {

        const QDoubleVector2D p = polygon.at(i).toVector2D();

        int x = 0;
        int y = 0;

        if (qFuzzyCompare(p.x(), m_sideLength * 1.0))
            x = m_sideLength - 1;
        else {
            x = static_cast<int>(p.x()) % m_sideLength;
            if ( !qFuzzyCompare(p.x(), 1.0 * x) && qFuzzyCompare(p.x(), 1.0 * (x + 1)) )
                x++;
        }

        if (qFuzzyCompare(p.y(), m_sideLength * 1.0))
            y = m_sideLength - 1;
        else {
            y = static_cast<int>(p.y()) % m_sideLength;
            if ( !qFuzzyCompare(p.y(), 1.0 * y) && qFuzzyCompare(p.y(), 1.0 * (y + 1)) )
                y++;
        }

        tilesX[i] = x;
        tilesY[i] = y;
    }

    QGeoCameraTilesPrivate::TileMap map;

    // walk along the edges of the polygon and add all tiles covered by them
    for (qsizetype i1 = 0; i1 < numPoints; ++i1) {
        const qsizetype i2 = (i1 + 1) % numPoints;

        const double x1 = polygon.at(i1).get(0);
        const double x2 = polygon.at(i2).get(0);

        const bool xFixed = qFuzzyCompare(x1, x2);
        const bool xIntegral = qFuzzyCompare(x1, std::floor(x1)) || qFuzzyCompare(x1 + 1.0, std::floor(x1 + 1.0));

        QList<QPair<double, int> > xIntersects
                = tileIntersections(x1,
                                    tilesX.at(i1),
                                    x2,
                                    tilesX.at(i2));

        const double y1 = polygon.at(i1).get(1);
        const double y2 = polygon.at(i2).get(1);

        const bool yFixed = qFuzzyCompare(y1, y2);
        const bool yIntegral = qFuzzyCompare(y1, std::floor(y1)) || qFuzzyCompare(y1 + 1.0, std::floor(y1 + 1.0));

        QList<QPair<double, int> > yIntersects
                = tileIntersections(y1,
                                    tilesY.at(i1),
                                    y2,
                                    tilesY.at(i2));

        int x = xIntersects.takeFirst().second;
        int y = yIntersects.takeFirst().second;


        /*
          If the polygon coincides with the tile edges we must be
          inclusive and grab all tiles on both sides. We also need
          to handle tiles with corners coindent with the
          corners of the polygon.
          e.g. all tiles marked with 'x' will be added

              "+" - tile boundaries
              "O" - polygon boundary

                + + + + + + + + + + + + + + + + + + + + +
                +       +       +       +       +       +
                +       +   x   +   x   +   x   +       +
                +       +       +       +       +       +
                + + + + + + + + O O O O O + + + + + + + +
                +       +       O       0       +       +
                +       +   x   O   x   0   x   +       +
                +       +       O       0       +       +
                + + + + + + + + O 0 0 0 0 + + + + + + + +
                +       +       +       +       +       +
                +       +   x   +   x   +   x   +       +
                +       +       +       +       +       +
                + + + + + + + + + + + + + + + + + + + + +
        */


        int xOther = x;
        int yOther = y;

        if (xFixed && xIntegral) {
             if (y2 < y1) {
                 xOther = qMax(0, x - 1);
            }
        }

        if (yFixed && yIntegral) {
            if (x1 < x2) {
                yOther = qMax(0, y - 1);

            }
        }

        if (xIntegral) {
            map.add(xOther, y);
            if (yIntegral)
                map.add(xOther, yOther);

        }

        if (yIntegral)
            map.add(x, yOther);

        map.add(x,y);

        // top left corner
        const qsizetype iPrev =  (i1 + numPoints - 1) % numPoints;
        const double xPrevious = polygon.at(iPrev).get(0);
        const double yPrevious = polygon.at(iPrev).get(1);
        const bool xPreviousFixed = qFuzzyCompare(xPrevious, x1);
        if (xIntegral && xPreviousFixed && yIntegral && yFixed) {
            if ((x2 > x1) && (yPrevious > y1)) {
                if ((x - 1) > 0 && (y - 1) > 0)
                    map.add(x - 1, y - 1);
            } else if ((x2 < x1) && (yPrevious < y1)) {
                // what?
            }
        }

        // for the simple case where intersections do not coincide with
        // the boundaries, we move along the edge and add tiles until
        // the x and y intersection lists are exhausted

        while (!xIntersects.isEmpty() && !yIntersects.isEmpty()) {
            const QPair<double, int> nextX = xIntersects.first();
            const QPair<double, int> nextY = yIntersects.first();
            if (nextX.first < nextY.first) {
                x = nextX.second;
                map.add(x, y);
                xIntersects.removeFirst();

            } else if (nextX.first > nextY.first) {
                y = nextY.second;
                map.add(x, y);
                yIntersects.removeFirst();

            } else {
                map.add(x, nextY.second);
                map.add(nextX.second, y);
                x = nextX.second;
                y = nextY.second;
                map.add(x, y);
                xIntersects.removeFirst();
                yIntersects.removeFirst();
            }
        }

        while (!xIntersects.isEmpty()) {
            x = xIntersects.takeFirst().second;
            map.add(x, y);
            if (yIntegral && yFixed)
                map.add(x, yOther);

        }

        while (!yIntersects.isEmpty()) {
            y = yIntersects.takeFirst().second;
            map.add(x, y);
            if (xIntegral && xFixed)
                map.add(xOther, y);
        }
    }

    QSet<QGeoTileSpec> results;

    const int z = m_intZoomLevel;
    for (auto i = map.data.constBegin(); i != map.data.constEnd(); ++i) {
        int y = i.key();
        int minX = i->first;
        int maxX = i->second;
        for (int x = minX; x <= maxX; ++x)
            results.insert(QGeoTileSpec(m_pluginString, m_mapType.mapId(), z, x, y, m_mapVersion));
    }

    return results;
}

QGeoCameraTilesPrivate::TileMap::TileMap() {}

void QGeoCameraTilesPrivate::TileMap::add(int tileX, int tileY)
{
    if (data.contains(tileY)) {
        int oldMinX = data.value(tileY).first;
        int oldMaxX = data.value(tileY).second;
        data.insert(tileY, QPair<int, int>(qMin(tileX, oldMinX), qMax(tileX, oldMaxX)));
    } else {
        data.insert(tileY, QPair<int, int>(tileX, tileX));
    }
}

QT_END_NAMESPACE
