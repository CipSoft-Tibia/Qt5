// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgeorectangle.h"
#include "qgeorectangle_p.h"

#include "qwebmercator_p.h"
#include "qdoublevector2d_p.h"
#include "qgeocoordinate.h"
#include "qnumeric.h"
#include "qlocationutils_p.h"
#include <QList>
QT_BEGIN_NAMESPACE

QT_IMPL_METATYPE_EXTERN(QGeoRectangle)

/*!
    \class QGeoRectangle
    \inmodule QtPositioning
    \ingroup QtPositioning-positioning
    \since 5.2

    \brief The QGeoRectangle class defines a rectangular geographic area.

    The rectangle is defined in terms of a QGeoCoordinate which specifies the
    top left coordinate of the rectangle and a QGeoCoordinate which specifies
    the bottom right coordinate of the rectangle.

    A geo rectangle is considered invalid if the top left or bottom right
    coordinates are invalid or if the top left coordinate is south of the
    bottom right coordinate.

    Geo rectangles can never cross the poles.

    Several methods behave as though the geo rectangle is defined in terms of a
    center coordinate, the width of the geo rectangle in degrees and the height
    of the geo rectangle in degrees.

    If the height or center of a geo rectangle is adjusted such that it would
    cross one of the poles the height is modified such that the geo rectangle
    touches but does not cross the pole and that the center coordinate is still
    in the center of the geo rectangle.

    This class is a \l Q_GADGET since Qt 5.5. It can be
    \l{Cpp_value_integration_positioning}{directly used from C++ and QML}.
*/

/*!
    \property QGeoRectangle::bottomLeft
    \brief This property holds the bottom left coorindate of this geo rectangle.

    While this property is introduced in Qt 5.5, the related accessor functions
    exist since the first version of this class.

    \since 5.5
*/

/*!
    \property QGeoRectangle::bottomRight
    \brief This property holds the bottom right coordinate of this geo rectangle.

    While this property is introduced in Qt 5.5, the related accessor functions
    exist since the first version of this class.

    \since 5.5
*/

/*!
    \property QGeoRectangle::topLeft
    \brief This property holds the top left coordinate of this geo rectangle.

    While this property is introduced in Qt 5.5, the related accessor functions
    exist since the first version of this class.

    \since 5.5
*/

/*!
    \property QGeoRectangle::topRight
    \brief This property holds the top right coordinate of this geo rectangle.

    While this property is introduced in Qt 5.5, the related accessor functions
    exist since the first version of this class.

    \since 5.5
*/

/*!
    \property QGeoRectangle::center
    \brief This property holds the center of this geo rectangle.

    While this property is introduced in Qt 5.5, the related accessor functions
    exist since the first version of this class.

    \sa QGeoShape::center

    \since 5.5
*/

/*!
    \property QGeoRectangle::width
    \brief This property holds the width of this geo rectangle in degrees.

    The property value is undefined if this geo rectangle is invalid.

    If the new width is less than 0.0 or if this geo rectangle is invalid, this
    function does nothing. To set up the values of an invalid
    geo rectangle based on the center, width, and height, you should use
    \l setCenter() first to make the geo rectangle valid.

    360.0 is the width used only if the new width is equal or greater than 360.
    In such cases the leftmost longitude of the geo rectangle is set to -180.0
    degrees and the rightmost longitude of the geo rectangle is set to 180.0
    degrees.

    While this property is introduced in Qt 5.5, the related accessor functions
    exist since the first version of this class.

    \since 5.5
*/

/*!
    \property QGeoRectangle::height
    \brief This property holds the height of this geo rectangle in degrees.

    The property value is undefined if this geo rectangle is invalid.

    If the new height is less than 0.0 or if this geo rectangle is invalid,
    the property is not changed. To set up the values of an invalid
    geo rectangle based on the center, width, and height, you should use
    \l setCenter() first to make the geo rectangle valid.

    If the change in height would cause the geo rectangle to cross a pole,
    the height is adjusted such that the geo rectangle only touches the pole.

    This change is done such that the center coordinate is still at the
    center of the geo rectangle, which may result in a geo rectangle with
    a smaller height than expected.

    180.0 is the height used only if the new height is greater or equal than 180.

    While this property is introduced in Qt 5.5, the related accessor functions
    exist since the first version of this class.

    \since 5.5
*/

inline QGeoRectanglePrivate *QGeoRectangle::d_func()
{
    return static_cast<QGeoRectanglePrivate *>(d_ptr.data());
}

inline const QGeoRectanglePrivate *QGeoRectangle::d_func() const
{
    return static_cast<const QGeoRectanglePrivate *>(d_ptr.constData());
}

/*!
    Constructs a new, invalid geo rectangle.
*/
QGeoRectangle::QGeoRectangle()
:   QGeoShape(new QGeoRectanglePrivate)
{
}

/*!
    Constructs a new geo rectangle centered at \a center with a
    width in degrees of \a degreesWidth and a height in degrees of \a degreesHeight.

    If \a degreesHeight would take the geo rectangle beyond one of the poles,
    the height of the geo rectangle will be truncated such that the geo rectangle
    only extends up to the pole. The center of the geo rectangle will be
    unchanged, and the height will be adjusted such that the center point is at
    the center of the truncated geo rectangle.
*/
QGeoRectangle::QGeoRectangle(const QGeoCoordinate &center, double degreesWidth, double degreesHeight)
{
    d_ptr = new QGeoRectanglePrivate(center, center);
    setWidth(degreesWidth);
    setHeight(degreesHeight);
}

/*!
    Constructs a new geo rectangle with a top left coordinate \a topLeft and a bottom right
    coordinate \a bottomRight.
*/
QGeoRectangle::QGeoRectangle(const QGeoCoordinate &topLeft, const QGeoCoordinate &bottomRight)
{
    d_ptr = new QGeoRectanglePrivate(topLeft, bottomRight);
}

/*!
    Constructs a new geo rectangle, of minimum size, containing all of the \a coordinates.
*/
QGeoRectangle::QGeoRectangle(const QList<QGeoCoordinate> &coordinates)
{
    if (coordinates.isEmpty()) {
        d_ptr = new QGeoRectanglePrivate;
    } else {
        const QGeoCoordinate &startCoordinate = coordinates.first();
        d_ptr = new QGeoRectanglePrivate(startCoordinate, startCoordinate);

        foreach (const QGeoCoordinate &coordinate, coordinates) {
            d_func()->extendRectangle(coordinate);
        }
    }
}

/*!
    Constructs a geo rectangle from the contents of \a other.
*/
QGeoRectangle::QGeoRectangle(const QGeoRectangle &other)
:   QGeoShape(other)
{
}

/*!
    Constructs a geo rectangle from the contents of \a other.
*/
QGeoRectangle::QGeoRectangle(const QGeoShape &other)
:   QGeoShape(other)
{
    if (type() != QGeoShape::RectangleType)
        d_ptr = new QGeoRectanglePrivate;
}

/*!
    Destroys this geo rectangle.
*/
QGeoRectangle::~QGeoRectangle()
{
}

/*!
    Assigns \a other to this geo rectangle and returns a reference to this geo rectangle.
*/
QGeoRectangle &QGeoRectangle::operator=(const QGeoRectangle &other)
{
    QGeoShape::operator=(other);
    return *this;
}

bool QGeoRectanglePrivate::isValid() const
{
    return topLeft.isValid() && bottomRight.isValid() &&
           topLeft.latitude() >= bottomRight.latitude();
}

bool QGeoRectanglePrivate::isEmpty() const
{
    if (!isValid())
        return true;

    return topLeft.latitude() == bottomRight.latitude() ||
           topLeft.longitude() == bottomRight.longitude();
}

/*!
    Sets the top left coordinate of this geo rectangle to \a topLeft.
*/
void QGeoRectangle::setTopLeft(const QGeoCoordinate &topLeft)
{
    Q_D(QGeoRectangle);

    d->topLeft = topLeft;
}

/*!
    Returns the top left coordinate of this geo rectangle.
*/
QGeoCoordinate QGeoRectangle::topLeft() const
{
    Q_D(const QGeoRectangle);

    return d->topLeft;
}

/*!
    Sets the top right coordinate of this geo rectangle to \a topRight.
*/
void QGeoRectangle::setTopRight(const QGeoCoordinate &topRight)
{
    Q_D(QGeoRectangle);

    d->topLeft.setLatitude(topRight.latitude());
    d->bottomRight.setLongitude(topRight.longitude());
}

/*!
    Returns the top right coordinate of this geo rectangle.
*/
QGeoCoordinate QGeoRectangle::topRight() const
{
    // TODO remove?
    if (!isValid())
        return QGeoCoordinate();

    Q_D(const QGeoRectangle);

    return QGeoCoordinate(d->topLeft.latitude(), d->bottomRight.longitude());
}

/*!
    Sets the bottom left coordinate of this geo rectangle to \a bottomLeft.
*/
void QGeoRectangle::setBottomLeft(const QGeoCoordinate &bottomLeft)
{
    Q_D(QGeoRectangle);

    d->bottomRight.setLatitude(bottomLeft.latitude());
    d->topLeft.setLongitude(bottomLeft.longitude());
}

/*!
    Returns the bottom left coordinate of this geo rectangle.
*/
QGeoCoordinate QGeoRectangle::bottomLeft() const
{
    // TODO remove?
    if (!isValid())
        return QGeoCoordinate();

    Q_D(const QGeoRectangle);

    return QGeoCoordinate(d->bottomRight.latitude(), d->topLeft.longitude());
}

/*!
    Sets the bottom right coordinate of this geo rectangle to \a bottomRight.
*/
void QGeoRectangle::setBottomRight(const QGeoCoordinate &bottomRight)
{
    Q_D(QGeoRectangle);

    d->bottomRight = bottomRight;
}

/*!
    Returns the bottom right coordinate of this geo rectangle.
*/
QGeoCoordinate QGeoRectangle::bottomRight() const
{
    Q_D(const QGeoRectangle);

    return d->bottomRight;
}

/*!
    Sets the center of this geo rectangle to \a center.

    If this causes the geo rectangle to cross on of the poles the height of the
    geo rectangle will be truncated such that the geo rectangle only extends up
    to the pole. The center of the geo rectangle will be unchanged, and the
    height will be adjusted such that the center point is at the center of the
    truncated geo rectangle.

*/
void QGeoRectangle::setCenter(const QGeoCoordinate &center)
{
    Q_D(QGeoRectangle);

    if (!isValid()) {
        d->topLeft = center;
        d->bottomRight = center;
        return;
    }
    double width = this->width();
    double height = this->height();

    double tlLat = center.latitude() + height / 2.0;
    double tlLon = center.longitude() - width / 2.0;
    double brLat = center.latitude() - height / 2.0;
    double brLon = center.longitude() + width / 2.0;
    tlLon = QLocationUtils::wrapLong(tlLon);
    brLon = QLocationUtils::wrapLong(brLon);

    if (tlLat > 90.0) {
        brLat = 2 * center.latitude() - 90.0;
        tlLat = 90.0;
    }

    if (tlLat < -90.0) {
        brLat = -90.0;
        tlLat = -90.0;
    }

    if (brLat > 90.0) {
        tlLat = 90.0;
        brLat = 90.0;
    }

    if (brLat < -90.0) {
        tlLat = 2 * center.latitude() + 90.0;
        brLat = -90.0;
    }

    if (width == 360.0) {
        tlLon = -180.0;
        brLon = 180.0;
    }

    d->topLeft = QGeoCoordinate(tlLat, tlLon);
    d->bottomRight = QGeoCoordinate(brLat, brLon);
}

/*!
    Returns the center of this geo rectangle. Equivalent to QGeoShape::center().
*/
QGeoCoordinate QGeoRectangle::center() const
{
    Q_D(const QGeoRectangle);

    return d->center();
}

/*!
    Sets the width of this geo rectangle in degrees to \a degreesWidth.
*/
void QGeoRectangle::setWidth(double degreesWidth)
{
    if (!isValid())
        return;

    if (degreesWidth < 0.0)
        return;

    Q_D(QGeoRectangle);

    if (degreesWidth >= 360.0) {
        d->topLeft.setLongitude(-180.0);
        d->bottomRight.setLongitude(180.0);
        return;
    }

    double tlLat = d->topLeft.latitude();
    double brLat = d->bottomRight.latitude();

    QGeoCoordinate c = center();

    double tlLon = c.longitude() - degreesWidth / 2.0;
    tlLon = QLocationUtils::wrapLong(tlLon);

    double brLon = c.longitude() + degreesWidth / 2.0;
    brLon = QLocationUtils::wrapLong(brLon);

    d->topLeft = QGeoCoordinate(tlLat, tlLon);
    d->bottomRight = QGeoCoordinate(brLat, brLon);
}

/*!
    Returns the width of this geo rectangle in degrees.

    The return value is undefined if this geo rectangle is invalid.
*/
double QGeoRectangle::width() const
{
    if (!isValid())
        return qQNaN();

    Q_D(const QGeoRectangle);

    double result = d->bottomRight.longitude() - d->topLeft.longitude();
    if (result < 0.0)
        result += 360.0;
    if (result > 360.0)
        result -= 360.0;

    return result;
}

/*!
    Sets the height of this geo rectangle in degrees to \a degreesHeight.
*/
void QGeoRectangle::setHeight(double degreesHeight)
{
    if (!isValid())
        return;

    if (degreesHeight < 0.0)
        return;

    if (degreesHeight >= 180.0) {
        degreesHeight = 180.0;
    }

    Q_D(QGeoRectangle);

    double tlLon = d->topLeft.longitude();
    double brLon = d->bottomRight.longitude();

    QGeoCoordinate c = center();

    double tlLat = c.latitude() + degreesHeight / 2.0;
    double brLat = c.latitude() - degreesHeight / 2.0;

    if (tlLat > 90.0) {
        brLat = 2* c.latitude() - 90.0;
        tlLat = 90.0;
    }

    if (tlLat < -90.0) {
        brLat = -90.0;
        tlLat = -90.0;
    }

    if (brLat > 90.0) {
        tlLat = 90.0;
        brLat = 90.0;
    }

    if (brLat < -90.0) {
        tlLat = 2 * c.latitude() + 90.0;
        brLat = -90.0;
    }

    d->topLeft = QGeoCoordinate(tlLat, tlLon);
    d->bottomRight = QGeoCoordinate(brLat, brLon);
}

/*!
    Returns the height of this geo rectangle in degrees.

    The return value is undefined if this geo rectangle is invalid.
*/
double QGeoRectangle::height() const
{
    if (!isValid())
        return qQNaN();

    Q_D(const QGeoRectangle);

    return d->topLeft.latitude() - d->bottomRight.latitude();
}

bool QGeoRectanglePrivate::contains(const QGeoCoordinate &coordinate) const
{
    if (!isValid() || !coordinate.isValid())
        return false;

    double left = topLeft.longitude();
    double right = bottomRight.longitude();
    double top = topLeft.latitude();
    double bottom = bottomRight.latitude();

    double lon = coordinate.longitude();
    double lat = coordinate.latitude();

    if (lat > top)
        return false;
    if (lat < bottom)
        return false;

    if ((lat == 90.0) && (top == 90.0))
        return true;

    if ((lat == -90.0) && (bottom == -90.0))
        return true;

    if (left <= right) {
        if ((lon < left) || (lon > right))
            return false;
    } else {
        if ((lon < left) && (lon > right))
            return false;
    }

    return true;
}

QGeoCoordinate QGeoRectanglePrivate::center() const
{
    if (!isValid())
        return QGeoCoordinate();

    double cLat = (topLeft.latitude() + bottomRight.latitude()) / 2.0;
    double cLon = (bottomRight.longitude() + topLeft.longitude()) / 2.0;

    if (topLeft.longitude() > bottomRight.longitude())
        cLon = cLon - 180.0;

    cLon = QLocationUtils::wrapLong(cLon);
    return QGeoCoordinate(cLat, cLon);
}

QGeoRectangle QGeoRectanglePrivate::boundingGeoRectangle() const
{
    return QGeoRectangle(topLeft, bottomRight);
}

/*!
    Returns whether the geo rectangle \a rectangle is contained within this
    geo rectangle.
*/
bool QGeoRectangle::contains(const QGeoRectangle &rectangle) const
{
    Q_D(const QGeoRectangle);

    return (d->contains(rectangle.topLeft())
            && d->contains(rectangle.topRight())
            && d->contains(rectangle.bottomLeft())
            && d->contains(rectangle.bottomRight()));
}

/*!
    Returns whether the geo rectangle \a rectangle intersects this geo rectangle.

    If the top or bottom edges of both geo rectangles are at one of the poles
    the geo rectangles are considered to be intersecting, since the longitude
    is irrelevant when the edges are at the pole.
*/
bool QGeoRectangle::intersects(const QGeoRectangle &rectangle) const
{
    Q_D(const QGeoRectangle);

    double left1 = d->topLeft.longitude();
    double right1 = d->bottomRight.longitude();
    double top1 = d->topLeft.latitude();
    double bottom1 = d->bottomRight.latitude();

    double left2 = rectangle.d_func()->topLeft.longitude();
    double right2 = rectangle.d_func()->bottomRight.longitude();
    double top2 = rectangle.d_func()->topLeft.latitude();
    double bottom2 = rectangle.d_func()->bottomRight.latitude();

    if (top1 < bottom2)
        return false;

    if (bottom1 > top2)
        return false;

    if ((top1 == 90.0) && (top1 == top2))
        return true;

    if ((bottom1 == -90.0) && (bottom1 == bottom2))
        return true;

    if (left1 < right1) {
        if (left2 < right2) {
            if ((left1 > right2) || (right1 < left2))
                return false;
        } else {
            if ((left1 > right2) && (right1 < left2))
                return false;
        }
    } else {
        if (left2 < right2) {
            if ((left2 > right1) && (right2 < left1))
                return false;
        } else {
            // if both wrap then they have to intersect
        }
    }

    return true;
}

/*!
    Translates this geo rectangle by \a degreesLatitude northwards and \a
    degreesLongitude eastwards.

    Negative values of \a degreesLatitude and \a degreesLongitude correspond to
    southward and westward translation respectively.

    If the translation would have caused the geo rectangle to cross a pole the
    geo rectangle will be translated until the top or bottom edge of the geo rectangle
    touches the pole but not further.
*/
void QGeoRectangle::translate(double degreesLatitude, double degreesLongitude)
{
    // TODO handle dlat, dlon larger than 360 degrees

    Q_D(QGeoRectangle);

    double tlLat = d->topLeft.latitude();
    double tlLon = d->topLeft.longitude();
    double brLat = d->bottomRight.latitude();
    double brLon = d->bottomRight.longitude();

    if (degreesLatitude >= 0.0)
        degreesLatitude = qMin(degreesLatitude, 90.0 - tlLat);
    else
        degreesLatitude = qMax(degreesLatitude, -90.0 - brLat);

    if ( (tlLon != -180.0) || (brLon != 180.0) ) {
        tlLon = QLocationUtils::wrapLong(tlLon + degreesLongitude);
        brLon = QLocationUtils::wrapLong(brLon + degreesLongitude);
    }

    tlLat += degreesLatitude;
    brLat += degreesLatitude;

    d->topLeft = QGeoCoordinate(tlLat, tlLon);
    d->bottomRight = QGeoCoordinate(brLat, brLon);
}

/*!
    Returns a copy of this geo rectangle translated by \a degreesLatitude northwards and \a
    degreesLongitude eastwards.

    Negative values of \a degreesLatitude and \a degreesLongitude correspond to
    southward and westward translation respectively.

    \sa translate()
*/
QGeoRectangle QGeoRectangle::translated(double degreesLatitude, double degreesLongitude) const
{
    QGeoRectangle result(*this);
    result.translate(degreesLatitude, degreesLongitude);
    return result;
}

/*!
    Extends the geo rectangle to also cover the coordinate \a coordinate

    \since 5.9
*/
void QGeoRectangle::extendRectangle(const QGeoCoordinate &coordinate)
{
    Q_D(QGeoRectangle);
    d->extendRectangle(coordinate);
}

/*!
    Returns the smallest geo rectangle which contains both this geo rectangle and \a rectangle.

    If the centers of the two geo rectangles are separated by exactly 180.0 degrees then the
    width is set to 360.0 degrees with the leftmost longitude set to -180.0 degrees and the
    rightmost longitude set to 180.0 degrees.  This is done to ensure that the result is
    independent of the order of the operands.

*/
QGeoRectangle QGeoRectangle::united(const QGeoRectangle &rectangle) const
{
    QGeoRectangle result(*this);
    if (rectangle.isValid())
        result |= rectangle;
    return result;
}

/*!
    Extends the rectangle in the smallest possible way to include \a coordinate in
    the shape.

    Both the rectangle and coordinate needs to be valid. If the rectangle already covers
    the coordinate noting happens.

*/
void QGeoRectanglePrivate::extendRectangle(const QGeoCoordinate &coordinate)
{
    if (!isValid() || !coordinate.isValid() || contains(coordinate))
        return;

    double left = topLeft.longitude();
    double right = bottomRight.longitude();
    double top = topLeft.latitude();
    double bottom = bottomRight.latitude();

    double inputLat = coordinate.latitude();
    double inputLon = coordinate.longitude();

    top = qMax(top, inputLat);
    bottom = qMin(bottom, inputLat);

    bool wrap = left > right;

    if (wrap && inputLon > right && inputLon < left) {
        if (qAbs(left - inputLon) < qAbs(right - inputLon))
            left = inputLon;
        else
            right = inputLon;
    } else if (!wrap) {
        if (inputLon < left) {
            if (360 - (right - inputLon) < left - inputLon)
                right = inputLon;
            else
                left = inputLon;
        } else if (inputLon > right) {
            if (360 - (inputLon - left) < inputLon - right)
                left = inputLon;
            else
                right = inputLon;
        }
    }
    topLeft = QGeoCoordinate(top, left);
    bottomRight = QGeoCoordinate(bottom, right);
}

/*!
    \fn QGeoRectangle QGeoRectangle::operator|(const QGeoRectangle &rectangle) const

    Returns the smallest geo rectangle which contains both this geo rectangle and \a rectangle.

    If the centers of the two geo rectangles are separated by exactly 180.0 degrees then the
    width is set to 360.0 degrees with the leftmost longitude set to -180.0 degrees and the
    rightmost longitude set to 180.0 degrees.  This is done to ensure that the result is
    independent of the order of the operands.

*/

/*!
    Returns the smallest geo rectangle which contains both this geo rectangle and \a rectangle.

    If the centers of the two geo rectangles are separated by exactly 180.0 degrees then the
    width is set to 360.0 degrees with the leftmost longitude set to -180.0 degrees and the
    rightmost longitude set to 180.0 degrees.  This is done to ensure that the result is
    independent of the order of the operands.

*/
QGeoRectangle &QGeoRectangle::operator|=(const QGeoRectangle &rectangle)
{
    // If non-intersecting goes for most narrow box

    Q_D(QGeoRectangle);

    double top = qMax(d->topLeft.latitude(), rectangle.d_func()->topLeft.latitude());
    double bottom = qMin(d->bottomRight.latitude(), rectangle.d_func()->bottomRight.latitude());

    QGeoRectangle candidate(
        {top, d->topLeft.longitude()},
        {bottom, rectangle.d_func()->bottomRight.longitude()}
    );
    QGeoRectangle otherCandidate(
        {top, rectangle.d_func()->topLeft.longitude()},
        {bottom, d->bottomRight.longitude()}
    );
    double unwrappedWidth = (candidate.width() < rectangle.width() ? 360 : 0) + candidate.width();
    double otherUnwrappedWidth = (otherCandidate.width() < width() ? 360 : 0) + otherCandidate.width();
    if (otherUnwrappedWidth < unwrappedWidth) {
        candidate = otherCandidate;
        unwrappedWidth = otherUnwrappedWidth;
    }
    if (360 <= unwrappedWidth) {
        candidate.d_func()->topLeft.setLongitude(-180.0);
        candidate.d_func()->bottomRight.setLongitude(180.0);
    }

    candidate = (candidate.width() < width() ? *this : candidate);
    candidate = (candidate.width() < rectangle.width() ? rectangle : candidate);

    double middle1 = center().longitude();
    double middle2 = rectangle.center().longitude();
    if ((middle1 <= middle2 ? 0 : 360) + middle2 - middle1 == 180) {
        candidate.d_func()->topLeft.setLongitude(-180.0);
        candidate.d_func()->bottomRight.setLongitude(180.0);
    }

    *this = candidate;
    this->d_func()->topLeft.setLatitude(top);
    this->d_func()->bottomRight.setLatitude(bottom);

    return *this;
}

/*!
    Returns the geo rectangle properties as a string.

    \since 5.5
*/
QString QGeoRectangle::toString() const
{
    if (type() != QGeoShape::RectangleType) {
        qWarning("Not a rectangle a %d\n", type());
        return QStringLiteral("QGeoRectangle(not a rectangle)");
    }

    return QStringLiteral("QGeoRectangle({%1, %2}, {%3, %4})")
        .arg(topLeft().latitude())
        .arg(topLeft().longitude())
        .arg(bottomRight().latitude())
        .arg(bottomRight().longitude());
}

/*******************************************************************************
*******************************************************************************/

QGeoRectanglePrivate::QGeoRectanglePrivate()
:   QGeoShapePrivate(QGeoShape::RectangleType)
{
}

QGeoRectanglePrivate::QGeoRectanglePrivate(const QGeoCoordinate &topLeft,
                                               const QGeoCoordinate &bottomRight)
:   QGeoShapePrivate(QGeoShape::RectangleType), topLeft(topLeft), bottomRight(bottomRight)
{
}

QGeoRectanglePrivate::QGeoRectanglePrivate(const QGeoRectanglePrivate &other)
:   QGeoShapePrivate(QGeoShape::RectangleType), topLeft(other.topLeft),
    bottomRight(other.bottomRight)
{
}

QGeoRectanglePrivate::~QGeoRectanglePrivate() {}

QGeoShapePrivate *QGeoRectanglePrivate::clone() const
{
    return new QGeoRectanglePrivate(*this);
}

bool QGeoRectanglePrivate::operator==(const QGeoShapePrivate &other) const
{
    if (!QGeoShapePrivate::operator==(other))
        return false;

    const QGeoRectanglePrivate &otherBox = static_cast<const QGeoRectanglePrivate &>(other);

    return topLeft == otherBox.topLeft && bottomRight == otherBox.bottomRight;
}

size_t QGeoRectanglePrivate::hash(size_t seed) const
{
    return qHashMulti(seed, topLeft, bottomRight);
}

QT_END_NAMESPACE

#include "moc_qgeorectangle.cpp"
