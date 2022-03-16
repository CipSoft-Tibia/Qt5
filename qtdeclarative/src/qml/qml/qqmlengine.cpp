/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qqmlengine_p.h"
#include "qqmlengine.h"
#include "qqmlcomponentattached_p.h"

#include "qqmlcontext_p.h"
#include "qqml.h"
#include "qqmlcontext.h"
#include "qqmlexpression.h"
#include "qqmlcomponent.h"
#include "qqmlvme_p.h"
#include "qqmlstringconverters_p.h"
#include "qqmlscriptstring.h"
#include "qqmlglobal_p.h"
#include "qqmlcomponent_p.h"
#include "qqmldirparser_p.h"
#include "qqmlextensioninterface.h"
#include "qqmllist_p.h"
#include "qqmltypenamecache_p.h"
#include "qqmlnotifier_p.h"
#include "qqmlincubator.h"
#include "qqmlabstracturlinterceptor.h"
#include <private/qqmlboundsignal_p.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qsettings.h>
#include <QtCore/qmetaobject.h>
#include <QDebug>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qcryptographichash.h>
#include <QtCore/qdir.h>
#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>
#include <private/qthread_p.h>

#if QT_CONFIG(qml_network)
#include "qqmlnetworkaccessmanagerfactory.h"
#include <QNetworkAccessManager>
#include <QtNetwork/qnetworkconfigmanager.h>
#endif

#include <private/qobject_p.h>
#include <private/qmetaobject_p.h>
#if QT_CONFIG(qml_locale)
#include <private/qqmllocale_p.h>
#endif
#include <private/qqmlbind_p.h>
#include <private/qqmlconnections_p.h>
#if QT_CONFIG(qml_animation)
#include <private/qqmltimer_p.h>
#endif
#if QT_CONFIG(qml_list_model)
#include <private/qqmllistmodel_p.h>
#endif
#include <private/qqmlplatform_p.h>
#include <private/qquickpackage_p.h>
#if QT_CONFIG(qml_delegate_model)
#include <private/qqmldelegatemodel_p.h>
#endif
#include <private/qqmlobjectmodel_p.h>
#if QT_CONFIG(qml_worker_script)
#include <private/qquickworkerscript_p.h>
#endif
#include <private/qqmlinstantiator_p.h>
#include <private/qqmlloggingcategory_p.h>

#ifdef Q_OS_WIN // for %APPDATA%
#  include <qt_windows.h>
#  ifndef Q_OS_WINRT
#    include <shlobj.h>
#  endif
#  include <qlibrary.h>
#  ifndef CSIDL_APPDATA
#    define CSIDL_APPDATA           0x001a  // <username>\Application Data
#  endif
#endif // Q_OS_WIN

Q_DECLARE_METATYPE(QQmlProperty)

QT_BEGIN_NAMESPACE

void qmlRegisterBaseTypes(const char *uri, int versionMajor, int versionMinor)
{
    QQmlEnginePrivate::registerBaseTypes(uri, versionMajor, versionMinor);
    QQmlEnginePrivate::registerQtQuick2Types(uri, versionMajor, versionMinor);
    QQmlValueTypeFactory::registerValueTypes(uri, versionMajor, versionMinor);
}

// Declared in qqml.h
int qmlRegisterUncreatableMetaObject(const QMetaObject &staticMetaObject,
                                     const char *uri, int versionMajor,
                                     int versionMinor, const char *qmlName,
                                     const QString& reason)
{
    QQmlPrivate::RegisterType type = {
        0,

        0,
        0,
        0,
        nullptr,
        reason,

        uri, versionMajor, versionMinor, qmlName, &staticMetaObject,

        QQmlAttachedPropertiesFunc(),
        nullptr,

        0,
        0,
        0,

        nullptr, nullptr,

        nullptr,
        0
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

/*!
  \qmltype QtObject
    \instantiates QObject
  \inqmlmodule QtQml
  \ingroup qml-utility-elements
  \brief A basic QML type.

  The QtObject type is a non-visual element which contains only the
  objectName property.

  It can be useful to create a QtObject if you need an extremely
  lightweight type to enclose a set of custom properties:

  \snippet qml/qtobject.qml 0

  It can also be useful for C++ integration, as it is just a plain
  QObject. See the QObject documentation for further details.
*/
/*!
  \qmlproperty string QtObject::objectName
  This property holds the QObject::objectName for this specific object instance.

  This allows a C++ application to locate an item within a QML component
  using the QObject::findChild() method. For example, the following C++
  application locates the child \l Rectangle item and dynamically changes its
  \c color value:

    \qml
    // MyRect.qml

    import QtQuick 2.0

    Item {
        width: 200; height: 200

        Rectangle {
            anchors.fill: parent
            color: "red"
            objectName: "myRect"
        }
    }
    \endqml

    \code
    // main.cpp

    QQuickView view;
    view.setSource(QUrl::fromLocalFile("MyRect.qml"));
    view.show();

    QQuickItem *item = view.rootObject()->findChild<QQuickItem*>("myRect");
    if (item)
        item->setProperty("color", QColor(Qt::yellow));
    \endcode
*/

bool QQmlEnginePrivate::qml_debugging_enabled = false;
bool QQmlEnginePrivate::s_designerMode = false;

// these types are part of the QML language
void QQmlEnginePrivate::registerBaseTypes(const char *uri, int versionMajor, int versionMinor)
{
    qmlRegisterType<QQmlComponent>(uri,versionMajor,versionMinor,"Component");
    qmlRegisterType<QObject>(uri,versionMajor,versionMinor,"QtObject");
    qmlRegisterType<QQmlBind>(uri, versionMajor, versionMinor,"Binding");
    qmlRegisterType<QQmlBind,8>(uri, versionMajor, (versionMinor < 8 ? 8 : versionMinor), "Binding"); //Only available in >=2.8
    qmlRegisterCustomType<QQmlConnections>(uri, versionMajor, 0, "Connections", new QQmlConnectionsParser);
    if (!strcmp(uri, "QtQuick"))
        qmlRegisterCustomType<QQmlConnections,1>(uri, versionMajor, 7, "Connections", new QQmlConnectionsParser); //Only available in QtQuick >=2.7
    else
        qmlRegisterCustomType<QQmlConnections,1>(uri, versionMajor, 3, "Connections", new QQmlConnectionsParser); //Only available in QtQml >=2.3
#if QT_CONFIG(qml_animation)
    qmlRegisterType<QQmlTimer>(uri, versionMajor, versionMinor,"Timer");
#endif
    qmlRegisterType<QQmlInstantiator>(uri, versionMajor, (versionMinor < 1 ? 1 : versionMinor), "Instantiator"); //Only available in >=2.1
    qmlRegisterType<QQmlInstanceModel>();

    qmlRegisterType<QQmlLoggingCategory>(uri, versionMajor, 8, "LoggingCategory"); //Only available in >=2.8
    qmlRegisterType<QQmlLoggingCategory,1>(uri, versionMajor, 12, "LoggingCategory"); //Only available in >=2.12
}


// These QtQuick types' implementation resides in the QtQml module
void QQmlEnginePrivate::registerQtQuick2Types(const char *uri, int versionMajor, int versionMinor)
{
#if QT_CONFIG(qml_list_model)
    qmlRegisterType<QQmlListElement>(uri, versionMajor, versionMinor, "ListElement"); // Now in QtQml.Models, here for compatibility
    qmlRegisterCustomType<QQmlListModel>(uri, versionMajor, versionMinor, "ListModel", new QQmlListModelParser); // Now in QtQml.Models, here for compatibility
#endif
#if QT_CONFIG(qml_worker_script)
    qmlRegisterType<QQuickWorkerScript>(uri, versionMajor, versionMinor, "WorkerScript");
#endif
    qmlRegisterType<QQuickPackage>(uri, versionMajor, versionMinor, "Package");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#if QT_CONFIG(qml_delegate_model)
    qmlRegisterType<QQmlDelegateModel>(uri, versionMajor, versionMinor, "VisualDataModel");
    qmlRegisterType<QQmlDelegateModelGroup>(uri, versionMajor, versionMinor, "VisualDataGroup");
#endif
    qmlRegisterType<QQmlObjectModel>(uri, versionMajor, versionMinor, "VisualItemModel");
#endif // < Qt 6
}

void QQmlEnginePrivate::defineQtQuick2Module()
{
    // register the base types into the QtQuick namespace
    registerBaseTypes("QtQuick",2,0);

    // register the QtQuick2 types which are implemented in the QtQml module.
    registerQtQuick2Types("QtQuick",2,0);
#if QT_CONFIG(qml_locale)
    qmlRegisterUncreatableType<QQmlLocale>("QtQuick", 2, 0, "Locale", QQmlEngine::tr("Locale cannot be instantiated.  Use Qt.locale()"));
#endif

    // Auto-increment the import to stay in sync with ALL future QtQuick minor versions from 5.11 onward
    qmlRegisterModule("QtQuick", 2, QT_VERSION_MINOR);
}

bool QQmlEnginePrivate::designerMode()
{
    return s_designerMode;
}

void QQmlEnginePrivate::activateDesignerMode()
{
    s_designerMode = true;
}


/*!
    \class QQmlImageProviderBase
    \brief The QQmlImageProviderBase class is used to register image providers in the QML engine.
    \inmodule QtQml

    Image providers must be registered with the QML engine.  The only information the QML
    engine knows about image providers is the type of image data they provide.  To use an
    image provider to acquire image data, you must cast the QQmlImageProviderBase pointer
    to a QQuickImageProvider pointer.

    \sa QQuickImageProvider, QQuickTextureFactory
*/

/*!
    \enum QQmlImageProviderBase::ImageType

    Defines the type of image supported by this image provider.

    \value Image The Image Provider provides QImage images.
        The QQuickImageProvider::requestImage() method will be called for all image requests.
    \value Pixmap The Image Provider provides QPixmap images.
        The QQuickImageProvider::requestPixmap() method will be called for all image requests.
    \value Texture The Image Provider provides QSGTextureProvider based images.
        The QQuickImageProvider::requestTexture() method will be called for all image requests.
    \value ImageResponse The Image provider provides QQuickTextureFactory based images.
        Should only be used in QQuickAsyncImageProvider or its subclasses.
        The QQuickAsyncImageProvider::requestImageResponse() method will be called for all image requests.
        Since Qt 5.6
    \omitvalue Invalid
*/

/*!
    \enum QQmlImageProviderBase::Flag

    Defines specific requirements or features of this image provider.

    \value ForceAsynchronousImageLoading Ensures that image requests to the provider are
        run in a separate thread, which allows the provider to spend as much time as needed
        on producing the image without blocking the main thread.
*/

/*!
    \fn QQmlImageProviderBase::imageType() const

    Implement this method to return the image type supported by this image provider.
*/

/*!
    \fn QQmlImageProviderBase::flags() const

    Implement this to return the properties of this image provider.
*/

/*! \internal */
QQmlImageProviderBase::QQmlImageProviderBase()
{
}

/*! \internal */
QQmlImageProviderBase::~QQmlImageProviderBase()
{
}


/*!
\qmltype Qt
\inqmlmodule QtQml
\instantiates QQmlEnginePrivate
\ingroup qml-utility-elements
\keyword QmlGlobalQtObject
\brief Provides a global object with useful enums and functions from Qt.

The \c Qt object is a global object with utility functions, properties and enums.

It is not instantiable; to use it, call the members of the global \c Qt object directly.
For example:

\qml
import QtQuick 2.0

Text {
    color: Qt.rgba(1, 0, 0, 1)
    text: Qt.md5("hello, world")
}
\endqml


\section1 Enums

The Qt object contains the enums available in the \l [QtCore]{Qt}{Qt Namespace}. For example, you can access
the \l Qt::LeftButton and \l Qt::RightButton enumeration values as \c Qt.LeftButton and \c Qt.RightButton.


\section1 Types

The Qt object also contains helper functions for creating objects of specific
data types. This is primarily useful when setting the properties of an item
when the property has one of the following types:
\list
\li \c rect - use \l{Qt::rect()}{Qt.rect()}
\li \c point - use \l{Qt::point()}{Qt.point()}
\li \c size - use \l{Qt::size()}{Qt.size()}
\endlist

If the \c QtQuick module has been imported, the following helper functions for
creating objects of specific data types are also available for clients to use:
\list
\li \c color - use \l{Qt::rgba()}{Qt.rgba()}, \l{Qt::hsla()}{Qt.hsla()}, \l{Qt::darker()}{Qt.darker()}, \l{Qt::lighter()}{Qt.lighter()} or \l{Qt::tint()}{Qt.tint()}
\li \c font - use \l{Qt::font()}{Qt.font()}
\li \c vector2d - use \l{Qt::vector2d()}{Qt.vector2d()}
\li \c vector3d - use \l{Qt::vector3d()}{Qt.vector3d()}
\li \c vector4d - use \l{Qt::vector4d()}{Qt.vector4d()}
\li \c quaternion - use \l{Qt::quaternion()}{Qt.quaternion()}
\li \c matrix4x4 - use \l{Qt::matrix4x4()}{Qt.matrix4x4()}
\endlist

There are also string based constructors for these types. See \l{qtqml-typesystem-basictypes.html}{QML Basic Types} for more information.

\section1 Date/Time Formatters

The Qt object contains several functions for formatting QDateTime, QDate and QTime values.

\list
    \li \l{Qt::formatDateTime}{string Qt.formatDateTime(datetime date, variant format)}
    \li \l{Qt::formatDate}{string Qt.formatDate(datetime date, variant format)}
    \li \l{Qt::formatTime}{string Qt.formatTime(datetime date, variant format)}
\endlist

The format specification is described at \l{Qt::formatDateTime}{Qt.formatDateTime}.


\section1 Dynamic Object Creation
The following functions on the global object allow you to dynamically create QML
items from files or strings. See \l{Dynamic QML Object Creation from JavaScript} for an overview
of their use.

\list
    \li \l{Qt::createComponent()}{object Qt.createComponent(url)}
    \li \l{Qt::createQmlObject()}{object Qt.createQmlObject(string qml, object parent, string filepath)}
\endlist


\section1 Other Functions

The following functions are also on the Qt object.

\list
    \li \l{Qt::quit()}{Qt.quit()}
    \li \l{Qt::md5()}{Qt.md5(string)}
    \li \l{Qt::btoa()}{string Qt.btoa(string)}
    \li \l{Qt::atob()}{string Qt.atob(string)}
    \li \l{Qt::binding()}{object Qt.binding(function)}
    \li \l{Qt::locale()}{object Qt.locale()}
    \li \l{Qt::resolvedUrl()}{string Qt.resolvedUrl(string)}
    \li \l{Qt::openUrlExternally()}{Qt.openUrlExternally(string)}
    \li \l{Qt::fontFamilies()}{list<string> Qt.fontFamilies()}
\endlist
*/

/*!
    \qmlproperty object Qt::platform
    \since 5.1

    The \c platform object provides info about the underlying platform.

    Its properties are:

    \table
    \row
    \li \c platform.os
    \li

    This read-only property contains the name of the operating system.

    Possible values are:

    \list
        \li \c "android" - Android
        \li \c "ios" - iOS
        \li \c "tvos" - tvOS
        \li \c "linux" - Linux
        \li \c "osx" - \macos
        \li \c "qnx" - QNX (since Qt 5.9.3)
        \li \c "unix" - Other Unix-based OS
        \li \c "windows" - Windows
        \li \c "winrt" - WinRT / UWP
    \endlist

    \row
    \li \c platform.pluginName
    \li This is the name of the platform set on the QGuiApplication instance
        as returned by \l QGuiApplication::platformName()

    \endtable
*/

/*!
    \qmlproperty object Qt::application
    \since 5.1

    The \c application object provides access to global application state
    properties shared by many QML components.

    Its properties are:

    \table
    \row
    \li \c application.active
    \li
    Deprecated, use Qt.application.state == Qt.ApplicationActive instead.

    \row
    \li \c application.state
    \li
    This read-only property indicates the current state of the application.

    Possible values are:

    \list
    \li Qt.ApplicationActive - The application is the top-most and focused application, and the
                               user is able to interact with the application.
    \li Qt.ApplicationInactive - The application is visible or partially visible, but not selected
                                 to be in front, the user cannot interact with the application.
                                 On desktop platforms, this typically means that the user activated
                                 another application. On mobile platforms, it is more common to
                                 enter this state when the OS is interrupting the user with for
                                 example incoming calls, SMS-messages or dialogs. This is usually a
                                 transient state during which the application is paused. The user
                                 may return focus to your application, but most of the time it will
                                 be the first indication that the application is going to be suspended.
                                 While in this state, consider pausing or stopping any activity that
                                 should not continue when the user cannot interact with your
                                 application, such as a video, a game, animations, or sensors.
                                 You should also avoid performing CPU-intensive tasks which might
                                 slow down the application in front.
    \li Qt.ApplicationSuspended - The application is suspended and not visible to the user. On
                                  mobile platforms, the application typically enters this state when
                                  the user returns to the home screen or switches to another
                                  application. While in this state, the application should ensure
                                  that the user perceives it as always alive and does not lose his
                                  progress, saving any persistent data. The application should cease
                                  all activities and be prepared for code execution to stop. While
                                  suspended, the application can be killed at any time without
                                  further warnings (for example when low memory forces the OS to purge
                                  suspended applications).
    \li Qt.ApplicationHidden - The application is hidden and runs in the background. This is the
                               normal state for applications that need to do background processing,
                               like playing music, while the user interacts with other applications.
                               The application should free up all graphical resources when entering
                               this state. A Qt Quick application should not usually handle this state
                               at the QML level. Instead, you should unload the entire UI and reload
                               the QML files whenever the application becomes active again.
    \endlist

    \row
    \li \c application.layoutDirection
    \li
    This read-only property can be used to query the default layout direction of the
    application. On system start-up, the default layout direction depends on the
    application's language. The property has a value of \c Qt.RightToLeft in locales
    where text and graphic elements are read from right to left, and \c Qt.LeftToRight
    where the reading direction flows from left to right. You can bind to this
    property to customize your application layouts to support both layout directions.

    Possible values are:

    \list
    \li Qt.LeftToRight - Text and graphics elements should be positioned
                        from left to right.
    \li Qt.RightToLeft - Text and graphics elements should be positioned
                        from right to left.
    \endlist
    \row
    \li \c application.font
    \li This read-only property holds the default application font as
        returned by \l QGuiApplication::font().
    \row
    \li \c application.arguments
    \li This is a string list of the arguments the executable was invoked with.
    \row
    \li \c application.name
    \li This is the application name set on the QCoreApplication instance. This property can be written
    to in order to set the application name.
    \row
    \li \c application.displayName (since Qt 5.9)
    \li This is the application display name set on the QGuiApplication instance. This property can be written
    to in order to set the application display name.
    \row
    \li \c application.version
    \li This is the application version set on the QCoreApplication instance. This property can be written
    to in order to set the application version.
    \row
    \li \c application.organization
    \li This is the organization name set on the QCoreApplication instance. This property can be written
    to in order to set the organization name.
    \row
    \li \c application.domain
    \li This is the organization domain set on the QCoreApplication instance. This property can be written
    to in order to set the organization domain.

    \row
    \li \c application.supportsMultipleWindows
    \li This read-only property can be used to determine whether or not the
        platform supports multiple windows. Some embedded platforms do not support
        multiple windows, for example.

    \row
    \li \c application.screens
    \li An array containing the descriptions of all connected screens. The
    elements of the array are objects with the same properties as the
    \l{Screen} attached object. In practice the array corresponds to the screen
    list returned by QGuiApplication::screens(). In addition to examining
    properties like name, width, height, etc., the array elements can also be
    assigned to the screen property of Window items, thus serving as an
    alternative to the C++ side's QWindow::setScreen(). This property has been
    added in Qt 5.9.

    \endtable

    The object also has one signal, aboutToQuit(), which is the same as \l QCoreApplication::aboutToQuit().

    The following example uses the \c application object to indicate
    whether the application is currently active:

    \snippet qml/application.qml document

    Note that when using QML without a QGuiApplication, the following properties will be undefined:
    \list
    \li application.active
    \li application.state
    \li application.layoutDirection
    \li application.font
    \endlist

    \sa Screen, Window, {QtQuick.Window::Window::screen}{Window.screen}
*/

/*!
    \qmlproperty object Qt::inputMethod
    \since 5.0

    The \c inputMethod object allows access to application's QInputMethod object
    and all its properties and slots. See the QInputMethod documentation for
    further details.
*/

/*!
    \qmlproperty object Qt::styleHints
    \since 5.5

    The \c styleHints object provides platform-specific style hints and settings.
    See the QStyleHints documentation for further details.

    \note The \c styleHints object is only available when using the Qt Quick module.

    The following example uses the \c styleHints object to determine whether an
    item should gain focus on mouse press or touch release:
    \code
    import QtQuick 2.4

    MouseArea {
        id: button

        onPressed: {
            if (!Qt.styleHints.setFocusOnTouchRelease)
                button.forceActiveFocus()
        }
        onReleased: {
            if (Qt.styleHints.setFocusOnTouchRelease)
                button.forceActiveFocus()
        }
    }
    \endcode
*/

/*!
\qmlmethod object Qt::include(string url, jsobject callback)

Includes another JavaScript file. This method can only be used from within JavaScript files,
and not regular QML files.

This imports all functions from \a url into the current script's namespace.

Qt.include() returns an object that describes the status of the operation.  The object has
a single property, \c {status}, that is set to one of the following values:

\table
\header \li Symbol \li Value \li Description
\row \li result.OK \li 0 \li The include completed successfully.
\row \li result.LOADING \li 1 \li Data is being loaded from the network.
\row \li result.NETWORK_ERROR \li 2 \li A network error occurred while fetching the url.
\row \li result.EXCEPTION \li 3 \li A JavaScript exception occurred while executing the included code.
An additional \c exception property will be set in this case.
\endtable

The \c status property will be updated as the operation progresses.

If provided, \a callback is invoked when the operation completes.  The callback is passed
the same object as is returned from the Qt.include() call.
*/
// Qt.include() is implemented in qv4include.cpp

QQmlEnginePrivate::QQmlEnginePrivate(QQmlEngine *e)
: propertyCapture(nullptr), rootContext(nullptr),
#if QT_CONFIG(qml_debug)
  profiler(nullptr),
#endif
  outputWarningsToMsgLog(true),
  cleanup(nullptr), erroredBindings(nullptr), inProgressCreations(0),
#if QT_CONFIG(qml_worker_script)
  workerScriptEngine(nullptr),
#endif
  activeObjectCreator(nullptr),
#if QT_CONFIG(qml_network)
  networkAccessManager(nullptr), networkAccessManagerFactory(nullptr),
#endif
  urlInterceptor(nullptr), scarceResourcesRefCount(0), importDatabase(e), typeLoader(e),
  uniqueId(1), incubatorCount(0), incubationController(nullptr)
{
}

QQmlEnginePrivate::~QQmlEnginePrivate()
{
    if (inProgressCreations)
        qWarning() << QQmlEngine::tr("There are still \"%1\" items in the process of being created at engine destruction.").arg(inProgressCreations);

    while (cleanup) {
        QQmlCleanup *c = cleanup;
        cleanup = c->next;
        if (cleanup) cleanup->prev = &cleanup;
        c->next = nullptr;
        c->prev = nullptr;
        c->clear();
    }

    doDeleteInEngineThread();

    if (incubationController) incubationController->d = nullptr;
    incubationController = nullptr;

    QQmlMetaType::freeUnusedTypesAndCaches();

    for (auto iter = m_compositeTypes.cbegin(), end = m_compositeTypes.cend(); iter != end; ++iter) {
        iter.value()->isRegisteredWithEngine = false;

        // since unregisterInternalCompositeType() will not be called in this
        // case, we have to clean up the type registration manually
        QMetaType::unregisterType(iter.value()->metaTypeId);
        QMetaType::unregisterType(iter.value()->listMetaTypeId);
    }
#if QT_CONFIG(qml_debug)
    delete profiler;
#endif
}

void QQmlPrivate::qdeclarativeelement_destructor(QObject *o)
{
    if (QQmlData *d = QQmlData::get(o)) {
        if (d->ownContext) {
            for (QQmlContextData *lc = d->ownContext->linkedContext; lc; lc = lc->linkedContext) {
                lc->invalidate();
                if (lc->contextObject == o)
                    lc->contextObject = nullptr;
            }
            d->ownContext->invalidate();
            if (d->ownContext->contextObject == o)
                d->ownContext->contextObject = nullptr;
            d->ownContext = nullptr;
            d->context = nullptr;
        }

        if (d->outerContext && d->outerContext->contextObject == o)
            d->outerContext->contextObject = nullptr;

        // Mark this object as in the process of deletion to
        // prevent it resolving in bindings
        QQmlData::markAsDeleted(o);

        // Disconnect the notifiers now - during object destruction this would be too late, since
        // the disconnect call wouldn't be able to call disconnectNotify(), as it isn't possible to
        // get the metaobject anymore.
        d->disconnectNotifiers();
    }
}

QQmlData::QQmlData()
    : ownedByQml1(false), ownMemory(true), indestructible(true), explicitIndestructibleSet(false),
      hasTaintedV4Object(false), isQueuedForDeletion(false), rootObjectInCreation(false),
      hasInterceptorMetaObject(false), hasVMEMetaObject(false), parentFrozen(false),
      bindingBitsArraySize(InlineBindingArraySize), notifyList(nullptr),
      bindings(nullptr), signalHandlers(nullptr), nextContextObject(nullptr), prevContextObject(nullptr),
      lineNumber(0), columnNumber(0), jsEngineId(0),
      propertyCache(nullptr), guards(nullptr), extendedData(nullptr)
{
    memset(bindingBitsValue, 0, sizeof(bindingBitsValue));
    init();
}

QQmlData::~QQmlData()
{
}

void QQmlData::destroyed(QAbstractDeclarativeData *d, QObject *o)
{
    QQmlData *ddata = static_cast<QQmlData *>(d);
    if (ddata->ownedByQml1)
        return;
    ddata->destroyed(o);
}

void QQmlData::parentChanged(QAbstractDeclarativeData *d, QObject *o, QObject *p)
{
    QQmlData *ddata = static_cast<QQmlData *>(d);
    if (ddata->ownedByQml1)
        return;
    ddata->parentChanged(o, p);
}

class QQmlThreadNotifierProxyObject : public QObject
{
public:
    QPointer<QObject> target;

    int qt_metacall(QMetaObject::Call, int methodIndex, void **a) override {
        if (!target)
            return -1;

        QMetaMethod method = target->metaObject()->method(methodIndex);
        Q_ASSERT(method.methodType() == QMetaMethod::Signal);
        int signalIndex = QMetaObjectPrivate::signalIndex(method);
        QQmlData *ddata = QQmlData::get(target, false);
        QQmlNotifierEndpoint *ep = ddata->notify(signalIndex);
        if (ep) QQmlNotifier::emitNotify(ep, a);

        delete this;

        return -1;
    }
};

void QQmlData::signalEmitted(QAbstractDeclarativeData *, QObject *object, int index, void **a)
{
    QQmlData *ddata = QQmlData::get(object, false);
    if (!ddata) return; // Probably being deleted
    if (ddata->ownedByQml1) return;

    // In general, QML only supports QObject's that live on the same thread as the QQmlEngine
    // that they're exposed to.  However, to make writing "worker objects" that calculate data
    // in a separate thread easier, QML allows a QObject that lives in the same thread as the
    // QQmlEngine to emit signals from a different thread.  These signals are then automatically
    // marshalled back onto the QObject's thread and handled by QML from there.  This is tested
    // by the qqmlecmascript::threadSignal() autotest.
    if (ddata->notifyList &&
        QThread::currentThreadId() != QObjectPrivate::get(object)->threadData->threadId.load()) {

        if (!QObjectPrivate::get(object)->threadData->thread)
            return;

        QMetaMethod m = QMetaObjectPrivate::signal(object->metaObject(), index);
        QList<QByteArray> parameterTypes = m.parameterTypes();

        int *types = (int *)malloc((parameterTypes.count() + 1) * sizeof(int));
        void **args = (void **) malloc((parameterTypes.count() + 1) *sizeof(void *));

        types[0] = 0; // return type
        args[0] = nullptr; // return value

        for (int ii = 0; ii < parameterTypes.count(); ++ii) {
            const QByteArray &typeName = parameterTypes.at(ii);
            if (typeName.endsWith('*'))
                types[ii + 1] = QMetaType::VoidStar;
            else
                types[ii + 1] = QMetaType::type(typeName);

            if (!types[ii + 1]) {
                qWarning("QObject::connect: Cannot queue arguments of type '%s'\n"
                         "(Make sure '%s' is registered using qRegisterMetaType().)",
                         typeName.constData(), typeName.constData());
                free(types);
                free(args);
                return;
            }

            args[ii + 1] = QMetaType::create(types[ii + 1], a[ii + 1]);
        }

        QMetaCallEvent *ev = new QMetaCallEvent(m.methodIndex(), 0, nullptr, object, index,
                                                parameterTypes.count() + 1, types, args);

        QQmlThreadNotifierProxyObject *mpo = new QQmlThreadNotifierProxyObject;
        mpo->target = object;
        mpo->moveToThread(QObjectPrivate::get(object)->threadData->thread);
        QCoreApplication::postEvent(mpo, ev);

    } else {
        QQmlNotifierEndpoint *ep = ddata->notify(index);
        if (ep) QQmlNotifier::emitNotify(ep, a);
    }
}

int QQmlData::receivers(QAbstractDeclarativeData *d, const QObject *, int index)
{
    QQmlData *ddata = static_cast<QQmlData *>(d);
    if (ddata->ownedByQml1)
        return 0;
    return ddata->endpointCount(index);
}

bool QQmlData::isSignalConnected(QAbstractDeclarativeData *d, const QObject *, int index)
{
    QQmlData *ddata = static_cast<QQmlData *>(d);
    if (ddata->ownedByQml1)
        return false;
    return ddata->signalHasEndpoint(index);
}

int QQmlData::endpointCount(int index)
{
    int count = 0;
    QQmlNotifierEndpoint *ep = notify(index);
    if (!ep)
        return count;
    ++count;
    while (ep->next) {
        ++count;
        ep = ep->next;
    }
    return count;
}

void QQmlData::markAsDeleted(QObject *o)
{
    QQmlData::setQueuedForDeletion(o);

    QObjectPrivate *p = QObjectPrivate::get(o);
    for (QList<QObject *>::const_iterator it = p->children.constBegin(), end = p->children.constEnd(); it != end; ++it) {
        QQmlData::markAsDeleted(*it);
    }
}

void QQmlData::setQueuedForDeletion(QObject *object)
{
    if (object) {
        if (QQmlData *ddata = QQmlData::get(object)) {
            if (ddata->ownContext) {
                Q_ASSERT(ddata->ownContext == ddata->context);
                ddata->context->emitDestruction();
                if (ddata->ownContext->contextObject == object)
                    ddata->ownContext->contextObject = nullptr;
                ddata->ownContext = nullptr;
                ddata->context = nullptr;
            }
            ddata->isQueuedForDeletion = true;
        }
    }
}

void QQmlData::flushPendingBindingImpl(QQmlPropertyIndex index)
{
    clearPendingBindingBit(index.coreIndex());

    // Find the binding
    QQmlAbstractBinding *b = bindings;
    while (b && (b->targetPropertyIndex().coreIndex() != index.coreIndex() ||
                 b->targetPropertyIndex().hasValueTypeIndex()))
        b = b->nextBinding();

    if (b && b->targetPropertyIndex().coreIndex() == index.coreIndex() &&
            !b->targetPropertyIndex().hasValueTypeIndex())
        b->setEnabled(true, QQmlPropertyData::BypassInterceptor |
                            QQmlPropertyData::DontRemoveBinding);
}

QQmlData::DeferredData::DeferredData()
{
}

QQmlData::DeferredData::~DeferredData()
{
}

bool QQmlEnginePrivate::baseModulesUninitialized = true;
void QQmlEnginePrivate::init()
{
    Q_Q(QQmlEngine);

    if (baseModulesUninitialized) {
        qmlRegisterType<QQmlComponent>("QML", 1, 0, "Component"); // required for the Compiler.
        registerBaseTypes("QtQml", 2, 0); // import which provides language building blocks.
#if QT_CONFIG(qml_locale)
        qmlRegisterUncreatableType<QQmlLocale>("QtQml", 2, 2, "Locale", QQmlEngine::tr("Locale cannot be instantiated.  Use Qt.locale()"));
#endif

        // Auto-increment the import to stay in sync with ALL future QtQml minor versions from 5.11 onward
        qmlRegisterModule("QtQml", 2, QT_VERSION_MINOR);

        QQmlData::init();
        baseModulesUninitialized = false;
    }

    qRegisterMetaType<QVariant>();
    qRegisterMetaType<QQmlScriptString>();
    qRegisterMetaType<QJSValue>();
    qRegisterMetaType<QQmlComponent::Status>();
    qRegisterMetaType<QList<QObject*> >();
    qRegisterMetaType<QList<int> >();
    qRegisterMetaType<QQmlV4Handle>();
    qRegisterMetaType<QQmlBinding*>();

    v8engine()->setEngine(q);

    rootContext = new QQmlContext(q,true);
}

#if QT_CONFIG(qml_worker_script)
QQuickWorkerScriptEngine *QQmlEnginePrivate::getWorkerScriptEngine()
{
    Q_Q(QQmlEngine);
    if (!workerScriptEngine)
        workerScriptEngine = new QQuickWorkerScriptEngine(q);
    return workerScriptEngine;
}
#endif

/*!
  \class QQmlEngine
  \since 5.0
  \inmodule QtQml
  \brief The QQmlEngine class provides an environment for instantiating QML components.

  Each QML component is instantiated in a QQmlContext.
  QQmlContext's are essential for passing data to QML
  components.  In QML, contexts are arranged hierarchically and this
  hierarchy is managed by the QQmlEngine.

  Prior to creating any QML components, an application must have
  created a QQmlEngine to gain access to a QML context.  The
  following example shows how to create a simple Text item.

  \code
  QQmlEngine engine;
  QQmlComponent component(&engine);
  component.setData("import QtQuick 2.0\nText { text: \"Hello world!\" }", QUrl());
  QQuickItem *item = qobject_cast<QQuickItem *>(component.create());

  //add item to view, etc
  ...
  \endcode

  In this case, the Text item will be created in the engine's
  \l {QQmlEngine::rootContext()}{root context}.

  \sa QQmlComponent, QQmlContext, {QML Global Object}
*/

/*!
  Create a new QQmlEngine with the given \a parent.
*/
QQmlEngine::QQmlEngine(QObject *parent)
: QJSEngine(*new QQmlEnginePrivate(this), parent)
{
    Q_D(QQmlEngine);
    d->init();
    QJSEnginePrivate::addToDebugServer(this);
}

/*!
* \internal
*/
QQmlEngine::QQmlEngine(QQmlEnginePrivate &dd, QObject *parent)
: QJSEngine(dd, parent)
{
    Q_D(QQmlEngine);
    d->init();
}

/*!
  Destroys the QQmlEngine.

  Any QQmlContext's created on this engine will be
  invalidated, but not destroyed (unless they are parented to the
  QQmlEngine object).

  See QJSEngine docs for details on cleaning up the JS engine.
*/
QQmlEngine::~QQmlEngine()
{
    Q_D(QQmlEngine);
    QJSEnginePrivate::removeFromDebugServer(this);

    // Emit onDestruction signals for the root context before
    // we destroy the contexts, engine, Singleton Types etc. that
    // may be required to handle the destruction signal.
    QQmlContextData::get(rootContext())->emitDestruction();

    // clean up all singleton type instances which we own.
    // we do this here and not in the private dtor since otherwise a crash can
    // occur (if we are the QObject parent of the QObject singleton instance)
    // XXX TODO: performance -- store list of singleton types separately?
    QList<QQmlType> singletonTypes = QQmlMetaType::qmlSingletonTypes();
    for (const QQmlType &currType : singletonTypes)
        currType.singletonInstanceInfo()->destroy(this);

    delete d->rootContext;
    d->rootContext = nullptr;

    d->typeLoader.invalidate();
}

/*! \fn void QQmlEngine::quit()
    This signal is emitted when the QML loaded by the engine would like to quit.

    \sa exit()
 */

/*! \fn void QQmlEngine::exit(int retCode)
    This signal is emitted when the QML loaded by the engine would like to exit
    from the event loop with the specified return code \a retCode.

    \since 5.8
    \sa quit()
 */


/*! \fn void QQmlEngine::warnings(const QList<QQmlError> &warnings)
    This signal is emitted when \a warnings messages are generated by QML.
 */

/*!
  Clears the engine's internal component cache.

  This function causes the property metadata of all components previously
  loaded by the engine to be destroyed.  All previously loaded components and
  the property bindings for all extant objects created from those components will
  cease to function.

  This function returns the engine to a state where it does not contain any loaded
  component data.  This may be useful in order to reload a smaller subset of the
  previous component set, or to load a new version of a previously loaded component.

  Once the component cache has been cleared, components must be loaded before
  any new objects can be created.

  \sa trimComponentCache()
 */
void QQmlEngine::clearComponentCache()
{
    Q_D(QQmlEngine);
    d->typeLoader.lock();
    d->typeLoader.clearCache();
    d->typeLoader.unlock();
}

/*!
  Trims the engine's internal component cache.

  This function causes the property metadata of any loaded components which are
  not currently in use to be destroyed.

  A component is considered to be in use if there are any extant instances of
  the component itself, any instances of other components that use the component,
  or any objects instantiated by any of those components.

  \sa clearComponentCache()
 */
void QQmlEngine::trimComponentCache()
{
    Q_D(QQmlEngine);
    d->typeLoader.trimCache();
}

/*!
  Returns the engine's root context.

  The root context is automatically created by the QQmlEngine.
  Data that should be available to all QML component instances
  instantiated by the engine should be put in the root context.

  Additional data that should only be available to a subset of
  component instances should be added to sub-contexts parented to the
  root context.
*/
QQmlContext *QQmlEngine::rootContext() const
{
    Q_D(const QQmlEngine);
    return d->rootContext;
}

/*!
  \internal
  This API is private for 5.1

  Sets the \a urlInterceptor to be used when resolving URLs in QML.
  This also applies to URLs used for loading script files and QML types.
  This should not be modifed while the engine is loading files, or URL
  selection may be inconsistent.
*/
void QQmlEngine::setUrlInterceptor(QQmlAbstractUrlInterceptor *urlInterceptor)
{
    Q_D(QQmlEngine);
    d->urlInterceptor = urlInterceptor;
}

/*!
  \internal
  This API is private for 5.1

  Returns the current QQmlAbstractUrlInterceptor. It must not be modified outside
  the GUI thread.
*/
QQmlAbstractUrlInterceptor *QQmlEngine::urlInterceptor() const
{
    Q_D(const QQmlEngine);
    return d->urlInterceptor;
}

void QQmlEnginePrivate::registerFinalizeCallback(QObject *obj, int index)
{
    if (activeObjectCreator) {
        activeObjectCreator->finalizeCallbacks()->append(qMakePair(QPointer<QObject>(obj), index));
    } else {
        void *args[] = { nullptr };
        QMetaObject::metacall(obj, QMetaObject::InvokeMetaMethod, index, args);
    }
}

#if QT_CONFIG(qml_network)
/*!
  Sets the \a factory to use for creating QNetworkAccessManager(s).

  QNetworkAccessManager is used for all network access by QML.  By
  implementing a factory it is possible to create custom
  QNetworkAccessManager with specialized caching, proxy and cookie
  support.

  The factory must be set before executing the engine.

  \note QQmlEngine does not take ownership of the factory.
*/
void QQmlEngine::setNetworkAccessManagerFactory(QQmlNetworkAccessManagerFactory *factory)
{
    Q_D(QQmlEngine);
    QMutexLocker locker(&d->networkAccessManagerMutex);
    d->networkAccessManagerFactory = factory;
}

/*!
  Returns the current QQmlNetworkAccessManagerFactory.

  \sa setNetworkAccessManagerFactory()
*/
QQmlNetworkAccessManagerFactory *QQmlEngine::networkAccessManagerFactory() const
{
    Q_D(const QQmlEngine);
    return d->networkAccessManagerFactory;
}

QNetworkAccessManager *QQmlEnginePrivate::createNetworkAccessManager(QObject *parent) const
{
    QMutexLocker locker(&networkAccessManagerMutex);
    QNetworkAccessManager *nam;
    if (networkAccessManagerFactory) {
        nam = networkAccessManagerFactory->create(parent);
    } else {
        nam = new QNetworkAccessManager(parent);
    }

    return nam;
}

QNetworkAccessManager *QQmlEnginePrivate::getNetworkAccessManager() const
{
    Q_Q(const QQmlEngine);
    if (!networkAccessManager)
        networkAccessManager = createNetworkAccessManager(const_cast<QQmlEngine*>(q));
    return networkAccessManager;
}

/*!
  Returns a common QNetworkAccessManager which can be used by any QML
  type instantiated by this engine.

  If a QQmlNetworkAccessManagerFactory has been set and a
  QNetworkAccessManager has not yet been created, the
  QQmlNetworkAccessManagerFactory will be used to create the
  QNetworkAccessManager; otherwise the returned QNetworkAccessManager
  will have no proxy or cache set.

  \sa setNetworkAccessManagerFactory()
*/
QNetworkAccessManager *QQmlEngine::networkAccessManager() const
{
    Q_D(const QQmlEngine);
    return d->getNetworkAccessManager();
}
#endif // qml_network

/*!

  Sets the \a provider to use for images requested via the \e
  image: url scheme, with host \a providerId. The QQmlEngine
  takes ownership of \a provider.

  Image providers enable support for pixmap and threaded image
  requests. See the QQuickImageProvider documentation for details on
  implementing and using image providers.

  All required image providers should be added to the engine before any
  QML sources files are loaded.

  \sa removeImageProvider(), QQuickImageProvider, QQmlImageProviderBase
*/
void QQmlEngine::addImageProvider(const QString &providerId, QQmlImageProviderBase *provider)
{
    Q_D(QQmlEngine);
    QMutexLocker locker(&d->mutex);
    d->imageProviders.insert(providerId.toLower(), QSharedPointer<QQmlImageProviderBase>(provider));
}

/*!
  Returns the image provider set for \a providerId if found; otherwise returns \nullptr.

  \sa QQuickImageProvider
*/
QQmlImageProviderBase *QQmlEngine::imageProvider(const QString &providerId) const
{
    Q_D(const QQmlEngine);
    QMutexLocker locker(&d->mutex);
    return d->imageProviders.value(providerId.toLower()).data();
}

/*!
  Removes the image provider for \a providerId.

  \sa addImageProvider(), QQuickImageProvider
*/
void QQmlEngine::removeImageProvider(const QString &providerId)
{
    Q_D(QQmlEngine);
    QMutexLocker locker(&d->mutex);
    d->imageProviders.take(providerId.toLower());
}

/*!
  Return the base URL for this engine.  The base URL is only used to
  resolve components when a relative URL is passed to the
  QQmlComponent constructor.

  If a base URL has not been explicitly set, this method returns the
  application's current working directory.

  \sa setBaseUrl()
*/
QUrl QQmlEngine::baseUrl() const
{
    Q_D(const QQmlEngine);
    if (d->baseUrl.isEmpty()) {
        return QUrl::fromLocalFile(QDir::currentPath() + QDir::separator());
    } else {
        return d->baseUrl;
    }
}

/*!
  Set the  base URL for this engine to \a url.

  \sa baseUrl()
*/
void QQmlEngine::setBaseUrl(const QUrl &url)
{
    Q_D(QQmlEngine);
    d->baseUrl = url;
}

/*!
  Returns true if warning messages will be output to stderr in addition
  to being emitted by the warnings() signal, otherwise false.

  The default value is true.
*/
bool QQmlEngine::outputWarningsToStandardError() const
{
    Q_D(const QQmlEngine);
    return d->outputWarningsToMsgLog;
}

/*!
  Set whether warning messages will be output to stderr to \a enabled.

  If \a enabled is true, any warning messages generated by QML will be
  output to stderr and emitted by the warnings() signal.  If \a enabled
  is false, only the warnings() signal will be emitted.  This allows
  applications to handle warning output themselves.

  The default value is true.
*/
void QQmlEngine::setOutputWarningsToStandardError(bool enabled)
{
    Q_D(QQmlEngine);
    d->outputWarningsToMsgLog = enabled;
}

/*!
  \fn template<typename T> T QQmlEngine::singletonInstance(int qmlTypeId)

  Returns the instance of a singleton type that was registered under \a qmlTypeId.

  The template argument \e T may be either QJSValue or a pointer to a QObject-derived
  type and depends on how the singleton was registered. If no instance of \e T has been
  created yet, it is created now. If \a qmlTypeId does not represent a valid singleton
  type, either a default constructed QJSValue or a \c nullptr is returned.

  QObject* example:
  \code
  class MySingleton : public QObject {
    Q_OBJECT
    static int typeId;
    // ...
  };

  // Register with QObject* callback
  MySingleton::typeId = qmlRegisterSingletonType<MySingleton>(...);

  // Retrieve as QObject*
  QQmlEngine engine;
  MySingleton* instance = engine.singletonInstance<MySingleton*>(MySingleton::typeId);
  \endcode

  QJSValue example:
  \code
  // Register with QJSValue callback
  int typeId = qmlRegisterSingletonType(...);

  // Retrieve as QJSValue
  QQmlEngine engine;
  QJSValue instance = engine.singletonInstance<QJSValue>(typeId);
  \endcode

  It is recommended to store the QML type id during registration, e.g. as a static member
  in the singleton class. Otherwise, a costly lookup via qmlTypeId() has to be performed
  at run-time.

  \sa qmlRegisterSingletonType(), qmlTypeId()
  \since 5.12
*/
template<>
QJSValue QQmlEngine::singletonInstance<QJSValue>(int qmlTypeId)
{
    QQmlType type = QQmlMetaType::qmlType(qmlTypeId, QQmlMetaType::TypeIdCategory::QmlType);

    if (!type.isValid() || !type.isSingleton())
        return QJSValue();

    QQmlType::SingletonInstanceInfo* info = type.singletonInstanceInfo();
    info->init(this);

    if (QObject* o = info->qobjectApi(this))
        return this->newQObject(o);
    else {
        QJSValue value = info->scriptApi(this);
        if (!value.isUndefined())
            return value;
    }

    return QJSValue();
}

/*!
  Refreshes all binding expressions that use strings marked for translation.

  Call this function after you have installed a new translator with
  QCoreApplication::installTranslator, to ensure that your user-interface
  shows up-to-date translations.

  \note Due to a limitation in the implementation, this function
  refreshes all the engine's bindings, not only those that use strings
  marked for translation.
  This may be optimized in a future release.

  \since 5.10
*/
void QQmlEngine::retranslate()
{
    Q_D(QQmlEngine);
    QQmlContextData *context = QQmlContextData::get(d->rootContext)->childContexts;
    while (context) {
        context->refreshExpressions();
        context = context->nextChild;
    }
}

/*!
  Returns the QQmlContext for the \a object, or 0 if no
  context has been set.

  When the QQmlEngine instantiates a QObject, the context is
  set automatically.

  \sa qmlContext(), qmlEngine()
  */
QQmlContext *QQmlEngine::contextForObject(const QObject *object)
{
    if(!object)
        return nullptr;

    QQmlData *data = QQmlData::get(object);
    if (data && data->outerContext)
        return data->outerContext->asQQmlContext();

    return nullptr;
}

/*!
  Sets the QQmlContext for the \a object to \a context.
  If the \a object already has a context, a warning is
  output, but the context is not changed.

  When the QQmlEngine instantiates a QObject, the context is
  set automatically.
 */
void QQmlEngine::setContextForObject(QObject *object, QQmlContext *context)
{
    if (!object || !context)
        return;

    QQmlData *data = QQmlData::get(object, true);
    if (data->context) {
        qWarning("QQmlEngine::setContextForObject(): Object already has a QQmlContext");
        return;
    }

    QQmlContextData *contextData = QQmlContextData::get(context);
    Q_ASSERT(data->context == nullptr);
    data->context = contextData;
    contextData->addObject(data);
}

/*!
  \enum QQmlEngine::ObjectOwnership

  ObjectOwnership controls whether or not QML automatically destroys the
  QObject when the corresponding JavaScript object is garbage collected by the
  engine. The two ownership options are:

  \value CppOwnership The object is owned by C++ code and QML will never delete
  it. The JavaScript destroy() method cannot be used on these objects. This
  option is similar to QScriptEngine::QtOwnership.

  \value JavaScriptOwnership The object is owned by JavaScript. When the object
  is returned to QML as the return value of a method call, QML will track it
  and delete it if there are no remaining JavaScript references to it and
  it has no QObject::parent(). An object tracked by one QQmlEngine will be
  deleted during that QQmlEngine's destructor. Thus, JavaScript references
  between objects with JavaScriptOwnership from two different engines will
  not be valid if one of these engines is deleted. This option is similar to
  QScriptEngine::ScriptOwnership.

  Generally an application doesn't need to set an object's ownership
  explicitly. QML uses a heuristic to set the default ownership. By default, an
  object that is created by QML has JavaScriptOwnership. The exception to this
  are the root objects created by calling QQmlComponent::create() or
  QQmlComponent::beginCreate(), which have CppOwnership by default. The
  ownership of these root-level objects is considered to have been transferred
  to the C++ caller.

  Objects not-created by QML have CppOwnership by default. The exception to this
  are objects returned from C++ method calls; their ownership will be set to
  JavaScriptOwnership. This applies only to explicit invocations of Q_INVOKABLE
  methods or slots, but not to property getter invocations.

  Calling setObjectOwnership() overrides the default ownership heuristic used by
  QML.
*/

/*!
  Sets the \a ownership of \a object.
*/
void QQmlEngine::setObjectOwnership(QObject *object, ObjectOwnership ownership)
{
    if (!object)
        return;

    QQmlData *ddata = QQmlData::get(object, true);
    if (!ddata)
        return;

    ddata->indestructible = (ownership == CppOwnership)?true:false;
    ddata->explicitIndestructibleSet = true;
}

/*!
  Returns the ownership of \a object.
*/
QQmlEngine::ObjectOwnership QQmlEngine::objectOwnership(QObject *object)
{
    if (!object)
        return CppOwnership;

    QQmlData *ddata = QQmlData::get(object, false);
    if (!ddata)
        return CppOwnership;
    else
        return ddata->indestructible?CppOwnership:JavaScriptOwnership;
}

/*!
   \reimp
*/
bool QQmlEngine::event(QEvent *e)
{
    Q_D(QQmlEngine);
    if (e->type() == QEvent::User)
        d->doDeleteInEngineThread();
    else if (e->type() == QEvent::LanguageChange) {
        retranslate();
    }

    return QJSEngine::event(e);
}

void QQmlEnginePrivate::doDeleteInEngineThread()
{
    QFieldList<Deletable, &Deletable::next> list;
    mutex.lock();
    list.copyAndClear(toDeleteInEngineThread);
    mutex.unlock();

    while (Deletable *d = list.takeFirst())
        delete d;
}

namespace QtQml {

void qmlExecuteDeferred(QObject *object)
{
    QQmlData *data = QQmlData::get(object);

    if (data && !data->deferredData.isEmpty() && !data->wasDeleted(object)) {
        QQmlEnginePrivate *ep = QQmlEnginePrivate::get(data->context->engine);

        QQmlComponentPrivate::DeferredState state;
        QQmlComponentPrivate::beginDeferred(ep, object, &state);

        // Release the reference for the deferral action (we still have one from construction)
        data->releaseDeferredData();

        QQmlComponentPrivate::completeDeferred(ep, &state);
    }
}

QQmlContext *qmlContext(const QObject *obj)
{
    return QQmlEngine::contextForObject(obj);
}

QQmlEngine *qmlEngine(const QObject *obj)
{
    QQmlData *data = QQmlData::get(obj, false);
    if (!data || !data->context)
        return nullptr;
    return data->context->engine;
}

static QObject *resolveAttachedProperties(QQmlAttachedPropertiesFunc pf, QQmlData *data,
                                          QObject *object, bool create)
{
    if (!pf)
        return nullptr;

    QObject *rv = data->hasExtendedData() ? data->attachedProperties()->value(pf) : 0;
    if (rv || !create)
        return rv;

    rv = pf(object);

    if (rv)
        data->attachedProperties()->insert(pf, rv);

    return rv;
}

QObject *qmlAttachedPropertiesObjectById(int id, const QObject *object, bool create)
{
    QQmlData *data = QQmlData::get(object, create);

    // Attached properties are only on objects created by QML,
    // unless explicitly requested (create==true)
    if (!data)
        return nullptr;

    QQmlEnginePrivate *engine = QQmlEnginePrivate::get(data->context);
    return resolveAttachedProperties(QQmlMetaType::attachedPropertiesFuncById(engine, id), data,
                                     const_cast<QObject *>(object), create);
}

QObject *qmlAttachedPropertiesObject(int *idCache, const QObject *object,
                                     const QMetaObject *attachedMetaObject, bool create)
{
    if (*idCache == -1) {
        QQmlEngine *engine = object ? qmlEngine(object) : nullptr;
        *idCache = QQmlMetaType::attachedPropertiesFuncId(engine ? QQmlEnginePrivate::get(engine) : nullptr, attachedMetaObject);
    }

    if (*idCache == -1 || !object)
        return nullptr;

    return qmlAttachedPropertiesObjectById(*idCache, object, create);
}

QQmlAttachedPropertiesFunc qmlAttachedPropertiesFunction(QObject *object,
                                                         const QMetaObject *attachedMetaObject)
{
    QQmlEngine *engine = object ? qmlEngine(object) : nullptr;
    return QQmlMetaType::attachedPropertiesFunc(engine ? QQmlEnginePrivate::get(engine) : nullptr,
                                                attachedMetaObject);
}

QObject *qmlAttachedPropertiesObject(QObject *object, QQmlAttachedPropertiesFunc func, bool create)
{
    if (!object)
        return nullptr;

    QQmlData *data = QQmlData::get(object, create);

    // Attached properties are only on objects created by QML,
    // unless explicitly requested (create==true)
    if (!data)
        return nullptr;

    return resolveAttachedProperties(func, data, object, create);
}

} // namespace QtQml

#if QT_DEPRECATED_SINCE(5, 1)

// Also define symbols outside namespace to keep binary compatibility with Qt 5.0

Q_QML_EXPORT void qmlExecuteDeferred(QObject *obj)
{
    QtQml::qmlExecuteDeferred(obj);
}

Q_QML_EXPORT QQmlContext *qmlContext(const QObject *obj)
{
    return QtQml::qmlContext(obj);
}

Q_QML_EXPORT QQmlEngine *qmlEngine(const QObject *obj)
{
    return QtQml::qmlEngine(obj);
}

Q_QML_EXPORT QObject *qmlAttachedPropertiesObjectById(int id, const QObject *obj, bool create)
{
    return QtQml::qmlAttachedPropertiesObjectById(id, obj, create);
}

Q_QML_EXPORT QObject *qmlAttachedPropertiesObject(int *idCache, const QObject *object,
                                                  const QMetaObject *attachedMetaObject,
                                                  bool create)
{
    return QtQml::qmlAttachedPropertiesObject(idCache, object, attachedMetaObject, create);
}

#endif // QT_DEPRECATED_SINCE(5, 1)

class QQmlDataExtended {
public:
    QQmlDataExtended();
    ~QQmlDataExtended();

    QHash<QQmlAttachedPropertiesFunc, QObject *> attachedProperties;
};

QQmlDataExtended::QQmlDataExtended()
{
}

QQmlDataExtended::~QQmlDataExtended()
{
}

void QQmlData::NotifyList::layout(QQmlNotifierEndpoint *endpoint)
{
    // Add a temporary sentinel at beginning of list. This will be overwritten
    // when the end point is inserted into the notifies further down.
    endpoint->prev = nullptr;

    while (endpoint->next) {
        Q_ASSERT(reinterpret_cast<QQmlNotifierEndpoint *>(endpoint->next->prev) == endpoint);
        endpoint = endpoint->next;
    }

    while (endpoint) {
        QQmlNotifierEndpoint *ep = (QQmlNotifierEndpoint *) endpoint->prev;

        int index = endpoint->sourceSignal;
        index = qMin(index, 0xFFFF - 1);

        endpoint->next = notifies[index];
        if (endpoint->next) endpoint->next->prev = &endpoint->next;
        endpoint->prev = &notifies[index];
        notifies[index] = endpoint;

        endpoint = ep;
    }
}

void QQmlData::NotifyList::layout()
{
    Q_ASSERT(maximumTodoIndex >= notifiesSize);

    if (todo) {
        QQmlNotifierEndpoint **old = notifies;
        const int reallocSize = (maximumTodoIndex + 1) * sizeof(QQmlNotifierEndpoint*);
        notifies = (QQmlNotifierEndpoint**)realloc(notifies, reallocSize);
        const int memsetSize = (maximumTodoIndex - notifiesSize + 1) *
                               sizeof(QQmlNotifierEndpoint*);
        memset(notifies + notifiesSize, 0, memsetSize);

        if (notifies != old) {
            for (int ii = 0; ii < notifiesSize; ++ii)
                if (notifies[ii])
                    notifies[ii]->prev = &notifies[ii];
        }

        notifiesSize = maximumTodoIndex + 1;

        layout(todo);
    }

    maximumTodoIndex = 0;
    todo = nullptr;
}

void QQmlData::deferData(int objectIndex, const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &compilationUnit, QQmlContextData *context)
{
    QQmlData::DeferredData *deferData = new QQmlData::DeferredData;
    deferData->deferredIdx = objectIndex;
    deferData->compilationUnit = compilationUnit;
    deferData->context = context;

    const QV4::CompiledData::Object *compiledObject = compilationUnit->objectAt(objectIndex);
    const QV4::CompiledData::BindingPropertyData &propertyData = compilationUnit->bindingPropertyDataPerObject.at(objectIndex);

    const QV4::CompiledData::Binding *binding = compiledObject->bindingTable();
    for (quint32 i = 0; i < compiledObject->nBindings; ++i, ++binding) {
        const QQmlPropertyData *property = propertyData.at(i);
        if (property && binding->flags & QV4::CompiledData::Binding::IsDeferredBinding)
            deferData->bindings.insert(property->coreIndex(), binding);
    }

    deferredData.append(deferData);
}

void QQmlData::releaseDeferredData()
{
    auto it = deferredData.begin();
    while (it != deferredData.end()) {
        DeferredData *deferData = *it;
        if (deferData->bindings.isEmpty()) {
            delete deferData;
            it = deferredData.erase(it);
        } else {
            ++it;
        }
    }
}

void QQmlData::addNotify(int index, QQmlNotifierEndpoint *endpoint)
{
    if (!notifyList) {
        notifyList = (NotifyList *)malloc(sizeof(NotifyList));
        notifyList->connectionMask = 0;
        notifyList->maximumTodoIndex = 0;
        notifyList->notifiesSize = 0;
        notifyList->todo = nullptr;
        notifyList->notifies = nullptr;
    }

    Q_ASSERT(!endpoint->isConnected());

    index = qMin(index, 0xFFFF - 1);
    notifyList->connectionMask |= (1ULL << quint64(index % 64));

    if (index < notifyList->notifiesSize) {

        endpoint->next = notifyList->notifies[index];
        if (endpoint->next) endpoint->next->prev = &endpoint->next;
        endpoint->prev = &notifyList->notifies[index];
        notifyList->notifies[index] = endpoint;

    } else {
        notifyList->maximumTodoIndex = qMax(int(notifyList->maximumTodoIndex), index);

        endpoint->next = notifyList->todo;
        if (endpoint->next) endpoint->next->prev = &endpoint->next;
        endpoint->prev = &notifyList->todo;
        notifyList->todo = endpoint;
    }
}

void QQmlData::disconnectNotifiers()
{
    if (notifyList) {
        while (notifyList->todo)
            notifyList->todo->disconnect();
        for (int ii = 0; ii < notifyList->notifiesSize; ++ii) {
            while (QQmlNotifierEndpoint *ep = notifyList->notifies[ii])
                ep->disconnect();
        }
        free(notifyList->notifies);
        free(notifyList);
        notifyList = nullptr;
    }
}

QHash<QQmlAttachedPropertiesFunc, QObject *> *QQmlData::attachedProperties() const
{
    if (!extendedData) extendedData = new QQmlDataExtended;
    return &extendedData->attachedProperties;
}

void QQmlData::destroyed(QObject *object)
{
    if (nextContextObject)
        nextContextObject->prevContextObject = prevContextObject;
    if (prevContextObject)
        *prevContextObject = nextContextObject;
    else if (outerContext && outerContext->contextObjects == this)
        outerContext->contextObjects = nextContextObject;

    QQmlAbstractBinding *binding = bindings;
    while (binding) {
        binding->setAddedToObject(false);
        binding = binding->nextBinding();
    }
    if (bindings && !bindings->ref.deref())
        delete bindings;

    compilationUnit = nullptr;

    qDeleteAll(deferredData);
    deferredData.clear();

    QQmlBoundSignal *signalHandler = signalHandlers;
    while (signalHandler) {
        if (signalHandler->isNotifying()) {
            // The object is being deleted during signal handler evaluation.
            // This will cause a crash due to invalid memory access when the
            // evaluation has completed.
            // Abort with a friendly message instead.
            QString locationString;
            QQmlBoundSignalExpression *expr = signalHandler->expression();
            if (expr) {
                QQmlSourceLocation location = expr->sourceLocation();
                if (location.sourceFile.isEmpty())
                    location.sourceFile = QStringLiteral("<Unknown File>");
                locationString.append(location.sourceFile);
                locationString.append(QStringLiteral(":%0: ").arg(location.line));
                QString source = expr->expression();
                if (source.size() > 100) {
                    source.truncate(96);
                    source.append(QLatin1String(" ..."));
                }
                locationString.append(source);
            } else {
                locationString = QStringLiteral("<Unknown Location>");
            }
            qFatal("Object %p destroyed while one of its QML signal handlers is in progress.\n"
                   "Most likely the object was deleted synchronously (use QObject::deleteLater() "
                   "instead), or the application is running a nested event loop.\n"
                   "This behavior is NOT supported!\n"
                   "%s", object, qPrintable(locationString));
        }

        QQmlBoundSignal *next = signalHandler->m_nextSignal;
        signalHandler->m_prevSignal = nullptr;
        signalHandler->m_nextSignal = nullptr;
        delete signalHandler;
        signalHandler = next;
    }

    if (bindingBitsArraySize > InlineBindingArraySize)
        free(bindingBits);

    if (propertyCache)
        propertyCache->release();

    ownContext = nullptr;

    while (guards) {
        QQmlGuard<QObject> *guard = static_cast<QQmlGuard<QObject> *>(guards);
        *guard = (QObject *)nullptr;
        guard->objectDestroyed(object);
    }

    disconnectNotifiers();

    if (extendedData)
        delete extendedData;

    // Dispose the handle.
    jsWrapper.clear();

    if (ownMemory)
        delete this;
    else
        this->~QQmlData();
}

DEFINE_BOOL_CONFIG_OPTION(parentTest, QML_PARENT_TEST);

void QQmlData::parentChanged(QObject *object, QObject *parent)
{
    if (parentTest()) {
        if (parentFrozen && !QObjectPrivate::get(object)->wasDeleted) {
            QString on;
            QString pn;

            { QDebug dbg(&on); dbg << object; on = on.left(on.length() - 1); }
            { QDebug dbg(&pn); dbg << parent; pn = pn.left(pn.length() - 1); }

            qFatal("Object %s has had its parent frozen by QML and cannot be changed.\n"
                   "User code is attempting to change it to %s.\n"
                   "This behavior is NOT supported!", qPrintable(on), qPrintable(pn));
        }
    }
}

QQmlData::BindingBitsType *QQmlData::growBits(QObject *obj, int bit)
{
    BindingBitsType *bits = (bindingBitsArraySize == InlineBindingArraySize) ? bindingBitsValue : bindingBits;
    int props = QQmlMetaObject(obj).propertyCount();
    Q_ASSERT(bit < 2 * props);
    Q_UNUSED(bit); // .. for Q_NO_DEBUG mode when the assert above expands to empty

    uint arraySize = (2 * static_cast<uint>(props) + BitsPerType - 1) / BitsPerType;
    Q_ASSERT(arraySize > 1);
    Q_ASSERT(arraySize <= 0xffff); // max for bindingBitsArraySize

    BindingBitsType *newBits = static_cast<BindingBitsType *>(malloc(arraySize*sizeof(BindingBitsType)));
    memcpy(newBits, bits, bindingBitsArraySize * sizeof(BindingBitsType));
    memset(newBits + bindingBitsArraySize, 0, sizeof(BindingBitsType) * (arraySize - bindingBitsArraySize));

    if (bindingBitsArraySize > InlineBindingArraySize)
        free(bits);
    bindingBits = newBits;
    bits = newBits;
    bindingBitsArraySize = arraySize;
    return bits;
}

QQmlData *QQmlData::createQQmlData(QObjectPrivate *priv)
{
    Q_ASSERT(priv);
    Q_ASSERT(!priv->isDeletingChildren);
    priv->declarativeData = new QQmlData;
    return static_cast<QQmlData *>(priv->declarativeData);
}

QQmlPropertyCache *QQmlData::createPropertyCache(QJSEngine *engine, QObject *object)
{
    QQmlData *ddata = QQmlData::get(object, /*create*/true);
    ddata->propertyCache = QJSEnginePrivate::get(engine)->cache(object);
    if (ddata->propertyCache)
        ddata->propertyCache->addref();
    return ddata->propertyCache;
}

void QQmlEnginePrivate::sendQuit()
{
    Q_Q(QQmlEngine);
    emit q->quit();
    if (q->receivers(SIGNAL(quit())) == 0) {
        qWarning("Signal QQmlEngine::quit() emitted, but no receivers connected to handle it.");
    }
}

void QQmlEnginePrivate::sendExit(int retCode)
{
    Q_Q(QQmlEngine);
    if (q->receivers(SIGNAL(exit(int))) == 0)
        qWarning("Signal QQmlEngine::exit() emitted, but no receivers connected to handle it.");
    emit q->exit(retCode);
}

static void dumpwarning(const QQmlError &error)
{
    switch (error.messageType()) {
    case QtDebugMsg:
        QMessageLogger(error.url().toString().toLatin1().constData(),
                       error.line(), nullptr).debug().nospace()
                << qPrintable(error.toString());
        break;
    case QtInfoMsg:
        QMessageLogger(error.url().toString().toLatin1().constData(),
                       error.line(), nullptr).info().nospace()
                << qPrintable(error.toString());
        break;
    case QtWarningMsg:
    case QtFatalMsg: // fatal does not support streaming, and furthermore, is actually fatal. Probably not desirable for QML.
        QMessageLogger(error.url().toString().toLatin1().constData(),
                       error.line(), nullptr).warning().nospace()
                << qPrintable(error.toString());
        break;
    case QtCriticalMsg:
        QMessageLogger(error.url().toString().toLatin1().constData(),
                       error.line(), nullptr).critical().nospace()
                << qPrintable(error.toString());
        break;
    }
}

static void dumpwarning(const QList<QQmlError> &errors)
{
    for (int ii = 0; ii < errors.count(); ++ii)
        dumpwarning(errors.at(ii));
}

void QQmlEnginePrivate::warning(const QQmlError &error)
{
    Q_Q(QQmlEngine);
    q->warnings(QList<QQmlError>() << error);
    if (outputWarningsToMsgLog)
        dumpwarning(error);
}

void QQmlEnginePrivate::warning(const QList<QQmlError> &errors)
{
    Q_Q(QQmlEngine);
    q->warnings(errors);
    if (outputWarningsToMsgLog)
        dumpwarning(errors);
}

void QQmlEnginePrivate::warning(QQmlEngine *engine, const QQmlError &error)
{
    if (engine)
        QQmlEnginePrivate::get(engine)->warning(error);
    else
        dumpwarning(error);
}

void QQmlEnginePrivate::warning(QQmlEngine *engine, const QList<QQmlError> &error)
{
    if (engine)
        QQmlEnginePrivate::get(engine)->warning(error);
    else
        dumpwarning(error);
}

void QQmlEnginePrivate::warning(QQmlEnginePrivate *engine, const QQmlError &error)
{
    if (engine)
        engine->warning(error);
    else
        dumpwarning(error);
}

void QQmlEnginePrivate::warning(QQmlEnginePrivate *engine, const QList<QQmlError> &error)
{
    if (engine)
        engine->warning(error);
    else
        dumpwarning(error);
}

QList<QQmlError> QQmlEnginePrivate::qmlErrorFromDiagnostics(const QString &fileName, const QList<DiagnosticMessage> &diagnosticMessages)
{
    QList<QQmlError> errors;
    for (const DiagnosticMessage &m : diagnosticMessages) {
        if (m.isWarning()) {
            qWarning("%s:%d : %s", qPrintable(fileName), m.loc.startLine, qPrintable(m.message));
            continue;
        }

        QQmlError error;
        error.setUrl(QUrl(fileName));
        error.setDescription(m.message);
        error.setLine(m.loc.startLine);
        error.setColumn(m.loc.startColumn);
        errors << error;
    }
    return errors;
}

void QQmlEnginePrivate::cleanupScarceResources()
{
    // iterate through the list and release them all.
    // note that the actual SRD is owned by the JS engine,
    // so we cannot delete the SRD; but we can free the
    // memory used by the variant in the SRD.
    QV4::ExecutionEngine *engine = v4engine();
    while (QV4::ExecutionEngine::ScarceResourceData *sr = engine->scarceResources.first()) {
        sr->data = QVariant();
        engine->scarceResources.remove(sr);
    }
}

/*!
  Adds \a path as a directory where the engine searches for
  installed modules in a URL-based directory structure.

  The \a path may be a local filesystem directory, a
  \l {The Qt Resource System}{Qt Resource} path (\c {:/imports}), a
  \l {The Qt Resource System}{Qt Resource} url (\c {qrc:/imports}) or a URL.

  The \a path will be converted into canonical form before it
  is added to the import path list.

  The newly added \a path will be first in the importPathList().

  \sa setImportPathList(), {QML Modules}
*/
void QQmlEngine::addImportPath(const QString& path)
{
    Q_D(QQmlEngine);
    d->importDatabase.addImportPath(path);
}

/*!
  Returns the list of directories where the engine searches for
  installed modules in a URL-based directory structure.

  For example, if \c /opt/MyApp/lib/imports is in the path, then QML that
  imports \c com.mycompany.Feature will cause the QQmlEngine to look
  in \c /opt/MyApp/lib/imports/com/mycompany/Feature/ for the components
  provided by that module. A \c qmldir file is required for defining the
  type version mapping and possibly QML extensions plugins.

  By default, the list contains the directory of the application executable,
  paths specified in the \c QML2_IMPORT_PATH environment variable,
  and the builtin \c Qml2ImportsPath from QLibraryInfo.

  \sa addImportPath(), setImportPathList()
*/
QStringList QQmlEngine::importPathList() const
{
    Q_D(const QQmlEngine);
    return d->importDatabase.importPathList();
}

/*!
  Sets \a paths as the list of directories where the engine searches for
  installed modules in a URL-based directory structure.

  By default, the list contains the directory of the application executable,
  paths specified in the \c QML2_IMPORT_PATH environment variable,
  and the builtin \c Qml2ImportsPath from QLibraryInfo.

  \sa importPathList(), addImportPath()
  */
void QQmlEngine::setImportPathList(const QStringList &paths)
{
    Q_D(QQmlEngine);
    d->importDatabase.setImportPathList(paths);
}


/*!
  Adds \a path as a directory where the engine searches for
  native plugins for imported modules (referenced in the \c qmldir file).

  By default, the list contains only \c .,  i.e. the engine searches
  in the directory of the \c qmldir file itself.

  The newly added \a path will be first in the pluginPathList().

  \sa setPluginPathList()
*/
void QQmlEngine::addPluginPath(const QString& path)
{
    Q_D(QQmlEngine);
    d->importDatabase.addPluginPath(path);
}


/*!
  Returns the list of directories where the engine searches for
  native plugins for imported modules (referenced in the \c qmldir file).

  By default, the list contains only \c .,  i.e. the engine searches
  in the directory of the \c qmldir file itself.

  \sa addPluginPath(), setPluginPathList()
*/
QStringList QQmlEngine::pluginPathList() const
{
    Q_D(const QQmlEngine);
    return d->importDatabase.pluginPathList();
}

/*!
  Sets the list of directories where the engine searches for
  native plugins for imported modules (referenced in the \c qmldir file)
  to \a paths.

  By default, the list contains only \c .,  i.e. the engine searches
  in the directory of the \c qmldir file itself.

  \sa pluginPathList(), addPluginPath()
  */
void QQmlEngine::setPluginPathList(const QStringList &paths)
{
    Q_D(QQmlEngine);
    d->importDatabase.setPluginPathList(paths);
}

/*!
  Imports the plugin named \a filePath with the \a uri provided.
  Returns true if the plugin was successfully imported; otherwise returns false.

  On failure and if non-null, the \a errors list will have any errors which occurred prepended to it.

  The plugin has to be a Qt plugin which implements the QQmlExtensionPlugin interface.
*/
bool QQmlEngine::importPlugin(const QString &filePath, const QString &uri, QList<QQmlError> *errors)
{
    Q_D(QQmlEngine);
    return d->importDatabase.importDynamicPlugin(filePath, uri, QString(), -1, errors);
}

/*!
  \property QQmlEngine::offlineStoragePath
  \brief the directory for storing offline user data

  Returns the directory where SQL and other offline
  storage is placed.

  The SQL databases created with openDatabase() are stored here.

  The default is QML/OfflineStorage in the platform-standard
  user application data directory.

  Note that the path may not currently exist on the filesystem, so
  callers wanting to \e create new files at this location should create
  it first - see QDir::mkpath().
*/
void QQmlEngine::setOfflineStoragePath(const QString& dir)
{
    Q_D(QQmlEngine);
    d->offlineStoragePath = dir;
}

QString QQmlEngine::offlineStoragePath() const
{
    Q_D(const QQmlEngine);

    if (d->offlineStoragePath.isEmpty()) {
        QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
        QQmlEnginePrivate *e = const_cast<QQmlEnginePrivate *>(d);
        if (!dataLocation.isEmpty())
            e->offlineStoragePath = dataLocation.replace(QLatin1Char('/'), QDir::separator())
                                  + QDir::separator() + QLatin1String("QML")
                                  + QDir::separator() + QLatin1String("OfflineStorage");
    }

    return d->offlineStoragePath;
}

/*!
  Returns the file path where a \l{QtQuick.LocalStorage}{Local Storage}
  database with the identifier \a databaseName is (or would be) located.

  \sa {openDatabaseSync}{LocalStorage.openDatabaseSync()}
  \since 5.9
*/
QString QQmlEngine::offlineStorageDatabaseFilePath(const QString &databaseName) const
{
    Q_D(const QQmlEngine);
    QCryptographicHash md5(QCryptographicHash::Md5);
    md5.addData(databaseName.toUtf8());
    return d->offlineStorageDatabaseDirectory() + QLatin1String(md5.result().toHex());
}

QString QQmlEnginePrivate::offlineStorageDatabaseDirectory() const
{
    Q_Q(const QQmlEngine);
    return q->offlineStoragePath() + QDir::separator() + QLatin1String("Databases") + QDir::separator();
}

bool QQmlEnginePrivate::isQObject(int t)
{
    Locker locker(this);
    return m_compositeTypes.contains(t) || QQmlMetaType::isQObject(t);
}

QObject *QQmlEnginePrivate::toQObject(const QVariant &v, bool *ok) const
{
    Locker locker(this);
    int t = v.userType();
    if (t == QMetaType::QObjectStar || m_compositeTypes.contains(t)) {
        if (ok) *ok = true;
        return *(QObject *const *)(v.constData());
    } else {
        return QQmlMetaType::toQObject(v, ok);
    }
}

QQmlMetaType::TypeCategory QQmlEnginePrivate::typeCategory(int t) const
{
    Locker locker(this);
    if (m_compositeTypes.contains(t))
        return QQmlMetaType::Object;
    return QQmlMetaType::typeCategory(t);
}

bool QQmlEnginePrivate::isList(int t) const
{
    return QQmlMetaType::isList(t);
}

int QQmlEnginePrivate::listType(int t) const
{
    return QQmlMetaType::listType(t);
}

QQmlMetaObject QQmlEnginePrivate::rawMetaObjectForType(int t) const
{
    Locker locker(this);
    auto iter = m_compositeTypes.constFind(t);
    if (iter != m_compositeTypes.cend()) {
        return QQmlMetaObject((*iter)->rootPropertyCache().data());
    } else {
        QQmlType type = QQmlMetaType::qmlType(t);
        return QQmlMetaObject(type.baseMetaObject());
    }
}

QQmlMetaObject QQmlEnginePrivate::metaObjectForType(int t) const
{
    Locker locker(this);
    auto iter = m_compositeTypes.constFind(t);
    if (iter != m_compositeTypes.cend()) {
        return QQmlMetaObject((*iter)->rootPropertyCache().data());
    } else {
        QQmlType type = QQmlMetaType::qmlType(t);
        return QQmlMetaObject(type.metaObject());
    }
}

QQmlPropertyCache *QQmlEnginePrivate::propertyCacheForType(int t)
{
    Locker locker(this);
    auto iter = m_compositeTypes.constFind(t);
    if (iter != m_compositeTypes.cend()) {
        return (*iter)->rootPropertyCache().data();
    } else {
        QQmlType type = QQmlMetaType::qmlType(t);
        locker.unlock();
        return type.isValid() ? cache(type.metaObject()) : nullptr;
    }
}

QQmlPropertyCache *QQmlEnginePrivate::rawPropertyCacheForType(int t, int minorVersion)
{
    Locker locker(this);
    auto iter = m_compositeTypes.constFind(t);
    if (iter != m_compositeTypes.cend()) {
        return (*iter)->rootPropertyCache().data();
    } else {
        QQmlType type = QQmlMetaType::qmlType(t);
        locker.unlock();

        if (minorVersion >= 0)
            return type.isValid() ? cache(type, minorVersion) : nullptr;
        else
            return type.isValid() ? cache(type.baseMetaObject()) : nullptr;
    }
}

void QQmlEnginePrivate::registerInternalCompositeType(QV4::CompiledData::CompilationUnit *compilationUnit)
{
    compilationUnit->isRegisteredWithEngine = true;

    Locker locker(this);
    // The QQmlCompiledData is not referenced here, but it is removed from this
    // hash in the QQmlCompiledData destructor
    m_compositeTypes.insert(compilationUnit->metaTypeId, compilationUnit);
}

void QQmlEnginePrivate::unregisterInternalCompositeType(QV4::CompiledData::CompilationUnit *compilationUnit)
{
    compilationUnit->isRegisteredWithEngine = false;

    Locker locker(this);
    m_compositeTypes.remove(compilationUnit->metaTypeId);
}

bool QQmlEnginePrivate::isTypeLoaded(const QUrl &url) const
{
    return typeLoader.isTypeLoaded(url);
}

bool QQmlEnginePrivate::isScriptLoaded(const QUrl &url) const
{
    return typeLoader.isScriptLoaded(url);
}

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
// Normalize a file name using Shell API. As opposed to converting it
// to a short 8.3 name and back, this also works for drives where 8.3 notation
// is disabled (see 8dot3name options of fsutil.exe).
static inline QString shellNormalizeFileName(const QString &name)
{
    const QString nativeSeparatorName(QDir::toNativeSeparators(name));
    const LPCTSTR nameC = reinterpret_cast<LPCTSTR>(nativeSeparatorName.utf16());
// The correct declaration of the SHGetPathFromIDList symbol is
// being used in mingw-w64 as of r6215, which is a v3 snapshot.
#if defined(Q_CC_MINGW) && (!defined(__MINGW64_VERSION_MAJOR) || __MINGW64_VERSION_MAJOR < 3)
    ITEMIDLIST *file;
    if (FAILED(SHParseDisplayName(nameC, NULL, reinterpret_cast<LPITEMIDLIST>(&file), 0, NULL)))
        return name;
#else
    PIDLIST_ABSOLUTE file;
    if (FAILED(SHParseDisplayName(nameC, NULL, &file, 0, NULL)))
        return name;
#endif
    TCHAR buffer[MAX_PATH];
    bool gotPath = SHGetPathFromIDList(file, buffer);
    ILFree(file);

    if (!gotPath)
        return name;

    QString canonicalName = QString::fromWCharArray(buffer);
    // Upper case drive letter
    if (canonicalName.size() > 2 && canonicalName.at(1) == QLatin1Char(':'))
        canonicalName[0] = canonicalName.at(0).toUpper();
    return QDir::cleanPath(canonicalName);
}
#endif // Q_OS_WIN && !Q_OS_WINRT

bool QQml_isFileCaseCorrect(const QString &fileName, int lengthIn /* = -1 */)
{
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    QFileInfo info(fileName);
    const QString absolute = info.absoluteFilePath();

#if defined(Q_OS_DARWIN) || defined(Q_OS_WINRT)
    const QString canonical = info.canonicalFilePath();
#elif defined(Q_OS_WIN)
    // No difference if the path is qrc based
    if (absolute[0] == QLatin1Char(':'))
        return true;
    const QString canonical = shellNormalizeFileName(absolute);
#endif

    const int absoluteLength = absolute.length();
    const int canonicalLength = canonical.length();

    int length = qMin(absoluteLength, canonicalLength);
    if (lengthIn >= 0) {
        length = qMin(lengthIn, length);
    } else {
        // No length given: Limit to file name. Do not trigger
        // on drive letters or folder names.
        int lastSlash = absolute.lastIndexOf(QLatin1Char('/'));
        if (lastSlash < 0)
            lastSlash = absolute.lastIndexOf(QLatin1Char('\\'));
        if (lastSlash >= 0) {
            const int fileNameLength = absoluteLength - 1 - lastSlash;
            length = qMin(length, fileNameLength);
        }
    }

    for (int ii = 0; ii < length; ++ii) {
        const QChar &a = absolute.at(absoluteLength - 1 - ii);
        const QChar &c = canonical.at(canonicalLength - 1 - ii);

        if (a.toLower() != c.toLower())
            return true;
        if (a != c)
            return false;
    }
#else
    Q_UNUSED(lengthIn)
    Q_UNUSED(fileName)
#endif
    return true;
}

/*!
    \fn QQmlEngine *qmlEngine(const QObject *object)
    \relates QQmlEngine

    Returns the QQmlEngine associated with \a object, if any.  This is equivalent to
    QQmlEngine::contextForObject(object)->engine(), but more efficient.

    \note Add \c{#include <QtQml>} to use this function.

    \sa {QQmlEngine::contextForObject()}{contextForObject()}, qmlContext()
*/

/*!
    \fn QQmlContext *qmlContext(const QObject *object)
    \relates QQmlEngine

    Returns the QQmlContext associated with \a object, if any.  This is equivalent to
    QQmlEngine::contextForObject(object).

    \note Add \c{#include <QtQml>} to use this function.

    \sa {QQmlEngine::contextForObject()}{contextForObject()}, qmlEngine()
*/

QT_END_NAMESPACE

#include "moc_qqmlengine.cpp"
