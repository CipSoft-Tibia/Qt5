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

#include "qgeoprojection_p.h"
#include <QtPositioning/private/qwebmercator_p.h>
#include <QtPositioning/private/qlocationutils_p.h>
#include <QtPositioning/private/qclipperutils_p.h>
#include <QtPositioning/QGeoPolygon>
#include <QtPositioning/QGeoRectangle>
#include <QSize>
#include <QtGui/QMatrix4x4>
#include <cmath>

namespace {
    static const double defaultTileSize = 256.0;
    static const QDoubleVector3D xyNormal(0.0, 0.0, 1.0);
    static const QGeoProjectionWebMercator::Plane xyPlane(QDoubleVector3D(0,0,0), QDoubleVector3D(0,0,1));
    static const QList<QDoubleVector2D> mercatorGeometry = {
                                                QDoubleVector2D(-1.0,0.0),
                                                QDoubleVector2D( 2.0,0.0),
                                                QDoubleVector2D( 2.0,1.0),
                                                QDoubleVector2D(-1.0,1.0) };
}

static QMatrix4x4 toMatrix4x4(const QDoubleMatrix4x4 &m)
{
    return QMatrix4x4(m(0,0), m(0,1), m(0,2), m(0,3),
                      m(1,0), m(1,1), m(1,2), m(1,3),
                      m(2,0), m(2,1), m(2,2), m(2,3),
                      m(3,0), m(3,1), m(3,2), m(3,3));
}

static QPointF centerOffset(const QSizeF &screenSize, const QRectF &visibleArea)
{
    QRectF va = visibleArea;
    if (va.isNull())
        va = QRectF(0, 0, screenSize.width(), screenSize.height());

    QRectF screen = QRectF(QPointF(0,0),screenSize);
    QPointF vaCenter = va.center();

    QPointF screenCenter = screen.center();
    QPointF diff = screenCenter - vaCenter;

    return diff;
}

static QPointF marginsOffset(const QSizeF &screenSize, const QRectF &visibleArea)
{
    QPointF diff = centerOffset(screenSize, visibleArea);
    qreal xdiffpct = diff.x() / qMax<double>(screenSize.width() - 1, 1);
    qreal ydiffpct = diff.y() / qMax<double>(screenSize.height() - 1, 1);

    return QPointF(-xdiffpct, -ydiffpct);
}

QT_BEGIN_NAMESPACE

QGeoProjection::QGeoProjection()
{

}

QGeoProjection::~QGeoProjection()
{

}

QGeoCoordinate QGeoProjection::anchorCoordinateToPoint(const QGeoCoordinate &coordinate, const QPointF &anchorPoint) const
{
    Q_UNUSED(coordinate)
    Q_UNUSED(anchorPoint)
    return QGeoCoordinate();
}

QGeoShape QGeoProjection::visibleRegion() const
{
    return QGeoShape();
}

bool QGeoProjection::setBearing(qreal bearing, const QGeoCoordinate &coordinate)
{
    Q_UNUSED(bearing)
    Q_UNUSED(coordinate)
    return false;
}


/*
 * QGeoProjectionWebMercator implementation
*/

QGeoCoordinate QGeoProjectionWebMercator::anchorCoordinateToPoint(const QGeoCoordinate &coordinate, const QPointF &anchorPoint) const
{
    // Approach: find the displacement in (wrapped) mercator space, and apply that to the center
    QDoubleVector2D centerProj = geoToWrappedMapProjection(cameraData().center());
    QDoubleVector2D coordProj  = geoToWrappedMapProjection(coordinate);

    QDoubleVector2D anchorProj = itemPositionToWrappedMapProjection(QDoubleVector2D(anchorPoint));
    // Y-clamping done in mercatorToCoord
    return wrappedMapProjectionToGeo(centerProj + coordProj - anchorProj);
}

bool QGeoProjectionWebMercator::setBearing(qreal bearing, const QGeoCoordinate &coordinate)
{
    const QDoubleVector2D coordWrapped = geoToWrappedMapProjection(coordinate);
    if (!isProjectable(coordWrapped))
        return false;
    const QPointF rotationPoint = wrappedMapProjectionToItemPosition(coordWrapped).toPointF();

    QGeoCameraData camera = cameraData();
    // first set bearing
    camera.setBearing(bearing);
    setCameraData(camera);
    camera = cameraData();

    // then reanchor
    const QGeoCoordinate center = anchorCoordinateToPoint(coordinate, rotationPoint);
    camera.setCenter(center);
    setCameraData(camera);
    return true;
}

QGeoProjectionWebMercator::QGeoProjectionWebMercator()
    : QGeoProjection(),
      m_mapEdgeSize(256), // at zl 0
      m_minimumZoom(0),
      m_cameraCenterXMercator(0),
      m_cameraCenterYMercator(0),
      m_viewportWidth(1),
      m_viewportHeight(1),
      m_1_viewportWidth(0),
      m_1_viewportHeight(0),
      m_sideLengthPixels(256),
      m_aperture(0.0),
      m_nearPlane(0.0),
      m_farPlane(0.0),
      m_halfWidth(0.0),
      m_halfHeight(0.0),
      m_minimumUnprojectableY(0.0),
      m_verticalEstateToSkip(0.0),
      m_visibleRegionDirty(false)
{
}

QGeoProjectionWebMercator::~QGeoProjectionWebMercator()
{

}

// This method returns the minimum zoom level that this specific qgeomap type allows
// at the current viewport size and for the default tile size of 256^2.
double QGeoProjectionWebMercator::minimumZoom() const
{
    return m_minimumZoom;
}

// This method recalculates the "no-trespassing" limits for the map center.
// This has to be used when:
// 1) the map is resized, because the meters per pixel remain the same, but
//    the amount of pixels between the center and the borders changes
// 2) when the zoom level changes, because the amount of pixels between the center
//    and the borders stays the same, but the meters per pixel change
double QGeoProjectionWebMercator::maximumCenterLatitudeAtZoom(const QGeoCameraData &cameraData) const
{
    double mapEdgeSize = std::pow(2.0, cameraData.zoomLevel()) * defaultTileSize;

    // At init time weird things happen
    int clampedWindowHeight = (m_viewportHeight > mapEdgeSize) ? mapEdgeSize : m_viewportHeight;
    QPointF offsetPct = centerOffset(QSizeF(m_viewportWidth, m_viewportHeight), m_visibleArea);
    double hpct = offsetPct.y() / qMax<double>(m_viewportHeight - 1, 1);

    // Use the window height divided by 2 as the topmost allowed center, with respect to the map size in pixels
    double mercatorTopmost = (clampedWindowHeight * (0.5 - hpct)) /  mapEdgeSize ;
    QGeoCoordinate topMost = QWebMercator::mercatorToCoord(QDoubleVector2D(0.0, mercatorTopmost));
    return topMost.latitude();
}

double QGeoProjectionWebMercator::minimumCenterLatitudeAtZoom(const QGeoCameraData &cameraData) const
{
    double mapEdgeSize = std::pow(2.0, cameraData.zoomLevel()) * defaultTileSize;

    // At init time weird things happen
    int clampedWindowHeight = (m_viewportHeight > mapEdgeSize) ? mapEdgeSize : m_viewportHeight;
    QPointF offsetPct = centerOffset(QSizeF(m_viewportWidth, m_viewportHeight), m_visibleArea);
    double hpct = offsetPct.y() / qMax<double>(m_viewportHeight - 1, 1);

    // Use the window height divided by 2 as the topmost allowed center, with respect to the map size in pixels
    double mercatorTopmost = (clampedWindowHeight * (0.5 + hpct)) /  mapEdgeSize ;
    QGeoCoordinate topMost = QWebMercator::mercatorToCoord(QDoubleVector2D(0.0, mercatorTopmost));
    return -topMost.latitude();
}

void QGeoProjectionWebMercator::setVisibleArea(const QRectF &visibleArea)
{
    m_visibleArea = visibleArea;
    setupCamera();
}

double QGeoProjectionWebMercator::mapWidth() const
{
    return m_mapEdgeSize;
}

double QGeoProjectionWebMercator::mapHeight() const
{
    return m_mapEdgeSize;
}

void QGeoProjectionWebMercator::setViewportSize(const QSize &size)
{
    if (int(m_viewportWidth) ==  size.width() && int(m_viewportHeight) == size.height())
        return;

    m_viewportWidth = size.width();
    m_viewportHeight = size.height();
    m_1_viewportWidth = 1.0 / m_viewportWidth;
    m_1_viewportHeight = 1.0 / m_viewportHeight;
    m_minimumZoom =  std::log(qMax(m_viewportWidth, m_viewportHeight) / defaultTileSize) / std::log(2.0);
    setupCamera();
}

void QGeoProjectionWebMercator::setCameraData(const QGeoCameraData &cameraData, bool force)
{
    if (m_cameraData == cameraData && !force)
        return;

    m_cameraData = cameraData;
    m_mapEdgeSize = std::pow(2.0, cameraData.zoomLevel()) * defaultTileSize;
    setupCamera();
}

QDoubleVector2D QGeoProjectionWebMercator::geoToMapProjection(const QGeoCoordinate &coordinate) const
{
    return QWebMercator::coordToMercator(coordinate);
}

QGeoCoordinate QGeoProjectionWebMercator::mapProjectionToGeo(const QDoubleVector2D &projection) const
{
    return QWebMercator::mercatorToCoord(projection);
}

//wraps around center
QDoubleVector2D QGeoProjectionWebMercator::wrapMapProjection(const QDoubleVector2D &projection) const
{
    double x = projection.x();
    if (m_cameraCenterXMercator < 0.5) {
        if (x - m_cameraCenterXMercator > 0.5 )
            x -= 1.0;
    } else if (m_cameraCenterXMercator > 0.5) {
        if (x - m_cameraCenterXMercator < -0.5 )
            x += 1.0;
    }

    return QDoubleVector2D(x, projection.y());
}

QDoubleVector2D QGeoProjectionWebMercator::unwrapMapProjection(const QDoubleVector2D &wrappedProjection) const
{
    double x = wrappedProjection.x();
    if (x > 1.0)
        return QDoubleVector2D(x - 1.0, wrappedProjection.y());
    if (x <= 0.0)
        return QDoubleVector2D(x + 1.0, wrappedProjection.y());
    return wrappedProjection;
}

QDoubleVector2D QGeoProjectionWebMercator::wrappedMapProjectionToItemPosition(const QDoubleVector2D &wrappedProjection) const
{
    return (m_transformation * wrappedProjection).toVector2D();
}

QDoubleVector2D QGeoProjectionWebMercator::itemPositionToWrappedMapProjection(const QDoubleVector2D &itemPosition) const
{
    const QPointF centerOff = centerOffset(QSizeF(m_viewportWidth, m_viewportHeight), m_visibleArea);
    QDoubleVector2D pos = itemPosition + QDoubleVector2D(centerOff);
    pos *= QDoubleVector2D(m_1_viewportWidth, m_1_viewportHeight);
    pos *= 2.0;
    pos -= QDoubleVector2D(1.0,1.0);

    double s;
    QDoubleVector2D res = viewportToWrappedMapProjection(pos, s);

    // a positive s means a point behind the camera. So do it again, after clamping Y. See QTBUG-61813
    if (s > 0.0) {
        pos = itemPosition;
        // when the camera is tilted, picking a point above the horizon returns a coordinate behind the camera
        pos.setY(m_minimumUnprojectableY);
        pos *= QDoubleVector2D(m_1_viewportWidth, m_1_viewportHeight);
        pos *= 2.0;
        pos -= QDoubleVector2D(1.0,1.0);
        res = viewportToWrappedMapProjection(pos, s);
    }

    return res;
}

/* Default implementations */
QGeoCoordinate QGeoProjectionWebMercator::itemPositionToCoordinate(const QDoubleVector2D &pos, bool clipToViewport) const
{
    if (qIsNaN(pos.x()) || qIsNaN(pos.y()))
        return QGeoCoordinate();

    if (clipToViewport) {
        int w = m_viewportWidth;
        int h = m_viewportHeight;

        if ((pos.x() < 0) || (w < pos.x()) || (pos.y() < 0) || (h < pos.y()))
            return QGeoCoordinate();
    }

    QDoubleVector2D wrappedMapProjection = itemPositionToWrappedMapProjection(pos);
    // With rotation/tilting, a screen position might end up outside the projection space.
    if (!isProjectable(wrappedMapProjection))
        return QGeoCoordinate();
    return mapProjectionToGeo(unwrapMapProjection(wrappedMapProjection));
}

QDoubleVector2D QGeoProjectionWebMercator::coordinateToItemPosition(const QGeoCoordinate &coordinate, bool clipToViewport) const
{
    if (!coordinate.isValid())
        return QDoubleVector2D(qQNaN(), qQNaN());

    QDoubleVector2D wrappedProjection = wrapMapProjection(geoToMapProjection(coordinate));
    if (!isProjectable(wrappedProjection))
        return QDoubleVector2D(qQNaN(), qQNaN());

    QDoubleVector2D pos = wrappedMapProjectionToItemPosition(wrappedProjection);

    if (clipToViewport) {
        int w = m_viewportWidth;
        int h = m_viewportHeight;
        double x = pos.x();
        double y = pos.y();
        if ((x < -0.5) || (x > w + 0.5) || (y < -0.5) || (y > h + 0.5) || qIsNaN(x) || qIsNaN(y))
            return QDoubleVector2D(qQNaN(), qQNaN());
    }
    return pos;
}

QDoubleVector2D QGeoProjectionWebMercator::geoToWrappedMapProjection(const QGeoCoordinate &coordinate) const
{
    return wrapMapProjection(geoToMapProjection(coordinate));
}

QGeoCoordinate QGeoProjectionWebMercator::wrappedMapProjectionToGeo(const QDoubleVector2D &wrappedProjection) const
{
    return mapProjectionToGeo(unwrapMapProjection(wrappedProjection));
}

QMatrix4x4 QGeoProjectionWebMercator::quickItemTransformation(const QGeoCoordinate &coordinate, const QPointF &anchorPoint, qreal zoomLevel) const
{
    const QDoubleVector2D coordWrapped = geoToWrappedMapProjection(coordinate);
    double scale = std::pow(0.5, zoomLevel - m_cameraData.zoomLevel());
    const QDoubleVector2D anchorScaled = QDoubleVector2D(anchorPoint.x(), anchorPoint.y()) * scale;
    const QDoubleVector2D anchorMercator = anchorScaled / mapWidth();

    const QDoubleVector2D coordAnchored = coordWrapped - anchorMercator;
    const QDoubleVector2D coordAnchoredScaled = coordAnchored * m_sideLengthPixels;
    QDoubleMatrix4x4 matTranslateScale;
    matTranslateScale.translate(coordAnchoredScaled.x(), coordAnchoredScaled.y(), 0.0);

    scale = std::pow(0.5, (zoomLevel - std::floor(zoomLevel)) +
                     (std::floor(zoomLevel) - std::floor(m_cameraData.zoomLevel())));
    matTranslateScale.scale(scale);

    /*
     *  The full transformation chain for quickItemTransformation() would be:
     *  matScreenShift * m_quickItemTransformation * matTranslate * matScale
     *  where:
     *  matScreenShift = translate(-coordOnScreen.x(), -coordOnScreen.y(), 0)
     *  matTranslate = translate(coordAnchoredScaled.x(), coordAnchoredScaled.y(), 0.0)
     *  matScale = scale(scale)
     *
     *  However, matScreenShift is removed, as setPosition(0,0) is used in place of setPositionOnScreen.
     */

    return toMatrix4x4(m_quickItemTransformation * matTranslateScale);
}

bool QGeoProjectionWebMercator::isProjectable(const QDoubleVector2D &wrappedProjection) const
{
    if (m_cameraData.tilt() == 0.0)
        return true;

    QDoubleVector3D pos = wrappedProjection * m_sideLengthPixels;
    // use m_centerNearPlane in order to add an offset to m_eye.
    QDoubleVector3D p = m_centerNearPlane - pos;
    double dot = QDoubleVector3D::dotProduct(p , m_viewNormalized);

    if (dot < 0.0) // behind the near plane
        return false;
    return true;
}

QList<QDoubleVector2D> QGeoProjectionWebMercator::visibleGeometry() const
{
    if (m_visibleRegionDirty)
        const_cast<QGeoProjectionWebMercator *>(this)->updateVisibleRegion();
    return m_visibleRegion;
}

QList<QDoubleVector2D> QGeoProjectionWebMercator::visibleGeometryExpanded() const
{
    if (m_visibleRegionDirty)
        const_cast<QGeoProjectionWebMercator *>(this)->updateVisibleRegion();
    return m_visibleRegionExpanded;
}

QList<QDoubleVector2D> QGeoProjectionWebMercator::projectableGeometry() const
{
    if (m_visibleRegionDirty)
        const_cast<QGeoProjectionWebMercator *>(this)->updateVisibleRegion();
    return m_projectableRegion;
}

QGeoShape QGeoProjectionWebMercator::visibleRegion() const
{
    const QList<QDoubleVector2D> &visibleRegion = visibleGeometry();
    QGeoPolygon poly;
    for (int i = 0; i < visibleRegion.size(); ++i) {
         const QDoubleVector2D &c = visibleRegion.at(i);
        // If a segment spans more than half of the map longitudinally, split in 2.
        if (i && qAbs(visibleRegion.at(i-1).x() - c.x()) >= 0.5) { // This assumes a segment is never >= 1.0 (whole map span)
            QDoubleVector2D extraPoint = (visibleRegion.at(i-1) + c) * 0.5;
            poly.addCoordinate(wrappedMapProjectionToGeo(extraPoint));
        }
        poly.addCoordinate(wrappedMapProjectionToGeo(c));
    }
    if (visibleRegion.size() >= 2 && qAbs(visibleRegion.last().x() - visibleRegion.first().x()) >= 0.5) {
        QDoubleVector2D extraPoint = (visibleRegion.last() + visibleRegion.first()) * 0.5;
        poly.addCoordinate(wrappedMapProjectionToGeo(extraPoint));
    }

    return poly;
}

QDoubleVector2D QGeoProjectionWebMercator::viewportToWrappedMapProjection(const QDoubleVector2D &itemPosition) const
{
    double s;
    return viewportToWrappedMapProjection(itemPosition, s);
}

/*
    actual implementation of itemPositionToWrappedMapProjection
*/
QDoubleVector2D QGeoProjectionWebMercator::viewportToWrappedMapProjection(const QDoubleVector2D &itemPosition, double &s) const
{
    QDoubleVector2D pos = itemPosition;
    pos *= QDoubleVector2D(m_halfWidth, m_halfHeight);

    // determine itemPosition on the near plane
    QDoubleVector3D p = m_centerNearPlane;
    p += m_up * pos.y();
    p += m_side * pos.x();

    // compute the ray using the eye position
    QDoubleVector3D ray = m_eye - p;
    ray.normalize();

    return (xyPlane.lineIntersection(m_eye, ray, s) / m_sideLengthPixels).toVector2D();
}

/*
    Returns a pair of <newCenter, newZoom>
*/
QPair<QGeoCoordinate, qreal> QGeoProjectionWebMercator::fitViewportToGeoRectangle(const QGeoRectangle &rectangle,
                                                                                  const QMargins &m) const
{
    QPair<QGeoCoordinate, qreal> res;
    res.second = qQNaN();
    if (m_viewportWidth <= m.left() + m.right() || m_viewportHeight <= m.top() + m.bottom())
        return res;

    QDoubleVector2D topLeftPoint = geoToMapProjection(rectangle.topLeft());
    QDoubleVector2D bottomRightPoint = geoToMapProjection(rectangle.bottomRight());
    if (bottomRightPoint.x() < topLeftPoint.x()) // crossing the dateline
        bottomRightPoint.setX(bottomRightPoint.x() + 1.0);

    // find center of the bounding box
    QDoubleVector2D center = (topLeftPoint + bottomRightPoint) * 0.5;
    center.setX(center.x() > 1.0 ? center.x() - 1.0 : center.x());
    res.first = mapProjectionToGeo(center);

    // if the shape is empty we just change center position, not zoom
    double bboxWidth  = (bottomRightPoint.x() - topLeftPoint.x()) * mapWidth();
    double bboxHeight = (bottomRightPoint.y() - topLeftPoint.y()) * mapHeight();

    if (bboxHeight == 0.0 && bboxWidth == 0.0)
        return res;

    double zoomRatio = qMax(bboxWidth / (m_viewportWidth - m.left() - m.right()),
                            bboxHeight / (m_viewportHeight - m.top() - m.bottom()));
    zoomRatio = std::log(zoomRatio) / std::log(2.0);
    res.second = m_cameraData.zoomLevel() - zoomRatio;

    return  res;
}

QGeoProjection::ProjectionGroup QGeoProjectionWebMercator::projectionGroup() const
{
    return QGeoProjection::ProjectionCylindrical;
}

QGeoProjection::Datum QGeoProjectionWebMercator::datum() const
{
    return QGeoProjection::DatumWGS84;
}

QGeoProjection::ProjectionType QGeoProjectionWebMercator::projectionType() const
{
    return QGeoProjection::ProjectionWebMercator;
}

void QGeoProjectionWebMercator::setupCamera()
{
    m_centerMercator = geoToMapProjection(m_cameraData.center());
    m_cameraCenterXMercator = m_centerMercator.x();
    m_cameraCenterYMercator = m_centerMercator.y();

    int intZoomLevel = static_cast<int>(std::floor(m_cameraData.zoomLevel()));
    m_sideLengthPixels = (1 << intZoomLevel) * defaultTileSize;
    m_center = m_centerMercator * m_sideLengthPixels;
    //aperture(90 / 2) = 1
    m_aperture = tan(QLocationUtils::radians(m_cameraData.fieldOfView()) * 0.5);

    double f = m_viewportHeight;
    double z = std::pow(2.0, m_cameraData.zoomLevel() - intZoomLevel) * defaultTileSize;
    double altitude = f / (2.0 * z);
    // Also in mercator space
    double z_mercator = std::pow(2.0, m_cameraData.zoomLevel()) * defaultTileSize;
    double altitude_mercator = f / (2.0 * z_mercator);

    // calculate eye
    m_eye = m_center;
    m_eye.setZ(altitude * defaultTileSize / m_aperture);

    // And in mercator space
    m_eyeMercator = m_centerMercator;
    m_eyeMercator.setZ(altitude_mercator  / m_aperture);

    m_view = m_eye - m_center;
    QDoubleVector3D side = QDoubleVector3D::normal(m_view, QDoubleVector3D(0.0, 1.0, 0.0));
    m_up = QDoubleVector3D::normal(side, m_view);

    // In mercator space too
    m_viewMercator = m_eyeMercator - m_centerMercator;
    QDoubleVector3D sideMercator = QDoubleVector3D::normal(m_viewMercator, QDoubleVector3D(0.0, 1.0, 0.0));
    m_upMercator = QDoubleVector3D::normal(sideMercator, m_viewMercator);

    if (m_cameraData.bearing() > 0.0) {
        QDoubleMatrix4x4 mBearing;
        mBearing.rotate(m_cameraData.bearing(), m_view);
        m_up = mBearing * m_up;

        // In mercator space too
        QDoubleMatrix4x4 mBearingMercator;
        mBearingMercator.rotate(m_cameraData.bearing(), m_viewMercator);
        m_upMercator = mBearingMercator * m_upMercator;
    }

    m_side = QDoubleVector3D::normal(m_up, m_view);
    m_sideMercator = QDoubleVector3D::normal(m_upMercator, m_viewMercator);

    if (m_cameraData.tilt() > 0.0) { // tilt has been already thresholded by QGeoCameraData::setTilt
        QDoubleMatrix4x4 mTilt;
        mTilt.rotate(-m_cameraData.tilt(), m_side);
        m_eye = mTilt * m_view + m_center;

        // In mercator space too
        QDoubleMatrix4x4 mTiltMercator;
        mTiltMercator.rotate(-m_cameraData.tilt(), m_sideMercator);
        m_eyeMercator = mTiltMercator * m_viewMercator + m_centerMercator;
    }

    m_view = m_eye - m_center; // ToDo: this should be inverted (center - eye), and the rest should follow
    m_viewNormalized = m_view.normalized();
    m_up = QDoubleVector3D::normal(m_view, m_side);

    m_nearPlane = 1.0;
    // At ZL 20 the map has 2^20 tiles per side. That is 1048576.
    // Placing the camera on one corner of the map, rotated toward the opposite corner, and tilted
    // at almost 90 degrees would  require a frustum that can span the whole size of this map.
    // For this reason, the far plane is set to 2 * 2^20 * defaultTileSize.
    // That is, in order to make sure that the whole map would fit in the frustum at this ZL.
    // Since we are using a double matrix, and since the largest value in the matrix is going to be
    // 2 * m_farPlane (as near plane is 1.0), there should be sufficient precision left.
    //
    // TODO: extend this to support clip distance.
    m_farPlane =  (altitude + 2097152.0) * defaultTileSize;

    m_viewMercator = m_eyeMercator - m_centerMercator;
    m_upMercator = QDoubleVector3D::normal(m_viewMercator, m_sideMercator);
    m_nearPlaneMercator = 0.000002; // this value works until ZL 18. Above that, a better progressive formula is needed, or
                                    // else, this clips too much.

    double aspectRatio = 1.0 * m_viewportWidth / m_viewportHeight;

    m_halfWidth = m_aperture * aspectRatio;
    m_halfHeight = m_aperture;

    double verticalHalfFOV = QLocationUtils::degrees(atan(m_aperture));

    QDoubleMatrix4x4 cameraMatrix;
    cameraMatrix.lookAt(m_eye, m_center, m_up);

    QDoubleMatrix4x4 projectionMatrix;
    projectionMatrix.frustum(-m_halfWidth, m_halfWidth, -m_halfHeight, m_halfHeight, m_nearPlane, m_farPlane);

    /*
     * The full transformation chain for m_transformation is:
     * matScreen * matScreenFit * matShift *  projectionMatrix * cameraMatrix * matZoomLevelScale
     * where:
     * matZoomLevelScale = scale(m_sideLength, m_sideLength, 1.0)
     * matShift = translate(1.0, 1.0, 0.0)
     * matScreenFit = scale(0.5, 0.5, 1.0)
     * matScreen = scale(m_viewportWidth, m_viewportHeight, 1.0)
     */

    QPointF offsetPct = marginsOffset(QSizeF(m_viewportWidth, m_viewportHeight), m_visibleArea);
    QDoubleMatrix4x4 matScreenTransformation;
    matScreenTransformation.scale(0.5 * m_viewportWidth, 0.5 * m_viewportHeight, 1.0);
    matScreenTransformation(0,3) = (0.5 + offsetPct.x()) * m_viewportWidth;
    matScreenTransformation(1,3) = (0.5 + offsetPct.y()) * m_viewportHeight;

    m_transformation = matScreenTransformation *  projectionMatrix * cameraMatrix;
    m_quickItemTransformation = m_transformation;
    m_transformation.scale(m_sideLengthPixels, m_sideLengthPixels, 1.0);

    m_centerNearPlane = m_eye - m_viewNormalized;
    m_centerNearPlaneMercator = m_eyeMercator - m_viewNormalized * m_nearPlaneMercator;

    // The method does not support tilting angles >= 90.0 or < 0.

    // The following formula is used to have a growing epsilon with the zoom level,
    // in order not to have too large values at low zl, which would overflow when converted to Clipper::cInt.
    const double upperBoundEpsilon = 1.0 / std::pow(10, 1.0 + m_cameraData.zoomLevel() / 5.0);
    const double elevationUpperBound = 90.0 - upperBoundEpsilon;
    const double maxRayElevation = qMin(elevationUpperBound - m_cameraData.tilt(), verticalHalfFOV);
    double maxHalfAperture = 0;
    m_verticalEstateToSkip = 0;
    if (maxRayElevation < verticalHalfFOV) {
        maxHalfAperture = tan(QLocationUtils::radians(maxRayElevation));
        m_verticalEstateToSkip = 1.0 - maxHalfAperture / m_aperture;
    }

    m_minimumUnprojectableY = m_verticalEstateToSkip * 0.5 * m_viewportHeight; // m_verticalEstateToSkip is relative to half aperture
    m_visibleRegionDirty = true;
}

void QGeoProjectionWebMercator::updateVisibleRegion()
{
    m_visibleRegionDirty = false;

    double viewportHalfWidth  = (!m_visibleArea.isEmpty()) ? m_visibleArea.width() / m_viewportWidth : 1.0;
    double viewportHalfHeight = (!m_visibleArea.isEmpty()) ? m_visibleArea.height() / m_viewportHeight : 1.0;

    double top = qMax<double>(-viewportHalfHeight, -1 + m_verticalEstateToSkip);
    double bottom = viewportHalfHeight;
    double left = -viewportHalfWidth;
    double right = viewportHalfWidth;

    QDoubleVector2D tl = viewportToWrappedMapProjection(QDoubleVector2D(left, top ));
    QDoubleVector2D tr = viewportToWrappedMapProjection(QDoubleVector2D(right, top ));
    QDoubleVector2D bl = viewportToWrappedMapProjection(QDoubleVector2D(left,  bottom ));
    QDoubleVector2D br = viewportToWrappedMapProjection(QDoubleVector2D(right, bottom ));

    // To make sure that what is returned can be safely converted back to lat/lon without risking overlaps
    double mapLeftLongitude = QLocationUtils::mapLeftLongitude(m_cameraData.center().longitude());
    double mapRightLongitude = QLocationUtils::mapRightLongitude(m_cameraData.center().longitude());
    double leftX = geoToWrappedMapProjection(QGeoCoordinate(0, mapLeftLongitude)).x();
    double rightX = geoToWrappedMapProjection(QGeoCoordinate(0, mapRightLongitude)).x();

    QList<QDoubleVector2D> mapRect;
    mapRect.push_back(QDoubleVector2D(leftX, 1.0));
    mapRect.push_back(QDoubleVector2D(rightX, 1.0));
    mapRect.push_back(QDoubleVector2D(rightX, 0.0));
    mapRect.push_back(QDoubleVector2D(leftX, 0.0));

    QList<QDoubleVector2D> viewportRect;
    viewportRect.push_back(bl);
    viewportRect.push_back(br);
    viewportRect.push_back(tr);
    viewportRect.push_back(tl);

    c2t::clip2tri clipper;
    clipper.clearClipper();
    clipper.addSubjectPath(QClipperUtils::qListToPath(mapRect), true);
    clipper.addClipPolygon(QClipperUtils::qListToPath(viewportRect));

    Paths res = clipper.execute(c2t::clip2tri::Intersection);
    m_visibleRegion.clear();
    if (res.size())
        m_visibleRegion = QClipperUtils::pathToQList(res[0]); // Intersection between two convex quadrilaterals should always be a single polygon

    m_projectableRegion.clear();
    mapRect.clear();
    // The full map rectangle in extended mercator space
    mapRect.push_back(QDoubleVector2D(-1.0, 1.0));
    mapRect.push_back(QDoubleVector2D( 2.0, 1.0));
    mapRect.push_back(QDoubleVector2D( 2.0, 0.0));
    mapRect.push_back(QDoubleVector2D(-1.0, 0.0));
    if (m_cameraData.tilt() == 0) {
        m_projectableRegion = mapRect;
    } else {
        QGeoProjectionWebMercator::Plane nearPlane(m_centerNearPlaneMercator, m_viewNormalized);
        Line2D nearPlaneXYIntersection = nearPlane.planeXYIntersection();
        double squareHalfSide = qMax(5.0, nearPlaneXYIntersection.m_point.length());
        QDoubleVector2D viewDirectionProjected = -m_viewNormalized.toVector2D().normalized();


        QDoubleVector2D tl = nearPlaneXYIntersection.m_point
                            - squareHalfSide * nearPlaneXYIntersection.m_direction
                            + 2 * squareHalfSide * viewDirectionProjected;
        QDoubleVector2D tr = nearPlaneXYIntersection.m_point
                            + squareHalfSide * nearPlaneXYIntersection.m_direction
                            + 2 * squareHalfSide * viewDirectionProjected;
        QDoubleVector2D bl = nearPlaneXYIntersection.m_point
                            - squareHalfSide * nearPlaneXYIntersection.m_direction;
        QDoubleVector2D br = nearPlaneXYIntersection.m_point
                            + squareHalfSide * nearPlaneXYIntersection.m_direction;

        QList<QDoubleVector2D> projectableRect;
        projectableRect.push_back(bl);
        projectableRect.push_back(br);
        projectableRect.push_back(tr);
        projectableRect.push_back(tl);


        c2t::clip2tri clipperProjectable;
        clipperProjectable.clearClipper();
        clipperProjectable.addSubjectPath(QClipperUtils::qListToPath(mapRect), true);
        clipperProjectable.addClipPolygon(QClipperUtils::qListToPath(projectableRect));

        Paths resProjectable = clipperProjectable.execute(c2t::clip2tri::Intersection);
        if (resProjectable.size())
            m_projectableRegion = QClipperUtils::pathToQList(resProjectable[0]); // Intersection between two convex quadrilaterals should always be a single polygon
        else
            m_projectableRegion = viewportRect;
    }

    // Compute m_visibleRegionExpanded as a clipped expanded version of m_visibleRegion
    QDoubleVector2D centroid;
    for (const QDoubleVector2D &v: qAsConst(m_visibleRegion))
        centroid += v;
    centroid /= m_visibleRegion.size();

    m_visibleRegionExpanded.clear();
    for (const QDoubleVector2D &v: qAsConst(m_visibleRegion)) {
        const QDoubleVector2D vc = v - centroid;
        m_visibleRegionExpanded.push_back(centroid + vc * 1.2); // fixing expansion factor to 1.2
    }

    c2t::clip2tri clipperExpanded;
    clipperExpanded.clearClipper();
    clipperExpanded.addSubjectPath(QClipperUtils::qListToPath(m_visibleRegionExpanded), true);
    clipperExpanded.addClipPolygon(QClipperUtils::qListToPath(m_projectableRegion));
    Paths resVisibleExpanded = clipperExpanded.execute(c2t::clip2tri::Intersection);
    if (resVisibleExpanded.size())
        m_visibleRegionExpanded = QClipperUtils::pathToQList(resVisibleExpanded[0]); // Intersection between two convex quadrilaterals should always be a single polygon
    else
        m_visibleRegionExpanded = m_visibleRegion;
}

QGeoCameraData QGeoProjectionWebMercator::cameraData() const
{
    return m_cameraData;
}

/*
 *
 *  Line implementation
 *
 */

QGeoProjectionWebMercator::Line2D::Line2D()
{

}

QGeoProjectionWebMercator::Line2D::Line2D(const QDoubleVector2D &linePoint, const QDoubleVector2D &lineDirection)
    :   m_point(linePoint), m_direction(lineDirection.normalized())
{

}

bool QGeoProjectionWebMercator::Line2D::isValid() const
{
    return (m_direction.length() > 0.5);
}

/*
 *
 *  Plane implementation
 *
 */

QGeoProjectionWebMercator::Plane::Plane()
{

}

QGeoProjectionWebMercator::Plane::Plane(const QDoubleVector3D &planePoint, const QDoubleVector3D &planeNormal)
    :   m_point(planePoint), m_normal(planeNormal.normalized()) { }

QDoubleVector3D QGeoProjectionWebMercator::Plane::lineIntersection(const QDoubleVector3D &linePoint, const QDoubleVector3D &lineDirection) const
{
    double s;
    return lineIntersection(linePoint, lineDirection, s);
}

QDoubleVector3D QGeoProjectionWebMercator::Plane::lineIntersection(const QDoubleVector3D &linePoint, const QDoubleVector3D &lineDirection, double &s) const
{
    QDoubleVector3D w = linePoint - m_point;
    // s = -n.dot(w) / n.dot(u).  p = p0 + su; u is lineDirection
    s = QDoubleVector3D::dotProduct(-m_normal, w) / QDoubleVector3D::dotProduct(m_normal, lineDirection);
    return linePoint + lineDirection * s;
}

QGeoProjectionWebMercator::Line2D QGeoProjectionWebMercator::Plane::planeXYIntersection() const
{
    // cross product of the two normals for the line direction
    QDoubleVector3D lineDirection = QDoubleVector3D::crossProduct(m_normal, xyNormal);
    lineDirection.setZ(0.0);
    lineDirection.normalize();

    // cross product of the line direction and the plane normal to find the direction on the plane
    // intersecting the xy plane
    QDoubleVector3D directionToXY = QDoubleVector3D::crossProduct(m_normal, lineDirection);
    QDoubleVector3D p = xyPlane.lineIntersection(m_point, directionToXY);
    return Line2D(p.toVector2D(), lineDirection.toVector2D());
}

bool QGeoProjectionWebMercator::Plane::isValid() const
{
    return (m_normal.length() > 0.5);
}

QT_END_NAMESPACE
