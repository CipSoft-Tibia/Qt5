// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickapplicationwindow_p.h"
#include "qquickcontentitem_p.h"
#include "qquickpopup_p_p.h"
#include "qquickcontrol_p_p.h"
#include "qquicktextarea_p.h"
#include "qquicktextfield_p.h"
#include "qquicktoolbar_p.h"
#include "qquicktooltip_p.h"
#include <private/qtquicktemplates2-config_p.h>
#if QT_CONFIG(quicktemplates2_container)
#include "qquicktabbar_p.h"
#include "qquickdialogbuttonbox_p.h"
#endif
#include "qquickdeferredexecute_p_p.h"
#include "qquickdeferredpointer_p_p.h"

#include <QtCore/private/qobject_p.h>
#include <QtCore/qscopedvaluerollback.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtQuick/private/qquickwindowmodule_p_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ApplicationWindow
    \inherits Window
//!     \instantiates QQuickApplicationWindow
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-containers
    \ingroup qtquickcontrols-focusscopes
    \brief Styled top-level window with support for a header and footer.

    ApplicationWindow is a \l Window which makes it convenient to add
    a \l {menuBar}{menu bar}, \l header and \l footer item to the window.

    You can declare ApplicationWindow as the root item of your application,
    and run it by using \l QQmlApplicationEngine.  In this way you can control
    the window's properties, appearance and layout from QML.

    \image qtquickcontrols-applicationwindow-wireframe.png

    \qml
    import QtQuick.Controls 2.12

    ApplicationWindow {
        visible: true

        menuBar: MenuBar {
            // ...
        }

        header: ToolBar {
            // ...
        }

        footer: TabBar {
            // ...
        }

        StackView {
            anchors.fill: parent
        }
    }
    \endqml

    \note By default, an ApplicationWindow is not visible.

    \section2 Attached ApplicationWindow Properties

    Due to how \l {Scope and Naming Resolution} works in QML, it is possible
    to reference the \c id of the application root element anywhere in its
    child QML objects. Even though this approach is fine for many applications
    and use cases, for a generic QML component it may not be acceptable as it
    creates a dependency to the surrounding environment.

    ApplicationWindow provides a set of attached properties that can be used
    to access the window and its building blocks from places where no direct
    access to the window is available, without creating a dependency to a
    certain window \c id. A QML component that uses the ApplicationWindow
    attached properties works in any window regardless of its \c id.

    \sa {Customizing ApplicationWindow}, Overlay, Page, {Container Controls},
        {Focus Management in Qt Quick Controls}
*/

static const QQuickItemPrivate::ChangeTypes ItemChanges = QQuickItemPrivate::Visibility
        | QQuickItemPrivate::Geometry | QQuickItemPrivate::ImplicitWidth | QQuickItemPrivate::ImplicitHeight;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickApplicationWindowPrivate
    : public QQuickWindowQmlImplPrivate
    , public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickApplicationWindow)

public:
    static QQuickApplicationWindowPrivate *get(QQuickApplicationWindow *window)
    {
        return window->d_func();
    }

    QQmlListProperty<QObject> contentData();

    void updateHasBackgroundFlags();
    void relayout();

    void itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &diff) override;
    void itemVisibilityChanged(QQuickItem *item) override;
    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;
    QPalette windowPalette() const override { return defaultPalette(); }

    void updateFont(const QFont &f);
    inline void setFont_helper(const QFont &f) {
        if (font.resolveMask() == f.resolveMask() && font == f)
            return;
        updateFont(f);
    }
    void resolveFont();

    void _q_updateActiveFocus();
    void setActiveFocusControl(QQuickItem *item);

    static void contentData_append(QQmlListProperty<QObject> *prop, QObject *obj);

    void cancelBackground();
    void executeBackground(bool complete = false);

    QPalette defaultPalette() const override { return QQuickTheme::palette(QQuickTheme::System); }
    void updateChildrenPalettes(const QPalette &parentPalette) override
    {
        // Update regular children
        QQuickWindowPrivate::updateChildrenPalettes(parentPalette);

        // And cover special cases
        for (auto &&child : q_func()->findChildren<QObject *>()) {
            if (auto *popup = qobject_cast<QQuickPopup *>(child))
                QQuickPopupPrivate::get(popup)->updateContentPalettes(parentPalette);
            else if (auto *toolTipAttached = qobject_cast<QQuickToolTipAttached *>(child)) {
                if (auto *toolTip = toolTipAttached->toolTip())
                    QQuickPopupPrivate::get(toolTip)->updateContentPalettes(parentPalette);
            }
        }
    }

    QQuickDeferredPointer<QQuickItem> background;
    QQuickItem *appWindowContentItem = nullptr;
    QQuickItem *menuBar = nullptr;
    QQuickItem *header = nullptr;
    QQuickItem *footer = nullptr;
    QFont font;
    QLocale locale;
    QQuickItem *activeFocusControl = nullptr;
    bool insideRelayout = false;
    bool hasBackgroundWidth = false;
    bool hasBackgroundHeight = false;
};

static void layoutItem(QQuickItem *item, qreal y, qreal width)
{
    if (!item)
        return;

    item->setY(y);
    QQuickItemPrivate *p = QQuickItemPrivate::get(item);
    if (!p->widthValid()) {
        item->setWidth(width);
        p->widthValidFlag = false;
    }
}

void QQuickApplicationWindowPrivate::updateHasBackgroundFlags()
{
    if (!background)
        return;

    QQuickItemPrivate *backgroundPrivate = QQuickItemPrivate::get(background);
    hasBackgroundWidth = backgroundPrivate->widthValid();
    hasBackgroundHeight = backgroundPrivate->heightValid();
}

void QQuickApplicationWindowPrivate::relayout()
{
    Q_Q(QQuickApplicationWindow);
    if (!componentComplete || insideRelayout)
        return;

    QScopedValueRollback<bool> guard(insideRelayout, true);
    QQuickItem *content = q->contentItem();
    qreal hh = header && header->isVisible() ? header->height() : 0;
    qreal fh = footer && footer->isVisible() ? footer->height() : 0;
    qreal mbh = menuBar && menuBar->isVisible() ? menuBar->height() : 0;

    content->setY(mbh + hh);
    content->setWidth(q->width());
    content->setHeight(q->height() - mbh - hh - fh);

    layoutItem(menuBar, -mbh - hh, q->width());
    layoutItem(header, -hh, q->width());
    layoutItem(footer, content->height(), q->width());

    if (background) {
        if (!hasBackgroundWidth && qFuzzyIsNull(background->x()))
            background->setWidth(q->width());
        if (!hasBackgroundHeight && qFuzzyIsNull(background->y()))
            background->setHeight(q->height());
    }
}

void QQuickApplicationWindowPrivate::itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &diff)
{
    Q_UNUSED(diff);

    if (!insideRelayout && item == background && change.sizeChange()) {
        // Any time the background is resized (excluding our own resizing),
        // we should respect it if it's explicit by storing the values of the flags.
        updateHasBackgroundFlags();
    }

    relayout();
}

void QQuickApplicationWindowPrivate::itemVisibilityChanged(QQuickItem *item)
{
    Q_UNUSED(item);
    relayout();
}

void QQuickApplicationWindowPrivate::itemImplicitWidthChanged(QQuickItem *item)
{
    Q_UNUSED(item);
    relayout();
}

void QQuickApplicationWindowPrivate::itemImplicitHeightChanged(QQuickItem *item)
{
    Q_UNUSED(item);
    relayout();
}

void QQuickApplicationWindowPrivate::updateFont(const QFont &f)
{
    Q_Q(QQuickApplicationWindow);
    const bool changed = font != f;
    font = f;

    QQuickControlPrivate::updateFontRecur(q->QQuickWindow::contentItem(), f);

    const QList<QQuickPopup *> popups = q->findChildren<QQuickPopup *>();
    for (QQuickPopup *popup : popups)
        QQuickControlPrivate::get(static_cast<QQuickControl *>(popup->popupItem()))->inheritFont(f);

    if (changed)
        emit q->fontChanged();
}

void QQuickApplicationWindowPrivate::resolveFont()
{
    QFont resolvedFont = font.resolve(QQuickTheme::font(QQuickTheme::System));
    setFont_helper(resolvedFont);
}

static QQuickItem *findActiveFocusControl(QQuickWindow *window)
{
    QQuickItem *item = window->activeFocusItem();
    while (item) {
        if (qobject_cast<QQuickControl *>(item) || qobject_cast<QQuickTextField *>(item) || qobject_cast<QQuickTextArea *>(item))
            return item;
        item = item->parentItem();
    }
    return item;
}

void QQuickApplicationWindowPrivate::_q_updateActiveFocus()
{
    Q_Q(QQuickApplicationWindow);
    setActiveFocusControl(findActiveFocusControl(q));
}

void QQuickApplicationWindowPrivate::setActiveFocusControl(QQuickItem *control)
{
    Q_Q(QQuickApplicationWindow);
    if (activeFocusControl != control) {
        activeFocusControl = control;
        emit q->activeFocusControlChanged();
    }
}

void QQuickApplicationWindowPrivate::contentData_append(QQmlListProperty<QObject> *prop, QObject *obj)
{
    QQuickItemPrivate::data_append(prop, obj);

    // associate "top-level" popups with the window as soon as they are added to the default property
    if (QQuickPopup *popup = qobject_cast<QQuickPopup *>(obj))
        QQuickPopupPrivate::get(popup)->setWindow(static_cast<QQuickApplicationWindow *>(prop->data));
}

void QQuickApplicationWindowPrivate::cancelBackground()
{
    Q_Q(QQuickApplicationWindow);
    quickCancelDeferred(q, backgroundName());
}

void QQuickApplicationWindowPrivate::executeBackground(bool complete)
{
    Q_Q(QQuickApplicationWindow);
    if (background.wasExecuted())
        return;

    if (!background || complete)
        quickBeginDeferred(q, backgroundName(), background);
    if (complete) {
        quickCompleteDeferred(q, backgroundName(), background);
        // See comment in setBackground for why we do this here.
        updateHasBackgroundFlags();
        relayout();
    }
}

QQuickApplicationWindow::QQuickApplicationWindow(QWindow *parent)
    : QQuickWindowQmlImpl(*(new QQuickApplicationWindowPrivate), parent)
{
    connect(this, SIGNAL(activeFocusItemChanged()), this, SLOT(_q_updateActiveFocus()));
}

QQuickApplicationWindow::~QQuickApplicationWindow()
{
    Q_D(QQuickApplicationWindow);
    d->setActiveFocusControl(nullptr);
    disconnect(this, SIGNAL(activeFocusItemChanged()), this, SLOT(_q_updateActiveFocus()));
    if (d->menuBar)
        QQuickItemPrivate::get(d->menuBar)->removeItemChangeListener(d, ItemChanges);
    if (d->header)
        QQuickItemPrivate::get(d->header)->removeItemChangeListener(d, ItemChanges);
    if (d->footer)
        QQuickItemPrivate::get(d->footer)->removeItemChangeListener(d, ItemChanges);
}

QQuickApplicationWindowAttached *QQuickApplicationWindow::qmlAttachedProperties(QObject *object)
{
    return new QQuickApplicationWindowAttached(object);
}

/*!
    \qmlproperty Item QtQuick.Controls::ApplicationWindow::background

    This property holds the background item.

    The background item is stacked under the \l {contentItem}{content item},
    but above the \l {Window::color}{background color} of the window.

    The background item is useful for images and gradients, for example,
    but the \l {Window::}{color} property is preferable for solid colors,
    as it doesn't need to create an item.

    \note If the background item has no explicit size specified, it automatically
          follows the control's size. In most cases, there is no need to specify
          width or height for a background item.

    \sa {Customizing ApplicationWindow}, contentItem, header, footer
*/
QQuickItem *QQuickApplicationWindow::background() const
{
    QQuickApplicationWindowPrivate *d = const_cast<QQuickApplicationWindowPrivate *>(d_func());
    if (!d->background)
        d->executeBackground();
    return d->background;
}

void QQuickApplicationWindow::setBackground(QQuickItem *background)
{
    Q_D(QQuickApplicationWindow);
    if (d->background == background)
        return;

    if (!d->background.isExecuting())
        d->cancelBackground();

    if (d->background) {
        d->hasBackgroundWidth = false;
        d->hasBackgroundHeight = false;
    }
    QQuickControlPrivate::hideOldItem(d->background);

    d->background = background;

    if (background) {
        background->setParentItem(QQuickWindow::contentItem());

        if (qFuzzyIsNull(background->z()))
            background->setZ(-1);

        // If the background hasn't finished executing then we don't know if its width and height
        // are valid or not, and so relayout would see that they haven't been set yet and override
        // any bindings the user might have.
        if (!d->background.isExecuting()) {
            d->updateHasBackgroundFlags();

            if (isComponentComplete())
                d->relayout();
        }
    }
    if (!d->background.isExecuting())
        emit backgroundChanged();
}

/*!
    \qmlproperty Item QtQuick.Controls::ApplicationWindow::header

    This property holds the window header item. The header item is positioned at the
    top of the window, below the menu bar, and resized to the width of the window.
    The default value is \c null.

    \code
    ApplicationWindow {
        header: TabBar {
            // ...
        }
    }
    \endcode

    \note Assigning a ToolBar, TabBar, or DialogButtonBox as a window header
    automatically sets the respective \l ToolBar::position, \l TabBar::position,
    or \l DialogButtonBox::position property to \c Header.

    \sa menuBar, footer, Page::header
*/
QQuickItem *QQuickApplicationWindow::header() const
{
    Q_D(const QQuickApplicationWindow);
    return d->header;
}

void QQuickApplicationWindow::setHeader(QQuickItem *header)
{
    Q_D(QQuickApplicationWindow);
    if (d->header == header)
        return;

    if (d->header) {
        QQuickItemPrivate::get(d->header)->removeItemChangeListener(d, ItemChanges);
        d->header->setParentItem(nullptr);
    }
    d->header = header;
    if (header) {
        header->setParentItem(contentItem());
        QQuickItemPrivate *p = QQuickItemPrivate::get(header);
        p->addItemChangeListener(d, ItemChanges);
        if (qFuzzyIsNull(header->z()))
            header->setZ(1);
        if (QQuickToolBar *toolBar = qobject_cast<QQuickToolBar *>(header))
            toolBar->setPosition(QQuickToolBar::Header);
#if QT_CONFIG(quicktemplates2_container)
        else if (QQuickTabBar *tabBar = qobject_cast<QQuickTabBar *>(header))
            tabBar->setPosition(QQuickTabBar::Header);
        else if (QQuickDialogButtonBox *buttonBox = qobject_cast<QQuickDialogButtonBox *>(header))
            buttonBox->setPosition(QQuickDialogButtonBox::Header);
#endif
    }
    if (isComponentComplete())
        d->relayout();
    emit headerChanged();
}

/*!
    \qmlproperty Item QtQuick.Controls::ApplicationWindow::footer

    This property holds the window footer item. The footer item is positioned to
    the bottom, and resized to the width of the window. The default value is \c null.

    \code
    ApplicationWindow {
        footer: ToolBar {
            // ...
        }
    }
    \endcode

    \note Assigning a ToolBar, TabBar, or DialogButtonBox as a window footer
    automatically sets the respective \l ToolBar::position, \l TabBar::position,
    or \l DialogButtonBox::position property to \c Footer.

    \sa menuBar, header, Page::footer
*/
QQuickItem *QQuickApplicationWindow::footer() const
{
    Q_D(const QQuickApplicationWindow);
    return d->footer;
}

void QQuickApplicationWindow::setFooter(QQuickItem *footer)
{
    Q_D(QQuickApplicationWindow);
    if (d->footer == footer)
        return;

    if (d->footer) {
        QQuickItemPrivate::get(d->footer)->removeItemChangeListener(d, ItemChanges);
        d->footer->setParentItem(nullptr);
    }
    d->footer = footer;
    if (footer) {
        footer->setParentItem(contentItem());
        QQuickItemPrivate *p = QQuickItemPrivate::get(footer);
        p->addItemChangeListener(d, ItemChanges);
        if (qFuzzyIsNull(footer->z()))
            footer->setZ(1);
        if (QQuickToolBar *toolBar = qobject_cast<QQuickToolBar *>(footer))
            toolBar->setPosition(QQuickToolBar::Footer);
#if QT_CONFIG(quicktemplates2_container)
        else if (QQuickTabBar *tabBar = qobject_cast<QQuickTabBar *>(footer))
            tabBar->setPosition(QQuickTabBar::Footer);
        else if (QQuickDialogButtonBox *buttonBox = qobject_cast<QQuickDialogButtonBox *>(footer))
            buttonBox->setPosition(QQuickDialogButtonBox::Footer);
#endif
    }
    if (isComponentComplete())
        d->relayout();
    emit footerChanged();
}

/*!
    \qmlproperty list<QtObject> QtQuick.Controls::ApplicationWindow::contentData
    \qmldefault

    This default property holds the list of all objects declared as children of
    the window.

    The data property allows you to freely mix visual children, resources and
    other windows in an ApplicationWindow.

    If you assign an Item to the contentData list, it becomes a child of the
    window's contentItem, so that it appears inside the window. The item's
    parent will be the window's \l contentItem.

    It should not generally be necessary to refer to the contentData property,
    as it is the default property for ApplicationWindow and thus all child
    items are automatically assigned to this property.

    \sa contentItem
*/
QQmlListProperty<QObject> QQuickApplicationWindowPrivate::contentData()
{
    Q_Q(QQuickApplicationWindow);
    return QQmlListProperty<QObject>(q->contentItem(), q,
                                     QQuickApplicationWindowPrivate::contentData_append,
                                     QQuickItemPrivate::data_count,
                                     QQuickItemPrivate::data_at,
                                     QQuickItemPrivate::data_clear);
}

/*!
    \qmlproperty Item QtQuick.Controls::ApplicationWindow::contentItem
    \readonly

    This property holds the window content item.

    The content item is stacked above the \l background item, and under the
    \l menuBar, \l header, and \l footer items.

    \sa background, menuBar, header, footer
*/
QQuickItem *QQuickApplicationWindow::contentItem() const
{
    QQuickApplicationWindowPrivate *d = const_cast<QQuickApplicationWindowPrivate *>(d_func());
    if (!d->appWindowContentItem) {
        d->appWindowContentItem = new QQuickContentItem(this, QQuickWindow::contentItem());
        d->appWindowContentItem->setFlag(QQuickItem::ItemIsFocusScope);
        d->appWindowContentItem->setFocus(true);
        d->relayout();
    }
    return d->appWindowContentItem;
}

/*!
    \qmlproperty Control QtQuick.Controls::ApplicationWindow::activeFocusControl
    \readonly

    This property holds the control that currently has active focus, or \c null if there is
    no control with active focus.

    The difference between \l Window::activeFocusItem and ApplicationWindow::activeFocusControl
    is that the former may point to a building block of a control, whereas the latter points
    to the enclosing control. For example, when SpinBox has focus, activeFocusItem points to
    the editor and activeFocusControl to the SpinBox itself.

    \sa Window::activeFocusItem
*/
QQuickItem *QQuickApplicationWindow::activeFocusControl() const
{
    Q_D(const QQuickApplicationWindow);
    return d->activeFocusControl;
}

/*!
    \qmlproperty font QtQuick.Controls::ApplicationWindow::font

    This property holds the font currently set for the window.

    The default font depends on the system environment. QGuiApplication maintains a system/theme
    font which serves as a default for all application windows. You can also set the default font
    for windows by passing a custom font to QGuiApplication::setFont(), before loading any QML.
    Finally, the font is matched against Qt's font database to find the best match.

    ApplicationWindow propagates explicit font properties to child controls. If you change a specific
    property on the window's font, that property propagates to all child controls in the window,
    overriding any system defaults for that property.

    \sa Control::font
*/
QFont QQuickApplicationWindow::font() const
{
    Q_D(const QQuickApplicationWindow);
    return d->font;
}

void QQuickApplicationWindow::setFont(const QFont &font)
{
    Q_D(QQuickApplicationWindow);
    if (d->font.resolveMask() == font.resolveMask() && d->font == font)
        return;

    QFont resolvedFont = font.resolve(QQuickTheme::font(QQuickTheme::System));
    d->setFont_helper(resolvedFont);
}

void QQuickApplicationWindow::resetFont()
{
    setFont(QFont());
}

/*!
    \qmlproperty Locale QtQuick.Controls::ApplicationWindow::locale

    This property holds the locale of the window.

    The default locale depends on the system environment. You can set the
    default locale by calling QLocale::setDefault(), before loading any QML.

    ApplicationWindow propagates the locale to child controls. If you change
    the window's locale, that locale propagates to all child controls in the
    window, overriding the system default locale.

    \sa Control::locale
*/
QLocale QQuickApplicationWindow::locale() const
{
    Q_D(const QQuickApplicationWindow);
    return d->locale;
}

void QQuickApplicationWindow::setLocale(const QLocale &locale)
{
    Q_D(QQuickApplicationWindow);
    if (d->locale == locale)
        return;

    d->locale = locale;
    QQuickControlPrivate::updateLocaleRecur(QQuickWindow::contentItem(), locale);

    // TODO: internal QQuickPopupManager that provides reliable access to all QQuickPopup instances
    const QList<QQuickPopup *> popups = QQuickWindow::contentItem()->findChildren<QQuickPopup *>();
    for (QQuickPopup *popup : popups)
        QQuickControlPrivate::get(static_cast<QQuickControl *>(popup->popupItem()))->updateLocale(locale, false); // explicit=false

    emit localeChanged();
}

void QQuickApplicationWindow::resetLocale()
{
    setLocale(QLocale());
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty Item QtQuick.Controls::ApplicationWindow::menuBar

    This property holds the window menu bar. The menu bar is positioned at the
    top of the window, above the header, and resized to the width of the window.
    The default value is \c null.

    \code
    ApplicationWindow {
        menuBar: MenuBar {
            // ...
        }
    }
    \endcode

    \sa header, footer, MenuBar
*/
QQuickItem *QQuickApplicationWindow::menuBar() const
{
    Q_D(const QQuickApplicationWindow);
    return d->menuBar;
}

void QQuickApplicationWindow::setMenuBar(QQuickItem *menuBar)
{
    Q_D(QQuickApplicationWindow);
    if (d->menuBar == menuBar)
        return;

    if (d->menuBar) {
        QQuickItemPrivate::get(d->menuBar)->removeItemChangeListener(d, ItemChanges);
        d->menuBar->setParentItem(nullptr);
    }
    d->menuBar = menuBar;
    if (menuBar) {
        menuBar->setParentItem(contentItem());
        QQuickItemPrivate *p = QQuickItemPrivate::get(menuBar);
        p->addItemChangeListener(d, ItemChanges);
        if (qFuzzyIsNull(menuBar->z()))
            menuBar->setZ(2);
    }
    if (isComponentComplete())
        d->relayout();
    emit menuBarChanged();
}

bool QQuickApplicationWindow::isComponentComplete() const
{
    Q_D(const QQuickApplicationWindow);
    return d->componentComplete;
}

void QQuickApplicationWindow::classBegin()
{
    Q_D(QQuickApplicationWindow);
    d->componentComplete = false;
    QQuickWindowQmlImpl::classBegin();
    d->resolveFont();
}

void QQuickApplicationWindow::componentComplete()
{
    Q_D(QQuickApplicationWindow);
    d->componentComplete = true;
    d->executeBackground(true);
    QQuickWindowQmlImpl::componentComplete();
    d->relayout();
}

void QQuickApplicationWindow::resizeEvent(QResizeEvent *event)
{
    Q_D(QQuickApplicationWindow);
    QQuickWindowQmlImpl::resizeEvent(event);
    d->relayout();
}

class QQuickApplicationWindowAttachedPrivate : public QObjectPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickApplicationWindowAttached)

    void windowChange(QQuickWindow *wnd);
    void activeFocusChange();

    QQuickWindow *window = nullptr;
    QQuickItem *activeFocusControl = nullptr;
};

void QQuickApplicationWindowAttachedPrivate::windowChange(QQuickWindow *wnd)
{
    Q_Q(QQuickApplicationWindowAttached);
    if (window == wnd)
        return;

    QQuickApplicationWindow *oldWindow = qobject_cast<QQuickApplicationWindow *>(window);
    if (oldWindow && !QQuickApplicationWindowPrivate::get(oldWindow))
        oldWindow = nullptr; // being deleted (QTBUG-52731)

    if (oldWindow) {
        disconnect(oldWindow, &QQuickApplicationWindow::activeFocusControlChanged,
                   this, &QQuickApplicationWindowAttachedPrivate::activeFocusChange);
        QObject::disconnect(oldWindow, &QQuickApplicationWindow::menuBarChanged,
                            q, &QQuickApplicationWindowAttached::menuBarChanged);
        QObject::disconnect(oldWindow, &QQuickApplicationWindow::headerChanged,
                            q, &QQuickApplicationWindowAttached::headerChanged);
        QObject::disconnect(oldWindow, &QQuickApplicationWindow::footerChanged,
                            q, &QQuickApplicationWindowAttached::footerChanged);
    } else if (window) {
        disconnect(window, &QQuickWindow::activeFocusItemChanged,
                   this, &QQuickApplicationWindowAttachedPrivate::activeFocusChange);
    }

    QQuickApplicationWindow *newWindow = qobject_cast<QQuickApplicationWindow *>(wnd);
    if (newWindow) {
        connect(newWindow, &QQuickApplicationWindow::activeFocusControlChanged,
                this, &QQuickApplicationWindowAttachedPrivate::activeFocusChange);
        QObject::connect(newWindow, &QQuickApplicationWindow::menuBarChanged,
                         q, &QQuickApplicationWindowAttached::menuBarChanged);
        QObject::connect(newWindow, &QQuickApplicationWindow::headerChanged,
                         q, &QQuickApplicationWindowAttached::headerChanged);
        QObject::connect(newWindow, &QQuickApplicationWindow::footerChanged,
                         q, &QQuickApplicationWindowAttached::footerChanged);
    } else if (wnd) {
        connect(wnd, &QQuickWindow::activeFocusItemChanged,
                this, &QQuickApplicationWindowAttachedPrivate::activeFocusChange);
    }

    window = wnd;
    emit q->windowChanged();
    emit q->contentItemChanged();

    activeFocusChange();
    if ((oldWindow && oldWindow->menuBar()) || (newWindow && newWindow->menuBar()))
        emit q->menuBarChanged();
    if ((oldWindow && oldWindow->header()) || (newWindow && newWindow->header()))
        emit q->headerChanged();
    if ((oldWindow && oldWindow->footer()) || (newWindow && newWindow->footer()))
        emit q->footerChanged();
}

void QQuickApplicationWindowAttachedPrivate::activeFocusChange()
{
    Q_Q(QQuickApplicationWindowAttached);
    QQuickItem *control = nullptr;
    if (QQuickApplicationWindow *appWindow = qobject_cast<QQuickApplicationWindow *>(window))
        control = appWindow->activeFocusControl();
    else if (window)
        control = findActiveFocusControl(window);
    if (activeFocusControl == control)
        return;

    activeFocusControl = control;
    emit q->activeFocusControlChanged();
}

QQuickApplicationWindowAttached::QQuickApplicationWindowAttached(QObject *parent)
    : QObject(*(new QQuickApplicationWindowAttachedPrivate), parent)
{
    Q_D(QQuickApplicationWindowAttached);
    if (QQuickItem *item = qobject_cast<QQuickItem *>(parent)) {
        d->windowChange(item->window());
        QObjectPrivate::connect(item, &QQuickItem::windowChanged, d, &QQuickApplicationWindowAttachedPrivate::windowChange);
        if (!d->window) {
            QQuickItem *p = item;
            while (p) {
                if (QQuickPopup *popup = qobject_cast<QQuickPopup *>(p->parent())) {
                    d->windowChange(popup->window());
                    QObjectPrivate::connect(popup, &QQuickPopup::windowChanged, d, &QQuickApplicationWindowAttachedPrivate::windowChange);
                }
                p = p->parentItem();
            }
        }
    } else if (QQuickPopup *popup = qobject_cast<QQuickPopup *>(parent)) {
        d->windowChange(popup->window());
        QObjectPrivate::connect(popup, &QQuickPopup::windowChanged, d, &QQuickApplicationWindowAttachedPrivate::windowChange);
    }
}

/*!
    \qmlattachedproperty ApplicationWindow QtQuick.Controls::ApplicationWindow::window
    \readonly

    This attached property holds the application window. The property can be attached
    to any item. The value is \c null if the item is not in an ApplicationWindow.

    \sa {Attached ApplicationWindow Properties}
*/
QQuickApplicationWindow *QQuickApplicationWindowAttached::window() const
{
    Q_D(const QQuickApplicationWindowAttached);
    return qobject_cast<QQuickApplicationWindow *>(d->window);
}

/*!
    \qmlattachedproperty Item QtQuick.Controls::ApplicationWindow::contentItem
    \readonly

    This attached property holds the window content item. The property can be attached
    to any item. The value is \c null if the item is not in an ApplicationWindow.

    \sa {Attached ApplicationWindow Properties}
*/
QQuickItem *QQuickApplicationWindowAttached::contentItem() const
{
    Q_D(const QQuickApplicationWindowAttached);
    if (QQuickApplicationWindow *window = qobject_cast<QQuickApplicationWindow *>(d->window))
        return window->contentItem();
    return nullptr;
}

/*!
    \qmlattachedproperty Control QtQuick.Controls::ApplicationWindow::activeFocusControl
    \readonly

    This attached property holds the control that currently has active focus, or \c null
    if there is no control with active focus. The property can be attached to any item.
    The value is \c null if the item is not in a window, or the window has no active focus.

    \sa Window::activeFocusItem, {Attached ApplicationWindow Properties}
*/
QQuickItem *QQuickApplicationWindowAttached::activeFocusControl() const
{
    Q_D(const QQuickApplicationWindowAttached);
    return d->activeFocusControl;
}

/*!
    \qmlattachedproperty Item QtQuick.Controls::ApplicationWindow::header
    \readonly

    This attached property holds the window header item. The property can be attached
    to any item. The value is \c null if the item is not in an ApplicationWindow, or
    the window has no header item.

    \sa {Attached ApplicationWindow Properties}
*/
QQuickItem *QQuickApplicationWindowAttached::header() const
{
    Q_D(const QQuickApplicationWindowAttached);
    if (QQuickApplicationWindow *window = qobject_cast<QQuickApplicationWindow *>(d->window))
        return window->header();
    return nullptr;
}

/*!
    \qmlattachedproperty Item QtQuick.Controls::ApplicationWindow::footer
    \readonly

    This attached property holds the window footer item. The property can be attached
    to any item. The value is \c null if the item is not in an ApplicationWindow, or
    the window has no footer item.

    \sa {Attached ApplicationWindow Properties}
*/
QQuickItem *QQuickApplicationWindowAttached::footer() const
{
    Q_D(const QQuickApplicationWindowAttached);
    if (QQuickApplicationWindow *window = qobject_cast<QQuickApplicationWindow *>(d->window))
        return window->footer();
    return nullptr;
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlattachedproperty Item QtQuick.Controls::ApplicationWindow::menuBar
    \readonly

    This attached property holds the window menu bar. The property can be attached
    to any item. The value is \c null if the item is not in an ApplicationWindow, or
    the window has no menu bar.

    \sa {Attached ApplicationWindow Properties}
*/
QQuickItem *QQuickApplicationWindowAttached::menuBar() const
{
    Q_D(const QQuickApplicationWindowAttached);
    if (QQuickApplicationWindow *window = qobject_cast<QQuickApplicationWindow *>(d->window))
        return window->menuBar();
    return nullptr;
}

QT_END_NAMESPACE

#include "moc_qquickapplicationwindow_p.cpp"
