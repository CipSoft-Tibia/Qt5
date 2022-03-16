/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
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

#include "qquicktabbar_p.h"
#include "qquicktabbutton_p.h"
#include "qquickcontainer_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype TabBar
    \inherits Container
    \instantiates QQuickTabBar
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols2-navigation
    \ingroup qtquickcontrols2-containers
    \ingroup qtquickcontrols2-focusscopes
    \brief Allows the user to switch between different views or subtasks.

    TabBar provides a tab-based navigation model.

    \image qtquickcontrols2-tabbar-wireframe.png

    TabBar is populated with TabButton controls, and can be used together with
    any layout or container control that provides \c currentIndex -property,
    such as \l StackLayout or \l SwipeView

    \snippet qtquickcontrols2-tabbar.qml 1

    As shown above, TabBar is typically populated with a static set of tab buttons
    that are defined inline as children of the tab bar. It is also possible to
    \l {Container::addItem()}{add}, \l {Container::insertItem()}{insert},
    \l {Container::moveItem()}{move}, and \l {Container::removeItem()}{remove}
    items dynamically at run time. The items can be accessed using
    \l {Container::}{itemAt()} or \l {Container::}{contentChildren}.

    \section2 Resizing Tabs

    By default, TabBar resizes its buttons to fit the width of the control.
    The available space is distributed equally to each button. The default
    resizing behavior can be overridden by setting an explicit width for the
    buttons.

    The following example illustrates how to keep each tab button at their
    implicit size instead of being resized to fit the tabbar:

    \borderedimage qtquickcontrols2-tabbar-explicit.png

    \snippet qtquickcontrols2-tabbar-explicit.qml 1

    \section2 Flickable Tabs

    If the total width of the buttons exceeds the available width of the tab bar,
    it automatically becomes flickable.

    \image qtquickcontrols2-tabbar-flickable.png

    \snippet qtquickcontrols2-tabbar-flickable.qml 1

    \sa TabButton, {Customizing TabBar}, {Navigation Controls}, {Container Controls},
        {Focus Management in Qt Quick Controls 2}
*/

class QQuickTabBarPrivate : public QQuickContainerPrivate
{
    Q_DECLARE_PUBLIC(QQuickTabBar)

public:
    void updateCurrentItem();
    void updateCurrentIndex();
    void updateLayout();

    qreal getContentWidth() const override;
    qreal getContentHeight() const override;

    void itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &diff) override;
    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;

    bool updatingLayout = false;
    QQuickTabBar::Position position = QQuickTabBar::Header;
};

class QQuickTabBarAttachedPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickTabBarAttached)

public:
    static QQuickTabBarAttachedPrivate *get(QQuickTabBarAttached *attached)
    {
        return attached->d_func();
    }

    void update(QQuickTabBar *tabBar, int index);

    int index = -1;
    QQuickTabBar *tabBar = nullptr;
};

void QQuickTabBarPrivate::updateCurrentItem()
{
    QQuickTabButton *button = qobject_cast<QQuickTabButton *>(contentModel->get(currentIndex));
    if (button)
        button->setChecked(true);
}

void QQuickTabBarPrivate::updateCurrentIndex()
{
    Q_Q(QQuickTabBar);
    QQuickTabButton *button = qobject_cast<QQuickTabButton *>(q->sender());
    if (button && button->isChecked())
        q->setCurrentIndex(contentModel->indexOf(button, nullptr));
}

void QQuickTabBarPrivate::updateLayout()
{
    Q_Q(QQuickTabBar);
    const int count = contentModel->count();
    if (count <= 0 || !contentItem)
        return;

    qreal reservedWidth = 0;
    int resizableCount = 0;

    QVector<QQuickItem *> allItems;
    allItems.reserve(count);

    for (int i = 0; i < count; ++i) {
        QQuickItem *item = q->itemAt(i);
        if (item) {
            QQuickItemPrivate *p = QQuickItemPrivate::get(item);
            if (!p->widthValid)
                ++resizableCount;
            else
                reservedWidth += item->width();
            allItems += item;
        }
    }

    const qreal totalSpacing = qMax(0, count - 1) * spacing;
    const qreal itemWidth = (contentItem->width() - reservedWidth - totalSpacing) / qMax(1, resizableCount);

    updatingLayout = true;
    for (QQuickItem *item : qAsConst(allItems)) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(item);
        if (!p->widthValid) {
            item->setWidth(itemWidth);
            p->widthValid = false;
        }
        if (!p->heightValid) {
            item->setHeight(contentHeight);
            p->heightValid = false;
        } else {
            item->setY((contentHeight - item->height()) / 2);
        }
    }
    updatingLayout = false;
}

qreal QQuickTabBarPrivate::getContentWidth() const
{
    Q_Q(const QQuickTabBar);
    const int count = contentModel->count();
    qreal totalWidth = qMax(0, count - 1) * spacing;
    for (int i = 0; i < count; ++i) {
        QQuickItem *item = q->itemAt(i);
        if (item) {
            QQuickItemPrivate *p = QQuickItemPrivate::get(item);
            if (!p->widthValid)
                totalWidth += item->implicitWidth();
            else
                totalWidth += item->width();
        }
    }
    return totalWidth;
}

qreal QQuickTabBarPrivate::getContentHeight() const
{
    Q_Q(const QQuickTabBar);
    const int count = contentModel->count();
    qreal maxHeight = 0;
    for (int i = 0; i < count; ++i) {
        QQuickItem *item = q->itemAt(i);
        if (item)
            maxHeight = qMax(maxHeight, item->implicitHeight());
    }
    return maxHeight;
}

void QQuickTabBarPrivate::itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &diff)
{
    QQuickContainerPrivate::itemGeometryChanged(item, change, diff);
    if (!updatingLayout) {
        if (change.sizeChange())
            updateImplicitContentSize();
        updateLayout();
    }
}

void QQuickTabBarPrivate::itemImplicitWidthChanged(QQuickItem *item)
{
    QQuickContainerPrivate::itemImplicitWidthChanged(item);
    if (item != contentItem)
        updateImplicitContentWidth();
}

void QQuickTabBarPrivate::itemImplicitHeightChanged(QQuickItem *item)
{
    QQuickContainerPrivate::itemImplicitHeightChanged(item);
    if (item != contentItem)
        updateImplicitContentHeight();
}

QQuickTabBar::QQuickTabBar(QQuickItem *parent)
    : QQuickContainer(*(new QQuickTabBarPrivate), parent)
{
    Q_D(QQuickTabBar);
    d->changeTypes |= QQuickItemPrivate::Geometry | QQuickItemPrivate::ImplicitWidth | QQuickItemPrivate::ImplicitHeight;
    setFlag(ItemIsFocusScope);
    QObjectPrivate::connect(this, &QQuickTabBar::currentIndexChanged, d, &QQuickTabBarPrivate::updateCurrentItem);
}

/*!
    \qmlproperty enumeration QtQuick.Controls::TabBar::position

    This property holds the position of the tab bar.

    \note If the tab bar is assigned as a header or footer of \l ApplicationWindow
    or \l Page, the appropriate position is set automatically.

    Possible values:
    \value TabBar.Header The tab bar is at the top, as a window or page header.
    \value TabBar.Footer The tab bar is at the bottom, as a window or page footer.

    The default value is style-specific.

    \sa ApplicationWindow::header, ApplicationWindow::footer, Page::header, Page::footer
*/
QQuickTabBar::Position QQuickTabBar::position() const
{
    Q_D(const QQuickTabBar);
    return d->position;
}

void QQuickTabBar::setPosition(Position position)
{
    Q_D(QQuickTabBar);
    if (d->position == position)
        return;

    d->position = position;
    emit positionChanged();
}

/*!
    \since QtQuick.Controls 2.2 (Qt 5.9)
    \qmlproperty real QtQuick.Controls::TabBar::contentWidth

    This property holds the content width. It is used for calculating the total
    implicit width of the tab bar.

    \note This property is available in TabBar since QtQuick.Controls 2.2 (Qt 5.9),
    but it was promoted to the Container base type in QtQuick.Controls 2.5 (Qt 5.12).

    \sa Container::contentWidth
*/

/*!
    \since QtQuick.Controls 2.2 (Qt 5.9)
    \qmlproperty real QtQuick.Controls::TabBar::contentHeight

    This property holds the content height. It is used for calculating the total
    implicit height of the tab bar.

    \note This property is available in TabBar since QtQuick.Controls 2.2 (Qt 5.9),
    but it was promoted to the Container base type in QtQuick.Controls 2.5 (Qt 5.12).

    \sa Container::contentHeight
*/

QQuickTabBarAttached *QQuickTabBar::qmlAttachedProperties(QObject *object)
{
    return new QQuickTabBarAttached(object);
}

void QQuickTabBar::updatePolish()
{
    Q_D(QQuickTabBar);
    QQuickContainer::updatePolish();
    d->updateLayout();
}

void QQuickTabBar::componentComplete()
{
    Q_D(QQuickTabBar);
    QQuickContainer::componentComplete();
    d->updateCurrentItem();
    d->updateLayout();
}

void QQuickTabBar::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickTabBar);
    QQuickContainer::geometryChanged(newGeometry, oldGeometry);
    d->updateLayout();
}

bool QQuickTabBar::isContent(QQuickItem *item) const
{
    return qobject_cast<QQuickTabButton *>(item);
}

void QQuickTabBar::itemAdded(int index, QQuickItem *item)
{
    Q_D(QQuickTabBar);
    Q_UNUSED(index);
    QQuickItemPrivate::get(item)->setCulled(true); // QTBUG-55129
    if (QQuickTabButton *button = qobject_cast<QQuickTabButton *>(item))
        QObjectPrivate::connect(button, &QQuickTabButton::checkedChanged, d, &QQuickTabBarPrivate::updateCurrentIndex);
    QQuickTabBarAttached *attached = qobject_cast<QQuickTabBarAttached *>(qmlAttachedPropertiesObject<QQuickTabBar>(item));
    if (attached)
        QQuickTabBarAttachedPrivate::get(attached)->update(this, index);
    d->updateImplicitContentSize();
    if (isComponentComplete())
        polish();
}

void QQuickTabBar::itemMoved(int index, QQuickItem *item)
{
    QQuickTabBarAttached *attached = qobject_cast<QQuickTabBarAttached *>(qmlAttachedPropertiesObject<QQuickTabBar>(item));
    if (attached)
        QQuickTabBarAttachedPrivate::get(attached)->update(this, index);
}

void QQuickTabBar::itemRemoved(int index, QQuickItem *item)
{
    Q_D(QQuickTabBar);
    Q_UNUSED(index);
    if (QQuickTabButton *button = qobject_cast<QQuickTabButton *>(item))
        QObjectPrivate::disconnect(button, &QQuickTabButton::checkedChanged, d, &QQuickTabBarPrivate::updateCurrentIndex);
    QQuickTabBarAttached *attached = qobject_cast<QQuickTabBarAttached *>(qmlAttachedPropertiesObject<QQuickTabBar>(item));
    if (attached)
        QQuickTabBarAttachedPrivate::get(attached)->update(nullptr, -1);
    d->updateImplicitContentSize();
    if (isComponentComplete())
        polish();
}

QFont QQuickTabBar::defaultFont() const
{
    return QQuickTheme::font(QQuickTheme::TabBar);
}

QPalette QQuickTabBar::defaultPalette() const
{
    return QQuickTheme::palette(QQuickTheme::TabBar);
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickTabBar::accessibleRole() const
{
    return QAccessible::PageTabList;
}
#endif

/*!
    \qmlattachedproperty int QtQuick.Controls::TabBar::index
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \readonly

    This attached property holds the index of each tab button in the TabBar.

    It is attached to each tab button of the TabBar.
*/

/*!
    \qmlattachedproperty TabBar QtQuick.Controls::TabBar::tabBar
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \readonly

    This attached property holds the tab bar that manages this tab button.

    It is attached to each tab button of the TabBar.
*/

/*!
    \qmlattachedproperty enumeration QtQuick.Controls::TabBar::position
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \readonly

    This attached property holds the position of the tab bar.

    It is attached to each tab button of the TabBar.

    Possible values:
    \value TabBar.Header The tab bar is at the top, as a window or page header.
    \value TabBar.Footer The tab bar is at the bottom, as a window or page footer.
*/

void QQuickTabBarAttachedPrivate::update(QQuickTabBar *newTabBar, int newIndex)
{
    Q_Q(QQuickTabBarAttached);
    const int oldIndex = index;
    const QQuickTabBar *oldTabBar = tabBar;
    const QQuickTabBar::Position oldPos = q->position();

    index = newIndex;
    tabBar = newTabBar;

    if (oldTabBar != newTabBar) {
        if (oldTabBar)
            QObject::disconnect(oldTabBar, &QQuickTabBar::positionChanged, q, &QQuickTabBarAttached::positionChanged);
        if (newTabBar)
            QObject::connect(newTabBar, &QQuickTabBar::positionChanged, q, &QQuickTabBarAttached::positionChanged);
        emit q->tabBarChanged();
    }

    if (oldIndex != newIndex)
        emit q->indexChanged();
    if (oldPos != q->position())
        emit q->positionChanged();
}

QQuickTabBarAttached::QQuickTabBarAttached(QObject *parent)
    : QObject(*(new QQuickTabBarAttachedPrivate), parent)
{
}

int QQuickTabBarAttached::index() const
{
    Q_D(const QQuickTabBarAttached);
    return d->index;
}

QQuickTabBar *QQuickTabBarAttached::tabBar() const
{
    Q_D(const QQuickTabBarAttached);
    return d->tabBar;
}

QQuickTabBar::Position QQuickTabBarAttached::position() const
{
    Q_D(const QQuickTabBarAttached);
    if (!d->tabBar)
        return QQuickTabBar::Header;
    return d->tabBar->position();
}

QT_END_NAMESPACE
