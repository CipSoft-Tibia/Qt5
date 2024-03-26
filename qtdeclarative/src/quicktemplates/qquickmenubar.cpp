// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmenubar_p.h"
#include "qquickmenubar_p_p.h"
#include "qquickmenubaritem_p_p.h"
#include "qquickmenu_p.h"
#include "qquickmenu_p_p.h"

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>

#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype MenuBar
    \inherits Container
//!     \instantiates QQuickMenuBar
    \inqmlmodule QtQuick.Controls
    \since 5.10
    \ingroup qtquickcontrols-menus
    \ingroup qtquickcontrols-focusscopes
    \brief Provides a window menu bar.

    \image qtquickcontrols-menubar.png

    MenuBar consists of drop-down menus, and is normally located at the top
    edge of the window.

    \quotefromfile qtquickcontrols-menubar.qml
    \skipuntil begin
    \printto skipfrom
    \skipuntil skipto
    \printto end

    Typically, menus are statically declared as children of the menu bar, but
    MenuBar also provides API to \l {addMenu}{add}, \l {insertMenu}{insert},
    \l {removeMenu}{remove}, and \l {takeMenu}{take} menus dynamically. The
    menus in a menu bar can be accessed using \l menuAt().

    \sa {Customizing MenuBar}, Menu, MenuBarItem, {Menu Controls},
        {Focus Management in Qt Quick Controls}
*/

QQuickItem *QQuickMenuBarPrivate::beginCreateItem(QQuickMenu *menu)
{
    Q_Q(QQuickMenuBar);
    if (!delegate)
        return nullptr;

    QQmlContext *context = delegate->creationContext();
    if (!context)
        context = qmlContext(q);

    QObject *object = delegate->beginCreate(context);
    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    if (!item) {
        delete object;
        return nullptr;
    }

    if (QQuickMenuBarItem *menuBarItem = qobject_cast<QQuickMenuBarItem *>(item))
        menuBarItem->setMenu(menu);
    item->setParentItem(q);
    QQml_setParent_noEvent(item, q);

    return item;
}

void QQuickMenuBarPrivate::completeCreateItem()
{
    if (!delegate)
        return;

    delegate->completeCreate();
}

QQuickItem *QQuickMenuBarPrivate::createItem(QQuickMenu *menu)
{
    QQuickItem *item = beginCreateItem(menu);
    completeCreateItem();
    return item;
}

void QQuickMenuBarPrivate::toggleCurrentMenu(bool visible, bool activate)
{
    if (!currentItem || visible == popupMode)
        return;

    QQuickMenu *menu = currentItem->menu();

    triggering = true;
    popupMode = visible;
    if (menu)
        menu->setVisible(visible);
    if (!visible)
        currentItem->forceActiveFocus();
    else if (menu && activate)
        menu->setCurrentIndex(0);
    triggering = false;
}

void QQuickMenuBarPrivate::activateItem(QQuickMenuBarItem *item)
{
    if (currentItem == item)
        return;

    if (currentItem) {
        currentItem->setHighlighted(false);
        if (popupMode) {
            if (QQuickMenu *menu = currentItem->menu())
                menu->dismiss();
        }
    }

    if (item) {
        item->setHighlighted(true);
        if (popupMode) {
            if (QQuickMenu *menu = item->menu())
                menu->open();
        }
    }

    currentItem = item;
}

void QQuickMenuBarPrivate::activateNextItem()
{
    int index = currentItem ? contentModel->indexOf(currentItem, nullptr) : -1;
    if (index >= contentModel->count() - 1)
        index = -1;
    activateItem(qobject_cast<QQuickMenuBarItem *>(itemAt(++index)));
}

void QQuickMenuBarPrivate::activatePreviousItem()
{
    int index = currentItem ? contentModel->indexOf(currentItem, nullptr) : contentModel->count();
    if (index <= 0)
        index = contentModel->count();
    activateItem(qobject_cast<QQuickMenuBarItem *>(itemAt(--index)));
}

void QQuickMenuBarPrivate::onItemHovered()
{
    Q_Q(QQuickMenuBar);
    QQuickMenuBarItem *item = qobject_cast<QQuickMenuBarItem *>(q->sender());
    if (!item || item == currentItem || !item->isHovered() || !item->isEnabled() || QQuickMenuBarItemPrivate::get(item)->touchId != -1)
        return;

    activateItem(item);
}

void QQuickMenuBarPrivate::onItemTriggered()
{
    Q_Q(QQuickMenuBar);
    QQuickMenuBarItem *item = qobject_cast<QQuickMenuBarItem *>(q->sender());
    if (!item)
        return;

    if (item == currentItem) {
        toggleCurrentMenu(!popupMode, false);
    } else {
        popupMode = true;
        activateItem(item);
    }
}

void QQuickMenuBarPrivate::onMenuAboutToHide()
{
    if (triggering || !currentItem || (currentItem->isHovered() && currentItem->isEnabled()) || !currentItem->isHighlighted())
        return;

    popupMode = false;
    activateItem(nullptr);
}

qreal QQuickMenuBarPrivate::getContentWidth() const
{
    Q_Q(const QQuickMenuBar);
    const int count = contentModel->count();
    qreal totalWidth = qMax(0, count - 1) * spacing;
    for (int i = 0; i < count; ++i) {
        QQuickItem *item = q->itemAt(i);
        if (item)
            totalWidth += item->implicitWidth();
    }
    return totalWidth;
}

qreal QQuickMenuBarPrivate::getContentHeight() const
{
    Q_Q(const QQuickMenuBar);
    const int count = contentModel->count();
    qreal maxHeight = 0;
    for (int i = 0; i < count; ++i) {
        QQuickItem *item = q->itemAt(i);
        if (item)
            maxHeight = qMax(maxHeight, item->implicitHeight());
    }
    return maxHeight;
}

void QQuickMenuBarPrivate::itemImplicitWidthChanged(QQuickItem *item)
{
    QQuickContainerPrivate::itemImplicitWidthChanged(item);
    if (item != contentItem)
        updateImplicitContentWidth();
}

void QQuickMenuBarPrivate::itemImplicitHeightChanged(QQuickItem *item)
{
    QQuickContainerPrivate::itemImplicitHeightChanged(item);
    if (item != contentItem)
        updateImplicitContentHeight();
}

void QQuickMenuBarPrivate::contentData_append(QQmlListProperty<QObject> *prop, QObject *obj)
{
    QQuickMenuBar *menuBar = static_cast<QQuickMenuBar *>(prop->object);
    if (QQuickMenu *menu = qobject_cast<QQuickMenu *>(obj))
        obj = QQuickMenuBarPrivate::get(menuBar)->createItem(menu);
    QQuickContainerPrivate::contentData_append(prop, obj);
}

void QQuickMenuBarPrivate::menus_append(QQmlListProperty<QQuickMenu> *prop, QQuickMenu *obj)
{
    QQuickMenuBar *menuBar = static_cast<QQuickMenuBar *>(prop->object);
    menuBar->addMenu(obj);
}

qsizetype QQuickMenuBarPrivate::menus_count(QQmlListProperty<QQuickMenu> *prop)
{
    QQuickMenuBar *menuBar = static_cast<QQuickMenuBar *>(prop->object);
    return menuBar->count();
}

QQuickMenu *QQuickMenuBarPrivate::menus_at(QQmlListProperty<QQuickMenu> *prop, qsizetype index)
{
    QQuickMenuBar *menuBar = static_cast<QQuickMenuBar *>(prop->object);
    return menuBar->menuAt(index);
}

void QQuickMenuBarPrivate::menus_clear(QQmlListProperty<QQuickMenu> *prop)
{
    QQuickMenuBar *menuBar = static_cast<QQuickMenuBar *>(prop->object);
    QQuickMenuBarPrivate::get(menuBar)->contentModel->clear();
}

QPalette QQuickMenuBarPrivate::defaultPalette() const
{
    return QQuickTheme::palette(QQuickTheme::MenuBar);
}

QQuickMenuBar::QQuickMenuBar(QQuickItem *parent)
    : QQuickContainer(*(new QQuickMenuBarPrivate), parent)
{
    Q_D(QQuickMenuBar);
    d->changeTypes |= QQuickItemPrivate::Geometry;
    setFlag(ItemIsFocusScope);
    setFocusPolicy(Qt::ClickFocus);
}

/*!
    \qmlproperty Component QtQuick.Controls::MenuBar::delegate

    This property holds the component that is used to create menu bar
    items to present menus in the menu bar.

    \sa MenuBarItem
*/
QQmlComponent *QQuickMenuBar::delegate() const
{
    Q_D(const QQuickMenuBar);
    return d->delegate;
}

void QQuickMenuBar::setDelegate(QQmlComponent *delegate)
{
    Q_D(QQuickMenuBar);
    if (d->delegate == delegate)
        return;

    d->delegate = delegate;
    emit delegateChanged();
}

/*!
    \qmlmethod Menu QtQuick.Controls::MenuBar::menuAt(int index)

    Returns the menu at \a index, or \c null if it does not exist.
*/
QQuickMenu *QQuickMenuBar::menuAt(int index) const
{
    Q_D(const QQuickMenuBar);
    QQuickMenuBarItem *item = qobject_cast<QQuickMenuBarItem *>(d->itemAt(index));
    if (!item)
        return nullptr;
    return item->menu();
}

/*!
    \qmlmethod void QtQuick.Controls::MenuBar::addMenu(Menu menu)

    Adds \a menu to the end of the list of menus.
*/
void QQuickMenuBar::addMenu(QQuickMenu *menu)
{
    Q_D(QQuickMenuBar);
    addItem(d->createItem(menu));
}

/*!
    \qmlmethod void QtQuick.Controls::MenuBar::insertMenu(int index, Menu menu)

    Inserts \a menu at \a index.
*/
void QQuickMenuBar::insertMenu(int index, QQuickMenu *menu)
{
    Q_D(QQuickMenuBar);
    insertItem(index, d->createItem(menu));
}

/*!
    \qmlmethod void QtQuick.Controls::MenuBar::removeMenu(Menu menu)

    Removes and destroys the specified \a menu.
*/
void QQuickMenuBar::removeMenu(QQuickMenu *menu)
{
    Q_D(QQuickMenuBar);
    if (!menu)
        return;

    const int count = d->contentModel->count();
    for (int i = 0; i < count; ++i) {
        QQuickMenuBarItem *item = qobject_cast<QQuickMenuBarItem *>(itemAt(i));
        if (!item || item->menu() != menu)
            continue;

        removeItem(item);
        break;
    }

    menu->deleteLater();
}

/*!
    \qmlmethod Menu QtQuick.Controls::MenuBar::takeMenu(int index)

    Removes and returns the menu at \a index.

    \note The ownership of the item is transferred to the caller.
*/
QQuickMenu *QQuickMenuBar::takeMenu(int index)
{
    Q_D(QQuickMenuBar);
    QQuickMenuBarItem *item = qobject_cast<QQuickMenuBarItem *>(itemAt(index));
    if (!item)
        return nullptr;

    QQuickMenu *menu = item->menu();
    if (!menu)
        return nullptr;

    d->removeItem(index, item);
    item->deleteLater();
    return menu;
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty real QtQuick.Controls::MenuBar::contentWidth

    This property holds the content width. It is used for calculating the total
    implicit width of the menu bar.

    \note This property is available in MenuBar since QtQuick.Controls 2.3 (Qt 5.10),
    but it was promoted to the Container base type in QtQuick.Controls 2.5 (Qt 5.12).

    \sa Container::contentWidth
*/

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty real QtQuick.Controls::MenuBar::contentHeight

    This property holds the content height. It is used for calculating the total
    implicit height of the menu bar.

    \note This property is available in MenuBar since QtQuick.Controls 2.3 (Qt 5.10),
    but it was promoted to the Container base type in QtQuick.Controls 2.5 (Qt 5.12).

    \sa Container::contentHeight
*/

/*!
    \qmlproperty list<Menu> QtQuick.Controls::MenuBar::menus

    This property holds the list of menus.

    The list contains all menus that have been declared in QML as children
    of the menu bar, and also menus that have been dynamically added or
    inserted using the \l addMenu() and \l insertMenu() methods, respectively.
*/
QQmlListProperty<QQuickMenu> QQuickMenuBarPrivate::menus()
{
    Q_Q(QQuickMenuBar);
    return QQmlListProperty<QQuickMenu>(q, nullptr,
                                        QQuickMenuBarPrivate::menus_append,
                                        QQuickMenuBarPrivate::menus_count,
                                        QQuickMenuBarPrivate::menus_at,
                                        QQuickMenuBarPrivate::menus_clear);
}

QQmlListProperty<QObject> QQuickMenuBarPrivate::contentData()
{
    Q_Q(QQuickMenuBar);
    return QQmlListProperty<QObject>(q, nullptr,
                                     QQuickMenuBarPrivate::contentData_append,
                                     QQuickContainerPrivate::contentData_count,
                                     QQuickContainerPrivate::contentData_at,
                                     QQuickContainerPrivate::contentData_clear);
}

bool QQuickMenuBar::eventFilter(QObject *object, QEvent *event)
{
    Q_D(QQuickMenuBar);

    if (d->altPressed) {
        switch (event->type()) {
        case QEvent::KeyRelease: {
            const QKeyEvent *keyEvent = static_cast<const QKeyEvent *>(event);
            if ((keyEvent->key() == Qt::Key_Alt || keyEvent->key() == Qt::Key_Meta)
                && keyEvent->modifiers() == Qt::NoModifier) {
                for (int i = 0; i < count(); ++i) {
                    if (auto *item = qobject_cast<QQuickMenuBarItem *>(d->itemAt(i))) {
                        d->activateItem(item);
                        setFocusReason(Qt::MenuBarFocusReason);
                        setFocus(true);
                        break;
                    }
                }
            }
            Q_FALLTHROUGH();
        }
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseMove:
        case QEvent::TabletPress:
        case QEvent::TabletMove:
        case QEvent::TabletRelease:
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        case QEvent::ActivationChange:
        case QEvent::Shortcut:
            d->altPressed = false;
            qApp->removeEventFilter(this);
            break;
        default:
            break;
        }
    } else if (isVisible() && event->type() == QEvent::ShortcutOverride) {
        const bool altKeyNavigation = QGuiApplicationPrivate::platformTheme()
                                    ->themeHint(QPlatformTheme::MenuBarFocusOnAltPressRelease).toBool();
        if (altKeyNavigation) {
            const QKeyEvent *keyEvent = static_cast<const QKeyEvent *>(event);
            if ((keyEvent->key() == Qt::Key_Alt || keyEvent->key() == Qt::Key_Meta)
                && keyEvent->modifiers() == Qt::AltModifier) {
                d->altPressed = true;
                qApp->installEventFilter(this);
            }
        }
    }
    return QObject::eventFilter(object, event);
}

void QQuickMenuBar::keyPressEvent(QKeyEvent *event)
{
    Q_D(QQuickMenuBar);
    QQuickContainer::keyReleaseEvent(event);

    switch (event->key()) {
    case Qt::Key_Up:
        d->toggleCurrentMenu(false, false);
        break;

    case Qt::Key_Down:
        d->toggleCurrentMenu(true, true);
        break;

    case Qt::Key_Left:
    case Qt::Key_Right:
        if (isMirrored() == (event->key() == Qt::Key_Left))
            d->activateNextItem();
        else
            d->activatePreviousItem();
        break;
    case Qt::Key_Escape:
        if (d->currentItem) {
            d->activateItem(nullptr);
            setFocus(false);
        }
        break;
    default:
#if QT_CONFIG(shortcut)
        if (!event->text().isEmpty() && event->modifiers() == Qt::NoModifier) {
            const QKeyCombination mnemonic(Qt::AltModifier, Qt::Key(event->key()));
            for (int i = 0; i < count(); ++i) {
                if (auto *item = qobject_cast<QQuickMenuBarItem *>(d->itemAt(i))) {
                    if (item->shortcut() == mnemonic) {
                        d->activateItem(item);
                        d->toggleCurrentMenu(true, true);
                    }
                }
            }
        }
#endif
        break;
    }
}

void QQuickMenuBar::keyReleaseEvent(QKeyEvent *event)
{
    QQuickContainer::keyReleaseEvent(event);

    switch (event->key()) {
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Escape:
        event->accept();
        break;

    default:
        event->ignore();
        break;
    }
}

void QQuickMenuBar::hoverLeaveEvent(QHoverEvent *event)
{
    Q_D(QQuickMenuBar);
    QQuickContainer::hoverLeaveEvent(event);
    if (!d->popupMode && d->currentItem)
        d->activateItem(nullptr);
}

bool QQuickMenuBar::isContent(QQuickItem *item) const
{
    return qobject_cast<QQuickMenuBarItem *>(item);
}

void QQuickMenuBar::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value)
{
    Q_D(QQuickMenuBar);
    QQuickContainer::itemChange(change, value);
    switch (change) {
    case ItemSceneChange:
        if (d->windowContentItem)
            d->windowContentItem->removeEventFilter(this);
        if (value.window) {
            d->windowContentItem = value.window->contentItem();
            if (d->windowContentItem)
                d->windowContentItem->installEventFilter(this);
        }
        break;
    default:
        break;
    }
}

void QQuickMenuBar::itemAdded(int index, QQuickItem *item)
{
    Q_D(QQuickMenuBar);
    QQuickContainer::itemAdded(index, item);
    if (QQuickMenuBarItem *menuBarItem = qobject_cast<QQuickMenuBarItem *>(item)) {
        QQuickMenuBarItemPrivate::get(menuBarItem)->setMenuBar(this);
        QObjectPrivate::connect(menuBarItem, &QQuickControl::hoveredChanged, d, &QQuickMenuBarPrivate::onItemHovered);
        QObjectPrivate::connect(menuBarItem, &QQuickMenuBarItem::triggered, d, &QQuickMenuBarPrivate::onItemTriggered);
        if (QQuickMenu *menu = menuBarItem->menu())
            QObjectPrivate::connect(menu, &QQuickPopup::aboutToHide, d, &QQuickMenuBarPrivate::onMenuAboutToHide);
    }
    d->updateImplicitContentSize();
    emit menusChanged();
}

void QQuickMenuBar::itemMoved(int index, QQuickItem *item)
{
    QQuickContainer::itemMoved(index, item);
    emit menusChanged();
}

void QQuickMenuBar::itemRemoved(int index, QQuickItem *item)
{
    Q_D(QQuickMenuBar);
    QQuickContainer::itemRemoved(index, item);
    if (QQuickMenuBarItem *menuBarItem = qobject_cast<QQuickMenuBarItem *>(item)) {
        QQuickMenuBarItemPrivate::get(menuBarItem)->setMenuBar(nullptr);
        QObjectPrivate::disconnect(menuBarItem, &QQuickControl::hoveredChanged, d, &QQuickMenuBarPrivate::onItemHovered);
        QObjectPrivate::disconnect(menuBarItem, &QQuickMenuBarItem::triggered, d, &QQuickMenuBarPrivate::onItemTriggered);
        if (QQuickMenu *menu = menuBarItem->menu())
            QObjectPrivate::disconnect(menu, &QQuickPopup::aboutToHide, d, &QQuickMenuBarPrivate::onMenuAboutToHide);
    }
    d->updateImplicitContentSize();
    emit menusChanged();
}

QFont QQuickMenuBar::defaultFont() const
{
    return QQuickTheme::font(QQuickTheme::MenuBar);
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickMenuBar::accessibleRole() const
{
    return QAccessible::MenuBar;
}
#endif

QT_END_NAMESPACE

#include "moc_qquickmenubar_p.cpp"
