/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Platform module of the Qt Toolkit.
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

#include "qquickplatformmenu_p.h"
#include "qquickplatformmenubar_p.h"
#include "qquickplatformmenuitem_p.h"
#include "qquickplatformiconloader_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtGui/qicon.h>
#include <QtGui/qcursor.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtQml/private/qqmlengine_p.h>
#include <QtQml/private/qv4scopedvalue_p.h>
#include <QtQml/private/qv4qobjectwrapper_p.h>
#include <QtQuick/qquickrendercontrol.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/qquickitem.h>

#include "widgets/qwidgetplatform_p.h"

#if QT_CONFIG(systemtrayicon)
#include "qquickplatformsystemtrayicon_p.h"
#endif

QT_BEGIN_NAMESPACE

/*!
    \qmltype Menu
    \inherits QtObject
    \instantiates QQuickPlatformMenu
    \inqmlmodule Qt.labs.platform
    \since 5.8
    \brief A native menu.

    The Menu type provides a QML API for native platform menu popups.

    \image qtlabsplatform-menu.png

    Menu can be used in a \l MenuBar, or as a stand-alone context menu.
    The following example shows how to open a context menu on right mouse
    click:

    \code
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: zoomMenu.open()
    }

    Menu {
        id: zoomMenu

        MenuItem {
            text: qsTr("Zoom In")
            shortcut: StandardKey.ZoomIn
            onTriggered: zoomIn()
        }

        MenuItem {
            text: qsTr("Zoom Out")
            shortcut: StandardKey.ZoomOut
            onTriggered: zoomOut()
        }
    }
    \endcode

    \section2 Submenus

    To create submenus, declare a Menu as a child of another Menu:

    \qml
    Menu {
        title: qsTr("Edit")

        Menu {
            title: qsTr("Advanced")

            MenuItem {
                text: qsTr("Auto-indent Selection")
                onTriggered: autoIndentSelection()
            }

            MenuItem {
                text: qsTr("Rewrap Paragraph")
                onTriggered: rewrapParagraph()
            }
        }
    }
    \endqml

    \section2 Dynamically Generating Menu Items

    It is possible to dynamically generate menu items. One of the easiest ways
    to do so is with \l Instantiator. For example, to implement a
    "Recent Files" submenu, where the items are based on a list of files stored
    in settings, the following code could be used:

    \qml
    Menu {
        title: qsTr("File")

        Menu {
            id: recentFilesSubMenu
            title: qsTr("Recent Files")
            enabled: recentFilesInstantiator.count > 0

            Instantiator {
                id: recentFilesInstantiator
                model: settings.recentFiles
                delegate: MenuItem {
                    text: settings.displayableFilePath(modelData)
                    onTriggered: loadFile(modelData)
                }

                onObjectAdded: recentFilesSubMenu.insertItem(index, object)
                onObjectRemoved: recentFilesSubMenu.removeItem(object)
            }

            MenuSeparator {}

            MenuItem {
                text: qsTr("Clear Recent Files")
                onTriggered: settings.clearRecentFiles()
            }
        }
    }
    \endqml

    \section2 Availability

    A native platform menu is currently available on the following platforms:

    \list
    \li macOS
    \li iOS
    \li Android
    \li Linux (only available as a stand-alone context menu when running with the GTK+ platform theme)
    \endlist

    \input includes/widgets.qdocinc 1

    \labs

    \sa MenuItem, MenuSeparator, MenuBar
*/

/*!
    \qmlsignal Qt.labs.platform::Menu::aboutToShow()

    This signal is emitted when the menu is about to be shown to the user.
*/

/*!
    \qmlsignal Qt.labs.platform::Menu::aboutToHide()

    This signal is emitted when the menu is about to be hidden from the user.
*/

Q_DECLARE_LOGGING_CATEGORY(qtLabsPlatformMenus)

QQuickPlatformMenu::QQuickPlatformMenu(QObject *parent)
    : QObject(parent),
      m_complete(false),
      m_enabled(true),
      m_visible(true),
      m_minimumWidth(-1),
      m_type(QPlatformMenu::DefaultMenu),
      m_menuBar(nullptr),
      m_parentMenu(nullptr),
      m_systemTrayIcon(nullptr),
      m_menuItem(nullptr),
      m_iconLoader(nullptr),
      m_handle(nullptr)
{
}

QQuickPlatformMenu::~QQuickPlatformMenu()
{
    if (m_menuBar)
        m_menuBar->removeMenu(this);
    if (m_parentMenu)
        m_parentMenu->removeMenu(this);

    unparentSubmenus();

    delete m_iconLoader;
    m_iconLoader = nullptr;
    delete m_handle;
    m_handle = nullptr;
}

void QQuickPlatformMenu::unparentSubmenus()
{
    for (QQuickPlatformMenuItem *item : qAsConst(m_items)) {
        if (QQuickPlatformMenu *subMenu = item->subMenu())
            subMenu->setParentMenu(nullptr);
        item->setMenu(nullptr);
    }
}

QPlatformMenu *QQuickPlatformMenu::handle() const
{
    return m_handle;
}

QPlatformMenu * QQuickPlatformMenu::create()
{
    if (!m_handle) {
        if (m_menuBar && m_menuBar->handle())
            m_handle = m_menuBar->handle()->createMenu();
        else if (m_parentMenu && m_parentMenu->handle())
            m_handle = m_parentMenu->handle()->createSubMenu();
#if QT_CONFIG(systemtrayicon)
        else if (m_systemTrayIcon && m_systemTrayIcon->handle())
            m_handle = m_systemTrayIcon->handle()->createMenu();
#endif

        // TODO: implement ^
        // - QCocoaMenuBar::createMenu()
        // - QCocoaMenu::createSubMenu()
        // - QCocoaSystemTrayIcon::createMenu()
        if (!m_handle)
            m_handle = QGuiApplicationPrivate::platformTheme()->createPlatformMenu();

        if (!m_handle)
            m_handle = QWidgetPlatform::createMenu();

        qCDebug(qtLabsPlatformMenus) << "Menu ->" << m_handle;

        if (m_handle) {
            connect(m_handle, &QPlatformMenu::aboutToShow, this, &QQuickPlatformMenu::aboutToShow);
            connect(m_handle, &QPlatformMenu::aboutToHide, this, &QQuickPlatformMenu::aboutToHide);

            for (QQuickPlatformMenuItem *item : qAsConst(m_items))
                m_handle->insertMenuItem(item->create(), nullptr);

            if (m_menuItem) {
                if (QPlatformMenuItem *handle = m_menuItem->create())
                    handle->setMenu(m_handle);
            }
        }
    }
    return m_handle;
}

void QQuickPlatformMenu::destroy()
{
    if (!m_handle)
        return;

    // Ensure that all submenus are unparented before we are destroyed,
    // so that they don't try to access a destroyed menu.
    unparentSubmenus();

    delete m_handle;
    m_handle = nullptr;
}

void QQuickPlatformMenu::sync()
{
    if (!m_complete || !create())
        return;

    m_handle->setText(m_title);
    m_handle->setEnabled(m_enabled);
    m_handle->setVisible(m_visible);
    m_handle->setMinimumWidth(m_minimumWidth);
    m_handle->setMenuType(m_type);
    m_handle->setFont(m_font);

    if (m_menuBar && m_menuBar->handle())
        m_menuBar->handle()->syncMenu(m_handle);
#if QT_CONFIG(systemtrayicon)
    else if (m_systemTrayIcon && m_systemTrayIcon->handle())
        m_systemTrayIcon->handle()->updateMenu(m_handle);
#endif

    for (QQuickPlatformMenuItem *item : qAsConst(m_items))
        item->sync();
}

/*!
    \default
    \qmlproperty list<Object> Qt.labs.platform::Menu::data

    This default property holds the list of all objects declared as children of
    the menu. The data property includes objects that are not \l MenuItem instances,
    such as \l Timer and \l QtObject.

    \sa items
*/
QQmlListProperty<QObject> QQuickPlatformMenu::data()
{
    return QQmlListProperty<QObject>(this, nullptr, data_append, data_count, data_at, data_clear);
}

/*!
    \qmlproperty list<MenuItem> Qt.labs.platform::Menu::items

    This property holds the list of items in the menu.
*/
QQmlListProperty<QQuickPlatformMenuItem> QQuickPlatformMenu::items()
{
    return QQmlListProperty<QQuickPlatformMenuItem>(this, nullptr, items_append, items_count, items_at, items_clear);
}

/*!
    \readonly
    \qmlproperty MenuBar Qt.labs.platform::Menu::menuBar

    This property holds the menubar that the menu belongs to, or \c null if the
    menu is not in a menubar.
*/
QQuickPlatformMenuBar *QQuickPlatformMenu::menuBar() const
{
    return m_menuBar;
}

void QQuickPlatformMenu::setMenuBar(QQuickPlatformMenuBar *menuBar)
{
    if (m_menuBar == menuBar)
        return;

    m_menuBar = menuBar;
    destroy();
    emit menuBarChanged();
}

/*!
    \readonly
    \qmlproperty Menu Qt.labs.platform::Menu::parentMenu

    This property holds the parent menu that the menu belongs to, or \c null if the
    menu is not a sub-menu.
*/
QQuickPlatformMenu *QQuickPlatformMenu::parentMenu() const
{
    return m_parentMenu;
}

void QQuickPlatformMenu::setParentMenu(QQuickPlatformMenu *menu)
{
    if (m_parentMenu == menu)
        return;

    m_parentMenu = menu;
    destroy();
    emit parentMenuChanged();
}

/*!
    \readonly
    \qmlproperty SystemTrayIcon Qt.labs.platform::Menu::systemTrayIcon

    This property holds the system tray icon that the menu belongs to, or \c null
    if the menu is not in a system tray icon.
*/
QQuickPlatformSystemTrayIcon *QQuickPlatformMenu::systemTrayIcon() const
{
    return m_systemTrayIcon;
}

void QQuickPlatformMenu::setSystemTrayIcon(QQuickPlatformSystemTrayIcon *icon)
{
    if (m_systemTrayIcon == icon)
        return;

    m_systemTrayIcon = icon;
    destroy();
    emit systemTrayIconChanged();
}

/*!
    \readonly
    \qmlproperty MenuItem Qt.labs.platform::Menu::menuItem

    This property holds the item that presents the menu (in a parent menu).
*/
QQuickPlatformMenuItem *QQuickPlatformMenu::menuItem() const
{
    if (!m_menuItem) {
        QQuickPlatformMenu *that = const_cast<QQuickPlatformMenu *>(this);
        m_menuItem = new QQuickPlatformMenuItem(that);
        m_menuItem->setSubMenu(that);
        m_menuItem->setText(m_title);
        m_menuItem->setIconName(iconName());
        m_menuItem->setIconSource(iconSource());
        m_menuItem->setVisible(m_visible);
        m_menuItem->setEnabled(m_enabled);
        m_menuItem->componentComplete();
    }
    return m_menuItem;
}

/*!
    \qmlproperty bool Qt.labs.platform::Menu::enabled

    This property holds whether the menu is enabled. The default value is \c true.
*/
bool QQuickPlatformMenu::isEnabled() const
{
    return m_enabled;
}

void QQuickPlatformMenu::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    if (m_menuItem)
        m_menuItem->setEnabled(enabled);

    m_enabled = enabled;
    sync();
    emit enabledChanged();
}

/*!
    \qmlproperty bool Qt.labs.platform::Menu::visible

    This property holds whether the menu is visible. The default value is \c true.
*/
bool QQuickPlatformMenu::isVisible() const
{
    return m_visible;
}

void QQuickPlatformMenu::setVisible(bool visible)
{
    if (m_visible == visible)
        return;

    if (m_menuItem)
        m_menuItem->setVisible(visible);

    m_visible = visible;
    sync();
    emit visibleChanged();
}

/*!
    \qmlproperty int Qt.labs.platform::Menu::minimumWidth

    This property holds the minimum width of the menu. The default value is \c -1 (no minimum width).
*/
int QQuickPlatformMenu::minimumWidth() const
{
    return m_minimumWidth;
}

void QQuickPlatformMenu::setMinimumWidth(int width)
{
    if (m_minimumWidth == width)
        return;

    m_minimumWidth = width;
    sync();
    emit minimumWidthChanged();
}

/*!
    \qmlproperty enumeration Qt.labs.platform::Menu::type

    This property holds the type of the menu.

    Available values:
    \value Menu.DefaultMenu A normal menu (default).
    \value Menu.EditMenu An edit menu with pre-populated cut, copy and paste items.
*/
QPlatformMenu::MenuType QQuickPlatformMenu::type() const
{
    return m_type;
}

void QQuickPlatformMenu::setType(QPlatformMenu::MenuType type)
{
    if (m_type == type)
        return;

    m_type = type;
    sync();
    emit typeChanged();
}

/*!
    \qmlproperty string Qt.labs.platform::Menu::title

    This property holds the menu's title.
*/
QString QQuickPlatformMenu::title() const
{
    return m_title;
}

void QQuickPlatformMenu::setTitle(const QString &title)
{
    if (m_title == title)
        return;

    if (m_menuItem)
        m_menuItem->setText(title);

    m_title = title;
    sync();
    emit titleChanged();
}

/*!
    \qmlproperty url Qt.labs.platform::Menu::iconSource
    \deprecated Use icon.source instead
*/
QUrl QQuickPlatformMenu::iconSource() const
{
    return icon().source();
}

void QQuickPlatformMenu::setIconSource(const QUrl& source)
{
    QQuickPlatformIcon newIcon = icon();
    if (source == newIcon.source())
        return;

    if (m_menuItem)
        m_menuItem->setIconSource(source);

    newIcon.setSource(source);
    iconLoader()->setIcon(newIcon);
    emit iconSourceChanged();
}

/*!
    \qmlproperty string Qt.labs.platform::Menu::iconName
    \deprecated Use icon.name instead
*/
QString QQuickPlatformMenu::iconName() const
{
    return icon().name();
}

void QQuickPlatformMenu::setIconName(const QString& name)
{
    QQuickPlatformIcon newIcon = icon();
    if (name == newIcon.name())
        return;

    if (m_menuItem)
        m_menuItem->setIconName(name);

    newIcon.setName(name);
    iconLoader()->setIcon(newIcon);
    emit iconNameChanged();}

/*!
    \qmlproperty font Qt.labs.platform::Menu::font

    This property holds the menu's font.

    \sa text
*/
QFont QQuickPlatformMenu::font() const
{
    return m_font;
}

void QQuickPlatformMenu::setFont(const QFont& font)
{
    if (m_font == font)
        return;

    m_font = font;
    sync();
    emit fontChanged();
}

/*!
    \since Qt.labs.platform 1.1 (Qt 5.12)
    \qmlpropertygroup Qt.labs.platform::MenuItem::icon
    \qmlproperty url Qt.labs.platform::MenuItem::icon.source
    \qmlproperty string Qt.labs.platform::MenuItem::icon.name
    \qmlproperty bool Qt.labs.platform::MenuItem::icon.mask

    This property holds the menu item's icon.
*/
QQuickPlatformIcon QQuickPlatformMenu::icon() const
{
    if (!m_iconLoader)
        return QQuickPlatformIcon();

    return iconLoader()->icon();
}

void QQuickPlatformMenu::setIcon(const QQuickPlatformIcon &icon)
{
    if (iconLoader()->icon() == icon)
        return;

    if (m_menuItem)
        m_menuItem->setIcon(icon);

    iconLoader()->setIcon(icon);
    emit iconChanged();
}

/*!
    \qmlmethod void Qt.labs.platform::Menu::addItem(MenuItem item)

    Adds an \a item to the end of the menu.
*/
void QQuickPlatformMenu::addItem(QQuickPlatformMenuItem *item)
{
    insertItem(m_items.count(), item);
}

/*!
    \qmlmethod void Qt.labs.platform::Menu::insertItem(int index, MenuItem item)

    Inserts an \a item at the specified \a index in the menu.
*/
void QQuickPlatformMenu::insertItem(int index, QQuickPlatformMenuItem *item)
{
    if (!item || m_items.contains(item))
        return;

    m_items.insert(index, item);
    m_data.append(item);
    item->setMenu(this);
    if (m_handle && item->create()) {
        QQuickPlatformMenuItem *before = m_items.value(index + 1);
        m_handle->insertMenuItem(item->handle(), before ? before->create() : nullptr);
    }
    sync();
    emit itemsChanged();
}

/*!
    \qmlmethod void Qt.labs.platform::Menu::removeItem(MenuItem item)

    Removes an \a item from the menu.
*/
void QQuickPlatformMenu::removeItem(QQuickPlatformMenuItem *item)
{
    if (!item || !m_items.removeOne(item))
        return;

    m_data.removeOne(item);
    if (m_handle)
        m_handle->removeMenuItem(item->handle());
    item->setMenu(nullptr);
    sync();
    emit itemsChanged();
}

/*!
    \qmlmethod void Qt.labs.platform::Menu::addMenu(Menu submenu)

    Adds a \a submenu to the end of the menu.
*/
void QQuickPlatformMenu::addMenu(QQuickPlatformMenu *menu)
{
    insertMenu(m_items.count(), menu);
}

/*!
    \qmlmethod void Qt.labs.platform::Menu::insertMenu(int index, Menu submenu)

    Inserts a \a submenu at the specified \a index in the menu.
*/
void QQuickPlatformMenu::insertMenu(int index, QQuickPlatformMenu *menu)
{
    if (!menu)
        return;

    menu->setParentMenu(this);
    insertItem(index, menu->menuItem());
}

/*!
    \qmlmethod void Qt.labs.platform::Menu::removeMenu(Menu submenu)

    Removes a \a submenu from the menu.
*/
void QQuickPlatformMenu::removeMenu(QQuickPlatformMenu *menu)
{
    if (!menu)
        return;

    menu->setParentMenu(nullptr);
    removeItem(menu->menuItem());
}

/*!
    \qmlmethod void Qt.labs.platform::Menu::clear()

    Removes all items from the menu.
*/
void QQuickPlatformMenu::clear()
{
    if (m_items.isEmpty())
        return;

    for (QQuickPlatformMenuItem *item : qAsConst(m_items)) {
        m_data.removeOne(item);
        if (m_handle)
            m_handle->removeMenuItem(item->handle());
        item->setMenu(nullptr);
        delete item;
    }

    m_items.clear();
    sync();
    emit itemsChanged();
}

/*!
    \qmlmethod void Qt.labs.platform::Menu::open(MenuItem item)

    Opens the menu at the current mouse position, optionally aligned to a menu \a item.
*/

/*!
    \qmlmethod void Qt.labs.platform::Menu::open(Item target, MenuItem item)

    Opens the menu at the specified \a target item, optionally aligned to a menu \a item.
*/
void QQuickPlatformMenu::open(QQmlV4Function *args)
{
    if (!m_handle)
        return;

    if (args->length() > 2) {
        args->v4engine()->throwTypeError();
        return;
    }

    QV4::ExecutionEngine *v4 = args->v4engine();
    QV4::Scope scope(v4);

    QQuickItem *targetItem = nullptr;
    if (args->length() > 0) {
        QV4::ScopedValue value(scope, (*args)[0]);
        QV4::Scoped<QV4::QObjectWrapper> object(scope, value->as<QV4::QObjectWrapper>());
        if (object)
            targetItem = qobject_cast<QQuickItem *>(object->object());
    }

    QQuickPlatformMenuItem *menuItem = nullptr;
    if (args->length() > 1) {
        QV4::ScopedValue value(scope, (*args)[1]);
        QV4::Scoped<QV4::QObjectWrapper> object(scope, value->as<QV4::QObjectWrapper>());
        if (object)
            menuItem = qobject_cast<QQuickPlatformMenuItem *>(object->object());
    }

    QPoint offset;
    QWindow *window = findWindow(targetItem, &offset);

    QRect targetRect;
    if (targetItem) {
        QRectF sceneBounds = targetItem->mapRectToScene(targetItem->boundingRect());
        targetRect = sceneBounds.toAlignedRect().translated(offset);
    } else {
#if QT_CONFIG(cursor)
        QPoint pos = QCursor::pos();
        if (window)
            pos = window->mapFromGlobal(pos);
        targetRect.moveTo(pos);
#endif
    }
    m_handle->showPopup(window,
                        QHighDpi::toNativePixels(targetRect, window),
                        menuItem ? menuItem->handle() : nullptr);
}

/*!
    \qmlmethod void Qt.labs.platform::Menu::close()

    Closes the menu.
*/
void QQuickPlatformMenu::close()
{
    if (m_handle)
        m_handle->dismiss();
}

void QQuickPlatformMenu::classBegin()
{
}

void QQuickPlatformMenu::componentComplete()
{
    m_complete = true;
    if (m_handle && m_iconLoader)
        m_iconLoader->setEnabled(true);
    sync();
}

QQuickPlatformIconLoader *QQuickPlatformMenu::iconLoader() const
{
    if (!m_iconLoader) {
        QQuickPlatformMenu *that = const_cast<QQuickPlatformMenu *>(this);
        static int slot = staticMetaObject.indexOfSlot("updateIcon()");
        m_iconLoader = new QQuickPlatformIconLoader(slot, that);
        m_iconLoader->setEnabled(m_complete);
    }
    return m_iconLoader;
}

static QWindow *effectiveWindow(QWindow *window, QPoint *offset)
{
    QQuickWindow *quickWindow = qobject_cast<QQuickWindow *>(window);
    if (quickWindow) {
        QWindow *renderWindow = QQuickRenderControl::renderWindowFor(quickWindow, offset);
        if (renderWindow)
            return renderWindow;
    }
    return window;
}

QWindow *QQuickPlatformMenu::findWindow(QQuickItem *target, QPoint *offset) const
{
    if (target)
        return effectiveWindow(target->window(), offset);

    if (m_menuBar && m_menuBar->window())
        return effectiveWindow(m_menuBar->window(), offset);

    QObject *obj = parent();
    while (obj) {
        QWindow *window = qobject_cast<QWindow *>(obj);
        if (window)
            return effectiveWindow(window, offset);

        QQuickItem *item = qobject_cast<QQuickItem *>(obj);
        if (item && item->window())
            return effectiveWindow(item->window(), offset);

        obj = obj->parent();
    }
    return nullptr;
}

void QQuickPlatformMenu::data_append(QQmlListProperty<QObject> *property, QObject *object)
{
    QQuickPlatformMenu *menu = static_cast<QQuickPlatformMenu *>(property->object);
    if (QQuickPlatformMenuItem *item = qobject_cast<QQuickPlatformMenuItem *>(object))
        menu->addItem(item);
    else if (QQuickPlatformMenu *subMenu = qobject_cast<QQuickPlatformMenu *>(object))
        menu->addMenu(subMenu);
    else
        menu->m_data.append(object);
}

int QQuickPlatformMenu::data_count(QQmlListProperty<QObject> *property)
{
    QQuickPlatformMenu *menu = static_cast<QQuickPlatformMenu *>(property->object);
    return menu->m_data.count();
}

QObject *QQuickPlatformMenu::data_at(QQmlListProperty<QObject> *property, int index)
{
    QQuickPlatformMenu *menu = static_cast<QQuickPlatformMenu *>(property->object);
    return menu->m_data.value(index);
}

void QQuickPlatformMenu::data_clear(QQmlListProperty<QObject> *property)
{
    QQuickPlatformMenu *menu = static_cast<QQuickPlatformMenu *>(property->object);
    menu->m_data.clear();
}

void QQuickPlatformMenu::items_append(QQmlListProperty<QQuickPlatformMenuItem> *property, QQuickPlatformMenuItem *item)
{
    QQuickPlatformMenu *menu = static_cast<QQuickPlatformMenu *>(property->object);
    menu->addItem(item);
}

int QQuickPlatformMenu::items_count(QQmlListProperty<QQuickPlatformMenuItem> *property)
{
    QQuickPlatformMenu *menu = static_cast<QQuickPlatformMenu *>(property->object);
    return menu->m_items.count();
}

QQuickPlatformMenuItem *QQuickPlatformMenu::items_at(QQmlListProperty<QQuickPlatformMenuItem> *property, int index)
{
    QQuickPlatformMenu *menu = static_cast<QQuickPlatformMenu *>(property->object);
    return menu->m_items.value(index);
}

void QQuickPlatformMenu::items_clear(QQmlListProperty<QQuickPlatformMenuItem> *property)
{
    QQuickPlatformMenu *menu = static_cast<QQuickPlatformMenu *>(property->object);
    menu->clear();
}

void QQuickPlatformMenu::updateIcon()
{
    if (!m_handle || !m_iconLoader)
        return;

    m_handle->setIcon(m_iconLoader->toQIcon());
    sync();
}

QT_END_NAMESPACE
