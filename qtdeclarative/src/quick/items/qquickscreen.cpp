/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickscreen_p.h"

#include "qquickitem.h"
#include "qquickitem_p.h"
#include "qquickwindow.h"

#include <QGuiApplication>
#include <QScreen>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Screen
    \instantiates QQuickScreenAttached
    \inqmlmodule QtQuick.Window
    \ingroup qtquick-visual-utility
    \brief The Screen attached object provides information about the Screen an Item or Window is displayed on.

    The Screen attached object is valid inside Item or Item derived types,
    after component completion. Inside these items it refers to the screen that
    the item is currently being displayed on.

    The attached object is also valid inside Window or Window derived types,
    after component completion. In that case it refers to the screen where the
    Window was created. It is generally better to access the Screen from the
    relevant Item instead, because on a multi-screen desktop computer, the user
    can drag a Window into a position where it spans across multiple screens.
    In that case some Items will be on one screen, and others on a different
    screen.

    To use this type, you will need to import the module with the following line:
    \code
    import QtQuick.Window 2.2
    \endcode
    It is a separate import in order to allow you to have a QML environment
    without access to window system features.

    Note that the Screen type is not valid at Component.onCompleted, because
    the Item or Window has not been displayed on a screen by this time.
*/

/*!
    \qmlattachedproperty string Screen::name
    \readonly
    \since 5.1

    The name of the screen.
*/
/*!
    \qmlattachedproperty int Screen::virtualX
    \readonly
    \since 5.9

    The x coordinate of the screen within the virtual desktop.
*/
/*!
    \qmlattachedproperty int Screen::virtualY
    \readonly
    \since 5.9

    The y coordinate of the screen within the virtual desktop.
*/
/*!
    \qmlattachedproperty string Screen::manufacturer
    \readonly
    \since 5.10

    The manufacturer of the screen.
*/
/*!
    \qmlattachedproperty string Screen::model
    \readonly
    \since 5.10

    The model of the screen.
*/
/*!
    \qmlattachedproperty string Screen::serialNumber
    \readonly
    \since 5.10

    The serial number of the screen.
*/
/*!
    \qmlattachedproperty int Screen::width
    \readonly

    This contains the width of the screen in pixels.
*/
/*!
    \qmlattachedproperty int Screen::height
    \readonly

    This contains the height of the screen in pixels.
*/
/*!
    \qmlattachedproperty int Screen::desktopAvailableWidth
    \readonly
    \since 5.1

    This contains the available width of the collection of screens which make
    up the virtual desktop, in pixels, excluding window manager reserved areas
    such as task bars and system menus. If you want to position a Window at
    the right of the desktop, you can bind to it like this:

    \code
    x: Screen.desktopAvailableWidth - width
    \endcode
*/
/*!
    \qmlattachedproperty int Screen::desktopAvailableHeight
    \readonly
    \since 5.1

    This contains the available height of the collection of screens which make
    up the virtual desktop, in pixels, excluding window manager reserved areas
    such as task bars and system menus. If you want to position a Window at
    the bottom of the desktop, you can bind to it like this:

    \code
    y: Screen.desktopAvailableHeight - height
    \endcode
*/
/*!
    \qmlattachedproperty real Screen::logicalPixelDensity
    \readonly
    \since 5.1
    \deprecated

    The number of logical pixels per millimeter. This is the effective pixel
    density provided by the platform to use in image scaling calculations.

    Due to inconsistencies in how logical pixel density is handled across
    the various platforms Qt supports, it is recommended to
    use physical pixels instead (via the \c pixelDensity property) for
    portability.

    \sa pixelDensity
*/
/*!
    \qmlattachedproperty real Screen::pixelDensity
    \readonly
    \since 5.2

    The number of physical pixels per millimeter.
*/
/*!
    \qmlattachedproperty real Screen::devicePixelRatio
    \readonly
    \since 5.4

    The ratio between physical pixels and device-independent pixels for the screen.

    Common values are 1.0 on normal displays and 2.0 on Apple "retina" displays.
*/
/*!
    \qmlattachedproperty Qt::ScreenOrientation Screen::primaryOrientation
    \readonly

    This contains the primary orientation of the screen.  If the
    screen's height is greater than its width, then the orientation is
    Qt.PortraitOrientation; otherwise it is Qt.LandscapeOrientation.

    If you are designing an application which changes its layout depending on
    device orientation, you probably want to use primaryOrientation to
    determine the layout. That is because on a desktop computer, you can expect
    primaryOrientation to change when the user rotates the screen via the
    operating system's control panel, even if the computer does not contain an
    accelerometer. Likewise on most handheld computers which do have
    accelerometers, the operating system will rotate the whole screen
    automatically, so again you will see the primaryOrientation change.
*/
/*!
    \qmlattachedproperty Qt::ScreenOrientation Screen::orientation
    \readonly

    This contains the current orientation of the screen, from the accelerometer
    (if any). On a desktop computer, this value typically does not change.

    If primaryOrientation == orientation, it means that the screen
    automatically rotates all content which is displayed, depending on how you
    hold it. But if orientation changes while primaryOrientation does NOT
    change, then probably you are using a device which does not rotate its own
    display. In that case you may need to use \l {Item::rotation}{Item.rotation} or
    \l {Item::transform}{Item.transform} to rotate your content.

    \note This property does not update unless a Screen::orientationUpdateMask
    is set to a value other than \c 0.
*/
/*!
    \qmlattachedmethod int Screen::angleBetween(Qt::ScreenOrientation a, Qt::ScreenOrientation b)

    Returns the rotation angle, in degrees, between the specified screen
    orientations \a a and \a b.
*/

/*!
    \qmlattachedproperty Qt::ScreenOrientations Screen::orientationUpdateMask
    \since 5.4

    This contains the update mask for the orientation. Screen::orientation
    only emits changes for the screen orientations matching this mask.

    By default it is set to the value of the QScreen that the window uses.
*/

QQuickScreenInfo::QQuickScreenInfo(QObject *parent, QScreen *wrappedScreen)
    : QObject(parent)
    , m_screen(wrappedScreen)
{
}

QString QQuickScreenInfo::name() const
{
    if (!m_screen)
        return QString();
    return m_screen->name();
}

QString QQuickScreenInfo::manufacturer() const
{
    if (!m_screen)
        return QString();
    return m_screen->manufacturer();
}

QString QQuickScreenInfo::model() const
{
    if (!m_screen)
        return QString();
    return m_screen->model();
}

QString QQuickScreenInfo::serialNumber() const
{
    if (!m_screen)
        return QString();
    return m_screen->serialNumber();
}

int QQuickScreenInfo::width() const
{
    if (!m_screen)
        return 0;
    return m_screen->size().width();
}

int QQuickScreenInfo::height() const
{
    if (!m_screen)
        return 0;
    return m_screen->size().height();
}

int QQuickScreenInfo::desktopAvailableWidth() const
{
    if (!m_screen)
        return 0;
    return m_screen->availableVirtualSize().width();
}

int QQuickScreenInfo::desktopAvailableHeight() const
{
    if (!m_screen)
        return 0;
    return m_screen->availableVirtualSize().height();
}

qreal QQuickScreenInfo::logicalPixelDensity() const
{
    if (!m_screen)
        return 0.0;
    return m_screen->logicalDotsPerInch() / 25.4;
}

qreal QQuickScreenInfo::pixelDensity() const
{
    if (!m_screen)
        return 0.0;
    return m_screen->physicalDotsPerInch() / 25.4;
}

qreal QQuickScreenInfo::devicePixelRatio() const
{
    if (!m_screen)
        return 1.0;
    return m_screen->devicePixelRatio();
}

Qt::ScreenOrientation QQuickScreenInfo::primaryOrientation() const
{
    if (!m_screen)
        return Qt::PrimaryOrientation;
    return m_screen->primaryOrientation();
}

Qt::ScreenOrientation QQuickScreenInfo::orientation() const
{
    if (!m_screen)
        return Qt::PrimaryOrientation;
    return m_screen->orientation();
}

int QQuickScreenInfo::virtualX() const
{
    if (!m_screen)
        return 0;
    return m_screen->geometry().topLeft().x();
}

int QQuickScreenInfo::virtualY() const
{
    if (!m_screen)
        return 0;
    return m_screen->geometry().topLeft().y();
}

void QQuickScreenInfo::setWrappedScreen(QScreen *screen)
{
    if (screen == m_screen)
        return;

    QScreen *oldScreen = m_screen;
    m_screen = screen;

    if (oldScreen)
        oldScreen->disconnect(this);

    if (!screen) //Don't bother emitting signals, because the new values are garbage anyways
        return;

    if (!oldScreen || screen->geometry() != oldScreen->geometry()) {
        emit virtualXChanged();
        emit virtualYChanged();
    }
    if (!oldScreen || screen->size() != oldScreen->size()) {
        emit widthChanged();
        emit heightChanged();
    }
    if (!oldScreen || screen->name() != oldScreen->name())
        emit nameChanged();
    if (!oldScreen || screen->manufacturer() != oldScreen->manufacturer())
        emit manufacturerChanged();
    if (!oldScreen || screen->model() != oldScreen->model())
        emit modelChanged();
    if (!oldScreen || screen->serialNumber() != oldScreen->serialNumber())
        emit serialNumberChanged();
    if (!oldScreen || screen->orientation() != oldScreen->orientation())
        emit orientationChanged();
    if (!oldScreen || screen->primaryOrientation() != oldScreen->primaryOrientation())
        emit primaryOrientationChanged();
    if (!oldScreen || screen->availableVirtualGeometry() != oldScreen->availableVirtualGeometry())
        emit desktopGeometryChanged();
    if (!oldScreen || screen->logicalDotsPerInch() != oldScreen->logicalDotsPerInch())
        emit logicalPixelDensityChanged();
    if (!oldScreen || screen->physicalDotsPerInch() != oldScreen->physicalDotsPerInch())
        emit pixelDensityChanged();
    if (!oldScreen || screen->devicePixelRatio() != oldScreen->devicePixelRatio())
        emit devicePixelRatioChanged();

    qmlobject_connect(screen, QScreen, SIGNAL(geometryChanged(QRect)),
            this, QQuickScreenInfo, SIGNAL(widthChanged()));
    qmlobject_connect(screen, QScreen, SIGNAL(geometryChanged(QRect)),
            this, QQuickScreenInfo, SIGNAL(heightChanged()));
    qmlobject_connect(screen, QScreen, SIGNAL(geometryChanged(QRect)),
            this, QQuickScreenInfo, SIGNAL(virtualXChanged()));
    qmlobject_connect(screen, QScreen, SIGNAL(geometryChanged(QRect)),
            this, QQuickScreenInfo, SIGNAL(virtualYChanged()));
    qmlobject_connect(screen, QScreen, SIGNAL(orientationChanged(Qt::ScreenOrientation)),
            this, QQuickScreenInfo, SIGNAL(orientationChanged()));
    qmlobject_connect(screen, QScreen, SIGNAL(primaryOrientationChanged(Qt::ScreenOrientation)),
            this, QQuickScreenInfo, SIGNAL(primaryOrientationChanged()));
    qmlobject_connect(screen, QScreen, SIGNAL(virtualGeometryChanged(QRect)),
            this, QQuickScreenInfo, SIGNAL(desktopGeometryChanged()));
    qmlobject_connect(screen, QScreen, SIGNAL(logicalDotsPerInchChanged(qreal)),
            this, QQuickScreenInfo, SIGNAL(logicalPixelDensityChanged()));
    qmlobject_connect(screen, QScreen, SIGNAL(physicalDotsPerInchChanged(qreal)),
            this, QQuickScreenInfo, SIGNAL(pixelDensityChanged()));
}

QScreen *QQuickScreenInfo::wrappedScreen() const
{
    return m_screen;
}

QQuickScreenAttached::QQuickScreenAttached(QObject* attachee)
    : QQuickScreenInfo(attachee)
{
    m_attachee = qobject_cast<QQuickItem*>(attachee);

    if (m_attachee) {
        QQuickItemPrivate::get(m_attachee)->extra.value().screenAttached = this;

        if (m_attachee->window()) //It might not be assigned to a window yet
            windowChanged(m_attachee->window());
    } else {
        QQuickWindow *window = qobject_cast<QQuickWindow*>(attachee);
        if (window)
            windowChanged(window);
    }

    if (!m_screen)
        screenChanged(QGuiApplication::primaryScreen());
}

Qt::ScreenOrientations QQuickScreenAttached::orientationUpdateMask() const
{
    return m_updateMask;
}

void QQuickScreenAttached::setOrientationUpdateMask(Qt::ScreenOrientations mask)
{
    m_updateMaskSet = true;
    if (m_updateMask == mask)
        return;

    m_updateMask = mask;

    if (m_screen)
        m_screen->setOrientationUpdateMask(m_updateMask);

    emit orientationUpdateMaskChanged();
}

int QQuickScreenAttached::angleBetween(int a, int b)
{
    if (!m_screen)
        return Qt::PrimaryOrientation;
    return m_screen->angleBetween((Qt::ScreenOrientation)a,(Qt::ScreenOrientation)b);
}

void QQuickScreenAttached::windowChanged(QQuickWindow* c)
{
    if (m_window)
        qmlobject_disconnect(m_window, QQuickWindow, SIGNAL(screenChanged(QScreen*)), this, QQuickScreenAttached, SLOT(screenChanged(QScreen*)));
    m_window = c;
    screenChanged(c ? c->screen() : nullptr);
    if (c)
        qmlobject_connect(c, QQuickWindow, SIGNAL(screenChanged(QScreen*)), this, QQuickScreenAttached, SLOT(screenChanged(QScreen*)));
}

void QQuickScreenAttached::screenChanged(QScreen *screen)
{
    //qDebug() << "QQuickScreenAttached::screenChanged" << (screen ? screen->name() : QString::fromLatin1("null"));
    if (screen != m_screen) {
        setWrappedScreen(screen);
        if (!m_screen)
            return;
        if (m_updateMaskSet) {
            m_screen->setOrientationUpdateMask(m_updateMask);
        } else if (m_updateMask != m_screen->orientationUpdateMask()) {
            m_updateMask = m_screen->orientationUpdateMask();
            emit orientationUpdateMaskChanged();
        }
    }
}

QT_END_NAMESPACE

#include "moc_qquickscreen_p.cpp"
