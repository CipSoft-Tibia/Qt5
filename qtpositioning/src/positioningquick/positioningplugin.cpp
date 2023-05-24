// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qpositioningquickglobal_p.h>
#include <QGeoCircle>
#include <QGeoCoordinate>
#include <QGeoPath>
#include <QGeoPolygon>
#include <QGeoRectangle>
#include <QtPositioningQuick/private/qquickgeocoordinateanimation_p.h>
#include <QtCore/QVariantAnimation>
#include <QtQml/QQmlEngineExtensionPlugin>

QT_BEGIN_NAMESPACE

/*!
    \qmlvaluetype coordinate
    \ingroup qmlvaluetypes
    \inqmlmodule QtPositioning
    \ingroup qml-QtPositioning5-basictypes
    \since 5.2

    \brief The coordinate type represents and stores a geographic position.

    This type is a QML representation of \l QGeoCoordinate and represents a geographic
    position in the form of \l {latitude}, \l longitude and \l altitude attributes.
    The \l latitude attribute specifies the number of
    decimal degrees above and below the equator.  A positive latitude indicates the Northern
    Hemisphere and a negative latitude indicates the Southern Hemisphere.  The \l longitude
    attribute specifies the number of decimal degrees east and west.  A positive longitude
    indicates the Eastern Hemisphere and a negative longitude indicates the Western Hemisphere.
    The \l altitude attribute specifies the number of meters above sea level.  Together, these
    attributes specify a 3-dimensional position anywhere on or near the Earth's surface.

    The \l isValid attribute can be used to test if a coordinate is valid.  A coordinate is
    considered valid if it has a valid latitude and longitude.  A valid altitude is not required.
    The latitude must be between -90 and 90 inclusive and the longitude must be between -180 and
    180 inclusive.

    The \c coordinate type is used by many other types in the Qt Location module, for specifying
    the position of an object on a Map, the current position of a device and many other tasks.
    They also feature a number of important utility methods that make otherwise complex
    calculations simple to use, such as \l {atDistanceAndAzimuth}().

    \section1 Accuracy

    The latitude, longitude and altitude attributes stored in the coordinate type are represented
    as doubles, giving them approximately 16 decimal digits of precision -- enough to specify
    micrometers.  The calculations performed in coordinate's methods such as \l {azimuthTo}() and
    \l {distanceTo}() also use doubles for all intermediate values, but the inherent inaccuracies in
    their spherical Earth model dominate the amount of error in their output.

    \section1 Example Usage

    Use properties of type \l var to store a \c {coordinate}.  To create a \c coordinate use
    one of the methods described below.  In all cases, specifying the \l altitude attribute is
    optional.

    To create a \c coordinate value, use the \l{QtPositioning::coordinate}{QtPositioning.coordinate()}
    function:

    \qml
    import QtPositioning

    Location { coordinate: QtPositioning.coordinate(-27.5, 153.1) }
    \endqml

    or as separate \l latitude, \l longitude and \l altitude components:

    \qml
    Location {
        coordinate {
            latitude: -27.5
            longitude: 153.1
        }
    }
    \endqml

    When integrating with C++, note that any QGeoCoordinate value passed into QML from C++ is
    automatically converted into a \c coordinate value, and vice-versa.

    \section1 Properties

    \section2 latitude

    \code
    real latitude
    \endcode

    This property holds the latitude value of the geographical position
    (decimal degrees). A positive latitude indicates the Northern Hemisphere,
    and a negative latitude indicates the Southern Hemisphere.
    If the property has not been set, its default value is NaN.

    For more details see the \l {QGeoCoordinate::latitude} property

    \section2 longitude

    \code
    real longitude
    \endcode

    This property holds the longitude value of the geographical position
    (decimal degrees). A positive longitude indicates the Eastern Hemisphere,
    and a negative longitude indicates the Western Hemisphere
    If the property has not been set, its default value is NaN.

    For more details see the \l {QGeoCoordinate::longitude} property

    \section2 altitude

    \code
    real altitude
    \endcode

    This property holds the altitude value (meters above sea level).
    If the property has not been set, its default value is NaN.

    For more details see the \l {QGeoCoordinate::altitude} property

    \section2 isValid

    \code
    bool isValid
    \endcode

    This property holds the current validity of the coordinate. Coordinates
    are considered valid if they have been set with a valid latitude and
    longitude (altitude is not required).

    The latitude must be between -90 to 90 inclusive to be considered valid,
    and the longitude must be between -180 to 180 inclusive to be considered
    valid.

    This is a read-only property.

    \section1 Methods

    \section2 distanceTo()

    \code
    real distanceTo(coordinate other)
    \endcode

    Returns the distance (in meters) from this coordinate to the coordinate specified by \a other.
    Altitude is not used in the calculation.

    This calculation returns the great-circle distance between the two coordinates, with an
    assumption that the Earth is spherical for the purpose of this calculation.

    \section2 azimuthTo()

    \code
    real azimuth(coordinate other)
    \endcode

    Returns the azimuth (or bearing) in degrees from this coordinate to the coordinate specified by
    \a other.  Altitude is not used in the calculation.

    There is an assumption that the Earth is spherical for the purpose of this calculation.

    \section2 atDistanceAndAzimuth()

    \code
    coordinate atDistanceAndAzimuth(real distance, real azimuth)
    \endcode

    Returns the coordinate that is reached by traveling \a distance metres from this coordinate at
    \a azimuth degrees along a great-circle.

    There is an assumption that the Earth is spherical for the purpose of this calculation.
*/

/*!
    \qmlvaluetype geoshape
    \ingroup qmlvaluetypes
    \inqmlmodule QtPositioning
    \ingroup qml-QtPositioning5-basictypes
    \since 5.2

    \brief A geoshape type represents an abstract geographic area.

    This type is a QML representation of \l QGeoShape which is an abstract geographic area.
    It includes attributes and methods common to all geographic areas. To create objects
    that represent a valid geographic area use \l {georectangle} or \l {geocircle}.

    The \l isValid attribute can be used to test if the geoshape represents a valid geographic
    area.

    The \l isEmpty attribute can be used to test if the geoshape represents a region with a
    geometrical area of 0.

    The \l {contains}() method can be used to test if a \l {coordinate} is
    within the geoshape.

    \section1 Example Usage

    Use properties of type \l var to store a \c {geoshape}.  To create a \c geoshape use one
    of the methods described below.

    To create a \c geoshape value, specify it as a "shape()" string:

    \qml
    import QtPositioning

    Item {
        property var region: "shape()"
    }
    \endqml

    or with the \l {QtPositioning::shape}{QtPositioning.shape()} function:

    \qml
    import QtPositioning

    Item {
        property var region: QtPositioning.shape()
    }
    \endqml

    When integrating with C++, note that any QGeoShape value passed into QML from C++ is
    automatically converted into a \c geoshape value, and vice-versa.

    \section1 Properties

    \section2 isEmpty

    \code
    bool isEmpty
    \endcode

    Returns whether this geoshape is empty. An empty geoshape is a region which has
    a geometrical area of 0.

    \section2 isValid

    \code
    bool isValid
    \endcode

    Returns whether this geoshape is valid.

    A geoshape is considered to be invalid if some of the data that is required to
    unambiguously describe the geoshape has not been set or has been set to an
    unsuitable value.

    \section2 type

    \code
    ShapeType type
    \endcode

    Returns the current type of the shape.

    \list
        \li \c GeoShape.UnknownType - The shape's type is not known.
        \li \c GeoShape.RectangleType - The shape is a \l georectangle.
        \li \c GeoShape.CircleType - The shape is a \l geocircle.
        \li \c GeoShape.PathType - The shape is a \l geopath. (Since Qt 5.9)
        \li \c GeoShape.PolygonType - The shape is a \l geopolygon. (Since Qt 5.10)
    \endlist

    This QML property was introduced by Qt 5.5.

    \section1 Methods

    \section2 contains()

    \code
    bool contains(coordinate coord)
    \endcode

    Returns true if the \l {QtPositioning::coordinate}{coordinate} specified by \a coord is within
    this geoshape; Otherwise returns false.
*/

/*!
    \qmlvaluetype georectangle
    \ingroup qmlvaluetypes
    \inqmlmodule QtPositioning
    \ingroup qml-QtPositioning5-basictypes
    \since 5.2

    \brief The georectangle type represents a rectangular geographic area.

    The \c georectangle type is a \l {geoshape} that represents a
    rectangular geographic area. The type is direct representation of a \l QGeoRectangle.
    It is defined by a pair of \l {coordinate}{coordinates} which represent the top-left
    and bottom-right corners of the \c {georectangle}.  The coordinates are accessible
    from the \l topLeft and \l bottomRight attributes.

    A \c georectangle is considered invalid if the top-left or bottom-right coordinates are invalid
    or if the top-left coordinate is south of the bottom-right coordinate.

    The coordinates of the four corners of the \c georectangle can be accessed with the
    \l {topLeft}, \l {topRight}, \l {bottomLeft} and \l {bottomRight} attributes.  The \l center
    attribute can be used to get the coordinate of the center of the \c georectangle.  The \l width
    and \l height attributes can be used to get the width and height of the \c georectangle in
    degrees.  Setting one of these attributes will cause the other attributes to be adjusted
    accordingly.

    \section1 Limitations

    A \c georectangle can never cross the poles.

    If the height or center of a \c georectangle is adjusted such that it would cross one of the
    poles the height is modified such that the \c georectangle touches but does not cross the pole
    and that the center coordinate is still in the center of the \c georectangle.

    \section1 Example Usage

    Use properties of type \l var to store a \c {georectangle}.  To create a \c georectangle
    value, use the \l {QtPositioning::rectangle}{QtPositioning.rectangle()} function:

    \qml
    import QtPositioning

    Item {
        property var region: QtPositioning.rectangle(QtPositioning.coordinate(-27.5, 153.1), QtPositioning.coordinate(-27.6, 153.2))
    }
    \endqml

    When integrating with C++, note that any QGeoRectangle value passed into QML from C++ is
    automatically converted into a \c georectangle value, and vice-versa.

    \section1 Properties

    \section2 bottomLeft

    \code
    coordinate bottomLeft
    \endcode

    This property holds the bottom left coordinate of this georectangle.

    \section2 bottomRight

    \code
    coordinate bottomRight
    \endcode

    This property holds the bottom right coordinate of this georectangle.

    \section2 center

    \code
    coordinate center
    \endcode

    This property holds the center coordinate of this georectangle. For more details
    see \l {QGeoRectangle::setCenter()}.

    \section2 height

    \code
    double height
    \endcode

    This property holds the height of this georectangle (in degrees). For more details
    see \l {QGeoRectangle::setHeight()}.

    \note If the georectangle is invalid, it is not possible to set the height. QtPositioning
    releases prior to Qt 5.5 permitted the setting of the height even on invalid georectangles.

    \section2 topLeft

    \code
    coordinate topLeft
    \endcode

    This property holds the top left coordinate of this georectangle.

    \section2 topRight

    \code
    coordinate topRight
    \endcode

    This property holds the top right coordinate of this georectangle.

    \section2 width

    \code
    double width
    \endcode

    This property holds the width of this georectangle (in degrees). For more details
    see \l {QGeoRectangle::setWidth()}.

    \note If the georectangle is invalid, it is not possible to set the width. QtPositioning
    releases prior to Qt 5.5 permitted the setting of the width even on invalid georectangles.
*/

/*!
    \qmlvaluetype geocircle
    \ingroup qmlvaluetypes
    \inqmlmodule QtPositioning
    \ingroup qml-QtPositioning5-basictypes
    \since 5.2

    \brief The geocircle type represents a circular geographic area.

    The \c geocircle type is a \l {geoshape} that represents a circular
    geographic area. It is a direct representation of a \l QGeoCircle and is defined
    in terms of a \l {coordinate} which specifies the \l center of the circle and
    a qreal which specifies the \l radius of the circle in meters.

    The circle is considered invalid if the \l center coordinate is invalid or if
    the \l radius is less than zero.

    \section1 Example Usage

    Use properties of type \l var to store a \c {geocircle}.  To create a \c geocircle value,
    use the \l {QtPositioning::circle}{QtPositioning.circle()} function:

    \qml
    import QtPositioning

    Item {
        property var region: QtPositioning.circle(QtPositioning.coordinate(-27.5, 153.1), 1000)
    }
    \endqml

    When integrating with C++, note that any QGeoCircle value passed into QML from C++ is
    automatically converted into a \c geocircle value, and vise-versa.

    \section1 Properties

    \section2 center

    \code
    coordinate radius
    \endcode

    This property holds the coordinate of the center of the geocircle.

    \section2 radius

    \code
    real radius
    \endcode

    This property holds the radius of the geocircle in meters.

    The default value for the radius is -1 indicating an invalid geocircle area.
*/

/*!
    \qmlvaluetype geopath
    \ingroup qmlvaluetypes
    \inqmlmodule QtPositioning
    \ingroup qml-QtPositioning5-basictypes
    \since 5.9

    \brief The geopath type represents a geographic path.

    The \c geopath type is a \l {geoshape} that represents a geographic
    path. It is a direct representation of a \l QGeoPath and is defined
    in terms of a \l {path} which holds the list of geo coordinates in the
    path.

    The path is considered invalid if it is empty.

    When integrating with C++, note that any QGeoPath value passed into QML from C++ is
    automatically converted into a \c geopath value, and vice versa.

    \section1 Properties

    \section2 path

    This property holds the list of coordinates defining the path.

    \section2 width

    This property holds the width of the path in meters. This is currently only used
    when calling the \l {contains}() method.

    The default value for the width is 0.
*/

/*!
   \qmlvaluetype geopolygon
   \ingroup qmlvaluetypes
   \inqmlmodule QtPositioning
   \ingroup qml-QtPositioning5-basictypes
   \since 5.10

   \brief The geopolygon type represents a geographic polygon.

   The \c geopolygon type is a \l [QML] geoshape that represents a geographic
   polygon. It is a direct representation of QGeoPolygon and is defined in
   terms of a \l path which holds a list of geo coordinates in the polygon.

   The polygon is considered invalid if its path holds less than three
   coordinates.

   When integrating with C++, note that any QGeoPolygon value passed into QML
   is automatically converted into a \c geopolygon, and vice versa.

   \section1 Properties

   \section2 path

   This property holds the list of coordinates defining the polygon.
*/

class QtPositioningDeclarativeModule: public QQmlEngineExtensionPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QtPositioningDeclarativeModule(QObject *parent = 0) : QQmlEngineExtensionPlugin(parent)
    {
        volatile auto registration = &qml_register_types_QtPositioning;
        Q_UNUSED(registration)
    }
};

namespace {

bool parseCoordinate(const QVariantMap &map, QGeoCoordinate &c)
{
    if (const auto it = map.find(QStringLiteral("latitude")); it != map.end())
        c.setLatitude(it.value().toDouble());
    else
        c.setLatitude(qQNaN());
    if (const auto it = map.find(QStringLiteral("longitude")); it != map.end())
        c.setLongitude(it.value().toDouble());
    else
        c.setLongitude(qQNaN());
    if (const auto it = map.find(QStringLiteral("altitude")); it != map.end())
        c.setAltitude(it.value().toDouble());
    else
        c.setAltitude(qQNaN());

    // Not considering the case where the map is valid but containing NaNs.
    return c.isValid();
}

bool parseRectangle(const QVariantMap &map, QGeoRectangle &rect)
{
    if (const auto it = map.find(QStringLiteral("topLeft")); it != map.end())
        rect.setTopLeft(it.value().value<QGeoCoordinate>());
    if (const auto it = map.find(QStringLiteral("bottomLeft")); it != map.end())
        rect.setBottomLeft(it.value().value<QGeoCoordinate>());
    if (const auto it = map.find(QStringLiteral("topRight")); it != map.end())
        rect.setTopRight(it.value().value<QGeoCoordinate>());
    if (const auto it = map.find(QStringLiteral("bottomRight")); it != map.end())
        rect.setBottomRight(it.value().value<QGeoCoordinate>());
    if (const auto it = map.find(QStringLiteral("center")); it != map.end())
        rect.setCenter(it.value().value<QGeoCoordinate>());
    if (const auto it = map.find(QStringLiteral("width")); it != map.end())
        rect.setWidth(it.value().toDouble());
    if (const auto it = map.find(QStringLiteral("height")); it != map.end())
        rect.setHeight(it.value().toDouble());

    // Not considering the case where the map is valid but containing NaNs.
    return rect.isValid();
}
}

void QtPositioningDeclarative_initializeModule()
{
    QMetaType::registerConverter<QGeoRectangle, QGeoShape>();
    QMetaType::registerConverter<QGeoShape, QGeoRectangle>();
    QMetaType::registerConverter<QGeoShape, QGeoCircle>();
    QMetaType::registerConverter<QGeoCircle, QGeoShape>();
    QMetaType::registerConverter<QGeoShape, QGeoPath>();
    QMetaType::registerConverter<QGeoPath, QGeoShape>();
    QMetaType::registerConverter<QGeoShape, QGeoPolygon>();
    QMetaType::registerConverter<QGeoPolygon, QGeoShape>();

    if (!QMetaType::registerConverterFunction([](const void *src, void *target) -> bool {
        const QVariantMap &map = *static_cast<const QVariantMap *>(src);
        QGeoCoordinate &coord = *static_cast<QGeoCoordinate *>(target);
        return parseCoordinate(map, coord);
    }, QMetaType::fromType<QVariantMap>(), QMetaType::fromType<QGeoCoordinate>())) {
        qWarning("Failed to register conversion function from QVariantMap to QGeoCoordinate");
    }

    if (!QMetaType::registerConverterFunction([](const void *src, void *target) -> bool {
        const QVariantMap &map = *static_cast<const QVariantMap *>(src);
        QGeoRectangle &rect = *static_cast<QGeoRectangle *>(target);
        return parseRectangle(map, rect);
    }, QMetaType::fromType<QVariantMap>(), QMetaType::fromType<QGeoRectangle>())) {
        qWarning("Failed to register conversion function from QVariantMap to QGeoRectangle");
    }

    qRegisterAnimationInterpolator<QGeoCoordinate>(q_coordinateInterpolator);
}

Q_CONSTRUCTOR_FUNCTION(QtPositioningDeclarative_initializeModule)

QT_END_NAMESPACE

#include "positioningplugin.moc"
