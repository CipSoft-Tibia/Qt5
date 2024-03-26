// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickitem.h"

#include "qquickwindow.h"
#include "qquickrendercontrol.h"
#include <QtQml/qjsengine.h>
#include "qquickwindow_p.h"

#include "qquickevents_p_p.h"
#include "qquickscreen_p.h"

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlinfo.h>
#include <QtGui/qpen.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qstylehints.h>
#include <QtGui/private/qeventpoint_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qpointingdevice_p.h>
#include <QtGui/qinputmethod.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/private/qnumeric_p.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/private/qduplicatetracker_p.h>

#include <private/qqmlglobal_p.h>
#include <private/qqmlengine_p.h>
#include <QtQuick/private/qquickstategroup_p.h>
#include <private/qqmlopenmetaobject_p.h>
#include <QtQuick/private/qquickstate_p.h>
#include <private/qquickitem_p.h>
#include <QtQuick/private/qquickaccessibleattached_p.h>
#include <QtQuick/private/qquickhoverhandler_p.h>
#include <QtQuick/private/qquickpointerhandler_p.h>
#include <QtQuick/private/qquickpointerhandler_p_p.h>

#include <private/qv4engine_p.h>
#include <private/qv4object_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qdebug_p.h>
#include <private/qqmlvaluetypewrapper_p.h>

#if QT_CONFIG(cursor)
# include <QtGui/qcursor.h>
#endif

#include <algorithm>
#include <limits>

// XXX todo Check that elements that create items handle memory correctly after visual ownership change

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcMouseTarget)
Q_DECLARE_LOGGING_CATEGORY(lcHoverTrace)
Q_DECLARE_LOGGING_CATEGORY(lcPtr)
Q_DECLARE_LOGGING_CATEGORY(lcTransient)
Q_LOGGING_CATEGORY(lcHandlerParent, "qt.quick.handler.parent")
Q_LOGGING_CATEGORY(lcVP, "qt.quick.viewport")
Q_LOGGING_CATEGORY(lcChangeListeners, "qt.quick.item.changelisteners")

// after 100ms, a mouse/non-mouse cursor conflict is resolved in favor of the mouse handler
static const quint64 kCursorOverrideTimeout = 100;

void debugFocusTree(QQuickItem *item, QQuickItem *scope = nullptr, int depth = 1)
{
    if (lcFocus().isEnabled(QtDebugMsg)) {
        qCDebug(lcFocus)
                << QByteArray(depth, '\t').constData()
                << (scope && QQuickItemPrivate::get(scope)->subFocusItem == item ? '*' : ' ')
                << item->hasFocus()
                << item->hasActiveFocus()
                << item->isFocusScope()
                << item;
        const auto childItems = item->childItems();
        for (QQuickItem *child : childItems) {
            debugFocusTree(
                    child,
                    item->isFocusScope() || !scope ? item : scope,
                    item->isFocusScope() || !scope ? depth + 1 : depth);
        }
    }
}

/*!
    \qmltype Transform
    \instantiates QQuickTransform
    \inqmlmodule QtQuick
    \ingroup qtquick-visual-transforms
    \brief For specifying advanced transformations on Items.

    The Transform type is a base type which cannot be instantiated directly.
    The following concrete Transform types are available:

    \list
    \li \l Rotation
    \li \l Scale
    \li \l Translate
    \li \l Matrix4x4
    \endlist

    The Transform types let you create and control advanced transformations that can be configured
    independently using specialized properties.

    You can assign any number of Transforms to an \l Item. Each Transform is applied in order,
    one at a time.
*/
QQuickTransformPrivate::QQuickTransformPrivate()
{
}

QQuickTransform::QQuickTransform(QObject *parent)
: QObject(*(new QQuickTransformPrivate), parent)
{
}

QQuickTransform::QQuickTransform(QQuickTransformPrivate &dd, QObject *parent)
: QObject(dd, parent)
{
}

QQuickTransform::~QQuickTransform()
{
    Q_D(QQuickTransform);
    for (int ii = 0; ii < d->items.size(); ++ii) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(d->items.at(ii));
        p->transforms.removeOne(this);
        p->dirty(QQuickItemPrivate::Transform);
    }
}

void QQuickTransform::update()
{
    Q_D(QQuickTransform);
    for (int ii = 0; ii < d->items.size(); ++ii) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(d->items.at(ii));
        p->dirty(QQuickItemPrivate::Transform);
    }
}

QQuickContents::QQuickContents(QQuickItem *item)
: m_item(item)
{
}

QQuickContents::~QQuickContents()
{
    QList<QQuickItem *> children = m_item->childItems();
    for (int i = 0; i < children.size(); ++i) {
        QQuickItem *child = children.at(i);
        QQuickItemPrivate::get(child)->removeItemChangeListener(this, QQuickItemPrivate::Geometry | QQuickItemPrivate::Destroyed);
    }
}

bool QQuickContents::calcHeight(QQuickItem *changed)
{
    qreal oldy = m_contents.y();
    qreal oldheight = m_contents.height();

    if (changed) {
        qreal top = oldy;
        qreal bottom = oldy + oldheight;
        qreal y = changed->y();
        if (y + changed->height() > bottom)
            bottom = y + changed->height();
        if (y < top)
            top = y;
        m_contents.setY(top);
        m_contents.setHeight(bottom - top);
    } else {
        qreal top = std::numeric_limits<qreal>::max();
        qreal bottom = -std::numeric_limits<qreal>::max();
        QList<QQuickItem *> children = m_item->childItems();
        for (int i = 0; i < children.size(); ++i) {
            QQuickItem *child = children.at(i);
            qreal y = child->y();
            if (y + child->height() > bottom)
                bottom = y + child->height();
            if (y < top)
                top = y;
        }
        if (!children.isEmpty())
            m_contents.setY(top);
        m_contents.setHeight(qMax(bottom - top, qreal(0.0)));
    }

    return (m_contents.height() != oldheight || m_contents.y() != oldy);
}

bool QQuickContents::calcWidth(QQuickItem *changed)
{
    qreal oldx = m_contents.x();
    qreal oldwidth = m_contents.width();

    if (changed) {
        qreal left = oldx;
        qreal right = oldx + oldwidth;
        qreal x = changed->x();
        if (x + changed->width() > right)
            right = x + changed->width();
        if (x < left)
            left = x;
        m_contents.setX(left);
        m_contents.setWidth(right - left);
    } else {
        qreal left = std::numeric_limits<qreal>::max();
        qreal right = -std::numeric_limits<qreal>::max();
        QList<QQuickItem *> children = m_item->childItems();
        for (int i = 0; i < children.size(); ++i) {
            QQuickItem *child = children.at(i);
            qreal x = child->x();
            if (x + child->width() > right)
                right = x + child->width();
            if (x < left)
                left = x;
        }
        if (!children.isEmpty())
            m_contents.setX(left);
        m_contents.setWidth(qMax(right - left, qreal(0.0)));
    }

    return (m_contents.width() != oldwidth || m_contents.x() != oldx);
}

void QQuickContents::complete()
{
    QQuickItemPrivate::get(m_item)->addItemChangeListener(this, QQuickItemPrivate::Children);

    QList<QQuickItem *> children = m_item->childItems();
    for (int i = 0; i < children.size(); ++i) {
        QQuickItem *child = children.at(i);
        QQuickItemPrivate::get(child)->addItemChangeListener(this, QQuickItemPrivate::Geometry | QQuickItemPrivate::Destroyed);
        //###what about changes to visibility?
    }
    calcGeometry();
}

void QQuickContents::updateRect()
{
    QQuickItemPrivate::get(m_item)->emitChildrenRectChanged(rectF());
}

void QQuickContents::itemGeometryChanged(QQuickItem *changed, QQuickGeometryChange change, const QRectF &)
{
    Q_UNUSED(changed);
    bool wChanged = false;
    bool hChanged = false;
    //### we can only pass changed if the left edge has moved left, or the right edge has moved right
    if (change.horizontalChange())
        wChanged = calcWidth(/*changed*/);
    if (change.verticalChange())
        hChanged = calcHeight(/*changed*/);
    if (wChanged || hChanged)
        updateRect();
}

void QQuickContents::itemDestroyed(QQuickItem *item)
{
    if (item)
        QQuickItemPrivate::get(item)->removeItemChangeListener(this, QQuickItemPrivate::Geometry | QQuickItemPrivate::Destroyed);
    calcGeometry();
}

void QQuickContents::itemChildRemoved(QQuickItem *, QQuickItem *item)
{
    if (item)
        QQuickItemPrivate::get(item)->removeItemChangeListener(this, QQuickItemPrivate::Geometry | QQuickItemPrivate::Destroyed);
    calcGeometry();
}

void QQuickContents::itemChildAdded(QQuickItem *, QQuickItem *item)
{
    if (item)
        QQuickItemPrivate::get(item)->addItemChangeListener(this, QQuickItemPrivate::Geometry | QQuickItemPrivate::Destroyed);
    calcGeometry(item);
}

QQuickItemKeyFilter::QQuickItemKeyFilter(QQuickItem *item)
: m_processPost(false), m_next(nullptr)
{
    QQuickItemPrivate *p = item?QQuickItemPrivate::get(item):nullptr;
    if (p) {
        m_next = p->extra.value().keyHandler;
        p->extra->keyHandler = this;
    }
}

QQuickItemKeyFilter::~QQuickItemKeyFilter()
{
}

void QQuickItemKeyFilter::keyPressed(QKeyEvent *event, bool post)
{
    if (m_next) m_next->keyPressed(event, post);
}

void QQuickItemKeyFilter::keyReleased(QKeyEvent *event, bool post)
{
    if (m_next) m_next->keyReleased(event, post);
}

#if QT_CONFIG(im)
void QQuickItemKeyFilter::inputMethodEvent(QInputMethodEvent *event, bool post)
{
    if (m_next)
        m_next->inputMethodEvent(event, post);
    else
        event->ignore();
}

QVariant QQuickItemKeyFilter::inputMethodQuery(Qt::InputMethodQuery query) const
{
    if (m_next) return m_next->inputMethodQuery(query);
    return QVariant();
}
#endif // im

void QQuickItemKeyFilter::shortcutOverrideEvent(QKeyEvent *event)
{
    if (m_next)
        m_next->shortcutOverrideEvent(event);
    else
        event->ignore();
}

void QQuickItemKeyFilter::componentComplete()
{
    if (m_next) m_next->componentComplete();
}
/*!
    \qmltype KeyNavigation
    \instantiates QQuickKeyNavigationAttached
    \inqmlmodule QtQuick
    \ingroup qtquick-input-handlers
    \brief Supports key navigation by arrow keys.

    Key-based user interfaces commonly allow the use of arrow keys to navigate between
    focusable items.  The KeyNavigation attached property enables this behavior by providing a
    convenient way to specify the item that should gain focus when an arrow or tab key is pressed.

    The following example provides key navigation for a 2x2 grid of items:

    \snippet qml/keynavigation.qml 0

    The top-left item initially receives focus by setting \l {Item::}{focus} to
    \c true. When an arrow key is pressed, the focus will move to the
    appropriate item, as defined by the value that has been set for
    the KeyNavigation \l left, \l right, \l up or \l down properties.

    Note that if a KeyNavigation attached property receives the key press and release
    events for a requested arrow or tab key, the event is accepted and does not
    propagate any further.

    By default, KeyNavigation receives key events after the item to which it is attached.
    If the item accepts the key event, the KeyNavigation attached property will not
    receive an event for that key.  Setting the \l priority property to
    \c KeyNavigation.BeforeItem allows the event to be used for key navigation
    before the item, rather than after.

    If the item to which the focus is switching is not enabled or visible, an attempt will
    be made to skip this item and focus on the next. This is possible if there are
    a chain of items with the same KeyNavigation handler. If multiple items in a row are not enabled
    or visible, they will also be skipped.

    KeyNavigation will implicitly set the other direction to return focus to this item. So if you set
    \l left to another item, \l right will be set on that item's KeyNavigation to set focus back to this
    item. However, if that item's KeyNavigation has had right explicitly set then no change will occur.
    This means that the example above could achieve the same behavior without specifying
    KeyNavigation.right or KeyNavigation.down for any of the items.

    \sa {Keys}{Keys attached property}
*/

/*!
    \qmlattachedproperty Item QtQuick::KeyNavigation::left

    This property holds the item to assign focus to
    when the left cursor key is pressed.
*/

/*!
    \qmlattachedproperty Item QtQuick::KeyNavigation::right

    This property holds the item to assign focus to
    when the right cursor key is pressed.
*/

/*!
    \qmlattachedproperty Item QtQuick::KeyNavigation::up

    This property holds the item to assign focus to
    when the up cursor key is pressed.
*/

/*!
    \qmlattachedproperty Item QtQuick::KeyNavigation::down

    This property holds the item to assign focus to
    when the down cursor key is pressed.
*/

/*!
    \qmlattachedproperty Item QtQuick::KeyNavigation::tab

    This property holds the item to assign focus to
    when the Tab key is pressed.
*/

/*!
    \qmlattachedproperty Item QtQuick::KeyNavigation::backtab

    This property holds the item to assign focus to
    when the Shift+Tab key combination (Backtab) is pressed.
*/

QQuickKeyNavigationAttached::QQuickKeyNavigationAttached(QObject *parent)
: QObject(*(new QQuickKeyNavigationAttachedPrivate), parent),
  QQuickItemKeyFilter(qmlobject_cast<QQuickItem*>(parent))
{
    m_processPost = true;
}

QQuickKeyNavigationAttached *
QQuickKeyNavigationAttached::qmlAttachedProperties(QObject *obj)
{
    return new QQuickKeyNavigationAttached(obj);
}

QQuickItem *QQuickKeyNavigationAttached::left() const
{
    Q_D(const QQuickKeyNavigationAttached);
    return d->left;
}

void QQuickKeyNavigationAttached::setLeft(QQuickItem *i)
{
    Q_D(QQuickKeyNavigationAttached);
    if (d->leftSet && d->left == i)
        return;
    d->leftSet = d->left != i;
    d->left = i;
    QQuickKeyNavigationAttached* other =
            qobject_cast<QQuickKeyNavigationAttached*>(qmlAttachedPropertiesObject<QQuickKeyNavigationAttached>(i));
    if (other && !other->d_func()->rightSet){
        other->d_func()->right = qobject_cast<QQuickItem*>(parent());
        emit other->rightChanged();
    }
    emit leftChanged();
}

QQuickItem *QQuickKeyNavigationAttached::right() const
{
    Q_D(const QQuickKeyNavigationAttached);
    return d->right;
}

void QQuickKeyNavigationAttached::setRight(QQuickItem *i)
{
    Q_D(QQuickKeyNavigationAttached);
    if (d->rightSet && d->right == i)
        return;
    d->rightSet = d->right != i;
    d->right = i;
    QQuickKeyNavigationAttached* other =
            qobject_cast<QQuickKeyNavigationAttached*>(qmlAttachedPropertiesObject<QQuickKeyNavigationAttached>(i));
    if (other && !other->d_func()->leftSet){
        other->d_func()->left = qobject_cast<QQuickItem*>(parent());
        emit other->leftChanged();
    }
    emit rightChanged();
}

QQuickItem *QQuickKeyNavigationAttached::up() const
{
    Q_D(const QQuickKeyNavigationAttached);
    return d->up;
}

void QQuickKeyNavigationAttached::setUp(QQuickItem *i)
{
    Q_D(QQuickKeyNavigationAttached);
    if (d->upSet && d->up == i)
        return;
    d->upSet = d->up != i;
    d->up = i;
    QQuickKeyNavigationAttached* other =
            qobject_cast<QQuickKeyNavigationAttached*>(qmlAttachedPropertiesObject<QQuickKeyNavigationAttached>(i));
    if (other && !other->d_func()->downSet){
        other->d_func()->down = qobject_cast<QQuickItem*>(parent());
        emit other->downChanged();
    }
    emit upChanged();
}

QQuickItem *QQuickKeyNavigationAttached::down() const
{
    Q_D(const QQuickKeyNavigationAttached);
    return d->down;
}

void QQuickKeyNavigationAttached::setDown(QQuickItem *i)
{
    Q_D(QQuickKeyNavigationAttached);
    if (d->downSet && d->down == i)
        return;
    d->downSet = d->down != i;
    d->down = i;
    QQuickKeyNavigationAttached* other =
            qobject_cast<QQuickKeyNavigationAttached*>(qmlAttachedPropertiesObject<QQuickKeyNavigationAttached>(i));
    if (other && !other->d_func()->upSet) {
        other->d_func()->up = qobject_cast<QQuickItem*>(parent());
        emit other->upChanged();
    }
    emit downChanged();
}

QQuickItem *QQuickKeyNavigationAttached::tab() const
{
    Q_D(const QQuickKeyNavigationAttached);
    return d->tab;
}

void QQuickKeyNavigationAttached::setTab(QQuickItem *i)
{
    Q_D(QQuickKeyNavigationAttached);
    if (d->tabSet && d->tab == i)
        return;
    d->tabSet = d->tab != i;
    d->tab = i;
    QQuickKeyNavigationAttached* other =
            qobject_cast<QQuickKeyNavigationAttached*>(qmlAttachedPropertiesObject<QQuickKeyNavigationAttached>(i));
    if (other && !other->d_func()->backtabSet) {
        other->d_func()->backtab = qobject_cast<QQuickItem*>(parent());
        emit other->backtabChanged();
    }
    emit tabChanged();
}

QQuickItem *QQuickKeyNavigationAttached::backtab() const
{
    Q_D(const QQuickKeyNavigationAttached);
    return d->backtab;
}

void QQuickKeyNavigationAttached::setBacktab(QQuickItem *i)
{
    Q_D(QQuickKeyNavigationAttached);
    if (d->backtabSet && d->backtab == i)
        return;
    d->backtabSet = d->backtab != i;
    d->backtab = i;
    QQuickKeyNavigationAttached* other =
            qobject_cast<QQuickKeyNavigationAttached*>(qmlAttachedPropertiesObject<QQuickKeyNavigationAttached>(i));
    if (other && !other->d_func()->tabSet) {
        other->d_func()->tab = qobject_cast<QQuickItem*>(parent());
        emit other->tabChanged();
    }
    emit backtabChanged();
}

/*!
    \qmlattachedproperty enumeration QtQuick::KeyNavigation::priority

    This property determines whether the keys are processed before
    or after the attached item's own key handling.

    \value KeyNavigation.BeforeItem     process the key events before normal
        item key processing.  If the event is used for key navigation, it will be accepted and
        will not be passed on to the item.
    \value KeyNavigation.AfterItem      (default) process the key events after normal item key
        handling.  If the item accepts the key event it will not be
        handled by the KeyNavigation attached property handler.
*/
QQuickKeyNavigationAttached::Priority QQuickKeyNavigationAttached::priority() const
{
    return m_processPost ? AfterItem : BeforeItem;
}

void QQuickKeyNavigationAttached::setPriority(Priority order)
{
    bool processPost = order == AfterItem;
    if (processPost != m_processPost) {
        m_processPost = processPost;
        emit priorityChanged();
    }
}

void QQuickKeyNavigationAttached::keyPressed(QKeyEvent *event, bool post)
{
    Q_D(QQuickKeyNavigationAttached);
    event->ignore();

    if (post != m_processPost) {
        QQuickItemKeyFilter::keyPressed(event, post);
        return;
    }

    bool mirror = false;
    switch (event->key()) {
    case Qt::Key_Left: {
        if (QQuickItem *parentItem = qobject_cast<QQuickItem*>(parent()))
            mirror = QQuickItemPrivate::get(parentItem)->effectiveLayoutMirror;
        QQuickItem* leftItem = mirror ? d->right : d->left;
        if (leftItem) {
            setFocusNavigation(leftItem, mirror ? "right" : "left", mirror ? Qt::TabFocusReason : Qt::BacktabFocusReason);
            event->accept();
        }
        break;
    }
    case Qt::Key_Right: {
        if (QQuickItem *parentItem = qobject_cast<QQuickItem*>(parent()))
            mirror = QQuickItemPrivate::get(parentItem)->effectiveLayoutMirror;
        QQuickItem* rightItem = mirror ? d->left : d->right;
        if (rightItem) {
            setFocusNavigation(rightItem, mirror ? "left" : "right", mirror ? Qt::BacktabFocusReason : Qt::TabFocusReason);
            event->accept();
        }
        break;
    }
    case Qt::Key_Up:
        if (d->up) {
            setFocusNavigation(d->up, "up", Qt::BacktabFocusReason);
            event->accept();
        }
        break;
    case Qt::Key_Down:
        if (d->down) {
            setFocusNavigation(d->down, "down", Qt::TabFocusReason);
            event->accept();
        }
        break;
    case Qt::Key_Tab:
        if (d->tab) {
            setFocusNavigation(d->tab, "tab", Qt::TabFocusReason);
            event->accept();
        }
        break;
    case Qt::Key_Backtab:
        if (d->backtab) {
            setFocusNavigation(d->backtab, "backtab", Qt::BacktabFocusReason);
            event->accept();
        }
        break;
    default:
        break;
    }

    if (!event->isAccepted()) QQuickItemKeyFilter::keyPressed(event, post);
}

void QQuickKeyNavigationAttached::keyReleased(QKeyEvent *event, bool post)
{
    Q_D(QQuickKeyNavigationAttached);
    event->ignore();

    if (post != m_processPost) {
        QQuickItemKeyFilter::keyReleased(event, post);
        return;
    }

    bool mirror = false;
    switch (event->key()) {
    case Qt::Key_Left:
        if (QQuickItem *parentItem = qobject_cast<QQuickItem*>(parent()))
            mirror = QQuickItemPrivate::get(parentItem)->effectiveLayoutMirror;
        if (mirror ? d->right : d->left)
            event->accept();
        break;
    case Qt::Key_Right:
        if (QQuickItem *parentItem = qobject_cast<QQuickItem*>(parent()))
            mirror = QQuickItemPrivate::get(parentItem)->effectiveLayoutMirror;
        if (mirror ? d->left : d->right)
            event->accept();
        break;
    case Qt::Key_Up:
        if (d->up) {
            event->accept();
        }
        break;
    case Qt::Key_Down:
        if (d->down) {
            event->accept();
        }
        break;
    case Qt::Key_Tab:
        if (d->tab) {
            event->accept();
        }
        break;
    case Qt::Key_Backtab:
        if (d->backtab) {
            event->accept();
        }
        break;
    default:
        break;
    }

    if (!event->isAccepted()) QQuickItemKeyFilter::keyReleased(event, post);
}

void QQuickKeyNavigationAttached::setFocusNavigation(QQuickItem *currentItem, const char *dir,
                                                     Qt::FocusReason reason)
{
    QQuickItem *initialItem = currentItem;
    bool isNextItem = false;
    QVector<QQuickItem *> visitedItems;
    do {
        isNextItem = false;
        if (currentItem->isVisible() && currentItem->isEnabled()) {
            currentItem->forceActiveFocus(reason);
        } else {
            QObject *attached =
                qmlAttachedPropertiesObject<QQuickKeyNavigationAttached>(currentItem, false);
            if (attached) {
                QQuickItem *tempItem = qvariant_cast<QQuickItem*>(attached->property(dir));
                if (tempItem) {
                    visitedItems.append(currentItem);
                    currentItem = tempItem;
                    isNextItem = true;
                }
            }
        }
    }
    while (currentItem != initialItem && isNextItem && !visitedItems.contains(currentItem));
}

struct SigMap {
    int key;
    const char *sig;
};

const SigMap sigMap[] = {
    { Qt::Key_Left, "leftPressed" },
    { Qt::Key_Right, "rightPressed" },
    { Qt::Key_Up, "upPressed" },
    { Qt::Key_Down, "downPressed" },
    { Qt::Key_Tab, "tabPressed" },
    { Qt::Key_Backtab, "backtabPressed" },
    { Qt::Key_Asterisk, "asteriskPressed" },
    { Qt::Key_NumberSign, "numberSignPressed" },
    { Qt::Key_Escape, "escapePressed" },
    { Qt::Key_Return, "returnPressed" },
    { Qt::Key_Enter, "enterPressed" },
    { Qt::Key_Delete, "deletePressed" },
    { Qt::Key_Space, "spacePressed" },
    { Qt::Key_Back, "backPressed" },
    { Qt::Key_Cancel, "cancelPressed" },
    { Qt::Key_Select, "selectPressed" },
    { Qt::Key_Yes, "yesPressed" },
    { Qt::Key_No, "noPressed" },
    { Qt::Key_Context1, "context1Pressed" },
    { Qt::Key_Context2, "context2Pressed" },
    { Qt::Key_Context3, "context3Pressed" },
    { Qt::Key_Context4, "context4Pressed" },
    { Qt::Key_Call, "callPressed" },
    { Qt::Key_Hangup, "hangupPressed" },
    { Qt::Key_Flip, "flipPressed" },
    { Qt::Key_Menu, "menuPressed" },
    { Qt::Key_VolumeUp, "volumeUpPressed" },
    { Qt::Key_VolumeDown, "volumeDownPressed" },
    { 0, nullptr }
};

QByteArray QQuickKeysAttached::keyToSignal(int key)
{
    QByteArray keySignal;
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        keySignal = "digit0Pressed";
        keySignal[5] = '0' + (key - Qt::Key_0);
    } else {
        int i = 0;
        while (sigMap[i].key && sigMap[i].key != key)
            ++i;
        keySignal = sigMap[i].sig;
    }
    return keySignal;
}

bool QQuickKeysAttached::isConnected(const char *signalName) const
{
    Q_D(const QQuickKeysAttached);
    int signal_index = d->signalIndex(signalName);
    return d->isSignalConnected(signal_index);
}

/*!
    \qmltype Keys
    \instantiates QQuickKeysAttached
    \inqmlmodule QtQuick
    \ingroup qtquick-input-handlers
    \brief Provides key handling to Items.

    All visual primitives support key handling via the Keys
    attached property.  Keys can be handled via the onPressed
    and onReleased signal properties.

    The signal properties have a \l KeyEvent parameter, named
    \e event which contains details of the event.  If a key is
    handled \e event.accepted should be set to true to prevent the
    event from propagating up the item hierarchy.

    \section1 Example Usage

    The following example shows how the general onPressed handler can
    be used to test for a certain key; in this case, the left cursor
    key:

    \snippet qml/keys/keys-pressed.qml key item

    Some keys may alternatively be handled via specific signal properties,
    for example \e onSelectPressed.  These handlers automatically set
    \e event.accepted to true.

    \snippet qml/keys/keys-handler.qml key item

    See \l{Qt::Key}{Qt.Key} for the list of keyboard codes.

    \section1 Key Handling Priorities

    The Keys attached property can be configured to handle key events
    before or after the item it is attached to. This makes it possible
    to intercept events in order to override an item's default behavior,
    or act as a fallback for keys not handled by the item.

    If \l priority is Keys.BeforeItem (default) the order of key event processing is:

    \list 1
    \li Items specified in \c forwardTo
    \li specific key handlers, e.g. onReturnPressed
    \li onPressed, onReleased handlers
    \li Item specific key handling, e.g. TextInput key handling
    \li parent item
    \endlist

    If priority is Keys.AfterItem the order of key event processing is:

    \list 1
    \li Item specific key handling, e.g. TextInput key handling
    \li Items specified in \c forwardTo
    \li specific key handlers, e.g. onReturnPressed
    \li onPressed, onReleased handlers
    \li parent item
    \endlist

    If the event is accepted during any of the above steps, key
    propagation stops.

    \sa KeyEvent, {KeyNavigation}{KeyNavigation attached property}
*/

/*!
    \qmlproperty bool QtQuick::Keys::enabled

    This flags enables key handling if true (default); otherwise
    no key handlers will be called.
*/

/*!
    \qmlproperty enumeration QtQuick::Keys::priority

    This property determines whether the keys are processed before
    or after the attached item's own key handling.

    \value Keys.BeforeItem  (default) process the key events before normal item key processing.
                            If the event is accepted, it will not be passed on to the item.
    \value Keys.AfterItem   process the key events after normal item key handling.  If the item
                            accepts the key event, it will not be handled by the
                            Keys attached property handler.

    \sa {Key Handling Priorities}
*/

/*!
    \qmlproperty list<Item> QtQuick::Keys::forwardTo

    This property provides a way to forward key presses, key releases, and keyboard input
    coming from input methods to other items. This can be useful when you want
    one item to handle some keys (e.g. the up and down arrow keys), and another item to
    handle other keys (e.g. the left and right arrow keys).  Once an item that has been
    forwarded keys accepts the event it is no longer forwarded to items later in the
    list.

    This example forwards key events to two lists:
    \qml
    Item {
        ListView {
            id: list1
            // ...
        }
        ListView {
            id: list2
            // ...
        }
        Keys.forwardTo: [list1, list2]
        focus: true
    }
    \endqml

    To see the order in which events are received when using forwardTo, see
    \l {Key Handling Priorities}.
*/

/*!
    \qmlsignal QtQuick::Keys::pressed(KeyEvent event)

    This signal is emitted when a key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::released(KeyEvent event)

    This signal is emitted when a key has been released. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::shortcutOverride(KeyEvent event)
    \since 5.9

    This signal is emitted when a key has been pressed that could potentially
    be used as a shortcut. The \a event parameter provides information about
    the event.

    Set \c event.accepted to \c true if you wish to prevent the pressed key
    from being used as a shortcut by other types, such as \l Shortcut. For
    example:

    \code
    Item {
        id: escapeItem
        focus: true

        // Ensure that we get escape key press events first.
        Keys.onShortcutOverride: (event)=> event.accepted = (event.key === Qt.Key_Escape)

        Keys.onEscapePressed: {
            console.log("escapeItem is handling escape");
            // event.accepted is set to true by default for the specific key handlers
        }
    }

    Shortcut {
        sequence: "Escape"
        onActivated: console.log("Shortcut is handling escape")
    }
    \endcode

    As with the other signals, \c shortcutOverride will only be emitted for an
    item if that item has \l {Item::}{activeFocus}.

    \sa Shortcut
*/

/*!
    \qmlsignal QtQuick::Keys::digit0Pressed(KeyEvent event)

    This signal is emitted when the digit '0' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::digit1Pressed(KeyEvent event)

    This signal is emitted when the digit '1' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::digit2Pressed(KeyEvent event)

    This signal is emitted when the digit '2' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::digit3Pressed(KeyEvent event)

    This signal is emitted when the digit '3' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::digit4Pressed(KeyEvent event)

    This signal is emitted when the digit '4' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::digit5Pressed(KeyEvent event)

    This signal is emitted when the digit '5' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::digit6Pressed(KeyEvent event)

    This signal is emitted when the digit '6' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::digit7Pressed(KeyEvent event)

    This signal is emitted when the digit '7' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::digit8Pressed(KeyEvent event)

    This signal is emitted when the digit '8' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::digit9Pressed(KeyEvent event)

    This signal is emitted when the digit '9' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::leftPressed(KeyEvent event)

    This signal is emitted when the Left arrow has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::rightPressed(KeyEvent event)

    This signal is emitted when the Right arrow has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::upPressed(KeyEvent event)

    This signal is emitted when the Up arrow has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::downPressed(KeyEvent event)

    This signal is emitted when the Down arrow has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::tabPressed(KeyEvent event)

    This signal is emitted when the Tab key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::backtabPressed(KeyEvent event)

    This signal is emitted when the Shift+Tab key combination (Backtab) has
    been pressed. The \a event parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::asteriskPressed(KeyEvent event)

    This signal is emitted when the Asterisk '*' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::escapePressed(KeyEvent event)

    This signal is emitted when the Escape key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::returnPressed(KeyEvent event)

    This signal is emitted when the Return key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::enterPressed(KeyEvent event)

    This signal is emitted when the Enter key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::deletePressed(KeyEvent event)

    This signal is emitted when the Delete key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::spacePressed(KeyEvent event)

    This signal is emitted when the Space key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::backPressed(KeyEvent event)

    This signal is emitted when the Back key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::cancelPressed(KeyEvent event)

    This signal is emitted when the Cancel key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::selectPressed(KeyEvent event)

    This signal is emitted when the Select key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::yesPressed(KeyEvent event)

    This signal is emitted when the Yes key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::noPressed(KeyEvent event)

    This signal is emitted when the No key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::context1Pressed(KeyEvent event)

    This signal is emitted when the Context1 key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::context2Pressed(KeyEvent event)

    This signal is emitted when the Context2 key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::context3Pressed(KeyEvent event)

    This signal is emitted when the Context3 key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::context4Pressed(KeyEvent event)

    This signal is emitted when the Context4 key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::callPressed(KeyEvent event)

    This signal is emitted when the Call key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::hangupPressed(KeyEvent event)

    This signal is emitted when the Hangup key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::flipPressed(KeyEvent event)

    This signal is emitted when the Flip key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::menuPressed(KeyEvent event)

    This signal is emitted when the Menu key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::volumeUpPressed(KeyEvent event)

    This signal is emitted when the VolumeUp key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick::Keys::volumeDownPressed(KeyEvent event)

    This signal is emitted when the VolumeDown key has been pressed. The \a event
    parameter provides information about the event.
*/

QQuickKeysAttached::QQuickKeysAttached(QObject *parent)
: QObject(*(new QQuickKeysAttachedPrivate), parent),
  QQuickItemKeyFilter(qmlobject_cast<QQuickItem*>(parent))
{
    Q_D(QQuickKeysAttached);
    m_processPost = false;
    d->item = qmlobject_cast<QQuickItem*>(parent);
    if (d->item != parent)
        qWarning() << "Could not attach Keys property to: " << parent << " is not an Item";
}

QQuickKeysAttached::~QQuickKeysAttached()
{
}

QQuickKeysAttached::Priority QQuickKeysAttached::priority() const
{
    return m_processPost ? AfterItem : BeforeItem;
}

void QQuickKeysAttached::setPriority(Priority order)
{
    bool processPost = order == AfterItem;
    if (processPost != m_processPost) {
        m_processPost = processPost;
        emit priorityChanged();
    }
}

void QQuickKeysAttached::componentComplete()
{
#if QT_CONFIG(im)
    Q_D(QQuickKeysAttached);
    if (d->item) {
        for (int ii = 0; ii < d->targets.size(); ++ii) {
            QQuickItem *targetItem = d->targets.at(ii);
            if (targetItem && (targetItem->flags() & QQuickItem::ItemAcceptsInputMethod)) {
                d->item->setFlag(QQuickItem::ItemAcceptsInputMethod);
                break;
            }
        }
    }
#endif
}

void QQuickKeysAttached::keyPressed(QKeyEvent *event, bool post)
{
    Q_D(QQuickKeysAttached);
    if (post != m_processPost || !d->enabled || d->inPress) {
        event->ignore();
        QQuickItemKeyFilter::keyPressed(event, post);
        return;
    }

    // first process forwards
    if (d->item && d->item->window()) {
        d->inPress = true;
        for (int ii = 0; ii < d->targets.size(); ++ii) {
            QQuickItem *i = d->targets.at(ii);
            if (i && i->isVisible()) {
                event->accept();
                QCoreApplication::sendEvent(i, event);
                if (event->isAccepted()) {
                    d->inPress = false;
                    return;
                }
            }
        }
        d->inPress = false;
    }

    QQuickKeyEvent &ke = d->theKeyEvent;
    ke.reset(*event);
    QByteArray keySignal = keyToSignal(event->key());
    if (!keySignal.isEmpty()) {
        keySignal += "(QQuickKeyEvent*)";
        if (isConnected(keySignal)) {
            // If we specifically handle a key then default to accepted
            ke.setAccepted(true);
            int idx = QQuickKeysAttached::staticMetaObject.indexOfSignal(keySignal);
            metaObject()->method(idx).invoke(this, Qt::DirectConnection, Q_ARG(QQuickKeyEvent*, &ke));
        }
    }
    if (!ke.isAccepted())
        emit pressed(&ke);
    event->setAccepted(ke.isAccepted());

    if (!event->isAccepted()) QQuickItemKeyFilter::keyPressed(event, post);
}

void QQuickKeysAttached::keyReleased(QKeyEvent *event, bool post)
{
    Q_D(QQuickKeysAttached);
    if (post != m_processPost || !d->enabled || d->inRelease) {
        event->ignore();
        QQuickItemKeyFilter::keyReleased(event, post);
        return;
    }

    if (d->item && d->item->window()) {
        d->inRelease = true;
        for (int ii = 0; ii < d->targets.size(); ++ii) {
            QQuickItem *i = d->targets.at(ii);
            if (i && i->isVisible()) {
                event->accept();
                QCoreApplication::sendEvent(i, event);
                if (event->isAccepted()) {
                    d->inRelease = false;
                    return;
                }
            }
        }
        d->inRelease = false;
    }

    QQuickKeyEvent &ke = d->theKeyEvent;
    ke.reset(*event);
    emit released(&ke);
    event->setAccepted(ke.isAccepted());

    if (!event->isAccepted()) QQuickItemKeyFilter::keyReleased(event, post);
}

#if QT_CONFIG(im)
void QQuickKeysAttached::inputMethodEvent(QInputMethodEvent *event, bool post)
{
    Q_D(QQuickKeysAttached);
    if (post == m_processPost && d->item && !d->inIM && d->item->window()) {
        d->inIM = true;
        for (int ii = 0; ii < d->targets.size(); ++ii) {
            QQuickItem *targetItem = d->targets.at(ii);
            if (targetItem && targetItem->isVisible() && (targetItem->flags() & QQuickItem::ItemAcceptsInputMethod)) {
                QCoreApplication::sendEvent(targetItem, event);
                if (event->isAccepted()) {
                    d->imeItem = targetItem;
                    d->inIM = false;
                    return;
                }
            }
        }
        d->inIM = false;
    }
    QQuickItemKeyFilter::inputMethodEvent(event, post);
}

QVariant QQuickKeysAttached::inputMethodQuery(Qt::InputMethodQuery query) const
{
    Q_D(const QQuickKeysAttached);
    if (d->item) {
        for (int ii = 0; ii < d->targets.size(); ++ii) {
            QQuickItem *i = d->targets.at(ii);
            if (i && i->isVisible() && (i->flags() & QQuickItem::ItemAcceptsInputMethod) && i == d->imeItem) {
                //### how robust is i == d->imeItem check?
                QVariant v = i->inputMethodQuery(query);
                if (v.userType() == QMetaType::QRectF)
                    v = d->item->mapRectFromItem(i, v.toRectF());  //### cost?
                return v;
            }
        }
    }
    return QQuickItemKeyFilter::inputMethodQuery(query);
}
#endif // im

void QQuickKeysAttached::shortcutOverrideEvent(QKeyEvent *event)
{
    Q_D(QQuickKeysAttached);
    QQuickKeyEvent &keyEvent = d->theKeyEvent;
    keyEvent.reset(*event);
    emit shortcutOverride(&keyEvent);

    event->setAccepted(keyEvent.isAccepted());
}

QQuickKeysAttached *QQuickKeysAttached::qmlAttachedProperties(QObject *obj)
{
    return new QQuickKeysAttached(obj);
}

/*!
    \qmltype LayoutMirroring
    \instantiates QQuickLayoutMirroringAttached
    \inqmlmodule QtQuick
    \ingroup qtquick-positioners
    \ingroup qml-utility-elements
    \brief Property used to mirror layout behavior.

    The LayoutMirroring attached property is used to horizontally mirror \l {anchor-layout}{Item anchors},
    \l{Item Positioners}{positioner} types (such as \l Row and \l Grid)
    and views (such as \l GridView and horizontal \l ListView). Mirroring is a visual change: left
    anchors become right anchors, and positioner types like \l Grid and \l Row reverse the
    horizontal layout of child items.

    Mirroring is enabled for an item by setting the \l enabled property to true. By default, this
    only affects the item itself; setting the \l childrenInherit property to true propagates the mirroring
    behavior to all child items as well. If the \c LayoutMirroring attached property has not been defined
    for an item, mirroring is not enabled.

    \note Since Qt 5.8, \c LayoutMirroring can be attached to a \l Window. In practice, it is the same as
    attaching \c LayoutMirroring to the window's \c contentItem.

    The following example shows mirroring in action. The \l Row below is specified as being anchored
    to the left of its parent. However, since mirroring has been enabled, the anchor is horizontally
    reversed and it is now anchored to the right. Also, since items in a \l Row are positioned
    from left to right by default, they are now positioned from right to left instead, as demonstrated
    by the numbering and opacity of the items:

    \snippet qml/layoutmirroring.qml 0

    \image layoutmirroring.png

    Layout mirroring is useful when it is necessary to support both left-to-right and right-to-left
    layout versions of an application to target different language areas. The \l childrenInherit
    property allows layout mirroring to be applied without manually setting layout configurations
    for every item in an application. Keep in mind, however, that mirroring does not affect any
    positioning that is defined by the \l Item \l {Item::}{x} coordinate value, so even with
    mirroring enabled, it will often be necessary to apply some layout fixes to support the
    desired layout direction. Also, it may be necessary to disable the mirroring of individual
    child items (by setting \l {enabled}{LayoutMirroring.enabled} to false for such items) if
    mirroring is not the desired behavior, or if the child item already implements mirroring in
    some custom way.

    To set the layout direction based on the \l {Default Layout Direction}{default layout direction}
    of the application, use the following code:

    \code
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    \endcode

    See \l {Right-to-left User Interfaces} for further details on using \c LayoutMirroring and
    other related features to implement right-to-left support for an application.
*/

/*!
    \qmlproperty bool QtQuick::LayoutMirroring::enabled

    This property holds whether the item's layout is mirrored horizontally. Setting this to true
    horizontally reverses \l {anchor-layout}{anchor} settings such that left anchors become right,
    and right anchors become left. For \l{Item Positioners}{positioner} types
    (such as \l Row and \l Grid) and view types (such as \l {GridView}{GridView} and \l {ListView}{ListView})
    this also mirrors the horizontal layout direction of the item.

    The default value is false.
*/

/*!
    \qmlproperty bool QtQuick::LayoutMirroring::childrenInherit

    This property holds whether the \l {enabled}{LayoutMirroring.enabled} value for this item
    is inherited by its children.

    The default value is false.
*/


QQuickLayoutMirroringAttached::QQuickLayoutMirroringAttached(QObject *parent) : QObject(parent), itemPrivate(nullptr)
{
    if (QQuickItem *item = qobject_cast<QQuickItem *>(parent))
        itemPrivate = QQuickItemPrivate::get(item);
    else if (QQuickWindow *window = qobject_cast<QQuickWindow *>(parent))
        itemPrivate = QQuickItemPrivate::get(window->contentItem());

    if (itemPrivate)
        itemPrivate->extra.value().layoutDirectionAttached = this;
    else
        qmlWarning(parent) << tr("LayoutDirection attached property only works with Items and Windows");
}

QQuickLayoutMirroringAttached * QQuickLayoutMirroringAttached::qmlAttachedProperties(QObject *object)
{
    return new QQuickLayoutMirroringAttached(object);
}

bool QQuickLayoutMirroringAttached::enabled() const
{
    return itemPrivate ? itemPrivate->effectiveLayoutMirror : false;
}

void QQuickLayoutMirroringAttached::setEnabled(bool enabled)
{
    if (!itemPrivate)
        return;

    itemPrivate->isMirrorImplicit = false;
    if (enabled != itemPrivate->effectiveLayoutMirror) {
        itemPrivate->setLayoutMirror(enabled);
        if (itemPrivate->inheritMirrorFromItem)
             itemPrivate->resolveLayoutMirror();
    }
}

void QQuickLayoutMirroringAttached::resetEnabled()
{
    if (itemPrivate && !itemPrivate->isMirrorImplicit) {
        itemPrivate->isMirrorImplicit = true;
        itemPrivate->resolveLayoutMirror();
    }
}

bool QQuickLayoutMirroringAttached::childrenInherit() const
{
    return itemPrivate ? itemPrivate->inheritMirrorFromItem : false;
}

void QQuickLayoutMirroringAttached::setChildrenInherit(bool childrenInherit) {
    if (itemPrivate && childrenInherit != itemPrivate->inheritMirrorFromItem) {
        itemPrivate->inheritMirrorFromItem = childrenInherit;
        itemPrivate->resolveLayoutMirror();
        childrenInheritChanged();
    }
}

void QQuickItemPrivate::resolveLayoutMirror()
{
    Q_Q(QQuickItem);
    if (QQuickItem *parentItem = q->parentItem()) {
        QQuickItemPrivate *parentPrivate = QQuickItemPrivate::get(parentItem);
        setImplicitLayoutMirror(parentPrivate->inheritedLayoutMirror, parentPrivate->inheritMirrorFromParent);
    } else {
        setImplicitLayoutMirror(isMirrorImplicit ? false : effectiveLayoutMirror, inheritMirrorFromItem);
    }
}

void QQuickItemPrivate::setImplicitLayoutMirror(bool mirror, bool inherit)
{
    inherit = inherit || inheritMirrorFromItem;
    if (!isMirrorImplicit && inheritMirrorFromItem)
        mirror = effectiveLayoutMirror;
    if (mirror == inheritedLayoutMirror && inherit == inheritMirrorFromParent)
        return;

    inheritMirrorFromParent = inherit;
    inheritedLayoutMirror = inheritMirrorFromParent ? mirror : false;

    if (isMirrorImplicit)
        setLayoutMirror(inherit ? inheritedLayoutMirror : false);
    for (int i = 0; i < childItems.size(); ++i) {
        if (QQuickItem *child = qmlobject_cast<QQuickItem *>(childItems.at(i))) {
            QQuickItemPrivate *childPrivate = QQuickItemPrivate::get(child);
            childPrivate->setImplicitLayoutMirror(inheritedLayoutMirror, inheritMirrorFromParent);
        }
    }
}

void QQuickItemPrivate::setLayoutMirror(bool mirror)
{
    if (mirror != effectiveLayoutMirror) {
        effectiveLayoutMirror = mirror;
        if (_anchors) {
            QQuickAnchorsPrivate *anchor_d = QQuickAnchorsPrivate::get(_anchors);
            anchor_d->fillChanged();
            anchor_d->centerInChanged();
            anchor_d->updateHorizontalAnchors();
        }
        mirrorChange();
        if (extra.isAllocated() && extra->layoutDirectionAttached) {
            emit extra->layoutDirectionAttached->enabledChanged();
        }
    }
}

/*!
    \qmltype EnterKey
    \instantiates QQuickEnterKeyAttached
    \inqmlmodule QtQuick
    \ingroup qtquick-input
    \since 5.6
    \brief Provides a property to manipulate the appearance of Enter key on
           an on-screen keyboard.

    The EnterKey attached property is used to manipulate the appearance and
    behavior of the Enter key on an on-screen keyboard.
*/

/*!
    \qmlattachedproperty enumeration QtQuick::EnterKey::type

    Holds the type of the Enter key.

    \note Not all of these values are supported on all platforms. For
          unsupported values the default key is used instead.

    \value Qt.EnterKeyDefault   The default Enter key. This can be either a
                                button to accept the input and close the
                                keyboard, or a \e Return button to enter a
                                newline in case of a multi-line input field.

    \value Qt.EnterKeyReturn    Show a \e Return button that inserts a
                                newline.

    \value Qt.EnterKeyDone      Show a \e {"Done"} button. Typically, the
                                keyboard is expected to close when the button
                                is pressed.

    \value Qt.EnterKeyGo        Show a \e {"Go"} button. Typically used in an
                                address bar when entering a URL.

    \value Qt.EnterKeySend      Show a \e {"Send"} button.

    \value Qt.EnterKeySearch    Show a \e {"Search"} button.

    \value Qt.EnterKeyNext      Show a \e {"Next"} button. Typically used in a
                                form to allow navigating to the next input
                                field without the keyboard closing.

    \value Qt.EnterKeyPrevious  Show a \e {"Previous"} button.
*/

QQuickEnterKeyAttached::QQuickEnterKeyAttached(QObject *parent)
    : QObject(parent), itemPrivate(nullptr), keyType(Qt::EnterKeyDefault)
{
    if (QQuickItem *item = qobject_cast<QQuickItem*>(parent)) {
        itemPrivate = QQuickItemPrivate::get(item);
        itemPrivate->extra.value().enterKeyAttached = this;
    } else
        qmlWarning(parent) << tr("EnterKey attached property only works with Items");
}

QQuickEnterKeyAttached *QQuickEnterKeyAttached::qmlAttachedProperties(QObject *object)
{
    return new QQuickEnterKeyAttached(object);
}

Qt::EnterKeyType QQuickEnterKeyAttached::type() const
{
    return keyType;
}

void QQuickEnterKeyAttached::setType(Qt::EnterKeyType type)
{
    if (keyType != type) {
        keyType = type;
#if QT_CONFIG(im)
        if (itemPrivate && itemPrivate->activeFocus)
            QGuiApplication::inputMethod()->update(Qt::ImEnterKeyType);
#endif
        typeChanged();
    }
}

void QQuickItemPrivate::setAccessible()
{
    isAccessible = true;
}

/*!
Clears all sub focus items from \a scope.
If \a focus is true, sets the scope's subFocusItem
to be this item.
*/
void QQuickItemPrivate::updateSubFocusItem(QQuickItem *scope, bool focus)
{
    Q_Q(QQuickItem);
    Q_ASSERT(scope);

    QQuickItemPrivate *scopePrivate = QQuickItemPrivate::get(scope);

    QQuickItem *oldSubFocusItem = scopePrivate->subFocusItem;
    // Correct focus chain in scope
    if (oldSubFocusItem) {
        QQuickItem *sfi = scopePrivate->subFocusItem->parentItem();
        while (sfi && sfi != scope) {
            QQuickItemPrivate::get(sfi)->subFocusItem = nullptr;
            sfi = sfi->parentItem();
        }
    }

    if (focus) {
        scopePrivate->subFocusItem = q;
        QQuickItem *sfi = scopePrivate->subFocusItem->parentItem();
        while (sfi && sfi != scope) {
            QQuickItemPrivate::get(sfi)->subFocusItem = q;
            sfi = sfi->parentItem();
        }
    } else {
        scopePrivate->subFocusItem = nullptr;
    }
}

/*!
    \class QQuickItem
    \brief The QQuickItem class provides the most basic of all visual items in \l {Qt Quick}.
    \inmodule QtQuick

    All visual items in Qt Quick inherit from QQuickItem. Although a QQuickItem
    instance has no visual appearance, it defines all the attributes that are
    common across visual items, such as x and y position, width and height,
    \l {Positioning with Anchors}{anchoring} and key handling support.

    You can subclass QQuickItem to provide your own custom visual item
    that inherits these features.

    \section1 Custom Scene Graph Items

    All visual QML items are rendered using the scene graph, the
    default implementation of which is a low-level, high-performance
    rendering stack, closely tied to accelerated graphics APIs, such
    as OpenGL, Vulkan, Metal, or Direct 3D. It is possible for
    subclasses of QQuickItem to add their own custom content into the
    scene graph by setting the QQuickItem::ItemHasContents flag and
    reimplementing the QQuickItem::updatePaintNode() function.

    \warning It is crucial that graphics operations and interaction with
    the scene graph happens exclusively on the rendering thread,
    primarily during the updatePaintNode() call. The best rule of
    thumb is to only use classes with the "QSG" prefix inside the
    QQuickItem::updatePaintNode() function.

    \note All classes with QSG prefix should be used solely on the scene graph's
    rendering thread. See \l {Scene Graph and Rendering} for more information.

    \section2 Graphics Resource Handling

    The preferred way to handle cleanup of graphics resources used in
    the scene graph, is to rely on the automatic cleanup of nodes. A
    QSGNode returned from QQuickItem::updatePaintNode() is
    automatically deleted on the right thread at the right time. Trees
    of QSGNode instances are managed through the use of
    QSGNode::OwnedByParent, which is set by default. So, for the
    majority of custom scene graph items, no extra work will be
    required.

    Implementations that store graphics resources outside the node
    tree, such as an item implementing QQuickItem::textureProvider(),
    will need to take care in cleaning it up correctly depending on
    how the item is used in QML. The situations to handle are:

    \list

    \li The scene graph is invalidated; This can happen, depending on
    the platform and QQuickWindow configuration, when the window is
    hidden using QQuickWindow::hide(), or when it is closed. If the
    item class implements a \c slot named \c invalidateSceneGraph(),
    this slot will be called on the rendering thread while the GUI
    thread is blocked. This is equivalent to connecting to
    QQuickWindow::sceneGraphInvalidated(). When rendering through
    OpenGL, the OpenGL context of this item's window will be bound
    when this slot is called. The only exception is if the native
    OpenGL has been destroyed outside Qt's control, for instance
    through \c EGL_CONTEXT_LOST.

    \li The item is removed from the scene; If an item is taken out of
    the scene, for instance because it's parent was set to \c null or
    an item in another window, the QQuickItem::releaseResources() will
    be called on the GUI thread. QQuickWindow::scheduleRenderJob()
    should be used to schedule cleanup of rendering resources.

    \li The item is deleted; When the destructor if an item runs, it
    should delete any graphics resources it has. If neither of the two
    conditions above were already met, the item will be part of a
    window and it is possible to use QQuickWindow::scheduleRenderJob()
    to have them cleaned up. If an implementation ignores the call to
    QQuickItem::releaseResources(), the item will in many cases no
    longer have access to a QQuickWindow and thus no means of
    scheduling cleanup.

    \endlist

    When scheduling cleanup of graphics resources using
    QQuickWindow::scheduleRenderJob(), one should use either
    QQuickWindow::BeforeSynchronizingStage or
    QQuickWindow::AfterSynchronizingStage. The \l {Scene Graph and
    Rendering}{synchronization stage} is where the scene graph is
    changed as a result of changes to the QML tree. If cleanup is
    scheduled at any other time, it may result in other parts of the
    scene graph referencing the newly deleted objects as these parts
    have not been updated.

    \note Use of QObject::deleteLater() to clean up graphics resources
    is strongly discouraged as this will make the \c delete operation
    run at an arbitrary time and it is unknown if there will be an
    OpenGL context bound when the deletion takes place.

    \section1 Custom QPainter Items

    The QQuickItem provides a subclass, QQuickPaintedItem, which
    allows the users to render content using QPainter.

    \warning Using QQuickPaintedItem uses an indirect 2D surface to
    render its content, using software rasterization, so the rendering
    is a two-step operation. First rasterize the surface, then draw
    the surface. Using scene graph API directly is always
    significantly faster.

    \section1 Behavior Animations

    If your Item uses the \l Behavior type to define animations for property
    changes, you should always use either QObject::setProperty(),
    QQmlProperty(), or QMetaProperty::write() when you need to modify those
    properties from C++. This ensures that the QML engine knows about the
    property change. Otherwise, the engine won't be able to carry out your
    requested animation.
    Note that these functions incur a slight performance penalty. For more
    details, see \l {Accessing Members of a QML Object Type from C++}.

    \sa QQuickWindow, QQuickPaintedItem
*/

/*!
    \qmltype Item
    \instantiates QQuickItem
    \inherits QtObject
    \inqmlmodule QtQuick
    \ingroup qtquick-visual
    \brief A basic visual QML type.

    The Item type is the base type for all visual items in Qt Quick.

    All visual items in Qt Quick inherit from Item. Although an Item
    object has no visual appearance, it defines all the attributes that are
    common across visual items, such as x and y position, width and height,
    \l {Positioning with Anchors}{anchoring} and key handling support.

    The Item type can be useful for grouping several items under a single
    root visual item. For example:

    \qml
    import QtQuick 2.0

    Item {
        Image {
            source: "tile.png"
        }
        Image {
            x: 80
            width: 100
            height: 100
            source: "tile.png"
        }
        Image {
            x: 190
            width: 100
            height: 100
            fillMode: Image.Tile
            source: "tile.png"
        }
    }
    \endqml


    \section2 Event Handling

    All Item-based visual types can use \l {Qt Quick Input Handlers}{Input Handlers}
    to handle incoming input events (subclasses of QInputEvent), such as mouse,
    touch and key events. This is the preferred declarative way to handle events.

    An alternative way to handle touch events is to subclass QQuickItem, call
    setAcceptTouchEvents() in the constructor, and override touchEvent().
    \l {QEvent::setAccepted()}{Accept} the entire event to stop delivery to
    items underneath, and to exclusively grab for all the event's touch points.
    Use QPointerEvent::setExclusiveGrabber() to grab only certain touchpoints,
    and allow the event to be delivered further.

    Likewise, a QQuickItem subclass can call setAcceptedMouseButtons()
    to register to receive mouse button events, setAcceptHoverEvents()
    to receive hover events (mouse movements while no button is pressed),
    and override the virtual functions mousePressEvent(), mouseMoveEvent(), and
    mouseReleaseEvent(). Those can also accept the event to prevent further
    delivery and get an implicit grab at the same time; or explicitly
    \l {QPointerEvent::setExclusiveGrabber()}{grab} the single QEventPoint
    that the QMouseEvent carries.

    Key handling is available to all Item-based visual types via the \l Keys
    attached property.  The \e Keys attached property provides basic signals
    such as \l {Keys::}{pressed} and \l {Keys::}{released}, as well as
    signals for specific keys, such as \l {Keys::}{spacePressed}.  The
    example below assigns \l {Keyboard Focus in Qt Quick}{keyboard focus} to
    the item and handles the left key via the general \c onPressed handler
    and the return key via the \c onReturnPressed handler:

    \qml
    import QtQuick 2.0

    Item {
        focus: true
        Keys.onPressed: (event)=> {
            if (event.key == Qt.Key_Left) {
                console.log("move left");
                event.accepted = true;
            }
        }
        Keys.onReturnPressed: console.log("Pressed return");
    }
    \endqml

    See the \l Keys attached property for detailed documentation.

    \section2 Layout Mirroring

    Item layouts can be mirrored using the \l LayoutMirroring attached
    property. This causes \l{anchors.top}{anchors} to be horizontally
    reversed, and also causes items that lay out or position their children
    (such as ListView or \l Row) to horizontally reverse the direction of
    their layouts.

    See LayoutMirroring for more details.

    \section1 Item Layers

    An Item will normally be rendered directly into the window it
    belongs to. However, by setting \l layer.enabled, it is possible
    to delegate the item and its entire subtree into an offscreen
    surface. Only the offscreen surface, a texture, will be then drawn
    into the window.

    If it is desired to have a texture size different from that of the
    item, this is possible using \l layer.textureSize. To render only
    a section of the item into the texture, use \l
    layer.sourceRect. It is also possible to specify \l
    layer.sourceRect so it extends beyond the bounds of the item. In
    this case, the exterior will be padded with transparent pixels.

    The item will use linear interpolation for scaling if
    \l layer.smooth is set to \c true and will use mipmap for
    downsampling if \l layer.mipmap is set to \c true. Mipmapping may
    improve visual quality of downscaled items. For mipmapping of
    single Image items, prefer Image::mipmap.

    \section2 Layer Opacity vs Item Opacity

    When applying \l opacity to an item hierarchy the opacity is
    applied to each item individually. This can lead to undesired
    visual results when the opacity is applied to a subtree. Consider
    the following example:

    \table
    \row
      \li \inlineimage qml-blending-nonlayered.png
      \li \b {Non-layered Opacity} \snippet qml/layerblending.qml non-layered
    \endtable

    A layer is rendered with the root item's opacity being 1, and then
    the root item's opacity is applied to the texture when it is
    drawn. This means that fading in a large item hierarchy from
    transparent to opaque, or vice versa, can be done without the
    overlap artifacts that the normal item by item alpha blending
    has. Here is the same example with layer enabled:

    \table
    \row
      \li \image qml-blending-layered.png
      \li \b {Layered Opacity} \snippet qml/layerblending.qml layered
    \endtable

    \section2 Combined with ShaderEffects

    Setting \l layer.enabled to true will turn the item into a \l
    {QQuickItem::isTextureProvider}{texture provider}, making it
    possible to use the item directly as a texture, for instance
    in combination with the ShaderEffect type.

    It is possible to apply an effect on a layer at runtime using
    layer.effect:

    \qml
    Item {
        id: layerRoot
        layer.enabled: true
        layer.effect: ShaderEffect {
            fragmentShader: "effect.frag.qsb"
        }
    }
    \endqml

    See ShaderEffect for more information about using effects.

    \note \l layer.enabled is actually just a more convenient way of using
    ShaderEffectSource.


    \section2 Memory and Performance

    When an item's layer is enabled, the scene graph will allocate memory
    in the GPU equal to \c {width x height x 4}. In memory constrained
    configurations, large layers should be used with care.

    In the QPainter / QWidget world, it is sometimes favorable to
    cache complex content in a pixmap, image or texture. In Qt Quick,
    because of the techniques already applied by the \l {Qt Quick
    Scene Graph Default Renderer} {scene graph renderer}, this will in most
    cases not be the case. Excessive draw calls are already reduced
    because of batching and a cache will in most cases end up blending
    more pixels than the original content. The overhead of rendering
    to an offscreen and the blending involved with drawing the
    resulting texture is therefore often more costly than simply
    letting the item and its children be drawn normally.

    Also, an item using a layer can not be \l {Batching} {batched} during
    rendering. This means that a scene with many layered items may
    have performance problems.

    Layering can be convenient and useful for visual effects, but
    should in most cases be enabled for the duration of the effect and
    disabled afterwards.

*/

/*!
    \enum QQuickItem::Flag

    This enum type is used to specify various item properties.

    \value ItemClipsChildrenToShape Indicates this item should visually clip
    its children so that they are rendered only within the boundaries of this
    item.
    \value ItemAcceptsInputMethod Indicates the item supports text input
    methods.
    \value ItemIsFocusScope Indicates the item is a focus scope. See
    \l {Keyboard Focus in Qt Quick} for more information.
    \value ItemHasContents Indicates the item has visual content and should be
    rendered by the scene graph.
    \value ItemAcceptsDrops Indicates the item accepts drag and drop events.
    \value ItemIsViewport Indicates that the item defines a viewport for its children.
    \value ItemObservesViewport Indicates that the item wishes to know the
    viewport bounds when any ancestor has the ItemIsViewport flag set.

    \sa setFlag(), setFlags(), flags()
*/

/*!
    \enum QQuickItem::ItemChange
    \brief Used in conjunction with QQuickItem::itemChange() to notify
    the item about certain types of changes.

    \value ItemChildAddedChange A child was added. ItemChangeData::item contains
    the added child.

    \value ItemChildRemovedChange A child was removed. ItemChangeData::item
    contains the removed child.

    \value ItemSceneChange The item was added to or removed from a scene. The
    QQuickWindow rendering the scene is specified in using ItemChangeData::window.
    The window parameter is null when the item is removed from a scene.

    \value ItemVisibleHasChanged The item's visibility has changed.
    ItemChangeData::boolValue contains the new visibility.

    \value ItemParentHasChanged The item's parent has changed.
    ItemChangeData::item contains the new parent.

    \value ItemOpacityHasChanged The item's opacity has changed.
    ItemChangeData::realValue contains the new opacity.

    \value ItemActiveFocusHasChanged The item's focus has changed.
    ItemChangeData::boolValue contains whether the item has focus or not.

    \value ItemRotationHasChanged The item's rotation has changed.
    ItemChangeData::realValue contains the new rotation.

    \value ItemDevicePixelRatioHasChanged The device pixel ratio of the screen
    the item is on has changed. ItemChangedData::realValue contains the new
    device pixel ratio.

    \value ItemAntialiasingHasChanged The antialiasing has changed. The current
    (boolean) value can be found in QQuickItem::antialiasing.

    \value ItemEnabledHasChanged The item's enabled state has changed.
    ItemChangeData::boolValue contains the new enabled state. (since Qt 5.10)
*/

/*!
    \class QQuickItem::ItemChangeData
    \inmodule QtQuick
    \brief Adds supplementary information to the QQuickItem::itemChange()
    function.

    The meaning of each member of this class is defined by the change type.

    \sa QQuickItem::ItemChange
*/

/*!
    \fn QQuickItem::ItemChangeData::ItemChangeData(QQuickItem *)
    \internal
 */

/*!
    \fn QQuickItem::ItemChangeData::ItemChangeData(QQuickWindow *)
    \internal
 */

/*!
    \fn QQuickItem::ItemChangeData::ItemChangeData(qreal)
    \internal
 */

/*!
    \fn QQuickItem::ItemChangeData::ItemChangeData(bool)
    \internal
 */

/*!
    \variable QQuickItem::ItemChangeData::realValue
    The numeric value that has changed: \l {QQuickItem::opacity()}{opacity},
    \l {QQuickItem::rotation()}{rotation}, or
    \l {QScreen::devicePixelRatio}{device pixel ratio}.
    \sa QQuickItem::ItemChange
 */

/*!
    \variable QQuickItem::ItemChangeData::boolValue
    The boolean value that has changed: \l {QQuickItem::isVisible()}{visible},
    \l {QQuickItem::isEnabled()}{enabled}, \l {QQuickItem::hasActiveFocus()}{activeFocus},
    or \l {QQuickItem::antialiasing()}{antialiasing}.
    \sa QQuickItem::ItemChange
 */

/*!
    \variable QQuickItem::ItemChangeData::item
    The item that has been added or removed as a \l{QQuickItem::childItems()}{child},
    or the new \l{QQuickItem::parentItem()}{parent}.
    \sa QQuickItem::ItemChange
 */

/*!
    \variable QQuickItem::ItemChangeData::window
    The \l{QQuickWindow}{window} in which the item has been shown, or \c nullptr
    if the item has been removed from a window.
    \sa QQuickItem::ItemChange
 */

/*!
    \enum QQuickItem::TransformOrigin

    Controls the point about which simple transforms like scale apply.

    \value TopLeft The top-left corner of the item.
    \value Top The center point of the top of the item.
    \value TopRight The top-right corner of the item.
    \value Left The left most point of the vertical middle.
    \value Center The center of the item.
    \value Right The right most point of the vertical middle.
    \value BottomLeft The bottom-left corner of the item.
    \value Bottom The center point of the bottom of the item.
    \value BottomRight The bottom-right corner of the item.

    \sa transformOrigin(), setTransformOrigin()
*/

/*!
    \fn void QQuickItem::childrenRectChanged(const QRectF &)
    \internal
*/

/*!
    \fn void QQuickItem::baselineOffsetChanged(qreal)
    \internal
*/

/*!
    \fn void QQuickItem::stateChanged(const QString &state)
    \internal
*/

/*!
    \fn void QQuickItem::parentChanged(QQuickItem *)
    \internal
*/

/*!
    \fn void QQuickItem::smoothChanged(bool)
    \internal
*/

/*!
    \fn void QQuickItem::antialiasingChanged(bool)
    \internal
*/

/*!
    \fn void QQuickItem::clipChanged(bool)
    \internal
*/

/*!
    \fn void QQuickItem::transformOriginChanged(TransformOrigin)
    \internal
*/

/*!
    \fn void QQuickItem::focusChanged(bool)
    \internal
*/

/*!
    \fn void QQuickItem::activeFocusChanged(bool)
    \internal
*/

/*!
    \fn void QQuickItem::activeFocusOnTabChanged(bool)
    \internal
*/

/*!
    \fn void QQuickItem::childrenChanged()
    \internal
*/

/*!
    \fn void QQuickItem::opacityChanged()
    \internal
*/

/*!
    \fn void QQuickItem::enabledChanged()
    \internal
*/

/*!
    \fn void QQuickItem::visibleChanged()
    \internal
*/

/*!
    \fn void QQuickItem::visibleChildrenChanged()
    \internal
*/

/*!
    \fn void QQuickItem::rotationChanged()
    \internal
*/

/*!
    \fn void QQuickItem::scaleChanged()
    \internal
*/

/*!
    \fn void QQuickItem::xChanged()
    \internal
*/

/*!
    \fn void QQuickItem::yChanged()
    \internal
*/

/*!
    \fn void QQuickItem::widthChanged()
    \internal
*/

/*!
    \fn void QQuickItem::heightChanged()
    \internal
*/

/*!
    \fn void QQuickItem::zChanged()
    \internal
*/

/*!
    \fn void QQuickItem::implicitWidthChanged()
    \internal
*/

/*!
    \fn void QQuickItem::implicitHeightChanged()
    \internal
*/

/*!
    \fn QQuickItem::QQuickItem(QQuickItem *parent)

    Constructs a QQuickItem with the given \a parent.

    The \c parent will be used as both the \l {setParentItem()}{visual parent}
    and the \l QObject parent.
*/
QQuickItem::QQuickItem(QQuickItem* parent)
: QObject(*(new QQuickItemPrivate), parent)
{
    Q_D(QQuickItem);
    d->init(parent);
}

/*! \internal
*/
QQuickItem::QQuickItem(QQuickItemPrivate &dd, QQuickItem *parent)
: QObject(dd, parent)
{
    Q_D(QQuickItem);
    d->init(parent);
}

/*!
    Destroys the QQuickItem.
*/
QQuickItem::~QQuickItem()
{
    Q_D(QQuickItem);
    d->inDestructor = true;

    if (d->windowRefCount > 1)
        d->windowRefCount = 1; // Make sure window is set to null in next call to derefWindow().
    if (d->parentItem)
        setParentItem(nullptr);
    else if (d->window)
        d->derefWindow();

    for (QQuickItem *child : std::as_const(d->childItems))
        child->setParentItem(nullptr);
    d->childItems.clear();

    d->notifyChangeListeners(QQuickItemPrivate::AllChanges, [this](const QQuickItemPrivate::ChangeListener &change){
        QQuickAnchorsPrivate *anchor = change.listener->anchorPrivate();
        if (anchor)
            anchor->clearItem(this);
    });
    /*
        update item anchors that depended on us unless they are our child (and will also be destroyed),
        or our sibling, and our parent is also being destroyed.
    */
    d->notifyChangeListeners(QQuickItemPrivate::AllChanges, [this](const QQuickItemPrivate::ChangeListener &change){
        QQuickAnchorsPrivate *anchor = change.listener->anchorPrivate();
        if (anchor && anchor->item && anchor->item->parentItem() && anchor->item->parentItem() != this)
            anchor->update();
    });
    d->notifyChangeListeners(QQuickItemPrivate::Destroyed, &QQuickItemChangeListener::itemDestroyed, this);
    d->changeListeners.clear();

    /*
       Remove any references our transforms have to us, in case they try to
       remove themselves from our list of transforms when that list has already
       been destroyed after ~QQuickItem() has run.
    */
    for (int ii = 0; ii < d->transforms.size(); ++ii) {
        QQuickTransform *t = d->transforms.at(ii);
        QQuickTransformPrivate *tp = QQuickTransformPrivate::get(t);
        tp->items.removeOne(this);
    }

    if (d->extra.isAllocated()) {
        delete d->extra->contents; d->extra->contents = nullptr;
#if QT_CONFIG(quick_shadereffect)
        delete d->extra->layer; d->extra->layer = nullptr;
#endif
    }

    delete d->_anchors; d->_anchors = nullptr;
    delete d->_stateGroup; d->_stateGroup = nullptr;

    d->isQuickItem = false;
}

/*!
    \internal
*/
bool QQuickItemPrivate::canAcceptTabFocus(QQuickItem *item)
{
    if (!item->window())
        return false;

    if (item == item->window()->contentItem())
        return true;

#if QT_CONFIG(accessibility)
    QAccessible::Role role = QQuickItemPrivate::get(item)->effectiveAccessibleRole();
    if (role == QAccessible::EditableText || role == QAccessible::Table || role == QAccessible::List) {
        return true;
    } else if (role == QAccessible::ComboBox || role == QAccessible::SpinBox) {
        if (QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(item))
            return iface->state().editable;
    }
#endif

    QVariant editable = item->property("editable");
    if (editable.isValid())
        return editable.toBool();

    QVariant readonly = item->property("readOnly");
    if (readonly.isValid() && !readonly.toBool() && item->property("text").isValid())
        return true;

    return false;
}

/*!
    \internal
    \brief QQuickItemPrivate::focusNextPrev focuses the next/prev item in the tab-focus-chain
    \param item The item that currently has the focus
    \param forward The direction
    \return Whether the next item in the focus chain is found or not

    If \a next is true, the next item visited will be in depth-first order relative to \a item.
    If \a next is false, the next item visited will be in reverse depth-first order relative to \a item.
*/
bool QQuickItemPrivate::focusNextPrev(QQuickItem *item, bool forward)
{
    QQuickItem *next = QQuickItemPrivate::nextPrevItemInTabFocusChain(item, forward);

    if (next == item)
        return false;

    next->forceActiveFocus(forward ? Qt::TabFocusReason : Qt::BacktabFocusReason);

    return true;
}

QQuickItem *QQuickItemPrivate::nextTabChildItem(const QQuickItem *item, int start)
{
    if (!item) {
        qWarning() << "QQuickItemPrivate::nextTabChildItem called with null item.";
        return nullptr;
    }
    const QList<QQuickItem *> &children = item->childItems();
    const int count = children.size();
    if (start < 0 || start >= count) {
        qWarning() << "QQuickItemPrivate::nextTabChildItem: Start index value out of range for item" << item;
        return nullptr;
    }
    while (start < count) {
        QQuickItem *child = children.at(start);
        if (!child->d_func()->isTabFence)
            return child;
        ++start;
    }
    return nullptr;
}

QQuickItem *QQuickItemPrivate::prevTabChildItem(const QQuickItem *item, int start)
{
    if (!item) {
        qWarning() << "QQuickItemPrivate::prevTabChildItem called with null item.";
        return nullptr;
    }
    const QList<QQuickItem *> &children = item->childItems();
    const int count = children.size();
    if (start == -1)
        start = count - 1;
    if (start < 0 || start >= count) {
        qWarning() << "QQuickItemPrivate::prevTabChildItem: Start index value out of range for item" << item;
        return nullptr;
    }
    while (start >= 0) {
        QQuickItem *child = children.at(start);
        if (!child->d_func()->isTabFence)
            return child;
        --start;
    }
    return nullptr;
}

QQuickItem* QQuickItemPrivate::nextPrevItemInTabFocusChain(QQuickItem *item, bool forward)
{
    Q_ASSERT(item);
    qCDebug(lcFocus) << "QQuickItemPrivate::nextPrevItemInTabFocusChain: item:" << item << ", forward:" << forward;

    if (!item->window())
        return item;
    const QQuickItem * const contentItem = item->window()->contentItem();
    if (!contentItem)
        return item;

    bool all = QGuiApplication::styleHints()->tabFocusBehavior() == Qt::TabFocusAllControls;

    QQuickItem *from = nullptr;
    bool isTabFence = item->d_func()->isTabFence;
    if (forward) {
        if (!isTabFence)
            from = item->parentItem();
    } else {
        if (!item->childItems().isEmpty())
            from = item->d_func()->childItems.constFirst();
        else if (!isTabFence)
            from = item->parentItem();
    }
    bool skip = false;

    QQuickItem *startItem = item;
    QQuickItem *originalStartItem = startItem;
    // Protect from endless loop:
    // If we start on an invisible item we will not find it again.
    // If there is no other item which can become the focus item, we have a forever loop,
    // since the protection only works if we encounter the first item again.
    while (startItem && !startItem->isVisible()) {
        startItem = startItem->parentItem();
    }
    if (!startItem)
        return item;

    QQuickItem *firstFromItem = from;
    QQuickItem *current = item;
    qCDebug(lcFocus) << "QQuickItemPrivate::nextPrevItemInTabFocusChain: startItem:" << startItem;
    qCDebug(lcFocus) << "QQuickItemPrivate::nextPrevItemInTabFocusChain: firstFromItem:" << firstFromItem;
    QDuplicateTracker<QQuickItem *> cycleDetector;
    do {
        qCDebug(lcFocus) << "QQuickItemPrivate::nextPrevItemInTabFocusChain: current:" << current;
        qCDebug(lcFocus) << "QQuickItemPrivate::nextPrevItemInTabFocusChain: from:" << from;
        skip = false;
        QQuickItem *last = current;

        bool hasChildren = !current->childItems().isEmpty() && current->isEnabled() && current->isVisible();
        QQuickItem *firstChild = nullptr;
        QQuickItem *lastChild = nullptr;
        if (hasChildren) {
            firstChild = nextTabChildItem(current, 0);
            if (!firstChild)
                hasChildren = false;
            else
                lastChild = prevTabChildItem(current, -1);
        }
        isTabFence = current->d_func()->isTabFence;
        if (isTabFence && !hasChildren)
            return current;

        // coming from parent: check children
        if (hasChildren && from == current->parentItem()) {
            if (forward) {
                current = firstChild;
            } else {
                current = lastChild;
                if (!current->childItems().isEmpty())
                    skip = true;
            }
        } else if (hasChildren && forward && from != lastChild) {
            // not last child going forwards
            int nextChild = current->childItems().indexOf(from) + 1;
            current = nextTabChildItem(current, nextChild);
        } else if (hasChildren && !forward && from != firstChild) {
            // not first child going backwards
            int prevChild = current->childItems().indexOf(from) - 1;
            current = prevTabChildItem(current, prevChild);
            if (!current->childItems().isEmpty())
                skip = true;
        // back to the parent
        } else if (QQuickItem *parent = !isTabFence ? current->parentItem() : nullptr) {
            // we would evaluate the parent twice, thus we skip
            if (forward) {
                skip = true;
            } else if (QQuickItem *firstSibling = !forward ? nextTabChildItem(parent, 0) : nullptr) {
                if (last != firstSibling
                    || (parent->isFocusScope() && parent->activeFocusOnTab() && parent->hasActiveFocus()))
                        skip = true;
            }
            current = parent;
        } else if (hasChildren) {
            // Wrap around after checking all items forward
            if (forward) {
                current = firstChild;
            } else {
                current = lastChild;
                if (!current->childItems().isEmpty())
                    skip = true;
            }
        }
        from = last;
        // if [from] item is equal to [firstFromItem], means we have traversed one path and
        // jump back to parent of the chain, and then we have to check whether we have
        // traversed all of the chain (by compare the [current] item with [startItem])
        // Since the [startItem] might be promoted to its parent if it is invisible,
        // we still have to check [current] item with original start item
        // We might also run into a cycle before we reach firstFromItem again
        // but note that we have to ignore current if we are meant to skip it
        if (((current == startItem || current == originalStartItem) && from == firstFromItem) ||
                (!skip && cycleDetector.hasSeen(current))) {
            // wrapped around, avoid endless loops
            if (item == contentItem) {
                qCDebug(lcFocus) << "QQuickItemPrivate::nextPrevItemInTabFocusChain: looped, return contentItem";
                return item;
            } else {
                qCDebug(lcFocus) << "QQuickItemPrivate::nextPrevItemInTabFocusChain: looped, return " << startItem;
                return startItem;
            }
        }
        if (!firstFromItem) {
            if (startItem->d_func()->isTabFence) {
                if (current == startItem)
                    firstFromItem = from;
            } else { //start from root
                startItem = current;
                firstFromItem = from;
            }
        }
    } while (skip || !current->activeFocusOnTab() || !current->isEnabled() || !current->isVisible()
                  || !(all || QQuickItemPrivate::canAcceptTabFocus(current)));

    return current;
}

/*!
    \qmlproperty Item QtQuick::Item::parent
    This property holds the visual parent of the item.

    \note The concept of the \e {visual parent} differs from that of the
    \e {QObject parent}. An item's visual parent may not necessarily be the
    same as its object parent. See \l {Concepts - Visual Parent in Qt Quick}
    for more details.
*/
/*!
    \property QQuickItem::parent
    This property holds the visual parent of the item.

    \note The concept of the \e {visual parent} differs from that of the
    \e {QObject parent}. An item's visual parent may not necessarily be the
    same as its object parent. See \l {Concepts - Visual Parent in Qt Quick}
    for more details.

    \note The notification signal for this property gets emitted during destruction
    of the visual parent. C++ signal handlers cannot assume that items in the
    visual parent hierarchy are still fully constructed. Use \l qobject_cast to
    verify that items in the parent hierarchy can be used safely as the expected
    type.
*/
QQuickItem *QQuickItem::parentItem() const
{
    Q_D(const QQuickItem);
    return d->parentItem;
}

void QQuickItem::setParentItem(QQuickItem *parentItem)
{
    Q_D(QQuickItem);
    if (parentItem == d->parentItem)
        return;

    if (parentItem) {
        QQuickItem *itemAncestor = parentItem;
        while (itemAncestor != nullptr) {
            if (Q_UNLIKELY(itemAncestor == this)) {
                qWarning() << "QQuickItem::setParentItem: Parent" << parentItem << "is already part of the subtree of" << this;
                return;
            }
            itemAncestor = itemAncestor->parentItem();
        }
    }

    d->removeFromDirtyList();

    QQuickItem *oldParentItem = d->parentItem;
    QQuickItem *scopeFocusedItem = nullptr;

    if (oldParentItem) {
        QQuickItemPrivate *op = QQuickItemPrivate::get(oldParentItem);

        QQuickItem *scopeItem = nullptr;

        if (hasFocus() || op->subFocusItem == this)
            scopeFocusedItem = this;
        else if (!isFocusScope() && d->subFocusItem)
            scopeFocusedItem = d->subFocusItem;

        if (scopeFocusedItem) {
            scopeItem = oldParentItem;
            while (!scopeItem->isFocusScope() && scopeItem->parentItem())
                scopeItem = scopeItem->parentItem();
            if (d->window) {
                d->deliveryAgentPrivate()->
                        clearFocusInScope(scopeItem, scopeFocusedItem, Qt::OtherFocusReason,
                                          QQuickDeliveryAgentPrivate::DontChangeFocusProperty);
                if (scopeFocusedItem != this)
                    QQuickItemPrivate::get(scopeFocusedItem)->updateSubFocusItem(this, true);
            } else {
                QQuickItemPrivate::get(scopeFocusedItem)->updateSubFocusItem(scopeItem, false);
            }
        }

        const bool wasVisible = isVisible();
        op->removeChild(this);
        if (wasVisible && !op->inDestructor)
            emit oldParentItem->visibleChildrenChanged();
    } else if (d->window) {
        QQuickWindowPrivate::get(d->window)->parentlessItems.remove(this);
    }

    QQuickWindow *parentWindow = parentItem ? QQuickItemPrivate::get(parentItem)->window : nullptr;
    bool alreadyAddedChild = false;
    if (d->window == parentWindow) {
        // Avoid freeing and reallocating resources if the window stays the same.
        d->parentItem = parentItem;
    } else {
        auto oldParentItem = d->parentItem;
        d->parentItem = parentItem;
        if (d->parentItem) {
            QQuickItemPrivate::get(d->parentItem)->addChild(this);
            alreadyAddedChild = true;
        }
        if (d->window) {
            d->derefWindow();
            // as we potentially changed d->parentWindow above
            // the check in derefWindow could not work
            // thus, we redo it here with the old parent
            // Also, the window may have been deleted by derefWindow()
            if (!oldParentItem && d->window) {
                QQuickWindowPrivate::get(d->window)->parentlessItems.remove(this);
            }
        }
        if (parentWindow)
            d->refWindow(parentWindow);
    }

    d->dirty(QQuickItemPrivate::ParentChanged);

    if (d->parentItem && !alreadyAddedChild)
        QQuickItemPrivate::get(d->parentItem)->addChild(this);
    else if (d->window && !alreadyAddedChild)
        QQuickWindowPrivate::get(d->window)->parentlessItems.insert(this);

    d->setEffectiveVisibleRecur(d->calcEffectiveVisible());
    d->setEffectiveEnableRecur(nullptr, d->calcEffectiveEnable());

    if (d->parentItem) {
        if (!scopeFocusedItem) {
            if (hasFocus())
                scopeFocusedItem = this;
            else if (!isFocusScope() && d->subFocusItem)
                scopeFocusedItem = d->subFocusItem;
        }

        if (scopeFocusedItem) {
            // We need to test whether this item becomes scope focused
            QQuickItem *scopeItem = d->parentItem;
            while (!scopeItem->isFocusScope() && scopeItem->parentItem())
                scopeItem = scopeItem->parentItem();

            if (QQuickItemPrivate::get(scopeItem)->subFocusItem
                    || (!scopeItem->isFocusScope() && scopeItem->hasFocus())) {
                if (scopeFocusedItem != this)
                    QQuickItemPrivate::get(scopeFocusedItem)->updateSubFocusItem(this, false);
                QQuickItemPrivate::get(scopeFocusedItem)->focus = false;
                emit scopeFocusedItem->focusChanged(false);
            } else {
                if (d->window) {
                    d->deliveryAgentPrivate()->
                            setFocusInScope(scopeItem, scopeFocusedItem, Qt::OtherFocusReason,
                                            QQuickDeliveryAgentPrivate::DontChangeFocusProperty);
                } else {
                    QQuickItemPrivate::get(scopeFocusedItem)->updateSubFocusItem(scopeItem, true);
                }
            }
        }
    }

    if (d->parentItem)
        d->resolveLayoutMirror();

    d->itemChange(ItemParentHasChanged, d->parentItem);

    if (!d->inDestructor)
        emit parentChanged(d->parentItem);
    if (isVisible() && d->parentItem && !QQuickItemPrivate::get(d->parentItem)->inDestructor)
        emit d->parentItem->visibleChildrenChanged();
}

/*!
    Moves the specified \a sibling item to the index before this item
    within the list of children. The order of children affects both the
    visual stacking order and tab focus navigation order.

    Assuming the z values of both items are the same, this will cause \a
    sibling to be rendered above this item.

    If both items have activeFocusOnTab set to \c true, this will also cause
    the tab focus order to change, with \a sibling receiving focus after this
    item.

    The given \a sibling must be a sibling of this item; that is, they must
    have the same immediate \l parent.

    \sa {Concepts - Visual Parent in Qt Quick}
*/
void QQuickItem::stackBefore(const QQuickItem *sibling)
{
    Q_D(QQuickItem);
    if (!sibling || sibling == this || !d->parentItem || d->parentItem != QQuickItemPrivate::get(sibling)->parentItem) {
        qWarning().nospace() << "QQuickItem::stackBefore: Cannot stack "
            << this << " before " << sibling << ", which must be a sibling";
        return;
    }

    QQuickItemPrivate *parentPrivate = QQuickItemPrivate::get(d->parentItem);

    int myIndex = parentPrivate->childItems.lastIndexOf(this);
    int siblingIndex = parentPrivate->childItems.lastIndexOf(const_cast<QQuickItem *>(sibling));

    Q_ASSERT(myIndex != -1 && siblingIndex != -1);

    if (myIndex == siblingIndex - 1)
        return;

    parentPrivate->childItems.move(myIndex, myIndex < siblingIndex ? siblingIndex - 1 : siblingIndex);

    parentPrivate->dirty(QQuickItemPrivate::ChildrenStackingChanged);
    parentPrivate->markSortedChildrenDirty(this);

    for (int ii = qMin(siblingIndex, myIndex); ii < parentPrivate->childItems.size(); ++ii)
        QQuickItemPrivate::get(parentPrivate->childItems.at(ii))->siblingOrderChanged();
}

/*!
    Moves the specified \a sibling item to the index after this item
    within the list of children. The order of children affects both the
    visual stacking order and tab focus navigation order.

    Assuming the z values of both items are the same, this will cause \a
    sibling to be rendered below this item.

    If both items have activeFocusOnTab set to \c true, this will also cause
    the tab focus order to change, with \a sibling receiving focus before this
    item.

    The given \a sibling must be a sibling of this item; that is, they must
    have the same immediate \l parent.

    \sa {Concepts - Visual Parent in Qt Quick}
*/
void QQuickItem::stackAfter(const QQuickItem *sibling)
{
    Q_D(QQuickItem);
    if (!sibling || sibling == this || !d->parentItem || d->parentItem != QQuickItemPrivate::get(sibling)->parentItem) {
        qWarning().nospace() << "QQuickItem::stackAfter: Cannot stack "
            << this << " after " << sibling << ", which must be a sibling";
        return;
    }

    QQuickItemPrivate *parentPrivate = QQuickItemPrivate::get(d->parentItem);

    int myIndex = parentPrivate->childItems.lastIndexOf(this);
    int siblingIndex = parentPrivate->childItems.lastIndexOf(const_cast<QQuickItem *>(sibling));

    Q_ASSERT(myIndex != -1 && siblingIndex != -1);

    if (myIndex == siblingIndex + 1)
        return;

    parentPrivate->childItems.move(myIndex, myIndex > siblingIndex ? siblingIndex + 1 : siblingIndex);

    parentPrivate->dirty(QQuickItemPrivate::ChildrenStackingChanged);
    parentPrivate->markSortedChildrenDirty(this);

    for (int ii = qMin(myIndex, siblingIndex + 1); ii < parentPrivate->childItems.size(); ++ii)
        QQuickItemPrivate::get(parentPrivate->childItems.at(ii))->siblingOrderChanged();
}

/*! \fn void QQuickItem::windowChanged(QQuickWindow *window)
    This signal is emitted when the item's \a window changes.
*/

/*!
  Returns the window in which this item is rendered.

  The item does not have a window until it has been assigned into a scene. The
  \l windowChanged() signal provides a notification both when the item is entered
  into a scene and when it is removed from a scene.
  */
QQuickWindow *QQuickItem::window() const
{
    Q_D(const QQuickItem);
    return d->window;
}

static bool itemZOrder_sort(QQuickItem *lhs, QQuickItem *rhs)
{
    return lhs->z() < rhs->z();
}

QList<QQuickItem *> QQuickItemPrivate::paintOrderChildItems() const
{
    if (sortedChildItems)
        return *sortedChildItems;

    // If none of the items have set Z then the paint order list is the same as
    // the childItems list.  This is by far the most common case.
    bool haveZ = false;
    for (int i = 0; i < childItems.size(); ++i) {
        if (QQuickItemPrivate::get(childItems.at(i))->z() != 0.) {
            haveZ = true;
            break;
        }
    }
    if (haveZ) {
        sortedChildItems = new QList<QQuickItem*>(childItems);
        std::stable_sort(sortedChildItems->begin(), sortedChildItems->end(), itemZOrder_sort);
        return *sortedChildItems;
    }

    sortedChildItems = const_cast<QList<QQuickItem*>*>(&childItems);

    return childItems;
}

void QQuickItemPrivate::addChild(QQuickItem *child)
{
    Q_Q(QQuickItem);

    Q_ASSERT(!childItems.contains(child));

    childItems.append(child);

    QQuickItemPrivate *childPrivate = QQuickItemPrivate::get(child);

#if QT_CONFIG(cursor)
    // if the added child has a cursor and we do not currently have any children
    // with cursors, bubble the notification up
    if (childPrivate->subtreeCursorEnabled && !subtreeCursorEnabled)
        setHasCursorInChild(true);
#endif

    if (childPrivate->subtreeHoverEnabled && !subtreeHoverEnabled)
        setHasHoverInChild(true);

    childPrivate->recursiveRefFromEffectItem(extra.value().recursiveEffectRefCount);
    markSortedChildrenDirty(child);
    dirty(QQuickItemPrivate::ChildrenChanged);

    itemChange(QQuickItem::ItemChildAddedChange, child);

    emit q->childrenChanged();
}

void QQuickItemPrivate::removeChild(QQuickItem *child)
{
    Q_Q(QQuickItem);

    Q_ASSERT(child);
    if (!inDestructor) {
        // if we are getting destroyed, then the destructor will clear the list
        Q_ASSERT(childItems.contains(child));
        childItems.removeOne(child);
        Q_ASSERT(!childItems.contains(child));
    }

    QQuickItemPrivate *childPrivate = QQuickItemPrivate::get(child);

#if QT_CONFIG(cursor)
    // turn it off, if nothing else is using it
    if (childPrivate->subtreeCursorEnabled && subtreeCursorEnabled)
        setHasCursorInChild(false);
#endif

    if (childPrivate->subtreeHoverEnabled && subtreeHoverEnabled)
        setHasHoverInChild(false);

    childPrivate->recursiveRefFromEffectItem(-extra.value().recursiveEffectRefCount);
    if (!inDestructor) {
        markSortedChildrenDirty(child);
        dirty(QQuickItemPrivate::ChildrenChanged);
    }

    itemChange(QQuickItem::ItemChildRemovedChange, child);

    if (!inDestructor)
        emit q->childrenChanged();
}

void QQuickItemPrivate::refWindow(QQuickWindow *c)
{
    // An item needs a window if it is referenced by another item which has a window.
    // Typically the item is referenced by a parent, but can also be referenced by a
    // ShaderEffect or ShaderEffectSource. 'windowRefCount' counts how many items with
    // a window is referencing this item. When the reference count goes from zero to one,
    // or one to zero, the window of this item is updated and propagated to the children.
    // As long as the reference count stays above zero, the window is unchanged.
    // refWindow() increments the reference count.
    // derefWindow() decrements the reference count.

    Q_Q(QQuickItem);
    Q_ASSERT((window != nullptr) == (windowRefCount > 0));
    Q_ASSERT(c);
    if (++windowRefCount > 1) {
        if (c != window)
            qWarning("QQuickItem: Cannot use same item on different windows at the same time.");
        return; // Window already set.
    }

    Q_ASSERT(window == nullptr);
    window = c;

    if (polishScheduled)
        QQuickWindowPrivate::get(window)->itemsToPolish.append(q);

    if (!parentItem)
        QQuickWindowPrivate::get(window)->parentlessItems.insert(q);

    for (int ii = 0; ii < childItems.size(); ++ii) {
        QQuickItem *child = childItems.at(ii);
        QQuickItemPrivate::get(child)->refWindow(c);
    }

    dirty(Window);

    if (extra.isAllocated() && extra->screenAttached)
        extra->screenAttached->windowChanged(c);
    itemChange(QQuickItem::ItemSceneChange, c);
}

void QQuickItemPrivate::derefWindow()
{
    Q_Q(QQuickItem);
    Q_ASSERT((window != nullptr) == (windowRefCount > 0));

    if (!window)
        return; // This can happen when destroying recursive shader effect sources.

    if (--windowRefCount > 0)
        return; // There are still other references, so don't set window to null yet.

    q->releaseResources();
    removeFromDirtyList();
    QQuickWindowPrivate *c = QQuickWindowPrivate::get(window);
    if (polishScheduled)
        c->itemsToPolish.removeOne(q);
#if QT_CONFIG(cursor)
    if (c->cursorItem == q) {
        c->cursorItem = nullptr;
        window->unsetCursor();
    }
#endif
    if (itemNodeInstance)
        c->cleanup(itemNodeInstance);
    if (!parentItem)
        c->parentlessItems.remove(q);

    window = nullptr;

    itemNodeInstance = nullptr;

    if (extra.isAllocated()) {
        extra->opacityNode = nullptr;
        extra->clipNode = nullptr;
        extra->rootNode = nullptr;
    }

    paintNode = nullptr;

    for (int ii = 0; ii < childItems.size(); ++ii) {
        if (QQuickItem *child = childItems.at(ii))
            QQuickItemPrivate::get(child)->derefWindow();
    }

    dirty(Window);

    if (extra.isAllocated() && extra->screenAttached)
        extra->screenAttached->windowChanged(nullptr);
    itemChange(QQuickItem::ItemSceneChange, (QQuickWindow *)nullptr);
}


/*!
    Returns a transform that maps points from window space into item space.
*/
QTransform QQuickItemPrivate::windowToItemTransform() const
{
    // XXX todo - optimize
    return itemToWindowTransform().inverted();
}

/*!
    Returns a transform that maps points from item space into window space.
*/
QTransform QQuickItemPrivate::itemToWindowTransform() const
{
    // item's parent must not be itself, otherwise calling itemToWindowTransform() on it is infinite recursion
    Q_ASSERT(!parentItem || QQuickItemPrivate::get(parentItem) != this);
    QTransform rv = parentItem ? QQuickItemPrivate::get(parentItem)->itemToWindowTransform() : QTransform();
    itemToParentTransform(&rv);
    return rv;
}

/*!
    Modifies \a t with this item's local transform relative to its parent.
*/
void QQuickItemPrivate::itemToParentTransform(QTransform *t) const
{
    /* Read the current x and y values. As this is an internal method,
       we don't care about it being usable in bindings. Instead, we
       care about performance here, and thus we read the value with
       valueBypassingBindings. This avoids any checks whether we are
       in a binding (which sholdn't be too expensive, but can add up).
    */

    qreal x = this->x.valueBypassingBindings();
    qreal y = this->y.valueBypassingBindings();
    if (x || y)
        t->translate(x, y);

    if (!transforms.isEmpty()) {
        QMatrix4x4 m(*t);
        for (int ii = transforms.size() - 1; ii >= 0; --ii)
            transforms.at(ii)->applyTo(&m);
        *t = m.toTransform();
    }

    if (scale() != 1. || rotation() != 0.) {
        QPointF tp = computeTransformOrigin();
        t->translate(tp.x(), tp.y());
        t->scale(scale(), scale());
        t->rotate(rotation());
        t->translate(-tp.x(), -tp.y());
    }
}

/*!
    Returns a transform that maps points from window space into global space.
*/
QTransform QQuickItemPrivate::windowToGlobalTransform() const
{
    if (Q_UNLIKELY(window == nullptr))
        return QTransform();

    QPoint quickWidgetOffset;
    QWindow *renderWindow = QQuickRenderControl::renderWindowFor(window, &quickWidgetOffset);
    QPointF pos = (renderWindow ? renderWindow : window)->mapToGlobal(quickWidgetOffset);
    return QTransform::fromTranslate(pos.x(), pos.y());
}

/*!
    Returns a transform that maps points from global space into window space.
*/
QTransform QQuickItemPrivate::globalToWindowTransform() const
{
    if (Q_UNLIKELY(window == nullptr))
        return QTransform();

    QPoint quickWidgetOffset;
    QWindow *renderWindow = QQuickRenderControl::renderWindowFor(window, &quickWidgetOffset);
    QPointF pos = (renderWindow ? renderWindow : window)->mapToGlobal(quickWidgetOffset);
    return QTransform::fromTranslate(-pos.x(), -pos.y());
}

/*!
    Returns true if construction of the QML component is complete; otherwise
    returns false.

    It is often desirable to delay some processing until the component is
    completed.

    \sa componentComplete()
*/
bool QQuickItem::isComponentComplete() const
{
    Q_D(const QQuickItem);
    return d->componentComplete;
}

QQuickItemPrivate::QQuickItemPrivate()
    : _anchors(nullptr)
    , _stateGroup(nullptr)
    , flags(0)
    , widthValidFlag(false)
    , heightValidFlag(false)
    , componentComplete(true)
    , keepMouse(false)
    , keepTouch(false)
    , hoverEnabled(false)
    , smooth(true)
    , antialiasing(false)
    , focus(false)
    , activeFocus(false)
    , notifiedFocus(false)
    , notifiedActiveFocus(false)
    , filtersChildMouseEvents(false)
    , explicitVisible(true)
    , effectiveVisible(true)
    , explicitEnable(true)
    , effectiveEnable(true)
    , polishScheduled(false)
    , inheritedLayoutMirror(false)
    , effectiveLayoutMirror(false)
    , isMirrorImplicit(true)
    , inheritMirrorFromParent(false)
    , inheritMirrorFromItem(false)
    , isAccessible(false)
    , culled(false)
    , hasCursor(false)
    , subtreeCursorEnabled(false)
    , subtreeHoverEnabled(false)
    , activeFocusOnTab(false)
    , implicitAntialiasing(false)
    , antialiasingValid(false)
    , isTabFence(false)
    , replayingPressEvent(false)
    , touchEnabled(false)
    , hasCursorHandler(false)
    , maybeHasSubsceneDeliveryAgent(true)
    , subtreeTransformChangedEnabled(true)
    , inDestructor(false)
    , dirtyAttributes(0)
    , nextDirtyItem(nullptr)
    , prevDirtyItem(nullptr)
    , window(nullptr)
    , windowRefCount(0)
    , parentItem(nullptr)
    , sortedChildItems(&childItems)
    , subFocusItem(nullptr)
    , x(0)
    , y(0)
    , width(0)
    , height(0)
    , implicitWidth(0)
    , implicitHeight(0)
    , baselineOffset(0)
    , itemNodeInstance(nullptr)
    , paintNode(nullptr)
{
}

QQuickItemPrivate::~QQuickItemPrivate()
{
    if (sortedChildItems != &childItems)
        delete sortedChildItems;
}

void QQuickItemPrivate::init(QQuickItem *parent)
{
    Q_Q(QQuickItem);

    isQuickItem = true;

    baselineOffset = 0.0;

    if (parent) {
        q->setParentItem(parent);
        QQuickItemPrivate *parentPrivate = QQuickItemPrivate::get(parent);
        setImplicitLayoutMirror(parentPrivate->inheritedLayoutMirror, parentPrivate->inheritMirrorFromParent);
    }
}

void QQuickItemPrivate::data_append(QQmlListProperty<QObject> *prop, QObject *o)
{
    if (!o)
        return;

    QQuickItem *that = static_cast<QQuickItem *>(prop->object);

    if (QQuickItem *item = qmlobject_cast<QQuickItem *>(o)) {
        item->setParentItem(that);
    } else {
        if (QQuickPointerHandler *pointerHandler = qmlobject_cast<QQuickPointerHandler *>(o)) {
            if (pointerHandler->parent() != that) {
                qCDebug(lcHandlerParent) << "reparenting handler" << pointerHandler << ":" << pointerHandler->parent() << "->" << that;
                pointerHandler->setParent(that);
            }
            QQuickItemPrivate::get(that)->addPointerHandler(pointerHandler);
        } else {
            QQuickWindow *thisWindow = qmlobject_cast<QQuickWindow *>(o);
            QQuickItem *item = that;
            QQuickWindow *itemWindow = that->window();
            while (!itemWindow && item && item->parentItem()) {
                item = item->parentItem();
                itemWindow = item->window();
            }

            if (thisWindow) {
                if (itemWindow) {
                    qCDebug(lcTransient) << thisWindow << "is transient for" << itemWindow;
                    thisWindow->setTransientParent(itemWindow);
                } else {
                    QObject::connect(item, SIGNAL(windowChanged(QQuickWindow*)),
                                     thisWindow, SLOT(setTransientParent_helper(QQuickWindow*)));
                }
            }
            o->setParent(that);
            resources_append(prop, o);
        }
    }
}

/*!
    \qmlproperty list<QtObject> QtQuick::Item::data
    \qmldefault

    The data property allows you to freely mix visual children and resources
    in an item.  If you assign a visual item to the data list it becomes
    a child and if you assign any other object type, it is added as a resource.

    So you can write:
    \qml
    Item {
        Text {}
        Rectangle {}
        Timer {}
    }
    \endqml

    instead of:
    \qml
    Item {
        children: [
            Text {},
            Rectangle {}
        ]
        resources: [
            Timer {}
        ]
    }
    \endqml

    It should not generally be necessary to refer to the \c data property,
    as it is the default property for Item and thus all child items are
    automatically assigned to this property.
 */

qsizetype QQuickItemPrivate::data_count(QQmlListProperty<QObject> *property)
{
    QQuickItem *item = static_cast<QQuickItem*>(property->object);
    QQuickItemPrivate *privateItem = QQuickItemPrivate::get(item);
    QQmlListProperty<QObject> resourcesProperty = privateItem->resources();
    QQmlListProperty<QQuickItem> childrenProperty = privateItem->children();

    return resources_count(&resourcesProperty) + children_count(&childrenProperty);
}

QObject *QQuickItemPrivate::data_at(QQmlListProperty<QObject> *property, qsizetype i)
{
    QQuickItem *item = static_cast<QQuickItem*>(property->object);
    QQuickItemPrivate *privateItem = QQuickItemPrivate::get(item);
    QQmlListProperty<QObject> resourcesProperty = privateItem->resources();
    QQmlListProperty<QQuickItem> childrenProperty = privateItem->children();

    qsizetype resourcesCount = resources_count(&resourcesProperty);
    if (i < resourcesCount)
        return resources_at(&resourcesProperty, i);
    const qsizetype j = i - resourcesCount;
    if (j < children_count(&childrenProperty))
        return children_at(&childrenProperty, j);
    return nullptr;
}

void QQuickItemPrivate::data_clear(QQmlListProperty<QObject> *property)
{
    QQuickItem *item = static_cast<QQuickItem*>(property->object);
    QQuickItemPrivate *privateItem = QQuickItemPrivate::get(item);
    QQmlListProperty<QObject> resourcesProperty = privateItem->resources();
    QQmlListProperty<QQuickItem> childrenProperty = privateItem->children();

    resources_clear(&resourcesProperty);
    children_clear(&childrenProperty);
}

void QQuickItemPrivate::data_removeLast(QQmlListProperty<QObject> *property)
{
    QQuickItem *item = static_cast<QQuickItem*>(property->object);
    QQuickItemPrivate *privateItem = QQuickItemPrivate::get(item);

    QQmlListProperty<QQuickItem> childrenProperty = privateItem->children();
    if (children_count(&childrenProperty) > 0) {
        children_removeLast(&childrenProperty);
        return;
    }

    QQmlListProperty<QObject> resourcesProperty = privateItem->resources();
    if (resources_count(&resourcesProperty) > 0)
        resources_removeLast(&resourcesProperty);
}

QObject *QQuickItemPrivate::resources_at(QQmlListProperty<QObject> *prop, qsizetype index)
{
    QQuickItemPrivate *quickItemPrivate = QQuickItemPrivate::get(static_cast<QQuickItem *>(prop->object));
    return quickItemPrivate->extra.isAllocated() ? quickItemPrivate->extra->resourcesList.value(index) : 0;
}

void QQuickItemPrivate::resources_append(QQmlListProperty<QObject> *prop, QObject *object)
{
    QQuickItem *quickItem = static_cast<QQuickItem *>(prop->object);
    QQuickItemPrivate *quickItemPrivate = QQuickItemPrivate::get(quickItem);
    if (!quickItemPrivate->extra.value().resourcesList.contains(object)) {
        quickItemPrivate->extra.value().resourcesList.append(object);
        qmlobject_connect(object, QObject, SIGNAL(destroyed(QObject*)),
                          quickItem, QQuickItem, SLOT(_q_resourceObjectDeleted(QObject*)));
    }
}

qsizetype QQuickItemPrivate::resources_count(QQmlListProperty<QObject> *prop)
{
    QQuickItemPrivate *quickItemPrivate = QQuickItemPrivate::get(static_cast<QQuickItem *>(prop->object));
    return  quickItemPrivate->extra.isAllocated() ? quickItemPrivate->extra->resourcesList.size() : 0;
}

void QQuickItemPrivate::resources_clear(QQmlListProperty<QObject> *prop)
{
    QQuickItem *quickItem = static_cast<QQuickItem *>(prop->object);
    QQuickItemPrivate *quickItemPrivate = QQuickItemPrivate::get(quickItem);
    if (quickItemPrivate->extra.isAllocated()) {//If extra is not allocated resources is empty.
        for (QObject *object : std::as_const(quickItemPrivate->extra->resourcesList)) {
            qmlobject_disconnect(object, QObject, SIGNAL(destroyed(QObject*)),
                                 quickItem, QQuickItem, SLOT(_q_resourceObjectDeleted(QObject*)));
        }
        quickItemPrivate->extra->resourcesList.clear();
    }
}

void QQuickItemPrivate::resources_removeLast(QQmlListProperty<QObject> *prop)
{
    QQuickItem *quickItem = static_cast<QQuickItem *>(prop->object);
    QQuickItemPrivate *quickItemPrivate = QQuickItemPrivate::get(quickItem);
    if (quickItemPrivate->extra.isAllocated()) {//If extra is not allocated resources is empty.
        QList<QObject *> *resources = &quickItemPrivate->extra->resourcesList;
        if (resources->isEmpty())
            return;

        qmlobject_disconnect(resources->last(), QObject, SIGNAL(destroyed(QObject*)),
                             quickItem, QQuickItem, SLOT(_q_resourceObjectDeleted(QObject*)));
        resources->removeLast();
    }
}

QQuickItem *QQuickItemPrivate::children_at(QQmlListProperty<QQuickItem> *prop, qsizetype index)
{
    QQuickItemPrivate *p = QQuickItemPrivate::get(static_cast<QQuickItem *>(prop->object));
    if (index >= p->childItems.size() || index < 0)
        return nullptr;
    else
        return p->childItems.at(index);
}

void QQuickItemPrivate::children_append(QQmlListProperty<QQuickItem> *prop, QQuickItem *o)
{
    if (!o)
        return;

    QQuickItem *that = static_cast<QQuickItem *>(prop->object);
    if (o->parentItem() == that)
        o->setParentItem(nullptr);

    o->setParentItem(that);
}

qsizetype QQuickItemPrivate::children_count(QQmlListProperty<QQuickItem> *prop)
{
    QQuickItemPrivate *p = QQuickItemPrivate::get(static_cast<QQuickItem *>(prop->object));
    return p->childItems.size();
}

void QQuickItemPrivate::children_clear(QQmlListProperty<QQuickItem> *prop)
{
    QQuickItem *that = static_cast<QQuickItem *>(prop->object);
    QQuickItemPrivate *p = QQuickItemPrivate::get(that);
    while (!p->childItems.isEmpty())
        p->childItems.at(0)->setParentItem(nullptr);
}

void QQuickItemPrivate::children_removeLast(QQmlListProperty<QQuickItem> *prop)
{
    QQuickItem *that = static_cast<QQuickItem *>(prop->object);
    QQuickItemPrivate *p = QQuickItemPrivate::get(that);
    if (!p->childItems.isEmpty())
        p->childItems.last()->setParentItem(nullptr);
}

qsizetype QQuickItemPrivate::visibleChildren_count(QQmlListProperty<QQuickItem> *prop)
{
    QQuickItemPrivate *p = QQuickItemPrivate::get(static_cast<QQuickItem *>(prop->object));
    qsizetype visibleCount = 0;
    qsizetype c = p->childItems.size();
    while (c--) {
        if (p->childItems.at(c)->isVisible()) visibleCount++;
    }

    return visibleCount;
}

QQuickItem *QQuickItemPrivate::visibleChildren_at(QQmlListProperty<QQuickItem> *prop, qsizetype index)
{
    QQuickItemPrivate *p = QQuickItemPrivate::get(static_cast<QQuickItem *>(prop->object));
    const qsizetype childCount = p->childItems.size();
    if (index >= childCount || index < 0)
        return nullptr;

    qsizetype visibleCount = -1;
    for (qsizetype i = 0; i < childCount; i++) {
        if (p->childItems.at(i)->isVisible()) visibleCount++;
        if (visibleCount == index) return p->childItems.at(i);
    }
    return nullptr;
}

qsizetype QQuickItemPrivate::transform_count(QQmlListProperty<QQuickTransform> *prop)
{
    QQuickItem *that = static_cast<QQuickItem *>(prop->object);
    QQuickItemPrivate *p = QQuickItemPrivate::get(that);

    return p->transforms.size();
}

void QQuickTransform::appendToItem(QQuickItem *item)
{
    Q_D(QQuickTransform);
    if (!item)
        return;

    QQuickItemPrivate *p = QQuickItemPrivate::get(item);

    if (!d->items.isEmpty() && !p->transforms.isEmpty() && p->transforms.contains(this)) {
        p->transforms.removeOne(this);
        p->transforms.append(this);
    } else {
        p->transforms.append(this);
        d->items.append(item);
    }

    p->dirty(QQuickItemPrivate::Transform);
}

void QQuickTransform::prependToItem(QQuickItem *item)
{
    Q_D(QQuickTransform);
    if (!item)
        return;

    QQuickItemPrivate *p = QQuickItemPrivate::get(item);

    if (!d->items.isEmpty() && !p->transforms.isEmpty() && p->transforms.contains(this)) {
        p->transforms.removeOne(this);
        p->transforms.prepend(this);
    } else {
        p->transforms.prepend(this);
        d->items.append(item);
    }

    p->dirty(QQuickItemPrivate::Transform);
}

void QQuickItemPrivate::transform_append(QQmlListProperty<QQuickTransform> *prop, QQuickTransform *transform)
{
    if (!transform)
        return;

    QQuickItem *that = static_cast<QQuickItem *>(prop->object);
    transform->appendToItem(that);
}

QQuickTransform *QQuickItemPrivate::transform_at(QQmlListProperty<QQuickTransform> *prop, qsizetype idx)
{
    QQuickItem *that = static_cast<QQuickItem *>(prop->object);
    QQuickItemPrivate *p = QQuickItemPrivate::get(that);

    if (idx < 0 || idx >= p->transforms.size())
        return nullptr;
    else
        return p->transforms.at(idx);
}

void QQuickItemPrivate::transform_clear(QQmlListProperty<QQuickTransform> *prop)
{
    QQuickItem *that = static_cast<QQuickItem *>(prop->object);
    QQuickItemPrivate *p = QQuickItemPrivate::get(that);

    for (qsizetype ii = 0; ii < p->transforms.size(); ++ii) {
        QQuickTransform *t = p->transforms.at(ii);
        QQuickTransformPrivate *tp = QQuickTransformPrivate::get(t);
        tp->items.removeOne(that);
    }

    p->transforms.clear();

    p->dirty(QQuickItemPrivate::Transform);
}

void QQuickItemPrivate::_q_resourceObjectDeleted(QObject *object)
{
    if (extra.isAllocated() && extra->resourcesList.contains(object))
        extra->resourcesList.removeAll(object);
}

/*!
  \qmlpropertygroup QtQuick::Item::anchors
  \qmlproperty AnchorLine QtQuick::Item::anchors.top
  \qmlproperty AnchorLine QtQuick::Item::anchors.bottom
  \qmlproperty AnchorLine QtQuick::Item::anchors.left
  \qmlproperty AnchorLine QtQuick::Item::anchors.right
  \qmlproperty AnchorLine QtQuick::Item::anchors.horizontalCenter
  \qmlproperty AnchorLine QtQuick::Item::anchors.verticalCenter
  \qmlproperty AnchorLine QtQuick::Item::anchors.baseline

  \qmlproperty Item QtQuick::Item::anchors.fill
  \qmlproperty Item QtQuick::Item::anchors.centerIn

  \qmlproperty real QtQuick::Item::anchors.margins
  \qmlproperty real QtQuick::Item::anchors.topMargin
  \qmlproperty real QtQuick::Item::anchors.bottomMargin
  \qmlproperty real QtQuick::Item::anchors.leftMargin
  \qmlproperty real QtQuick::Item::anchors.rightMargin
  \qmlproperty real QtQuick::Item::anchors.horizontalCenterOffset
  \qmlproperty real QtQuick::Item::anchors.verticalCenterOffset
  \qmlproperty real QtQuick::Item::anchors.baselineOffset

  \qmlproperty bool QtQuick::Item::anchors.alignWhenCentered

  Anchors provide a way to position an item by specifying its
  relationship with other items.

  Margins apply to top, bottom, left, right, and fill anchors.
  The \l anchors.margins property can be used to set all of the various margins at once, to the same value.
  It will not override a specific margin that has been previously set; to clear an explicit margin
  set its value to \c undefined.
  Note that margins are anchor-specific and are not applied if an item does not
  use anchors.

  Offsets apply for horizontal center, vertical center, and baseline anchors.

  \table
  \row
  \li \image declarative-anchors_example.png
  \li Text anchored to Image, horizontally centered and vertically below, with a margin.
  \qml
  Item {
      Image {
          id: pic
          // ...
      }
      Text {
          id: label
          anchors.horizontalCenter: pic.horizontalCenter
          anchors.top: pic.bottom
          anchors.topMargin: 5
          // ...
      }
  }
  \endqml
  \row
  \li \image declarative-anchors_example2.png
  \li
  Left of Text anchored to right of Image, with a margin. The y
  property of both defaults to 0.

  \qml
  Item {
      Image {
          id: pic
          // ...
      }
      Text {
          id: label
          anchors.left: pic.right
          anchors.leftMargin: 5
          // ...
      }
  }
  \endqml
  \endtable

  \l anchors.fill provides a convenient way for one item to have the
  same geometry as another item, and is equivalent to connecting all
  four directional anchors.

  To clear an anchor value, set it to \c undefined.

  \l anchors.alignWhenCentered (default \c true) forces centered anchors to align to a
  whole pixel; if the item being centered has an odd \l width or \l height, the item
  will be positioned on a whole pixel rather than being placed on a half-pixel.
  This ensures the item is painted crisply.  There are cases where this is not
  desirable, for example when rotating the item jitters may be apparent as the
  center is rounded.

  \note You can only anchor an item to siblings or a parent.

  For more information see \l {anchor-layout}{Anchor Layouts}.
*/
QQuickAnchors *QQuickItemPrivate::anchors() const
{
    if (!_anchors) {
        Q_Q(const QQuickItem);
        _anchors = new QQuickAnchors(const_cast<QQuickItem *>(q));
        if (!componentComplete)
            _anchors->classBegin();
    }
    return _anchors;
}

void QQuickItemPrivate::siblingOrderChanged()
{
    Q_Q(QQuickItem);
    notifyChangeListeners(QQuickItemPrivate::SiblingOrder, &QQuickItemChangeListener::itemSiblingOrderChanged, q);
}

QQmlListProperty<QObject> QQuickItemPrivate::data()
{
    // Do not synthesize replace().
    // It would be extremely expensive and wouldn't work with most methods.
    QQmlListProperty<QObject> result;
    result.object = q_func();
    result.append = QQuickItemPrivate::data_append;
    result.count = QQuickItemPrivate::data_count;
    result.at = QQuickItemPrivate::data_at;
    result.clear = QQuickItemPrivate::data_clear;
    result.removeLast = QQuickItemPrivate::data_removeLast;
    return result;
}

/*!
    \qmlpropertygroup QtQuick::Item::childrenRect
    \qmlproperty real QtQuick::Item::childrenRect.x
    \qmlproperty real QtQuick::Item::childrenRect.y
    \qmlproperty real QtQuick::Item::childrenRect.width
    \qmlproperty real QtQuick::Item::childrenRect.height
    \readonly

    This read-only property holds the collective position and size of the item's
    children.

    This property is useful if you need to access the collective geometry
    of an item's children in order to correctly size the item.

    The geometry that is returned is local to the item. For example:

    \snippet qml/item/childrenRect.qml local
*/
/*!
    \property QQuickItem::childrenRect

    This property holds the collective position and size of the item's
    children.

    This property is useful if you need to access the collective geometry
    of an item's children in order to correctly size the item.

    The geometry that is returned is local to the item. For example:

    \snippet qml/item/childrenRect.qml local
*/
QRectF QQuickItem::childrenRect()
{
    Q_D(QQuickItem);
    if (!d->extra.isAllocated() || !d->extra->contents) {
        d->extra.value().contents = new QQuickContents(this);
        if (d->componentComplete)
            d->extra->contents->complete();
    }
    return d->extra->contents->rectF();
}

/*!
    Returns the children of this item.
  */
QList<QQuickItem *> QQuickItem::childItems() const
{
    Q_D(const QQuickItem);
    return d->childItems;
}

/*!
  \qmlproperty bool QtQuick::Item::clip
  This property holds whether clipping is enabled. The default clip value is \c false.

  If clipping is enabled, an item will clip its own painting, as well
  as the painting of its children, to its bounding rectangle.

  \note Clipping can affect rendering performance. See \l {Clipping} for more
  information.
*/
/*!
  \property QQuickItem::clip
  This property holds whether clipping is enabled. The default clip value is \c false.

  If clipping is enabled, an item will clip its own painting, as well
  as the painting of its children, to its bounding rectangle. If you set
  clipping during an item's paint operation, remember to re-set it to
  prevent clipping the rest of your scene.

  \note Clipping can affect rendering performance. See \l {Clipping} for more
  information.

  \note For the sake of QML, setting clip to \c true also sets the
  \l ItemIsViewport flag, which sometimes acts as an optimization: child items
  that have the \l ItemObservesViewport flag may forego creating scene graph nodes
  that fall outside the viewport. But the \c ItemIsViewport flag can also be set
  independently.
*/
bool QQuickItem::clip() const
{
    return flags() & ItemClipsChildrenToShape;
}

void QQuickItem::setClip(bool c)
{
    if (clip() == c)
        return;

    setFlag(ItemClipsChildrenToShape, c);
    if (c)
        setFlag(ItemIsViewport);
    else if (!(inherits("QQuickFlickable") || inherits("QQuickRootItem")))
        setFlag(ItemIsViewport, false);

    emit clipChanged(c);
}

/*!
  \since 6.0

  This function is called to handle this item's changes in
  geometry from \a oldGeometry to \a newGeometry. If the two
  geometries are the same, it doesn't do anything.

  Derived classes must call the base class method within their implementation.
 */
void QQuickItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickItem);

    if (d->_anchors)
        QQuickAnchorsPrivate::get(d->_anchors)->updateMe();

    QQuickGeometryChange change;
    change.setXChange(newGeometry.x() != oldGeometry.x());
    change.setYChange(newGeometry.y() != oldGeometry.y());
    change.setWidthChange(newGeometry.width() != oldGeometry.width());
    change.setHeightChange(newGeometry.height() != oldGeometry.height());

    d->notifyChangeListeners(QQuickItemPrivate::Geometry, [&](const QQuickItemPrivate::ChangeListener &listener){
        if (change.matches(listener.gTypes))
            listener.listener->itemGeometryChanged(this, change, oldGeometry);
    });

    // The notify method takes care of emitting the signal, and also notifies any
    // property observers.
    if (change.xChange())
        d->x.notify();
    if (change.yChange())
        d->y.notify();
    if (change.widthChange())
        d->width.notify();
    if (change.heightChange())
        d->height.notify();
#if QT_CONFIG(accessibility)
    if (QAccessible::isActive()) {
        if (QObject *acc = QQuickAccessibleAttached::findAccessible(this)) {
            QAccessibleEvent ev(acc, QAccessible::LocationChanged);
            QAccessible::updateAccessibility(&ev);
        }
    }
#endif
}

/*!
    Called on the render thread when it is time to sync the state
    of the item with the scene graph.

    The function is called as a result of QQuickItem::update(), if
    the user has set the QQuickItem::ItemHasContents flag on the item.

    The function should return the root of the scene graph subtree for
    this item. Most implementations will return a single
    QSGGeometryNode containing the visual representation of this item.
    \a oldNode is the node that was returned the last time the
    function was called. \a updatePaintNodeData provides a pointer to
    the QSGTransformNode associated with this QQuickItem.

    \code
    QSGNode *MyItem::updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
    {
        QSGSimpleRectNode *n = static_cast<QSGSimpleRectNode *>(node);
        if (!n) {
            n = new QSGSimpleRectNode();
            n->setColor(Qt::red);
        }
        n->setRect(boundingRect());
        return n;
    }
    \endcode

    The main thread is blocked while this function is executed so it is safe to read
    values from the QQuickItem instance and other objects in the main thread.

    If no call to QQuickItem::updatePaintNode() result in actual scene graph
    changes, like QSGNode::markDirty() or adding and removing nodes, then
    the underlying implementation may decide to not render the scene again as
    the visual outcome is identical.

    \warning It is crucial that graphics operations and interaction with
    the scene graph happens exclusively on the render thread,
    primarily during the QQuickItem::updatePaintNode() call. The best
    rule of thumb is to only use classes with the "QSG" prefix inside
    the QQuickItem::updatePaintNode() function.

    \warning This function is called on the render thread. This means any
    QObjects or thread local storage that is created will have affinity to the
    render thread, so apply caution when doing anything other than rendering
    in this function. Similarly for signals, these will be emitted on the render
    thread and will thus often be delivered via queued connections.

    \note All classes with QSG prefix should be used solely on the scene graph's
    rendering thread. See \l {Scene Graph and Rendering} for more information.

    \sa QSGMaterial, QSGGeometryNode, QSGGeometry,
    QSGFlatColorMaterial, QSGTextureMaterial, QSGNode::markDirty(), {Graphics Resource Handling}
 */

QSGNode *QQuickItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData)
{
    Q_UNUSED(updatePaintNodeData);
    delete oldNode;
    return nullptr;
}

QQuickItem::UpdatePaintNodeData::UpdatePaintNodeData()
: transformNode(nullptr)
{
}

/*!
    This function is called when an item should release graphics
    resources which are not already managed by the nodes returned from
    QQuickItem::updatePaintNode().

    This happens when the item is about to be removed from the window it
    was previously rendering to. The item is guaranteed to have a
    \l {QQuickItem::window()}{window} when the function is called.

    The function is called on the GUI thread and the state of the
    rendering thread, when it is used, is unknown. Objects should
    not be deleted directly, but instead scheduled for cleanup
    using QQuickWindow::scheduleRenderJob().

    \sa {Graphics Resource Handling}
 */

void QQuickItem::releaseResources()
{
}

QSGTransformNode *QQuickItemPrivate::createTransformNode()
{
    return new QSGTransformNode;
}

/*!
    This function should perform any layout as required for this item.

    When polish() is called, the scene graph schedules a polish event for this
    item. When the scene graph is ready to render this item, it calls
    updatePolish() to do any item layout as required before it renders the
    next frame.

    \sa ensurePolished()
  */
void QQuickItem::updatePolish()
{
}

#define PRINT_LISTENERS() \
do { \
    qDebug().nospace() << q_func() << " (" << this \
        << ") now has the following listeners:"; \
    for (const auto &listener : std::as_const(changeListeners)) { \
        const auto objectPrivate = dynamic_cast<QObjectPrivate*>(listener.listener); \
        qDebug().nospace() << "- " << listener << " (QObject: " << (objectPrivate ? objectPrivate->q_func() : nullptr) << ")"; \
    } \
} \
while (false)

void QQuickItemPrivate::addItemChangeListener(QQuickItemChangeListener *listener, ChangeTypes types)
{
    changeListeners.append(ChangeListener(listener, types));

    if (lcChangeListeners().isDebugEnabled())
        PRINT_LISTENERS();
}

void QQuickItemPrivate::updateOrAddItemChangeListener(QQuickItemChangeListener *listener, ChangeTypes types)
{
    const ChangeListener changeListener(listener, types);
    const int index = changeListeners.indexOf(changeListener);
    if (index > -1)
        changeListeners[index].types = changeListener.types;
    else
        changeListeners.append(changeListener);

    if (lcChangeListeners().isDebugEnabled())
        PRINT_LISTENERS();
}

void QQuickItemPrivate::removeItemChangeListener(QQuickItemChangeListener *listener, ChangeTypes types)
{
    ChangeListener change(listener, types);
    changeListeners.removeOne(change);

    if (lcChangeListeners().isDebugEnabled())
        PRINT_LISTENERS();
}

void QQuickItemPrivate::updateOrAddGeometryChangeListener(QQuickItemChangeListener *listener,
                                                          QQuickGeometryChange types)
{
    ChangeListener change(listener, types);
    int index = changeListeners.indexOf(change);
    if (index > -1)
        changeListeners[index].gTypes = change.gTypes;  //we may have different GeometryChangeTypes
    else
        changeListeners.append(change);

    if (lcChangeListeners().isDebugEnabled())
        PRINT_LISTENERS();
}

void QQuickItemPrivate::updateOrRemoveGeometryChangeListener(QQuickItemChangeListener *listener,
                                                             QQuickGeometryChange types)
{
    ChangeListener change(listener, types);
    if (types.noChange()) {
        changeListeners.removeOne(change);
    } else {
        int index = changeListeners.indexOf(change);
        if (index > -1)
            changeListeners[index].gTypes = change.gTypes;  //we may have different GeometryChangeTypes
    }

    if (lcChangeListeners().isDebugEnabled())
        PRINT_LISTENERS();
}

/*!
    This event handler can be reimplemented in a subclass to receive key
    press events for an item. The event information is provided by the
    \a event parameter.

    \input item.qdocinc accepting-events
  */
void QQuickItem::keyPressEvent(QKeyEvent *event)
{
    event->ignore();
}

/*!
    This event handler can be reimplemented in a subclass to receive key
    release events for an item. The event information is provided by the
    \a event parameter.

    \input item.qdocinc accepting-events
  */
void QQuickItem::keyReleaseEvent(QKeyEvent *event)
{
    event->ignore();
}

#if QT_CONFIG(im)
/*!
    This event handler can be reimplemented in a subclass to receive input
    method events for an item. The event information is provided by the
    \a event parameter.

    \input item.qdocinc accepting-events
  */
void QQuickItem::inputMethodEvent(QInputMethodEvent *event)
{
    event->ignore();
}
#endif // im

/*!
    This event handler can be reimplemented in a subclass to receive focus-in
    events for an item. The event information is provided by the \c event
    parameter.

    \input item.qdocinc accepting-events

    If you do reimplement this function, you should call the base class
    implementation.
  */
void QQuickItem::focusInEvent(QFocusEvent * /*event*/)
{
#if QT_CONFIG(accessibility)
    if (QAccessible::isActive()) {
        if (QObject *acc = QQuickAccessibleAttached::findAccessible(this)) {
            QAccessibleEvent ev(acc, QAccessible::Focus);
            QAccessible::updateAccessibility(&ev);
        }
    }
#endif
}

/*!
    This event handler can be reimplemented in a subclass to receive focus-out
    events for an item. The event information is provided by the \c event
    parameter.

    \input item.qdocinc accepting-events
  */
void QQuickItem::focusOutEvent(QFocusEvent * /*event*/)
{
}

/*!
    This event handler can be reimplemented in a subclass to receive mouse
    press events for an item. The event information is provided by the
    \a event parameter.

    In order to receive mouse press events, \l acceptedMouseButtons() must
    return the relevant mouse button.

    \input item.qdocinc accepting-events
  */
void QQuickItem::mousePressEvent(QMouseEvent *event)
{
    event->ignore();
}

/*!
    This event handler can be reimplemented in a subclass to receive mouse
    move events for an item. The event information is provided by the
    \a event parameter.

    In order to receive mouse movement events, the preceding mouse press event
    must be accepted (by overriding \l mousePressEvent(), for example) and
    \l acceptedMouseButtons() must return the relevant mouse button.

    \input item.qdocinc accepting-events
  */
void QQuickItem::mouseMoveEvent(QMouseEvent *event)
{
    event->ignore();
}

/*!
    This event handler can be reimplemented in a subclass to receive mouse
    release events for an item. The event information is provided by the
    \a event parameter.

    In order to receive mouse release events, the preceding mouse press event
    must be accepted (by overriding \l mousePressEvent(), for example) and
    \l acceptedMouseButtons() must return the relevant mouse button.

    \input item.qdocinc accepting-events
  */
void QQuickItem::mouseReleaseEvent(QMouseEvent *event)
{
    event->ignore();
}

/*!
    This event handler can be reimplemented in a subclass to receive mouse
    double-click events for an item. The event information is provided by the
    \a event parameter.

    \input item.qdocinc accepting-events
  */
void QQuickItem::mouseDoubleClickEvent(QMouseEvent *event)
{
    event->ignore();
}

/*!
    This event handler can be reimplemented in a subclass to be notified
    when a mouse ungrab event has occurred on this item.
  */
void QQuickItem::mouseUngrabEvent()
{
    // XXX todo
}

/*!
    This event handler can be reimplemented in a subclass to be notified
    when a touch ungrab event has occurred on this item.
  */
void QQuickItem::touchUngrabEvent()
{
    // XXX todo
}

#if QT_CONFIG(wheelevent)
/*!
    This event handler can be reimplemented in a subclass to receive
    wheel events for an item. The event information is provided by the
    \a event parameter.

    \input item.qdocinc accepting-events
  */
void QQuickItem::wheelEvent(QWheelEvent *event)
{
    event->ignore();
}
#endif

/*!
    This event handler can be reimplemented in a subclass to receive touch
    events for an item. The event information is provided by the
    \a event parameter.

    \input item.qdocinc accepting-events
  */
void QQuickItem::touchEvent(QTouchEvent *event)
{
    event->ignore();
}

/*!
    This event handler can be reimplemented in a subclass to receive hover-enter
    events for an item. The event information is provided by the
    \a event parameter.

    Hover events are only provided if acceptHoverEvents() is true.

    \input item.qdocinc accepting-events
  */
void QQuickItem::hoverEnterEvent(QHoverEvent *event)
{
    event->ignore();
}

/*!
    This event handler can be reimplemented in a subclass to receive hover-move
    events for an item. The event information is provided by the
    \a event parameter.

    Hover events are only provided if acceptHoverEvents() is true.

    \input item.qdocinc accepting-events
  */
void QQuickItem::hoverMoveEvent(QHoverEvent *event)
{
    event->ignore();
}

/*!
    This event handler can be reimplemented in a subclass to receive hover-leave
    events for an item. The event information is provided by the
    \a event parameter.

    Hover events are only provided if acceptHoverEvents() is true.

    \input item.qdocinc accepting-events
  */
void QQuickItem::hoverLeaveEvent(QHoverEvent *event)
{
    event->ignore();
}

#if QT_CONFIG(quick_draganddrop)
/*!
    This event handler can be reimplemented in a subclass to receive drag-enter
    events for an item. The event information is provided by the
    \a event parameter.

    Drag and drop events are only provided if the ItemAcceptsDrops flag
    has been set for this item.

    \input item.qdocinc accepting-events

    \sa Drag, {Drag and Drop}
  */
void QQuickItem::dragEnterEvent(QDragEnterEvent *event)
{
    Q_UNUSED(event);
}

/*!
    This event handler can be reimplemented in a subclass to receive drag-move
    events for an item. The event information is provided by the
    \a event parameter.

    Drag and drop events are only provided if the ItemAcceptsDrops flag
    has been set for this item.

    \input item.qdocinc accepting-events

    \sa Drag, {Drag and Drop}
  */
void QQuickItem::dragMoveEvent(QDragMoveEvent *event)
{
    Q_UNUSED(event);
}

/*!
    This event handler can be reimplemented in a subclass to receive drag-leave
    events for an item. The event information is provided by the
    \a event parameter.

    Drag and drop events are only provided if the ItemAcceptsDrops flag
    has been set for this item.

    \input item.qdocinc accepting-events

    \sa Drag, {Drag and Drop}
  */
void QQuickItem::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
}

/*!
    This event handler can be reimplemented in a subclass to receive drop
    events for an item. The event information is provided by the
    \a event parameter.

    Drag and drop events are only provided if the ItemAcceptsDrops flag
    has been set for this item.

    \input item.qdocinc accepting-events

    \sa Drag, {Drag and Drop}
  */
void QQuickItem::dropEvent(QDropEvent *event)
{
    Q_UNUSED(event);
}
#endif // quick_draganddrop

/*!
    Reimplement this method to filter the pointer events that are received by
    this item's children.

    This method will only be called if filtersChildMouseEvents() is \c true.

    Return \c true if the specified \a event should not be passed on to the
    specified child \a item, and \c false otherwise. If you return \c true, you
    should also \l {QEvent::accept()}{accept} or \l {QEvent::ignore()}{ignore}
    the \a event, to signal if event propagation should stop or continue.
    The \a event will, however, always be sent to all childMouseEventFilters
    up the parent chain.

    \note Despite the name, this function filters all QPointerEvent instances
    during delivery to all children (typically mouse, touch, and tablet
    events). When overriding this function in a subclass, we suggest writing
    generic event-handling code using only the accessors found in
    QPointerEvent. Alternatively you can switch on \c event->type() and/or
    \c event->device()->type() to handle different event types in different ways.

    \note Filtering is just one way to share responsibility in case of gestural
    ambiguity (for example on press, you don't know whether the user will tap
    or drag). Another way is to call QPointerEvent::addPassiveGrabber() on
    press, so as to non-exclusively monitor the progress of the QEventPoint.
    In either case, the item or pointer handler that is monitoring can steal
    the exclusive grab later on, when it becomes clear that the gesture fits
    the pattern that it is expecting.

    \sa setFiltersChildMouseEvents()
  */
bool QQuickItem::childMouseEventFilter(QQuickItem *item, QEvent *event)
{
    Q_UNUSED(item);
    Q_UNUSED(event);
    return false;
}

#if QT_CONFIG(im)
/*!
    This method is only relevant for input items.

    If this item is an input item, this method should be reimplemented to
    return the relevant input method flags for the given \a query.

    \sa QWidget::inputMethodQuery()
  */
QVariant QQuickItem::inputMethodQuery(Qt::InputMethodQuery query) const
{
    Q_D(const QQuickItem);
    QVariant v;

    switch (query) {
    case Qt::ImEnabled:
        v = (bool)(flags() & ItemAcceptsInputMethod);
        break;
    case Qt::ImHints:
    case Qt::ImAnchorRectangle:
    case Qt::ImCursorRectangle:
    case Qt::ImFont:
    case Qt::ImCursorPosition:
    case Qt::ImSurroundingText:
    case Qt::ImCurrentSelection:
    case Qt::ImMaximumTextLength:
    case Qt::ImAnchorPosition:
    case Qt::ImPreferredLanguage:
    case Qt::ImReadOnly:
        if (d->extra.isAllocated() && d->extra->keyHandler)
            v = d->extra->keyHandler->inputMethodQuery(query);
        break;
    case Qt::ImEnterKeyType:
        if (d->extra.isAllocated() && d->extra->enterKeyAttached)
            v = d->extra->enterKeyAttached->type();
        break;
    case Qt::ImInputItemClipRectangle:
        if (!(!window() ||!isVisible() || qFuzzyIsNull(opacity()))) {
            QRectF rect = QRectF(0,0, width(), height());
            const QQuickItem *par = this;
            while (QQuickItem *parpar = par->parentItem()) {
                rect = parpar->mapRectFromItem(par, rect);
                if (parpar->clip())
                    rect = rect.intersected(parpar->clipRect());
                par = parpar;
            }
            rect = par->mapRectToScene(rect);
            // once we have the rect in scene coordinates, clip to window
            rect = rect.intersected(QRectF(QPoint(0,0), window()->size()));
            // map it back to local coordinates
            v = mapRectFromScene(rect);
        }
        break;
    default:
        break;
    }

    return v;
}
#endif // im

QQuickAnchorLine QQuickItemPrivate::left() const
{
    Q_Q(const QQuickItem);
    return QQuickAnchorLine(const_cast<QQuickItem *>(q), QQuickAnchors::LeftAnchor);
}

QQuickAnchorLine QQuickItemPrivate::right() const
{
    Q_Q(const QQuickItem);
    return QQuickAnchorLine(const_cast<QQuickItem *>(q), QQuickAnchors::RightAnchor);
}

QQuickAnchorLine QQuickItemPrivate::horizontalCenter() const
{
    Q_Q(const QQuickItem);
    return QQuickAnchorLine(const_cast<QQuickItem *>(q), QQuickAnchors::HCenterAnchor);
}

QQuickAnchorLine QQuickItemPrivate::top() const
{
    Q_Q(const QQuickItem);
    return QQuickAnchorLine(const_cast<QQuickItem *>(q), QQuickAnchors::TopAnchor);
}

QQuickAnchorLine QQuickItemPrivate::bottom() const
{
    Q_Q(const QQuickItem);
    return QQuickAnchorLine(const_cast<QQuickItem *>(q), QQuickAnchors::BottomAnchor);
}

QQuickAnchorLine QQuickItemPrivate::verticalCenter() const
{
    Q_Q(const QQuickItem);
    return QQuickAnchorLine(const_cast<QQuickItem *>(q), QQuickAnchors::VCenterAnchor);
}

QQuickAnchorLine QQuickItemPrivate::baseline() const
{
    Q_Q(const QQuickItem);
    return QQuickAnchorLine(const_cast<QQuickItem *>(q), QQuickAnchors::BaselineAnchor);
}

/*!
  \qmlproperty int QtQuick::Item::baselineOffset

  Specifies the position of the item's baseline in local coordinates.

  The baseline of a \l Text item is the imaginary line on which the text
  sits. Controls containing text usually set their baseline to the
  baseline of their text.

  For non-text items, a default baseline offset of 0 is used.
*/
/*!
  \property QQuickItem::baselineOffset

  Specifies the position of the item's baseline in local coordinates.

  The baseline of a \l Text item is the imaginary line on which the text
  sits. Controls containing text usually set their baseline to the
  baseline of their text.

  For non-text items, a default baseline offset of 0 is used.
*/
qreal QQuickItem::baselineOffset() const
{
    Q_D(const QQuickItem);
    return d->baselineOffset;
}

void QQuickItem::setBaselineOffset(qreal offset)
{
    Q_D(QQuickItem);
    if (offset == d->baselineOffset)
        return;

    d->baselineOffset = offset;

    d->notifyChangeListeners(QQuickItemPrivate::Geometry, [](const QQuickItemPrivate::ChangeListener &change){
        QQuickAnchorsPrivate *anchor = change.listener->anchorPrivate();
        if (anchor)
            anchor->updateVerticalAnchors();
    });

    if (d->_anchors && (d->_anchors->usedAnchors() & QQuickAnchors::BaselineAnchor))
        QQuickAnchorsPrivate::get(d->_anchors)->updateVerticalAnchors();

    emit baselineOffsetChanged(offset);
}


/*!
 * Schedules a call to updatePaintNode() for this item.
 *
 * The call to QQuickItem::updatePaintNode() will always happen if the
 * item is showing in a QQuickWindow.
 *
 * Only items which specify QQuickItem::ItemHasContents are allowed
 * to call QQuickItem::update().
 */
void QQuickItem::update()
{
    Q_D(QQuickItem);
    if (!(flags() & ItemHasContents)) {
#ifndef QT_NO_DEBUG
        qWarning() << metaObject()->className() << ": Update called for a item without content";
#endif
        return;
    }
    d->dirty(QQuickItemPrivate::Content);
}

/*!
    Schedules a polish event for this item.

    When the scene graph processes the request, it will call updatePolish()
    on this item.

    \sa updatePolish(), QQuickTest::qIsPolishScheduled(), ensurePolished()
  */
void QQuickItem::polish()
{
    Q_D(QQuickItem);
    if (!d->polishScheduled) {
        d->polishScheduled = true;
        if (d->window) {
            QQuickWindowPrivate *p = QQuickWindowPrivate::get(d->window);
            bool maybeupdate = p->itemsToPolish.isEmpty();
            p->itemsToPolish.append(this);
            if (maybeupdate) d->window->maybeUpdate();
        }
    }
}

/*!
    \since 6.3

    Calls updatePolish()

    This can be useful for items such as Layouts (or Positioners) which delay calculation of
    their implicitWidth and implicitHeight until they receive a PolishEvent.

    Normally, if e.g. a child item is added or removed to a Layout, the implicit size is not
    immediately calculated (this is an optimization). In some cases it might be desirable to
    query the implicit size of the layout right after a child item has been added.
    If this is the case, use this function right before querying the implicit size.

    \sa updatePolish(), polish()
  */
void QQuickItem::ensurePolished()
{
    updatePolish();
}

#if QT_DEPRECATED_SINCE(6, 5)
static bool unwrapMapFromToFromItemArgs(QQmlV4Function *args, const QQuickItem *itemForWarning, const QString &functionNameForWarning,
                                        QQuickItem **itemObj, qreal *x, qreal *y, qreal *w, qreal *h, bool *isRect)
{
    QV4::ExecutionEngine *v4 = args->v4engine();
    if (args->length() != 2 && args->length() != 3 && args->length() != 5) {
        v4->throwTypeError();
        return false;
    }

    QV4::Scope scope(v4);
    QV4::ScopedValue item(scope, (*args)[0]);

    *itemObj = nullptr;
    if (!item->isNull()) {
        QV4::Scoped<QV4::QObjectWrapper> qobjectWrapper(scope, item->as<QV4::QObjectWrapper>());
        if (qobjectWrapper)
            *itemObj = qobject_cast<QQuickItem*>(qobjectWrapper->object());
    }

    if (!(*itemObj) && !item->isNull()) {
        qmlWarning(itemForWarning) << functionNameForWarning << " given argument \"" << item->toQStringNoThrow()
                                   << "\" which is neither null nor an Item";
        v4->throwTypeError();
        return false;
    }

    *isRect = false;

    if (args->length() == 2) {
        QV4::ScopedValue sv(scope, (*args)[1]);
        if (sv->isNull()) {
            qmlWarning(itemForWarning) << functionNameForWarning << "given argument \"" << sv->toQStringNoThrow()
                                       << "\" which is neither a point nor a rect";
            v4->throwTypeError();
            return false;
        }
        const QV4::Scoped<QV4::QQmlValueTypeWrapper> variantWrapper(scope, sv->as<QV4::QQmlValueTypeWrapper>());
        const QVariant v = variantWrapper ? variantWrapper->toVariant() : QVariant();
        if (v.canConvert<QPointF>()) {
            const QPointF p = v.toPointF();
            *x = p.x();
            *y = p.y();
        } else if (v.canConvert<QRectF>()) {
            const QRectF r = v.toRectF();
            *x = r.x();
            *y = r.y();
            *w = r.width();
            *h = r.height();
            *isRect = true;
        } else {
            qmlWarning(itemForWarning) << functionNameForWarning << "given argument \"" << sv->toQStringNoThrow()
                                       << "\" which is neither a point nor a rect";
            v4->throwTypeError();
            return false;
        }
    } else {
        QV4::ScopedValue vx(scope, (*args)[1]);
        QV4::ScopedValue vy(scope, (*args)[2]);

        if (!vx->isNumber() || !vy->isNumber()) {
            v4->throwTypeError();
            return false;
        }

        *x = vx->asDouble();
        *y = vy->asDouble();

        if (args->length() > 3) {
            QV4::ScopedValue vw(scope, (*args)[3]);
            QV4::ScopedValue vh(scope, (*args)[4]);
            if (!vw->isNumber() || !vh->isNumber()) {
                v4->throwTypeError();
                return false;
            }
            *w = vw->asDouble();
            *h = vh->asDouble();
            *isRect = true;
        }
    }

    return true;
}
#endif

/*!
    \qmlmethod point QtQuick::Item::mapFromItem(Item item, real x, real y)
    \qmlmethod point QtQuick::Item::mapFromItem(Item item, point p)
    \qmlmethod rect QtQuick::Item::mapFromItem(Item item, real x, real y, real width, real height)
    \qmlmethod rect QtQuick::Item::mapFromItem(Item item, rect r)

    Maps the point (\a x, \a y) or rect (\a x, \a y, \a width, \a height), which is in \a
    item's coordinate system, to this item's coordinate system, and returns a \l point or \l rect
    matching the mapped coordinate.

    \input item.qdocinc mapping

    If \a item is a \c null value, this maps the point or rect from the coordinate system of
    the \l{Scene Coordinates}{scene}.

    The versions accepting point and rect are since Qt 5.15.
*/

#if QT_DEPRECATED_SINCE(6, 5)
/*!
    \internal
  */
void QQuickItem::mapFromItem(QQmlV4Function *args) const
{
    QV4::ExecutionEngine *v4 = args->v4engine();
    QV4::Scope scope(v4);

    qreal x, y, w, h;
    bool isRect;
    QQuickItem *itemObj;
    if (!unwrapMapFromToFromItemArgs(args, this, QStringLiteral("mapFromItem()"), &itemObj, &x, &y, &w, &h, &isRect))
        return;

    const QVariant result = isRect ? QVariant(mapRectFromItem(itemObj, QRectF(x, y, w, h)))
                                   : QVariant(mapFromItem(itemObj, QPointF(x, y)));

    QV4::ScopedObject rv(scope, v4->fromVariant(result));
    args->setReturnValue(rv.asReturnedValue());
}
#endif

/*!
    \internal
  */
QTransform QQuickItem::itemTransform(QQuickItem *other, bool *ok) const
{
    Q_D(const QQuickItem);

    // XXX todo - we need to be able to handle common parents better and detect
    // invalid cases
    if (ok) *ok = true;

    QTransform t = d->itemToWindowTransform();
    if (other) t *= QQuickItemPrivate::get(other)->windowToItemTransform();

    return t;
}

/*!
    \qmlmethod point QtQuick::Item::mapToItem(Item item, real x, real y)
    \qmlmethod point QtQuick::Item::mapToItem(Item item, point p)
    \qmlmethod rect QtQuick::Item::mapToItem(Item item, real x, real y, real width, real height)
    \qmlmethod rect QtQuick::Item::mapToItem(Item item, rect r)

    Maps the point (\a x, \a y) or rect (\a x, \a y, \a width, \a height), which is in this
    item's coordinate system, to \a item's coordinate system, and returns a \l point or \l rect
    matching the mapped coordinate.

    \input item.qdocinc mapping

    If \a item is a \c null value, this maps the point or rect to the coordinate system of the
    \l{Scene Coordinates}{scene}.

    The versions accepting point and rect are since Qt 5.15.
*/

#if QT_DEPRECATED_SINCE(6, 5)
/*!
    \internal
  */
void QQuickItem::mapToItem(QQmlV4Function *args) const
{
    QV4::ExecutionEngine *v4 = args->v4engine();
    QV4::Scope scope(v4);

    qreal x, y, w, h;
    bool isRect;
    QQuickItem *itemObj;
    if (!unwrapMapFromToFromItemArgs(args, this, QStringLiteral("mapToItem()"), &itemObj, &x, &y, &w, &h, &isRect))
        return;

    const QVariant result = isRect ? QVariant(mapRectToItem(itemObj, QRectF(x, y, w, h)))
                                   : QVariant(mapToItem(itemObj, QPointF(x, y)));

    QV4::ScopedObject rv(scope, v4->fromVariant(result));
    args->setReturnValue(rv.asReturnedValue());
}

static bool unwrapMapFromToFromGlobalArgs(QQmlV4Function *args, const QQuickItem *itemForWarning, const QString &functionNameForWarning, qreal *x, qreal *y)
{
    QV4::ExecutionEngine *v4 = args->v4engine();
    if (args->length() != 1 && args->length() != 2) {
        v4->throwTypeError();
        return false;
    }

    QV4::Scope scope(v4);

    if (args->length() == 1) {
        QV4::ScopedValue sv(scope, (*args)[0]);
        if (sv->isNull()) {
            qmlWarning(itemForWarning) << functionNameForWarning << "given argument \"" << sv->toQStringNoThrow()
                                       << "\" which is not a point";
            v4->throwTypeError();
            return false;
        }
        const QV4::Scoped<QV4::QQmlValueTypeWrapper> variantWrapper(scope, sv->as<QV4::QQmlValueTypeWrapper>());
        const QVariant v = variantWrapper ? variantWrapper->toVariant() : QVariant();
        if (v.canConvert<QPointF>()) {
            const QPointF p = v.toPointF();
            *x = p.x();
            *y = p.y();
        } else {
            qmlWarning(itemForWarning) << functionNameForWarning << "given argument \"" << sv->toQStringNoThrow()
                                       << "\" which is not a point";
            v4->throwTypeError();
            return false;
        }
    } else {
        QV4::ScopedValue vx(scope, (*args)[0]);
        QV4::ScopedValue vy(scope, (*args)[1]);

        if (!vx->isNumber() || !vy->isNumber()) {
            v4->throwTypeError();
            return false;
        }

        *x = vx->asDouble();
        *y = vy->asDouble();
    }

    return true;
}

/*!
    \since 5.7
    \qmlmethod point QtQuick::Item::mapFromGlobal(real x, real y)

    Maps the point (\a x, \a y), which is in the global coordinate system, to the
    item's coordinate system, and returns a \l point  matching the mapped coordinate.

    \input item.qdocinc mapping
*/
/*!
    \internal
  */
void QQuickItem::mapFromGlobal(QQmlV4Function *args) const
{
    QV4::ExecutionEngine *v4 = args->v4engine();
    QV4::Scope scope(v4);

    qreal x, y;
    if (!unwrapMapFromToFromGlobalArgs(args, this, QStringLiteral("mapFromGlobal()"), &x, &y))
        return;

    QVariant result = mapFromGlobal(QPointF(x, y));

    QV4::ScopedObject rv(scope, v4->fromVariant(result));
    args->setReturnValue(rv.asReturnedValue());
}
#endif

/*!
    \since 5.7
    \qmlmethod point QtQuick::Item::mapToGlobal(real x, real y)

    Maps the point (\a x, \a y), which is in this item's coordinate system, to the
    global coordinate system, and returns a \l point  matching the mapped coordinate.

    \input item.qdocinc mapping
*/

#if QT_DEPRECATED_SINCE(6, 5)
/*!
    \internal
  */
void QQuickItem::mapToGlobal(QQmlV4Function *args) const
{
    QV4::ExecutionEngine *v4 = args->v4engine();
    QV4::Scope scope(v4);

    qreal x, y;
    if (!unwrapMapFromToFromGlobalArgs(args, this, QStringLiteral("mapFromGlobal()"), &x, &y))
        return;

    QVariant result = mapToGlobal(QPointF(x, y));

    QV4::ScopedObject rv(scope, v4->fromVariant(result));
    args->setReturnValue(rv.asReturnedValue());
}
#endif

/*!
    \qmlmethod QtQuick::Item::forceActiveFocus()

    Forces active focus on the item.

    This method sets focus on the item and ensures that all ancestor
    FocusScope objects in the object hierarchy are also given \l focus.

    The reason for the focus change will be \l [CPP] Qt::OtherFocusReason. Use
    the overloaded method to specify the focus reason to enable better
    handling of the focus change.

    \sa activeFocus
*/
/*!
    Forces active focus on the item.

    This method sets focus on the item and ensures that all ancestor
    FocusScope objects in the object hierarchy are also given \l focus.

    The reason for the focus change will be \l [CPP] Qt::OtherFocusReason. Use
    the overloaded method to specify the focus reason to enable better
    handling of the focus change.

    \sa activeFocus
*/
void QQuickItem::forceActiveFocus()
{
    forceActiveFocus(Qt::OtherFocusReason);
}

/*!
    \qmlmethod QtQuick::Item::forceActiveFocus(Qt::FocusReason reason)
    \overload

    Forces active focus on the item with the given \a reason.

    This method sets focus on the item and ensures that all ancestor
    FocusScope objects in the object hierarchy are also given \l focus.

    \since 5.1

    \sa activeFocus, Qt::FocusReason
*/
/*!
    \overload
    Forces active focus on the item with the given \a reason.

    This method sets focus on the item and ensures that all ancestor
    FocusScope objects in the object hierarchy are also given \l focus.

    \since 5.1

    \sa activeFocus, Qt::FocusReason
*/

void QQuickItem::forceActiveFocus(Qt::FocusReason reason)
{
    Q_D(QQuickItem);
    setFocus(true, reason);
    QQuickItem *parent = parentItem();
    QQuickItem *scope = nullptr;
    while (parent) {
        if (parent->flags() & QQuickItem::ItemIsFocusScope) {
            parent->setFocus(true, reason);
            if (!scope)
                scope = parent;
        }
        parent = parent->parentItem();
    }
    // In certain reparenting scenarios, d->focus might be true and the scope
    // might also have focus, so that setFocus() returns early without actually
    // acquiring active focus, because it thinks it already has it. In that
    // case, try to set the DeliveryAgent's active focus. (QTBUG-89736).
    if (scope && !d->activeFocus) {
        if (auto da = d->deliveryAgentPrivate())
            da->setFocusInScope(scope, this, Qt::OtherFocusReason);
    }
}

/*!
    \qmlmethod QtQuick::Item::nextItemInFocusChain(bool forward)

    \since 5.1

    Returns the item in the focus chain which is next to this item.
    If \a forward is \c true, or not supplied, it is the next item in
    the forwards direction. If \a forward is \c false, it is the next
    item in the backwards direction.
*/
/*!
    Returns the item in the focus chain which is next to this item.
    If \a forward is \c true, or not supplied, it is the next item in
    the forwards direction. If \a forward is \c false, it is the next
    item in the backwards direction.
*/

QQuickItem *QQuickItem::nextItemInFocusChain(bool forward)
{
    return QQuickItemPrivate::nextPrevItemInTabFocusChain(this, forward);
}

/*!
    \qmlmethod QtQuick::Item::childAt(real x, real y)

    Returns the first visible child item found at point (\a x, \a y) within
    the coordinate system of this item.

    Returns \c null if there is no such item.
*/
/*!
    Returns the first visible child item found at point (\a x, \a y) within
    the coordinate system of this item.

    Returns \nullptr if there is no such item.
*/
QQuickItem *QQuickItem::childAt(qreal x, qreal y) const
{
    const QList<QQuickItem *> children = childItems();
    for (int i = children.size()-1; i >= 0; --i) {
        QQuickItem *child = children.at(i);
        // Map coordinates to the child element's coordinate space
        QPointF point = mapToItem(child, QPointF(x, y));
        if (child->isVisible() && child->contains(point))
            return child;
    }
    return nullptr;
}

/*!
    \qmlmethod QtQuick::Item::dumpItemTree()

    Dumps some details about the
    \l {Concepts - Visual Parent in Qt Quick}{visual tree of Items} starting
    with this item and its children, recursively.

    The output looks similar to that of this QML code:

    \qml
    function dump(object, indent) {
        console.log(indent + object)
        for (const i in object.children)
            dump(object.children[i], indent + "    ")
    }

    dump(myItem, "")
    \endqml

    So if you want more details, you can implement your own function and add
    extra output to the console.log, such as values of specific properties.

    \sa QObject::dumpObjectTree()
    \since 6.3
*/
/*!
    Dumps some details about the
    \l {Concepts - Visual Parent in Qt Quick}{visual tree of Items} starting
    with this item, recursively.

    \note QObject::dumpObjectTree() dumps a similar tree; but, as explained
    in \l {Concepts - Visual Parent in Qt Quick}, an item's QObject::parent()
    sometimes differs from its QQuickItem::parentItem(). You can dump
    both trees to see the difference.

    \note The exact output format may change in future versions of Qt.

    \since 6.3
    \sa {Debugging Techniques}
    \sa {https://doc.qt.io/GammaRay/gammaray-qtquick2-inspector.html}{GammaRay's Qt Quick Inspector}
*/
void QQuickItem::dumpItemTree() const
{
    Q_D(const QQuickItem);
    d->dumpItemTree(0);
}

void QQuickItemPrivate::dumpItemTree(int indent) const
{
    Q_Q(const QQuickItem);

    const auto indentStr = QString(indent * 4, QLatin1Char(' '));
    qDebug().nospace().noquote() << indentStr <<
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
                                    const_cast<QQuickItem *>(q);
#else
                                    q;
#endif
    if (extra.isAllocated()) {
        for (const auto handler : extra->pointerHandlers)
            qDebug().nospace().noquote() << indentStr << u"  \u26ee " << handler;
    }
    for (const QQuickItem *ch : childItems) {
        auto itemPriv = QQuickItemPrivate::get(ch);
        itemPriv->dumpItemTree(indent + 1);
    }
}

QQmlListProperty<QObject> QQuickItemPrivate::resources()
{
    // Do not synthesize replace().
    // It would be extremely expensive and wouldn't work with most methods.
    QQmlListProperty<QObject> result;
    result.object = q_func();
    result.append = QQuickItemPrivate::resources_append;
    result.count = QQuickItemPrivate::resources_count;
    result.at = QQuickItemPrivate::resources_at;
    result.clear = QQuickItemPrivate::resources_clear;
    result.removeLast = QQuickItemPrivate::resources_removeLast;
    return result;
}

/*!
    \qmlproperty list<Item> QtQuick::Item::children
    \qmlproperty list<QtObject> QtQuick::Item::resources

    The children property contains the list of visual children of this item.
    The resources property contains non-visual resources that you want to
    reference by name.

    It is not generally necessary to refer to these properties when adding
    child items or resources, as the default \l data property will
    automatically assign child objects to the \c children and \c resources
    properties as appropriate. See the \l data documentation for details.
*/
/*!
    \property QQuickItem::children
    \internal
*/
QQmlListProperty<QQuickItem> QQuickItemPrivate::children()
{
    // Do not synthesize replace().
    // It would be extremely expensive and wouldn't work with most methods.
    QQmlListProperty<QQuickItem> result;
    result.object = q_func();
    result.append = QQuickItemPrivate::children_append;
    result.count = QQuickItemPrivate::children_count;
    result.at = QQuickItemPrivate::children_at;
    result.clear = QQuickItemPrivate::children_clear;
    result.removeLast = QQuickItemPrivate::children_removeLast;
    return result;
}

/*!
  \qmlproperty list<Item> QtQuick::Item::visibleChildren
  This read-only property lists all of the item's children that are currently visible.
  Note that a child's visibility may have changed explicitly, or because the visibility
  of this (it's parent) item or another grandparent changed.
*/
/*!
    \property QQuickItem::visibleChildren
    \internal
*/
QQmlListProperty<QQuickItem> QQuickItemPrivate::visibleChildren()
{
    return QQmlListProperty<QQuickItem>(q_func(),
                                        nullptr,
                                        QQuickItemPrivate::visibleChildren_count,
                                        QQuickItemPrivate::visibleChildren_at);

}

/*!
    \qmlproperty list<State> QtQuick::Item::states

    This property holds the list of possible states for this item. To change
    the state of this item, set the \l state property to one of these states,
    or set the \l state property to an empty string to revert the item to its
    default state.

    This property is specified as a list of \l State objects. For example,
    below is an item with "red_color" and "blue_color" states:

    \qml
    import QtQuick 2.0

    Rectangle {
        id: root
        width: 100; height: 100

        states: [
            State {
                name: "red_color"
                PropertyChanges { root.color: "red" }
            },
            State {
                name: "blue_color"
                PropertyChanges { root.color: "blue" }
            }
        ]
    }
    \endqml

    See \l{Qt Quick States} and \l{Animation and Transitions in Qt Quick} for
    more details on using states and transitions.

    \sa transitions
*/
/*!
    \property QQuickItem::states
    \internal
  */
QQmlListProperty<QQuickState> QQuickItemPrivate::states()
{
    return _states()->statesProperty();
}

/*!
    \qmlproperty list<Transition> QtQuick::Item::transitions

    This property holds the list of transitions for this item. These define the
    transitions to be applied to the item whenever it changes its \l state.

    This property is specified as a list of \l Transition objects. For example:

    \qml
    import QtQuick 2.0

    Item {
        transitions: [
            Transition {
                //...
            },
            Transition {
                //...
            }
        ]
    }
    \endqml

    See \l{Qt Quick States} and \l{Animation and Transitions in Qt Quick} for
    more details on using states and transitions.

    \sa states
*/
/*!
    \property QQuickItem::transitions
    \internal
  */
QQmlListProperty<QQuickTransition> QQuickItemPrivate::transitions()
{
    return _states()->transitionsProperty();
}

QString QQuickItemPrivate::state() const
{
    if (!_stateGroup)
        return QString();
    else
        return _stateGroup->state();
}

void QQuickItemPrivate::setState(const QString &state)
{
    _states()->setState(state);
}

/*!
    \qmlproperty string QtQuick::Item::state

    This property holds the name of the current state of the item.

    If the item is in its default state, that is, no explicit state has been
    set, then this property holds an empty string. Likewise, you can return
    an item to its default state by setting this property to an empty string.

    \sa {Qt Quick States}
*/
/*!
    \property QQuickItem::state

    This property holds the name of the current state of the item.

    If the item is in its default state, that is, no explicit state has been
    set, then this property holds an empty string. Likewise, you can return
    an item to its default state by setting this property to an empty string.

    \sa {Qt Quick States}
*/
QString QQuickItem::state() const
{
    Q_D(const QQuickItem);
    return d->state();
}

void QQuickItem::setState(const QString &state)
{
    Q_D(QQuickItem);
    d->setState(state);
}

/*!
  \qmlproperty list<Transform> QtQuick::Item::transform
  This property holds the list of transformations to apply.

  For more information see \l Transform.
*/
/*!
    \property QQuickItem::transform
    \internal
  */
/*!
    \internal
  */
QQmlListProperty<QQuickTransform> QQuickItem::transform()
{
    return QQmlListProperty<QQuickTransform>(this, nullptr, QQuickItemPrivate::transform_append,
                                                     QQuickItemPrivate::transform_count,
                                                     QQuickItemPrivate::transform_at,
                                                     QQuickItemPrivate::transform_clear);
}

/*!
  \reimp
  Derived classes should call the base class method before adding their own action to
  perform at classBegin.
*/
void QQuickItem::classBegin()
{
    Q_D(QQuickItem);
    d->componentComplete = false;
    if (d->_stateGroup)
        d->_stateGroup->classBegin();
    if (d->_anchors)
        d->_anchors->classBegin();
#if QT_CONFIG(quick_shadereffect)
    if (d->extra.isAllocated() && d->extra->layer)
        d->extra->layer->classBegin();
#endif
}

/*!
  \reimp
  Derived classes should call the base class method before adding their own actions to
  perform at componentComplete.
*/
void QQuickItem::componentComplete()
{
    Q_D(QQuickItem);
    d->componentComplete = true;
    if (d->_stateGroup)
        d->_stateGroup->componentComplete();
    if (d->_anchors) {
        d->_anchors->componentComplete();
        QQuickAnchorsPrivate::get(d->_anchors)->updateOnComplete();
    }

    if (d->extra.isAllocated()) {
#if QT_CONFIG(quick_shadereffect)
        if (d->extra->layer)
            d->extra->layer->componentComplete();
#endif

        if (d->extra->keyHandler)
            d->extra->keyHandler->componentComplete();

        if (d->extra->contents)
            d->extra->contents->complete();
    }

    if (d->window && d->dirtyAttributes) {
        d->addToDirtyList();
        QQuickWindowPrivate::get(d->window)->dirtyItem(this);
    }

#if QT_CONFIG(accessibility)
    if (d->isAccessible && d->effectiveVisible) {
        QAccessibleEvent ev(this, QAccessible::ObjectShow);
        QAccessible::updateAccessibility(&ev);
    }
#endif
}

QQuickStateGroup *QQuickItemPrivate::_states()
{
    Q_Q(QQuickItem);
    if (!_stateGroup) {
        _stateGroup = new QQuickStateGroup;
        if (!componentComplete)
            _stateGroup->classBegin();
        qmlobject_connect(_stateGroup, QQuickStateGroup, SIGNAL(stateChanged(QString)),
                          q, QQuickItem, SIGNAL(stateChanged(QString)));
    }

    return _stateGroup;
}

QPointF QQuickItemPrivate::computeTransformOrigin() const
{
    switch (origin()) {
    default:
    case QQuickItem::TopLeft:
        return QPointF(0, 0);
    case QQuickItem::Top:
        return QPointF(width / 2., 0);
    case QQuickItem::TopRight:
        return QPointF(width, 0);
    case QQuickItem::Left:
        return QPointF(0, height / 2.);
    case QQuickItem::Center:
        return QPointF(width / 2., height / 2.);
    case QQuickItem::Right:
        return QPointF(width, height / 2.);
    case QQuickItem::BottomLeft:
        return QPointF(0, height);
    case QQuickItem::Bottom:
        return QPointF(width / 2., height);
    case QQuickItem::BottomRight:
        return QPointF(width, height);
    }
}

/*!
    \internal
    QQuickItemPrivate::dirty() calls transformChanged(q) to inform this item and
    all its children that its transform has changed, with \a transformedItem always
    being the parent item that caused the change.  Override to react, e.g. to
    call update() if the item needs to re-generate SG nodes based on visible extents.
    If you override in a subclass, you must also call this (superclass) function
    and return the value from it.

    This function recursively visits all children as long as
    subtreeTransformChangedEnabled is true, returns \c true if any of those
    children still has the ItemObservesViewport flag set, but otherwise
    turns subtreeTransformChangedEnabled off, if no children are observing.
*/
bool QQuickItemPrivate::transformChanged(QQuickItem *transformedItem)
{
    Q_Q(QQuickItem);

    bool childWantsIt = false;
    if (subtreeTransformChangedEnabled) {
        // Inform the children in paint order: by the time we visit leaf items,
        // they can see any consequences in their parents
        for (auto child : paintOrderChildItems())
            childWantsIt |= QQuickItemPrivate::get(child)->transformChanged(transformedItem);
    }

#if QT_CONFIG(quick_shadereffect)
    if (q == transformedItem) {
        if (extra.isAllocated() && extra->layer)
            extra->layer->updateMatrix();
    }
#endif
    const bool thisWantsIt = q->flags().testFlag(QQuickItem::ItemObservesViewport);
    const bool ret = childWantsIt || thisWantsIt;
    if (!ret && componentComplete && subtreeTransformChangedEnabled) {
        qCDebug(lcVP) << "turned off subtree transformChanged notification after checking all children of" << q;
        subtreeTransformChangedEnabled = false;
    }
    // If ItemObservesViewport, clipRect() calculates the intersection with the viewport;
    // so each time the item moves in the viewport, its clipnode needs to be updated.
    if (thisWantsIt && q->clip())
        dirty(QQuickItemPrivate::Clip);
    return ret;
}

/*! \internal
    Returns the new position (proposed values for the x and y properties)
    to which this item should be moved to compensate for the given change
    in scale from \a startScale to \a activeScale and in rotation from
    \a startRotation to \a activeRotation. \a centroidParentPos is the
    point that we wish to hold in place (and then apply \a activeTranslation to),
    in this item's parent's coordinate system. \a startPos is this item's
    position in its parent's coordinate system when the gesture began.
    \a activeTranslation is the amount of translation that should be added to
    the return value, i.e. the displacement by which the centroid is expected
    to move.

    If \a activeTranslation is \c (0, 0) the centroid is to be held in place.
    If \a activeScale is \c 1, it means scale is intended to be held constant,
    the same as \a startScale. If \a activeRotation is \c 0, it means rotation
    is intended to be held constant, the same as \a startRotation.
*/
QPointF QQuickItemPrivate::adjustedPosForTransform(const QPointF &centroidParentPos,
                                                   const QPointF &startPos,
                                                   const QVector2D &activeTranslation,
                                                   qreal startScale,
                                                   qreal activeScale,
                                                   qreal startRotation,
                                                   qreal activeRotation)
{
    Q_Q(QQuickItem);
    QVector3D xformOrigin(q->transformOriginPoint());
    QMatrix4x4 startMatrix;
    startMatrix.translate(float(startPos.x()), float(startPos.y()));
    startMatrix.translate(xformOrigin);
    startMatrix.scale(float(startScale));
    startMatrix.rotate(float(startRotation), 0, 0, -1);
    startMatrix.translate(-xformOrigin);

    const QVector3D centroidParentVector(centroidParentPos);
    QMatrix4x4 mat;
    mat.translate(centroidParentVector);
    mat.rotate(float(activeRotation), 0, 0, 1);
    mat.scale(float(activeScale));
    mat.translate(-centroidParentVector);
    mat.translate(QVector3D(activeTranslation));

    mat = mat * startMatrix;

    QPointF xformOriginPoint = q->transformOriginPoint();
    QPointF pos = mat.map(xformOriginPoint);
    pos -= xformOriginPoint;

    return pos;
}

/*! \internal
    Returns the delivery agent for the narrowest subscene containing this item,
    but falls back to QQuickWindowPrivate::deliveryAgent if there are no subscenes.

    If this item is not sure whether it's in a subscene (as by default), we need to
    explore the parents to find out.

    If this item is in a subscene, we will find that DA during the exploration,
    and return it.

    If we find the root item without finding a DA, then we know that this item
    does NOT belong to a subscene, so we remember that by setting
    maybeHasSubsceneDeliveryAgent to false, so that exploration of the parents
    can be avoided next time.

    In the usual case in normal 2D scenes without subscenes,
    maybeHasSubsceneDeliveryAgent gets set to false here.

    \note When a Qt Quick scene is shown in the usual way in its own window,
    subscenes are ignored, and QQuickWindowPrivate::deliveryAgent is used.
    Subscene delivery agents are used only in QtQuick 3D so far.
*/
QQuickDeliveryAgent *QQuickItemPrivate::deliveryAgent()
{
    Q_Q(QQuickItem);
    if (maybeHasSubsceneDeliveryAgent) {
        QQuickItemPrivate *p = this;
        do {
            if (qmlobject_cast<QQuickRootItem *>(p->q_ptr)) {
                // found the root item without finding a different DA:
                // it means we don't need to repeat this search next time.
                // TODO maybe optimize further: make this function recursive, and
                // set it to false on each item that we visit in the tail
                maybeHasSubsceneDeliveryAgent = false;
                break;
            }
            if (p->extra.isAllocated()) {
                if (auto da = p->extra->subsceneDeliveryAgent)
                    return da;
            }
            p = p->parentItem ? QQuickItemPrivate::get(p->parentItem) : nullptr;
        } while (p);
        // arriving here is somewhat unexpected: a detached root can easily be created (just set an item's parent to null),
        // but why would we deliver events to that subtree? only if root got detached while an item in that subtree still has a grab?
        qCDebug(lcPtr) << "detached root of" << q << "is not a QQuickRootItem and also does not have its own DeliveryAgent";
    }
    if (window)
        return QQuickWindowPrivate::get(window)->deliveryAgent;
    return nullptr;
}

QQuickDeliveryAgentPrivate *QQuickItemPrivate::deliveryAgentPrivate()
{
    auto da = deliveryAgent();
    return da ? static_cast<QQuickDeliveryAgentPrivate *>(QQuickDeliveryAgentPrivate::get(da)) : nullptr;
}

/*! \internal
    Ensures that this item, presumably the root of a subscene (e.g. because it
    is mapped onto a 3D object in Qt Quick 3D), has a delivery agent to be used
    when delivering events to the subscene: i.e. when the viewport delivers an
    event to the subscene, or when the outer delivery agent delivers an update
    to an item that grabbed during a previous subscene delivery.  Creates a new
    agent if it was not already created, and returns a pointer to the instance.
*/
QQuickDeliveryAgent *QQuickItemPrivate::ensureSubsceneDeliveryAgent()
{
    Q_Q(QQuickItem);
    // We are (about to be) sure that it has one now; but just to save space,
    // we avoid storing a DA pointer in each item; so deliveryAgent() always needs to
    // go up the hierarchy to find it. maybeHasSubsceneDeliveryAgent tells it to do that.
    maybeHasSubsceneDeliveryAgent = true;
    if (extra.isAllocated() && extra->subsceneDeliveryAgent)
        return extra->subsceneDeliveryAgent;
    extra.value().subsceneDeliveryAgent = new QQuickDeliveryAgent(q);
    qCDebug(lcPtr) << "created new" << extra->subsceneDeliveryAgent;
    // every subscene root needs to be a focus scope so that when QQuickItem::forceActiveFocus()
    // goes up the parent hierarchy, it finds the subscene root and calls setFocus() on it
    q->setFlag(QQuickItem::ItemIsFocusScope);
    return extra->subsceneDeliveryAgent;
}

bool QQuickItemPrivate::filterKeyEvent(QKeyEvent *e, bool post)
{
    if (!extra.isAllocated() || !extra->keyHandler)
        return false;

    if (post)
        e->accept();

    if (e->type() == QEvent::KeyPress)
        extra->keyHandler->keyPressed(e, post);
    else
        extra->keyHandler->keyReleased(e, post);

    return e->isAccepted();
}

void QQuickItemPrivate::deliverKeyEvent(QKeyEvent *e)
{
    Q_Q(QQuickItem);

    Q_ASSERT(e->isAccepted());
    if (filterKeyEvent(e, false))
        return;
    else
        e->accept();

    if (e->type() == QEvent::KeyPress)
        q->keyPressEvent(e);
    else
        q->keyReleaseEvent(e);

    if (e->isAccepted())
        return;

    if (filterKeyEvent(e, true) || !q->window())
        return;

    //only care about KeyPress now
    if (e->type() == QEvent::KeyPress &&
            (q == q->window()->contentItem() || q->activeFocusOnTab())) {
        bool res = false;
        if (!(e->modifiers() & (Qt::ControlModifier | Qt::AltModifier))) {  //### Add MetaModifier?
            if (e->key() == Qt::Key_Backtab
                || (e->key() == Qt::Key_Tab && (e->modifiers() & Qt::ShiftModifier)))
                res = QQuickItemPrivate::focusNextPrev(q, false);
            else if (e->key() == Qt::Key_Tab)
                res = QQuickItemPrivate::focusNextPrev(q, true);
            if (res)
                e->setAccepted(true);
        }
    }
}

#if QT_CONFIG(im)
void QQuickItemPrivate::deliverInputMethodEvent(QInputMethodEvent *e)
{
    Q_Q(QQuickItem);

    Q_ASSERT(e->isAccepted());
    if (extra.isAllocated() && extra->keyHandler) {
        extra->keyHandler->inputMethodEvent(e, false);

        if (e->isAccepted())
            return;
        else
            e->accept();
    }

    q->inputMethodEvent(e);

    if (e->isAccepted())
        return;

    if (extra.isAllocated() && extra->keyHandler) {
        e->accept();

        extra->keyHandler->inputMethodEvent(e, true);
    }
}
#endif // im

void QQuickItemPrivate::deliverShortcutOverrideEvent(QKeyEvent *event)
{
    if (extra.isAllocated() && extra->keyHandler)
        extra->keyHandler->shortcutOverrideEvent(event);
    else
        event->ignore();
}

bool QQuickItemPrivate::anyPointerHandlerWants(const QPointerEvent *event, const QEventPoint &point) const
{
    if (!hasPointerHandlers())
        return false;
    for (QQuickPointerHandler *handler : extra->pointerHandlers) {
        if (handler->wantsEventPoint(event, point))
            return true;
    }
    return false;
}

/*!
    \internal
    Deliver the \a event to all this item's PointerHandlers, but skip
    HoverHandlers if the event is a QMouseEvent or QWheelEvent (they are visited
    in QQuickDeliveryAgentPrivate::deliverHoverEventToItem()), and skip handlers
    that are in QQuickPointerHandlerPrivate::deviceDeliveryTargets().
    If \a avoidGrabbers is true, also skip delivery to any handler that
    is exclusively or passively grabbing any point within \a event
    (because delivery to grabbers is handled separately).
*/
bool QQuickItemPrivate::handlePointerEvent(QPointerEvent *event, bool avoidGrabbers)
{
    bool delivered = false;
    if (extra.isAllocated()) {
        for (QQuickPointerHandler *handler : extra->pointerHandlers) {
            bool avoidThisHandler = false;
            if (QQuickDeliveryAgentPrivate::isMouseOrWheelEvent(event) &&
                    qmlobject_cast<const QQuickHoverHandler *>(handler)) {
                avoidThisHandler = true;
            } else if (avoidGrabbers) {
                for (auto &p : event->points()) {
                    if (event->exclusiveGrabber(p) == handler || event->passiveGrabbers(p).contains(handler)) {
                        avoidThisHandler = true;
                        break;
                    }
                }
            }
            if (!avoidThisHandler &&
                    !QQuickPointerHandlerPrivate::deviceDeliveryTargets(event->device()).contains(handler)) {
                handler->handlePointerEvent(event);
                delivered = true;
            }
        }
    }
    return delivered;
}

/*!
    Called when \a change occurs for this item.

    \a value contains extra information relating to the change, when
    applicable.

    If you re-implement this method in a subclass, be sure to call
    \code
    QQuickItem::itemChange(change, value);
    \endcode
    typically at the end of your implementation, to ensure the
    \l windowChanged() signal will be emitted.
  */
void QQuickItem::itemChange(ItemChange change, const ItemChangeData &value)
{
    if (change == ItemSceneChange)
        emit windowChanged(value.window);
}

#if QT_CONFIG(im)
/*!
    Notify input method on updated query values if needed. \a queries indicates
    the changed attributes.
*/
void QQuickItem::updateInputMethod(Qt::InputMethodQueries queries)
{
    if (hasActiveFocus())
        QGuiApplication::inputMethod()->update(queries);
}
#endif // im

/*!
    Returns the extents of the item in its own coordinate system:
    a rectangle from \c{0, 0} to \l width() and \l height().
*/
QRectF QQuickItem::boundingRect() const
{
    Q_D(const QQuickItem);
    return QRectF(0, 0, d->width, d->height);
}

/*!
    Returns the rectangular area within this item that is currently visible in
    \l viewportItem(), if there is a viewport and the \l ItemObservesViewport
    flag is set; otherwise, the extents of this item in its own coordinate
    system: a rectangle from \c{0, 0} to \l width() and \l height(). This is
    the region intended to remain visible if \l clip is \c true. It can also be
    used in updatePaintNode() to limit the graphics added to the scene graph.

    For example, a large drawing or a large text document might be shown in a
    Flickable that occupies only part of the application's Window: in that
    case, Flickable is the viewport item, and a custom content-rendering item
    may choose to omit scene graph nodes that fall outside the area that is
    currently visible. If the \l ItemObservesViewport flag is set, this area
    will change each time the user scrolls the content in the Flickable.

    In case of nested viewport items, clipRect() is the intersection of the
    \c {boundingRect}s of all ancestors that have the \l ItemIsViewport flag set,
    mapped to the coordinate system of \e this item.

    \sa boundingRect()
*/
QRectF QQuickItem::clipRect() const
{
    Q_D(const QQuickItem);
    QRectF ret(0, 0, d->width, d->height);
    if (flags().testFlag(QQuickItem::ItemObservesViewport)) {
        if (QQuickItem *viewport = viewportItem()) {
            // if the viewport is already "this", there's nothing to intersect;
            // and don't call clipRect() again, to avoid infinite recursion
            if (viewport == this)
                return ret;
            const auto mappedViewportRect = mapRectFromItem(viewport, viewport->clipRect());
            qCDebug(lcVP) << this << "intersecting" << viewport << mappedViewportRect << ret << "->" << mappedViewportRect.intersected(ret);
            return mappedViewportRect.intersected(ret);
        }
    }
    return ret;
}

/*!
    If the \l ItemObservesViewport flag is set,
    returns the nearest parent with the \l ItemIsViewport flag.
    Returns the window's contentItem if the flag is not set,
    or if no other viewport item is found.

    Returns \nullptr only if there is no viewport item and this item is not
    shown in a window.

    \sa clipRect()
*/
QQuickItem *QQuickItem::viewportItem() const
{
    if (flags().testFlag(ItemObservesViewport)) {
        QQuickItem *par = parentItem();
        while (par) {
            if (par->flags().testFlag(QQuickItem::ItemIsViewport))
                return par;
            par = par->parentItem();
        }
    }
    return (window() ? window()->contentItem() : nullptr);
}

/*!
    \qmlproperty enumeration QtQuick::Item::transformOrigin
    This property holds the origin point around which scale and rotation transform.

    Nine transform origins are available, as shown in the image below.
    The default transform origin is \c Item.Center.

    \image declarative-transformorigin.png

    This example rotates an image around its bottom-right corner.
    \qml
    Image {
        source: "myimage.png"
        transformOrigin: Item.BottomRight
        rotation: 45
    }
    \endqml

    To set an arbitrary transform origin point use the \l Scale or \l Rotation
    transform types with \l transform.
*/
/*!
    \property QQuickItem::transformOrigin
    This property holds the origin point around which scale and rotation transform.

    Nine transform origins are available, as shown in the image below.
    The default transform origin is \c Item.Center.

    \image declarative-transformorigin.png
*/
QQuickItem::TransformOrigin QQuickItem::transformOrigin() const
{
    Q_D(const QQuickItem);
    return d->origin();
}

void QQuickItem::setTransformOrigin(TransformOrigin origin)
{
    Q_D(QQuickItem);
    if (origin == d->origin())
        return;

    d->extra.value().origin = origin;
    d->dirty(QQuickItemPrivate::TransformOrigin);

    emit transformOriginChanged(d->origin());
}

/*!
    \property QQuickItem::transformOriginPoint
    \internal
  */
/*!
  \internal
  */
QPointF QQuickItem::transformOriginPoint() const
{
    Q_D(const QQuickItem);
    if (d->extra.isAllocated() && !d->extra->userTransformOriginPoint.isNull())
        return d->extra->userTransformOriginPoint;
    return d->computeTransformOrigin();
}

/*!
  \internal
  */
void QQuickItem::setTransformOriginPoint(const QPointF &point)
{
    Q_D(QQuickItem);
    if (d->extra.value().userTransformOriginPoint == point)
        return;

    d->extra->userTransformOriginPoint = point;
    d->dirty(QQuickItemPrivate::TransformOrigin);
}

/*!
  \qmlproperty real QtQuick::Item::z

  Sets the stacking order of sibling items.  By default the stacking order is 0.

  Items with a higher stacking value are drawn on top of siblings with a
  lower stacking order.  Items with the same stacking value are drawn
  bottom up in the order they appear.  Items with a negative stacking
  value are drawn under their parent's content.

  The following example shows the various effects of stacking order.

  \table
  \row
  \li \image declarative-item_stacking1.png
  \li Same \c z - later children above earlier children:
  \qml
  Item {
      Rectangle {
          color: "red"
          width: 100; height: 100
      }
      Rectangle {
          color: "blue"
          x: 50; y: 50; width: 100; height: 100
      }
  }
  \endqml
  \row
  \li \image declarative-item_stacking2.png
  \li Higher \c z on top:
  \qml
  Item {
      Rectangle {
          z: 1
          color: "red"
          width: 100; height: 100
      }
      Rectangle {
          color: "blue"
          x: 50; y: 50; width: 100; height: 100
      }
  }
  \endqml
  \row
  \li \image declarative-item_stacking3.png
  \li Same \c z - children above parents:
  \qml
  Item {
      Rectangle {
          color: "red"
          width: 100; height: 100
          Rectangle {
              color: "blue"
              x: 50; y: 50; width: 100; height: 100
          }
      }
  }
  \endqml
  \row
  \li \image declarative-item_stacking4.png
  \li Lower \c z below:
  \qml
  Item {
      Rectangle {
          color: "red"
          width: 100; height: 100
          Rectangle {
              z: -1
              color: "blue"
              x: 50; y: 50; width: 100; height: 100
          }
      }
  }
  \endqml
  \endtable
 */
/*!
  \property QQuickItem::z

  Sets the stacking order of sibling items.  By default the stacking order is 0.

  Items with a higher stacking value are drawn on top of siblings with a
  lower stacking order.  Items with the same stacking value are drawn
  bottom up in the order they appear.  Items with a negative stacking
  value are drawn under their parent's content.

  The following example shows the various effects of stacking order.

  \table
  \row
  \li \image declarative-item_stacking1.png
  \li Same \c z - later children above earlier children:
  \qml
  Item {
      Rectangle {
          color: "red"
          width: 100; height: 100
      }
      Rectangle {
          color: "blue"
          x: 50; y: 50; width: 100; height: 100
      }
  }
  \endqml
  \row
  \li \image declarative-item_stacking2.png
  \li Higher \c z on top:
  \qml
  Item {
      Rectangle {
          z: 1
          color: "red"
          width: 100; height: 100
      }
      Rectangle {
          color: "blue"
          x: 50; y: 50; width: 100; height: 100
      }
  }
  \endqml
  \row
  \li \image declarative-item_stacking3.png
  \li Same \c z - children above parents:
  \qml
  Item {
      Rectangle {
          color: "red"
          width: 100; height: 100
          Rectangle {
              color: "blue"
              x: 50; y: 50; width: 100; height: 100
          }
      }
  }
  \endqml
  \row
  \li \image declarative-item_stacking4.png
  \li Lower \c z below:
  \qml
  Item {
      Rectangle {
          color: "red"
          width: 100; height: 100
          Rectangle {
              z: -1
              color: "blue"
              x: 50; y: 50; width: 100; height: 100
          }
      }
  }
  \endqml
  \endtable
  */
qreal QQuickItem::z() const
{
    Q_D(const QQuickItem);
    return d->z();
}

void QQuickItem::setZ(qreal v)
{
    Q_D(QQuickItem);
    if (d->z() == v)
        return;

    d->extra.value().z = v;

    d->dirty(QQuickItemPrivate::ZValue);
    if (d->parentItem) {
        QQuickItemPrivate::get(d->parentItem)->dirty(QQuickItemPrivate::ChildrenStackingChanged);
        QQuickItemPrivate::get(d->parentItem)->markSortedChildrenDirty(this);
    }

    emit zChanged();

#if QT_CONFIG(quick_shadereffect)
    if (d->extra.isAllocated() && d->extra->layer)
        d->extra->layer->updateZ();
#endif
}

/*!
  \qmlproperty real QtQuick::Item::rotation
  This property holds the rotation of the item in degrees clockwise around
  its transformOrigin.

  The default value is 0 degrees (that is, no rotation).

  \table
  \row
  \li \image declarative-rotation.png
  \li
  \qml
  Rectangle {
      color: "blue"
      width: 100; height: 100
      Rectangle {
          color: "red"
          x: 25; y: 25; width: 50; height: 50
          rotation: 30
      }
  }
  \endqml
  \endtable

  \sa Transform, Rotation
*/
/*!
  \property QQuickItem::rotation
  This property holds the rotation of the item in degrees clockwise around
  its transformOrigin.

  The default value is 0 degrees (that is, no rotation).

  \table
  \row
  \li \image declarative-rotation.png
  \li
  \qml
  Rectangle {
      color: "blue"
      width: 100; height: 100
      Rectangle {
          color: "red"
          x: 25; y: 25; width: 50; height: 50
          rotation: 30
      }
  }
  \endqml
  \endtable

  \sa Transform, Rotation
  */
qreal QQuickItem::rotation() const
{
    Q_D(const QQuickItem);
    return d->rotation();
}

void QQuickItem::setRotation(qreal r)
{
    Q_D(QQuickItem);
    if (d->rotation() == r)
        return;

    d->extra.value().rotation = r;

    d->dirty(QQuickItemPrivate::BasicTransform);

    d->itemChange(ItemRotationHasChanged, r);

    emit rotationChanged();
}

/*!
  \qmlproperty real QtQuick::Item::scale
  This property holds the scale factor for this item.

  A scale of less than 1.0 causes the item to be rendered at a smaller
  size, and a scale greater than 1.0 renders the item at a larger size.
  A negative scale causes the item to be mirrored when rendered.

  The default value is 1.0.

  Scaling is applied from the transformOrigin.

  \table
  \row
  \li \image declarative-scale.png
  \li
  \qml
  import QtQuick 2.0

  Rectangle {
      color: "blue"
      width: 100; height: 100

      Rectangle {
          color: "green"
          width: 25; height: 25
      }

      Rectangle {
          color: "red"
          x: 25; y: 25; width: 50; height: 50
          scale: 1.4
          transformOrigin: Item.TopLeft
      }
  }
  \endqml
  \endtable

  \sa Transform, Scale
*/
/*!
  \property QQuickItem::scale
  This property holds the scale factor for this item.

  A scale of less than 1.0 causes the item to be rendered at a smaller
  size, and a scale greater than 1.0 renders the item at a larger size.
  A negative scale causes the item to be mirrored when rendered.

  The default value is 1.0.

  Scaling is applied from the transformOrigin.

  \table
  \row
  \li \image declarative-scale.png
  \li
  \qml
  import QtQuick 2.0

  Rectangle {
      color: "blue"
      width: 100; height: 100

      Rectangle {
          color: "green"
          width: 25; height: 25
      }

      Rectangle {
          color: "red"
          x: 25; y: 25; width: 50; height: 50
          scale: 1.4
      }
  }
  \endqml
  \endtable

  \sa Transform, Scale
  */
qreal QQuickItem::scale() const
{
    Q_D(const QQuickItem);
    return d->scale();
}

void QQuickItem::setScale(qreal s)
{
    Q_D(QQuickItem);
    if (d->scale() == s)
        return;

    d->extra.value().scale = s;

    d->dirty(QQuickItemPrivate::BasicTransform);

    emit scaleChanged();
}

/*!
  \qmlproperty real QtQuick::Item::opacity

  This property holds the opacity of the item.  Opacity is specified as a
  number between 0.0 (fully transparent) and 1.0 (fully opaque). The default
  value is 1.0.

  When this property is set, the specified opacity is also applied
  individually to child items. This may have an unintended effect in some
  circumstances. For example in the second set of rectangles below, the red
  rectangle has specified an opacity of 0.5, which affects the opacity of
  its blue child rectangle even though the child has not specified an opacity.

  \table
  \row
  \li \image declarative-item_opacity1.png
  \li
  \qml
    Item {
        Rectangle {
            color: "red"
            width: 100; height: 100
            Rectangle {
                color: "blue"
                x: 50; y: 50; width: 100; height: 100
            }
        }
    }
  \endqml
  \row
  \li \image declarative-item_opacity2.png
  \li
  \qml
    Item {
        Rectangle {
            opacity: 0.5
            color: "red"
            width: 100; height: 100
            Rectangle {
                color: "blue"
                x: 50; y: 50; width: 100; height: 100
            }
        }
    }
  \endqml
  \endtable

  Changing an item's opacity does not affect whether the item receives user
  input events. (In contrast, setting \l visible property to \c false stops
  mouse events, and setting the \l enabled property to \c false stops mouse
  and keyboard events, and also removes active focus from the item.)

  \sa visible
*/
/*!
  \property QQuickItem::opacity

  This property holds the opacity of the item.  Opacity is specified as a
  number between 0.0 (fully transparent) and 1.0 (fully opaque). The default
  value is 1.0.

  When this property is set, the specified opacity is also applied
  individually to child items. This may have an unintended effect in some
  circumstances. For example in the second set of rectangles below, the red
  rectangle has specified an opacity of 0.5, which affects the opacity of
  its blue child rectangle even though the child has not specified an opacity.

  Values outside the range of 0 to 1 will be clamped.

  \table
  \row
  \li \image declarative-item_opacity1.png
  \li
  \qml
    Item {
        Rectangle {
            color: "red"
            width: 100; height: 100
            Rectangle {
                color: "blue"
                x: 50; y: 50; width: 100; height: 100
            }
        }
    }
  \endqml
  \row
  \li \image declarative-item_opacity2.png
  \li
  \qml
    Item {
        Rectangle {
            opacity: 0.5
            color: "red"
            width: 100; height: 100
            Rectangle {
                color: "blue"
                x: 50; y: 50; width: 100; height: 100
            }
        }
    }
  \endqml
  \endtable

  Changing an item's opacity does not affect whether the item receives user
  input events. (In contrast, setting \l visible property to \c false stops
  mouse events, and setting the \l enabled property to \c false stops mouse
  and keyboard events, and also removes active focus from the item.)

  \sa visible
*/
qreal QQuickItem::opacity() const
{
    Q_D(const QQuickItem);
    return d->opacity();
}

void QQuickItem::setOpacity(qreal newOpacity)
{
    Q_D(QQuickItem);
    qreal o = qBound<qreal>(0, newOpacity, 1);
    if (d->opacity() == o)
        return;

    d->extra.value().opacity = o;

    d->dirty(QQuickItemPrivate::OpacityValue);

    d->itemChange(ItemOpacityHasChanged, o);

    emit opacityChanged();
}

/*!
    \qmlproperty bool QtQuick::Item::visible

    This property holds whether the item is visible. By default this is true.

    Setting this property directly affects the \c visible value of child
    items. When set to \c false, the \c visible values of all child items also
    become \c false. When set to \c true, the \c visible values of child items
    are returned to \c true, unless they have explicitly been set to \c false.

    (Because of this flow-on behavior, using the \c visible property may not
    have the intended effect if a property binding should only respond to
    explicit property changes. In such cases it may be better to use the
    \l opacity property instead.)

    If this property is set to \c false, the item will no longer receive mouse
    events, but will continue to receive key events and will retain the keyboard
    \l focus if it has been set. (In contrast, setting the \l enabled property
    to \c false stops both mouse and keyboard events, and also removes focus
    from the item.)

    \note This property's value is only affected by changes to this property or
    the parent's \c visible property. It does not change, for example, if this
    item moves off-screen, or if the \l opacity changes to 0.

    \sa opacity, enabled
*/
/*!
    \property QQuickItem::visible

    This property holds whether the item is visible. By default this is true.

    Setting this property directly affects the \c visible value of child
    items. When set to \c false, the \c visible values of all child items also
    become \c false. When set to \c true, the \c visible values of child items
    are returned to \c true, unless they have explicitly been set to \c false.

    (Because of this flow-on behavior, using the \c visible property may not
    have the intended effect if a property binding should only respond to
    explicit property changes. In such cases it may be better to use the
    \l opacity property instead.)

    If this property is set to \c false, the item will no longer receive mouse
    events, but will continue to receive key events and will retain the keyboard
    \l focus if it has been set. (In contrast, setting the \l enabled property
    to \c false stops both mouse and keyboard events, and also removes focus
    from the item.)

    \note This property's value is only affected by changes to this property or
    the parent's \c visible property. It does not change, for example, if this
    item moves off-screen, or if the \l opacity changes to 0. However, for
    historical reasons, this property is true after the item's construction, even
    if the item hasn't been added to a scene yet. Changing or reading this
    property of an item that has not been added to a scene might not produce
    the expected results.

    \note The notification signal for this property gets emitted during destruction
    of the visual parent. C++ signal handlers cannot assume that items in the
    visual parent hierarchy are still fully constructed. Use \l qobject_cast to
    verify that items in the parent hierarchy can be used safely as the expected
    type.

    \sa opacity, enabled
*/
bool QQuickItem::isVisible() const
{
    Q_D(const QQuickItem);
    return d->effectiveVisible;
}

void QQuickItemPrivate::setVisible(bool visible)
{
    if (visible == explicitVisible)
        return;

    explicitVisible = visible;
    if (!visible)
        dirty(QQuickItemPrivate::Visible);

    const bool childVisibilityChanged = setEffectiveVisibleRecur(calcEffectiveVisible());
    if (childVisibilityChanged && parentItem)
        emit parentItem->visibleChildrenChanged();   // signal the parent, not this!
}

void QQuickItem::setVisible(bool v)
{
    Q_D(QQuickItem);
    d->setVisible(v);
}

/*!
    \qmlproperty bool QtQuick::Item::enabled

    This property holds whether the item receives mouse and keyboard events.
    By default this is true.

    Setting this property directly affects the \c enabled value of child
    items. When set to \c false, the \c enabled values of all child items also
    become \c false. When set to \c true, the \c enabled values of child items
    are returned to \c true, unless they have explicitly been set to \c false.

    Setting this property to \c false automatically causes \l activeFocus to be
    set to \c false, and this item will no longer receive keyboard events.

    \sa visible
*/
/*!
    \property QQuickItem::enabled

    This property holds whether the item receives mouse and keyboard events.
    By default this is true.

    Setting this property directly affects the \c enabled value of child
    items. When set to \c false, the \c enabled values of all child items also
    become \c false. When set to \c true, the \c enabled values of child items
    are returned to \c true, unless they have explicitly been set to \c false.

    Setting this property to \c false automatically causes \l activeFocus to be
    set to \c false, and this item will longer receive keyboard events.

    \note Hover events are enabled separately by \l setAcceptHoverEvents().
    Thus, a disabled item can continue to receive hover events, even when this
    property is \c false. This makes it possible to show informational feedback
    (such as \l ToolTip) even when an interactive item is disabled.
    The same is also true for any \l {HoverHandler}{HoverHandlers}
    added as children of the item. A HoverHandler can, however, be
    \l {PointerHandler::enabled}{disabled} explicitly, or for example
    be bound to the \c enabled state of the item.

    \sa visible
*/
bool QQuickItem::isEnabled() const
{
    Q_D(const QQuickItem);
    return d->effectiveEnable;
}

void QQuickItem::setEnabled(bool e)
{
    Q_D(QQuickItem);
    if (e == d->explicitEnable)
        return;

    d->explicitEnable = e;

    QQuickItem *scope = parentItem();
    while (scope && !scope->isFocusScope())
        scope = scope->parentItem();

    d->setEffectiveEnableRecur(scope, d->calcEffectiveEnable());
}

bool QQuickItemPrivate::calcEffectiveVisible() const
{
    // An item is visible if it is a child of a visible parent, and not explicitly hidden.
    return explicitVisible && parentItem && QQuickItemPrivate::get(parentItem)->effectiveVisible;
}

bool QQuickItemPrivate::setEffectiveVisibleRecur(bool newEffectiveVisible)
{
    Q_Q(QQuickItem);

    if (newEffectiveVisible && !explicitVisible) {
        // This item locally overrides visibility
        return false;   // effective visibility didn't change
    }

    if (newEffectiveVisible == effectiveVisible) {
        // No change necessary
        return false;   // effective visibility didn't change
    }

    effectiveVisible = newEffectiveVisible;
    dirty(Visible);
    if (parentItem)
        QQuickItemPrivate::get(parentItem)->dirty(ChildrenStackingChanged);
    if (window)
        if (auto agent = deliveryAgentPrivate(); agent)
            agent->removeGrabber(q, true, true, true);

    bool childVisibilityChanged = false;
    for (int ii = 0; ii < childItems.size(); ++ii)
        childVisibilityChanged |= QQuickItemPrivate::get(childItems.at(ii))->setEffectiveVisibleRecur(newEffectiveVisible);

    itemChange(QQuickItem::ItemVisibleHasChanged, bool(effectiveVisible));
#if QT_CONFIG(accessibility)
    if (isAccessible) {
        QAccessibleEvent ev(q, effectiveVisible ? QAccessible::ObjectShow : QAccessible::ObjectHide);
        QAccessible::updateAccessibility(&ev);
    }
#endif
    if (!inDestructor) {
        emit q->visibleChanged();
        if (childVisibilityChanged)
            emit q->visibleChildrenChanged();
    }

    return true;    // effective visibility DID change
}

bool QQuickItemPrivate::calcEffectiveEnable() const
{
    // XXX todo - Should the effective enable of an element with no parent just be the current
    // effective enable?  This would prevent pointless re-processing in the case of an element
    // moving to/from a no-parent situation, but it is different from what graphics view does.
    return explicitEnable && (!parentItem || QQuickItemPrivate::get(parentItem)->effectiveEnable);
}

void QQuickItemPrivate::setEffectiveEnableRecur(QQuickItem *scope, bool newEffectiveEnable)
{
    Q_Q(QQuickItem);

    if (newEffectiveEnable && !explicitEnable) {
        // This item locally overrides enable
        return;
    }

    if (newEffectiveEnable == effectiveEnable) {
        // No change necessary
        return;
    }

    effectiveEnable = newEffectiveEnable;

    QQuickDeliveryAgentPrivate *da = deliveryAgentPrivate();
    if (da) {
        da->removeGrabber(q, true, true, true);
        if (scope && !effectiveEnable && activeFocus) {
            da->clearFocusInScope(scope, q, Qt::OtherFocusReason,
                                  QQuickDeliveryAgentPrivate::DontChangeFocusProperty |
                                  QQuickDeliveryAgentPrivate::DontChangeSubFocusItem);
        }
    }

    for (int ii = 0; ii < childItems.size(); ++ii) {
        QQuickItemPrivate::get(childItems.at(ii))->setEffectiveEnableRecur(
                (flags & QQuickItem::ItemIsFocusScope) && scope ? q : scope, newEffectiveEnable);
    }

    if (scope && effectiveEnable && focus && da) {
        da->setFocusInScope(scope, q, Qt::OtherFocusReason,
                            QQuickDeliveryAgentPrivate::DontChangeFocusProperty |
                            QQuickDeliveryAgentPrivate::DontChangeSubFocusItem);
    }

    itemChange(QQuickItem::ItemEnabledHasChanged, bool(effectiveEnable));
#if QT_CONFIG(accessibility)
    if (isAccessible) {
        QAccessible::State changedState;
        changedState.disabled = true;
        changedState.focusable = true;
        QAccessibleStateChangeEvent ev(q, changedState);
        QAccessible::updateAccessibility(&ev);
    }
#endif
    emit q->enabledChanged();
}

bool QQuickItemPrivate::isTransparentForPositioner() const
{
    return extra.isAllocated() && extra.value().transparentForPositioner;
}

void QQuickItemPrivate::setTransparentForPositioner(bool transparent)
{
    extra.value().transparentForPositioner = transparent;
}


QString QQuickItemPrivate::dirtyToString() const
{
#define DIRTY_TO_STRING(value) if (dirtyAttributes & value) { \
    if (!rv.isEmpty()) \
        rv.append(QLatin1Char('|')); \
    rv.append(QLatin1String(#value)); \
}

//    QString rv = QLatin1String("0x") + QString::number(dirtyAttributes, 16);
    QString rv;

    DIRTY_TO_STRING(TransformOrigin);
    DIRTY_TO_STRING(Transform);
    DIRTY_TO_STRING(BasicTransform);
    DIRTY_TO_STRING(Position);
    DIRTY_TO_STRING(Size);
    DIRTY_TO_STRING(ZValue);
    DIRTY_TO_STRING(Content);
    DIRTY_TO_STRING(Smooth);
    DIRTY_TO_STRING(OpacityValue);
    DIRTY_TO_STRING(ChildrenChanged);
    DIRTY_TO_STRING(ChildrenStackingChanged);
    DIRTY_TO_STRING(ParentChanged);
    DIRTY_TO_STRING(Clip);
    DIRTY_TO_STRING(Window);
    DIRTY_TO_STRING(EffectReference);
    DIRTY_TO_STRING(Visible);
    DIRTY_TO_STRING(HideReference);
    DIRTY_TO_STRING(Antialiasing);

    return rv;
}

void QQuickItemPrivate::dirty(DirtyType type)
{
    Q_Q(QQuickItem);
    if (type & (TransformOrigin | Transform | BasicTransform | Position | Size))
        transformChanged(q);

    if (!(dirtyAttributes & type) || (window && !prevDirtyItem)) {
        dirtyAttributes |= type;
        if (window && componentComplete) {
            addToDirtyList();
            QQuickWindowPrivate::get(window)->dirtyItem(q);
        }
    }
}

void QQuickItemPrivate::addToDirtyList()
{
    Q_Q(QQuickItem);

    Q_ASSERT(window);
    if (!prevDirtyItem) {
        Q_ASSERT(!nextDirtyItem);

        QQuickWindowPrivate *p = QQuickWindowPrivate::get(window);
        nextDirtyItem = p->dirtyItemList;
        if (nextDirtyItem) QQuickItemPrivate::get(nextDirtyItem)->prevDirtyItem = &nextDirtyItem;
        prevDirtyItem = &p->dirtyItemList;
        p->dirtyItemList = q;
        p->dirtyItem(q);
    }
    Q_ASSERT(prevDirtyItem);
}

void QQuickItemPrivate::removeFromDirtyList()
{
    if (prevDirtyItem) {
        if (nextDirtyItem) QQuickItemPrivate::get(nextDirtyItem)->prevDirtyItem = prevDirtyItem;
        *prevDirtyItem = nextDirtyItem;
        prevDirtyItem = nullptr;
        nextDirtyItem = nullptr;
    }
    Q_ASSERT(!prevDirtyItem);
    Q_ASSERT(!nextDirtyItem);
}

void QQuickItemPrivate::refFromEffectItem(bool hide)
{
    ++extra.value().effectRefCount;
    if (extra->effectRefCount == 1) {
        dirty(EffectReference);
        if (parentItem)
            QQuickItemPrivate::get(parentItem)->dirty(ChildrenStackingChanged);
    }
    if (hide) {
        if (++extra->hideRefCount == 1)
            dirty(HideReference);
    }
    recursiveRefFromEffectItem(1);
}

void QQuickItemPrivate::recursiveRefFromEffectItem(int refs)
{
    Q_Q(QQuickItem);
    if (!refs)
        return;
    extra.value().recursiveEffectRefCount += refs;
    for (int ii = 0; ii < childItems.size(); ++ii) {
        QQuickItem *child = childItems.at(ii);
        QQuickItemPrivate::get(child)->recursiveRefFromEffectItem(refs);
    }
    // Polish may rely on the effect ref count so trigger one, if item is not visible
    // (if visible, it will be triggered automatically).
    if (!effectiveVisible && refs > 0 && extra.value().recursiveEffectRefCount == 1) // it wasn't referenced, now it's referenced
        q->polish();
}

void QQuickItemPrivate::derefFromEffectItem(bool unhide)
{
    Q_ASSERT(extra->effectRefCount);
    --extra->effectRefCount;
    if (extra->effectRefCount == 0) {
        dirty(EffectReference);
        if (parentItem)
            QQuickItemPrivate::get(parentItem)->dirty(ChildrenStackingChanged);
    }
    if (unhide) {
        if (--extra->hideRefCount == 0)
            dirty(HideReference);
    }
    recursiveRefFromEffectItem(-1);
}

void QQuickItemPrivate::setCulled(bool cull)
{
    if (cull == culled)
        return;

    culled = cull;
    if ((cull && ++extra.value().hideRefCount == 1) || (!cull && --extra.value().hideRefCount == 0))
        dirty(HideReference);
}

void QQuickItemPrivate::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &data)
{
    Q_Q(QQuickItem);
    switch (change) {
    case QQuickItem::ItemChildAddedChange: {
        q->itemChange(change, data);
        if (!subtreeTransformChangedEnabled)
            subtreeTransformChangedEnabled = true;
        notifyChangeListeners(QQuickItemPrivate::Children, &QQuickItemChangeListener::itemChildAdded, q, data.item);
        break;
    }
    case QQuickItem::ItemChildRemovedChange: {
        q->itemChange(change, data);
        notifyChangeListeners(QQuickItemPrivate::Children, &QQuickItemChangeListener::itemChildRemoved, q, data.item);
        break;
    }
    case QQuickItem::ItemSceneChange:
        q->itemChange(change, data);
        break;
    case QQuickItem::ItemVisibleHasChanged: {
        q->itemChange(change, data);
        notifyChangeListeners(QQuickItemPrivate::Visibility, &QQuickItemChangeListener::itemVisibilityChanged, q);
        break;
    }
    case QQuickItem::ItemEnabledHasChanged: {
        q->itemChange(change, data);
        notifyChangeListeners(QQuickItemPrivate::Enabled, &QQuickItemChangeListener::itemEnabledChanged, q);
        break;
    }
    case QQuickItem::ItemParentHasChanged: {
        q->itemChange(change, data);
        notifyChangeListeners(QQuickItemPrivate::Parent, &QQuickItemChangeListener::itemParentChanged, q, data.item);
        break;
    }
    case QQuickItem::ItemOpacityHasChanged: {
        q->itemChange(change, data);
        notifyChangeListeners(QQuickItemPrivate::Opacity, &QQuickItemChangeListener::itemOpacityChanged, q);
        break;
    }
    case QQuickItem::ItemActiveFocusHasChanged:
        q->itemChange(change, data);
        break;
    case QQuickItem::ItemRotationHasChanged: {
        q->itemChange(change, data);
        notifyChangeListeners(QQuickItemPrivate::Rotation, &QQuickItemChangeListener::itemRotationChanged, q);
        break;
    }
    case QQuickItem::ItemAntialiasingHasChanged:
        // fall through
    case QQuickItem::ItemDevicePixelRatioHasChanged:
        q->itemChange(change, data);
        break;
    }
}

/*!
    \qmlproperty bool QtQuick::Item::smooth

    Primarily used in image based items to decide if the item should use smooth
    sampling or not. Smooth sampling is performed using linear interpolation, while
    non-smooth is performed using nearest neighbor.

    In Qt Quick 2.0, this property has minimal impact on performance.

    By default, this property is set to \c true.
*/
/*!
    \property QQuickItem::smooth
    \brief Specifies whether the item is smoothed or not

    Primarily used in image based items to decide if the item should use smooth
    sampling or not. Smooth sampling is performed using linear interpolation, while
    non-smooth is performed using nearest neighbor.

    In Qt Quick 2.0, this property has minimal impact on performance.

    By default, this property is set to \c true.
*/
bool QQuickItem::smooth() const
{
    Q_D(const QQuickItem);
    return d->smooth;
}
void QQuickItem::setSmooth(bool smooth)
{
    Q_D(QQuickItem);
    if (d->smooth == smooth)
        return;

    d->smooth = smooth;
    d->dirty(QQuickItemPrivate::Smooth);

    emit smoothChanged(smooth);
}

/*!
    \qmlproperty bool QtQuick::Item::activeFocusOnTab

    This property holds whether the item wants to be in the tab focus
    chain. By default, this is set to \c false.

    The tab focus chain traverses elements by first visiting the
    parent, and then its children in the order they occur in the
    children property. Pressing the tab key on an item in the tab
    focus chain will move keyboard focus to the next item in the
    chain. Pressing BackTab (normally Shift+Tab) will move focus
    to the previous item.

    To set up a manual tab focus chain, see \l KeyNavigation. Tab
    key events used by Keys or KeyNavigation have precedence over
    focus chain behavior; ignore the events in other key handlers
    to allow it to propagate.
*/
/*!
    \property QQuickItem::activeFocusOnTab

    This property holds whether the item wants to be in the tab focus
    chain. By default, this is set to \c false.
*/
bool QQuickItem::activeFocusOnTab() const
{
    Q_D(const QQuickItem);
    return d->activeFocusOnTab;
}
void QQuickItem::setActiveFocusOnTab(bool activeFocusOnTab)
{
    Q_D(QQuickItem);
    if (d->activeFocusOnTab == activeFocusOnTab)
        return;

    if (window()) {
        if ((this == window()->activeFocusItem()) && this != window()->contentItem() && !activeFocusOnTab) {
            qWarning("QQuickItem: Cannot set activeFocusOnTab to false once item is the active focus item.");
            return;
        }
    }

    d->activeFocusOnTab = activeFocusOnTab;

    emit activeFocusOnTabChanged(activeFocusOnTab);
}

/*!
    \qmlproperty bool QtQuick::Item::antialiasing

    Used by visual elements to decide if the item should use antialiasing or not.
    In some cases items with antialiasing require more memory and are potentially
    slower to render (see \l {Antialiasing} for more details).

    The default is false, but may be overridden by derived elements.
*/
/*!
    \property QQuickItem::antialiasing
    \brief Specifies whether the item is antialiased or not

    Used by visual elements to decide if the item should use antialiasing or not.
    In some cases items with antialiasing require more memory and are potentially
    slower to render (see \l {Antialiasing} for more details).

    The default is false, but may be overridden by derived elements.
*/
bool QQuickItem::antialiasing() const
{
    Q_D(const QQuickItem);
    return d->antialiasingValid ? d->antialiasing : d->implicitAntialiasing;
}

void QQuickItem::setAntialiasing(bool aa)
{
    Q_D(QQuickItem);

    if (!d->antialiasingValid) {
        d->antialiasingValid = true;
        d->antialiasing = d->implicitAntialiasing;
    }

    if (aa == d->antialiasing)
        return;

    d->antialiasing = aa;
    d->dirty(QQuickItemPrivate::Antialiasing);

    d->itemChange(ItemAntialiasingHasChanged, bool(d->antialiasing));

    emit antialiasingChanged(antialiasing());
}

void QQuickItem::resetAntialiasing()
{
    Q_D(QQuickItem);
    if (!d->antialiasingValid)
        return;

    d->antialiasingValid = false;

    if (d->implicitAntialiasing != d->antialiasing)
        emit antialiasingChanged(antialiasing());
}

void QQuickItemPrivate::setImplicitAntialiasing(bool antialiasing)
{
    Q_Q(QQuickItem);
    bool prev = q->antialiasing();
    implicitAntialiasing = antialiasing;
    if (componentComplete && (q->antialiasing() != prev))
        emit q->antialiasingChanged(q->antialiasing());
}

/*!
    Returns the item flags for this item.

    \sa setFlag()
  */
QQuickItem::Flags QQuickItem::flags() const
{
    Q_D(const QQuickItem);
    return (QQuickItem::Flags)d->flags;
}

/*!
    Enables the specified \a flag for this item if \a enabled is true;
    if \a enabled is false, the flag is disabled.

    These provide various hints for the item; for example, the
    ItemClipsChildrenToShape flag indicates that all children of this
    item should be clipped to fit within the item area.
  */
void QQuickItem::setFlag(Flag flag, bool enabled)
{
    Q_D(QQuickItem);
    if (enabled)
        setFlags((Flags)(d->flags | (quint32)flag));
    else
        setFlags((Flags)(d->flags & ~(quint32)flag));

    // We don't return early if the flag did not change. That's useful in case
    // we need to intentionally trigger this parent-chain traversal again.
    if (enabled && flag == ItemObservesViewport) {
        QQuickItem *par = parentItem();
        while (par) {
            auto parPriv = QQuickItemPrivate::get(par);
            if (!parPriv->subtreeTransformChangedEnabled)
                qCDebug(lcVP) << "turned on transformChanged notification for subtree of" << par;
            parPriv->subtreeTransformChangedEnabled = true;
            par = par->parentItem();
        }
    }
}

/*!
    Enables the specified \a flags for this item.

    \sa setFlag()
  */
void QQuickItem::setFlags(Flags flags)
{
    Q_D(QQuickItem);

    if (int(flags & ItemIsFocusScope) != int(d->flags & ItemIsFocusScope)) {
        if (flags & ItemIsFocusScope && !d->childItems.isEmpty() && d->window) {
            qWarning("QQuickItem: Cannot set FocusScope once item has children and is in a window.");
            flags &= ~ItemIsFocusScope;
        } else if (d->flags & ItemIsFocusScope) {
            qWarning("QQuickItem: Cannot unset FocusScope flag.");
            flags |= ItemIsFocusScope;
        }
    }

    if (int(flags & ItemClipsChildrenToShape) != int(d->flags & ItemClipsChildrenToShape))
        d->dirty(QQuickItemPrivate::Clip);

    d->flags = flags;
}

/*!
  \qmlproperty real QtQuick::Item::x
  \qmlproperty real QtQuick::Item::y
  \qmlproperty real QtQuick::Item::width
  \qmlproperty real QtQuick::Item::height

  Defines the item's position and size.
  The default value is \c 0.

  The (x,y) position is relative to the \l parent.

  \qml
  Item { x: 100; y: 100; width: 100; height: 100 }
  \endqml
 */
/*!
  \property QQuickItem::x

  Defines the item's x position relative to its parent.
  */
/*!
  \property QQuickItem::y

  Defines the item's y position relative to its parent.
  */
qreal QQuickItem::x() const
{
    Q_D(const QQuickItem);
    return d->x;
}

qreal QQuickItem::y() const
{
    Q_D(const QQuickItem);
    return d->y;
}

/*!
    \internal
  */
QPointF QQuickItem::position() const
{
    Q_D(const QQuickItem);
    return QPointF(d->x, d->y);
}

void QQuickItem::setX(qreal v)
{
    Q_D(QQuickItem);
    /* There are two ways in which this function might be called:
       a) Either directly by the user, or
       b) when a binding has evaluated to a new value and it writes
          the value back
       In the first case, we want to remove an existing binding, in
       the second case, we don't want to remove the binding which
       just wrote the value.
       removeBindingUnlessInWrapper takes care of this.
     */
    d->x.removeBindingUnlessInWrapper();
    if (qt_is_nan(v))
        return;

    const qreal oldx = d->x.valueBypassingBindings();
    if (oldx == v)
        return;

    d->x.setValueBypassingBindings(v);

    d->dirty(QQuickItemPrivate::Position);

    const qreal y = d->y.valueBypassingBindings();
    const qreal w = d->width.valueBypassingBindings();
    const qreal h = d->height.valueBypassingBindings();
    geometryChange(QRectF(v, y, w, h), QRectF(oldx, y, w, h));
}

void QQuickItem::setY(qreal v)
{
    Q_D(QQuickItem);
    d->y.removeBindingUnlessInWrapper();
    if (qt_is_nan(v))
        return;

    const qreal oldy = d->y.valueBypassingBindings();
    if (oldy == v)
        return;

    d->y.setValueBypassingBindings(v);

    d->dirty(QQuickItemPrivate::Position);

    // we use v instead of d->y, as that avoid a method call
    // and we have v anyway in scope
    const qreal x = d->x.valueBypassingBindings();
    const qreal w = d->width.valueBypassingBindings();
    const qreal h = d->height.valueBypassingBindings();
    geometryChange(QRectF(x, v, w, h), QRectF(x, oldy, w, h));
}

/*!
    \internal
  */
void QQuickItem::setPosition(const QPointF &pos)
{
    Q_D(QQuickItem);

    const qreal oldx = d->x.valueBypassingBindings();
    const qreal oldy = d->y.valueBypassingBindings();

    if (QPointF(oldx, oldy) == pos)
        return;

    /* This preserves the bindings, because that was what the code used to do
       The effect of this is that you can have
       Item {
            Rectangle {
                x: someValue; y: someValue
                DragHandler {}
            }
       }
       and you can move the rectangle around; once someValue changes, the position gets
       reset again (even when a drag is currently ongoing).
       Whether we want this is up to discussion.
    */

    d->x.setValueBypassingBindings(pos.x()); //TODO: investigate whether to break binding here or not
    d->y.setValueBypassingBindings(pos.y());

    d->dirty(QQuickItemPrivate::Position);

    const qreal w = d->width.valueBypassingBindings();
    const qreal h = d->height.valueBypassingBindings();
    geometryChange(QRectF(pos.x(), pos.y(), w, h), QRectF(oldx, oldy, w, h));
}

/* The bindable methods return an object which supports inspection (hasBinding) and
   modification (setBinding, removeBinding) of the properties bindable state.
*/
QBindable<qreal> QQuickItem::bindableX()
{
    return QBindable<qreal>(&d_func()->x);
}

QBindable<qreal> QQuickItem::bindableY()
{
    return QBindable<qreal>(&d_func()->y);
}

/*!
    \property QQuickItem::width

    This property holds the width of this item.
  */
qreal QQuickItem::width() const
{
    Q_D(const QQuickItem);
    return d->width;
}

void QQuickItem::setWidth(qreal w)
{
    Q_D(QQuickItem);
    d->width.removeBindingUnlessInWrapper();
    if (qt_is_nan(w))
        return;

    d->widthValidFlag = true;
    const qreal oldWidth = d->width.valueBypassingBindings();
    if (oldWidth == w)
        return;

    d->width.setValueBypassingBindings(w);

    d->dirty(QQuickItemPrivate::Size);

    const qreal x = d->x.valueBypassingBindings();
    const qreal y = d->y.valueBypassingBindings();
    const qreal h = d->height.valueBypassingBindings();
    geometryChange(QRectF(x, y, w, h), QRectF(x, y, oldWidth, h));
}

void QQuickItem::resetWidth()
{
    Q_D(QQuickItem);
    d->width.takeBinding();
    d->widthValidFlag = false;
    setImplicitWidth(implicitWidth());
}

void QQuickItemPrivate::implicitWidthChanged()
{
    Q_Q(QQuickItem);
    notifyChangeListeners(QQuickItemPrivate::ImplicitWidth, &QQuickItemChangeListener::itemImplicitWidthChanged, q);
    emit q->implicitWidthChanged();
}

qreal QQuickItemPrivate::getImplicitWidth() const
{
    return implicitWidth;
}
/*!
    Returns the width of the item that is implied by other properties that determine the content.
*/
qreal QQuickItem::implicitWidth() const
{
    Q_D(const QQuickItem);
    return d->getImplicitWidth();
}

QBindable<qreal> QQuickItem::bindableWidth()
{
    return QBindable<qreal>(&d_func()->width);
}

/*!
    \qmlproperty real QtQuick::Item::implicitWidth
    \qmlproperty real QtQuick::Item::implicitHeight

    Defines the preferred width or height of the Item.

    If \l width or \l height is not specified, an item's effective size will be
    determined by its \l implicitWidth or \l implicitHeight.

    However, if an item is the child of a \l {Qt Quick Layouts}{layout}, the
    layout will determine the item's preferred size using its implicit size.
    In such a scenario, the explicit \l width or \l height will be ignored.

    The default implicit size for most items is 0x0, however some items have an inherent
    implicit size which cannot be overridden, for example, \l [QML] Image and \l [QML] Text.

    Setting the implicit size is useful for defining components that have a preferred size
    based on their content, for example:

    \qml
    // Label.qml
    import QtQuick 2.0

    Item {
        property alias icon: image.source
        property alias label: text.text
        implicitWidth: text.implicitWidth + image.implicitWidth
        implicitHeight: Math.max(text.implicitHeight, image.implicitHeight)
        Image { id: image }
        Text {
            id: text
            wrapMode: Text.Wrap
            anchors.left: image.right; anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    \endqml

    \note Using implicitWidth of \l [QML] Text or \l [QML] TextEdit and setting the width explicitly
    incurs a performance penalty as the text must be laid out twice.
*/
/*!
    \property QQuickItem::implicitWidth
    \property QQuickItem::implicitHeight

    Defines the preferred width or height of the Item.

    If \l width or \l height is not specified, an item's effective size will be
    determined by its \l implicitWidth or \l implicitHeight.

    However, if an item is the child of a \l {Qt Quick Layouts}{layout}, the
    layout will determine the item's preferred size using its implicit size.
    In such a scenario, the explicit \l width or \l height will be ignored.

    The default implicit size for most items is 0x0, however some items have an inherent
    implicit size which cannot be overridden, for example, \l [QML] Image and \l [QML] Text.

    Setting the implicit size is useful for defining components that have a preferred size
    based on their content, for example:

    \qml
    // Label.qml
    import QtQuick 2.0

    Item {
        property alias icon: image.source
        property alias label: text.text
        implicitWidth: text.implicitWidth + image.implicitWidth
        implicitHeight: Math.max(text.implicitHeight, image.implicitHeight)
        Image { id: image }
        Text {
            id: text
            wrapMode: Text.Wrap
            anchors.left: image.right; anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    \endqml

    \note Using implicitWidth of \l [QML] Text or \l [QML] TextEdit and setting the width explicitly
    incurs a performance penalty as the text must be laid out twice.
*/
void QQuickItem::setImplicitWidth(qreal w)
{
    Q_D(QQuickItem);
    bool changed = w != d->implicitWidth;
    d->implicitWidth = w;
    // this uses valueBypassingBindings simply to avoid repeated "am I in a binding" checks
    if (d->width.valueBypassingBindings() == w || widthValid()) {
        if (changed)
            d->implicitWidthChanged();
        if (d->width.valueBypassingBindings() == w || widthValid())
            return;
        changed = false;
    }

    const qreal oldWidth = d->width.valueBypassingBindings();
    Q_ASSERT(!d->width.hasBinding() || QQmlPropertyBinding::isUndefined(d->width.binding()));
    // we need to keep the binding if its undefined (therefore we can't use operator=/setValue)
    d->width.setValueBypassingBindings(w);

    d->dirty(QQuickItemPrivate::Size);

    const qreal x = d->x.valueBypassingBindings();
    const qreal y = d->y.valueBypassingBindings();
    const qreal width = w;
    const qreal height = d->height.valueBypassingBindings();
    geometryChange(QRectF(x, y, width, height), QRectF(x, y, oldWidth, height));

    if (changed)
        d->implicitWidthChanged();
}

/*!
    Returns whether the width property has been set explicitly.
*/
bool QQuickItem::widthValid() const
{
    Q_D(const QQuickItem);
    /* Logic: The width is valid if we assigned a value
       or a binding to it. Note that a binding evaluation to
       undefined (and thus calling resetWidth) is detached [1];
       hasBinding will thus return false for it, which is
       what we want here, as resetting width should mean that
       width is invalid (until the binding evaluates to a
       non-undefined value again).

       [1]: A detached binding is a binding which is not set on a property.
       In the case of QQmlPropertyBinding and resettable properties, it
       still gets reevaluated when it was detached due to the binding
       returning undefined, and it gets re-attached, once the binding changes
       to a non-undefined value (unless another binding has beenset in the
       meantime).
       See QQmlPropertyBinding::isUndefined and handleUndefinedAssignment
    */

    return d->widthValid();
}

/*!
    \property QQuickItem::height

    This property holds the height of this item.
  */
qreal QQuickItem::height() const
{
    Q_D(const QQuickItem);
    return d->height;
}

void QQuickItem::setHeight(qreal h)
{
    Q_D(QQuickItem);
    // Note that we call removeUnlessInWrapper before returning in the
    // NaN and equal value cases; that ensures that an explicit setHeight
    // always removes the binding
    d->height.removeBindingUnlessInWrapper();
    if (qt_is_nan(h))
        return;

    d->heightValidFlag = true;
    const qreal oldHeight = d->height.valueBypassingBindings();
    if (oldHeight == h)
        return;

    d->height.setValueBypassingBindings(h);

    d->dirty(QQuickItemPrivate::Size);

    const qreal x = d->x.valueBypassingBindings();
    const qreal y = d->y.valueBypassingBindings();
    const qreal w = d->width.valueBypassingBindings();
    geometryChange(QRectF(x, y, w, h), QRectF(x, y, w, oldHeight));
}

void QQuickItem::resetHeight()
{
    Q_D(QQuickItem);
    // using takeBinding, we remove any existing binding from the
    // property, but preserve the existing value (and avoid some overhead
    // compared to calling setHeight(height())
    d->height.takeBinding();
    d->heightValidFlag = false;
    setImplicitHeight(implicitHeight());
}

void QQuickItemPrivate::implicitHeightChanged()
{
    Q_Q(QQuickItem);
    notifyChangeListeners(QQuickItemPrivate::ImplicitHeight, &QQuickItemChangeListener::itemImplicitHeightChanged, q);
    emit q->implicitHeightChanged();
}

qreal QQuickItemPrivate::getImplicitHeight() const
{
    return implicitHeight;
}

qreal QQuickItem::implicitHeight() const
{
    Q_D(const QQuickItem);
    return d->getImplicitHeight();
}

QBindable<qreal> QQuickItem::bindableHeight()
{
    return QBindable<qreal>(&d_func()->height);
}

void QQuickItem::setImplicitHeight(qreal h)
{
    Q_D(QQuickItem);
    bool changed = h != d->implicitHeight;
    d->implicitHeight = h;
    if (d->height.valueBypassingBindings() == h || heightValid()) {
        if (changed)
            d->implicitHeightChanged();
        if (d->height.valueBypassingBindings() == h || heightValid())
            return;
        changed = false;
    }

    const qreal oldHeight = d->height.valueBypassingBindings();
    Q_ASSERT(!d->height.hasBinding() || QQmlPropertyBinding::isUndefined(d->height.binding()));
    // we need to keep the binding if its undefined (therefore we can't use operator=/setValue)
    d->height.setValueBypassingBindings(h);

    d->dirty(QQuickItemPrivate::Size);

    const qreal x = d->x.valueBypassingBindings();
    const qreal y = d->y.valueBypassingBindings();
    const qreal width = d->width.valueBypassingBindings();
    const qreal height = d->height.valueBypassingBindings();
    geometryChange(QRectF(x, y, width, height),
                   QRectF(x, y, width, oldHeight));

    if (changed)
        d->implicitHeightChanged();
}

/*!
    \internal
  */
void QQuickItem::setImplicitSize(qreal w, qreal h)
{
    Q_D(QQuickItem);
    bool wChanged = w != d->implicitWidth;
    bool hChanged = h != d->implicitHeight;

    d->implicitWidth = w;
    d->implicitHeight = h;

    bool wDone = false;
    bool hDone = false;
    qreal width = d->width.valueBypassingBindings();
    qreal height = d->height.valueBypassingBindings();
    if (width == w || widthValid()) {
        if (wChanged)
            d->implicitWidthChanged();
        wDone = width == w || widthValid();
        wChanged = false;
    }
    if (height == h || heightValid()) {
        if (hChanged)
            d->implicitHeightChanged();
        hDone = height == h || heightValid();
        hChanged = false;
    }
    if (wDone && hDone)
        return;

    const qreal oldWidth = width;
    const qreal oldHeight = height;
    if (!wDone) {
        width = w;
        d->width.setValueBypassingBindings(w);
    }
    if (!hDone) {
        height = h;
        d->height.setValueBypassingBindings(h);
    }

    d->dirty(QQuickItemPrivate::Size);

    const qreal x = d->x.valueBypassingBindings();
    const qreal y = d->y.valueBypassingBindings();
    geometryChange(QRectF(x, y, width, height),
                   QRectF(x, y, oldWidth, oldHeight));

    if (!wDone && wChanged)
        d->implicitWidthChanged();
    if (!hDone && hChanged)
        d->implicitHeightChanged();
}

/*!
    Returns whether the height property has been set explicitly.
*/
bool QQuickItem::heightValid() const
{
    Q_D(const QQuickItem);
    return d->heightValid();
}

/*!
    \since 5.10

    Returns the size of the item.

    \sa setSize, width, height
 */

QSizeF QQuickItem::size() const
{
    Q_D(const QQuickItem);
    return QSizeF(d->width, d->height);
}


/*!
    \since 5.10

    Sets the size of the item to \a size.
    This methods preserves any existing binding on width and height;
    thus any change that triggers the binding to execute again will
    override the set values.

    \sa size, setWidth, setHeight
 */
void QQuickItem::setSize(const QSizeF &size)
{
    Q_D(QQuickItem);
    d->heightValidFlag = true;
    d->widthValidFlag = true;

    const qreal oldHeight = d->height.valueBypassingBindings();
    const qreal oldWidth = d->width.valueBypassingBindings();

    if (oldWidth == size.width() && oldHeight == size.height())
        return;

    d->height.setValueBypassingBindings(size.height());
    d->width.setValueBypassingBindings(size.width());

    d->dirty(QQuickItemPrivate::Size);

    const qreal x = d->x.valueBypassingBindings();
    const qreal y = d->y.valueBypassingBindings();
    geometryChange(QRectF(x, y, size.width(), size.height()), QRectF(x, y, oldWidth, oldHeight));
}

/*!
    \qmlproperty bool QtQuick::Item::activeFocus
    \readonly

    This read-only property indicates whether the item has active focus.

    If activeFocus is true, either this item is the one that currently
    receives keyboard input, or it is a FocusScope ancestor of the item
    that currently receives keyboard input.

    Usually, activeFocus is gained by setting \l focus on an item and its
    enclosing FocusScope objects. In the following example, the \c input
    and \c focusScope objects will have active focus, while the root
    rectangle object will not.

    \qml
    import QtQuick 2.0

    Rectangle {
        width: 100; height: 100

        FocusScope {
            id: focusScope
            focus: true

            TextInput {
                id: input
                focus: true
            }
        }
    }
    \endqml

    \sa focus, {Keyboard Focus in Qt Quick}
*/
/*!
    \property QQuickItem::activeFocus
    \readonly

    This read-only property indicates whether the item has active focus.

    If activeFocus is true, either this item is the one that currently
    receives keyboard input, or it is a FocusScope ancestor of the item
    that currently receives keyboard input.

    Usually, activeFocus is gained by setting \l focus on an item and its
    enclosing FocusScope objects. In the following example, the \c input
    and \c focusScope objects will have active focus, while the root
    rectangle object will not.

    \qml
    import QtQuick 2.0

    Rectangle {
        width: 100; height: 100

        FocusScope {
            focus: true

            TextInput {
                id: input
                focus: true
            }
        }
    }
    \endqml

    \sa focus, {Keyboard Focus in Qt Quick}
*/
bool QQuickItem::hasActiveFocus() const
{
    Q_D(const QQuickItem);
    return d->activeFocus;
}

/*!
    \qmlproperty bool QtQuick::Item::focus

    This property holds whether the item has focus within the enclosing
    FocusScope. If true, this item will gain active focus when the
    enclosing FocusScope gains active focus.

    In the following example, \c input will be given active focus when
    \c scope gains active focus:

    \qml
    import QtQuick 2.0

    Rectangle {
        width: 100; height: 100

        FocusScope {
            id: scope

            TextInput {
                id: input
                focus: true
            }
        }
    }
    \endqml

    For the purposes of this property, the scene as a whole is assumed
    to act like a focus scope. On a practical level, that means the
    following QML will give active focus to \c input on startup.

    \qml
    Rectangle {
        width: 100; height: 100

        TextInput {
              id: input
              focus: true
        }
    }
    \endqml

    \sa activeFocus, {Keyboard Focus in Qt Quick}
*/
/*!
    \property QQuickItem::focus

    This property holds whether the item has focus within the enclosing
    FocusScope. If true, this item will gain active focus when the
    enclosing FocusScope gains active focus.

    In the following example, \c input will be given active focus when
    \c scope gains active focus:

    \qml
    import QtQuick 2.0

    Rectangle {
        width: 100; height: 100

        FocusScope {
            id: scope

            TextInput {
                id: input
                focus: true
            }
        }
    }
    \endqml

    For the purposes of this property, the scene as a whole is assumed
    to act like a focus scope. On a practical level, that means the
    following QML will give active focus to \c input on startup.

    \qml
    Rectangle {
        width: 100; height: 100

        TextInput {
              id: input
              focus: true
        }
    }
    \endqml

    \sa activeFocus, {Keyboard Focus in Qt Quick}
*/
bool QQuickItem::hasFocus() const
{
    Q_D(const QQuickItem);
    return d->focus;
}

void QQuickItem::setFocus(bool focus)
{
    setFocus(focus, Qt::OtherFocusReason);
}

void QQuickItem::setFocus(bool focus, Qt::FocusReason reason)
{
    Q_D(QQuickItem);
    if (d->focus == focus)
        return;

    bool notifyListeners = false;
    if (d->window || d->parentItem) {
        // Need to find our nearest focus scope
        QQuickItem *scope = parentItem();
        while (scope && !scope->isFocusScope() && scope->parentItem())
            scope = scope->parentItem();
        if (d->window) {
            auto da = d->deliveryAgentPrivate();
            Q_ASSERT(da);
            if (focus)
                da->setFocusInScope(scope, this, reason);
            else
                da->clearFocusInScope(scope, this, reason);
        } else {
            // do the focus changes from setFocusInScope/clearFocusInScope that are
            // unrelated to a window
            QVarLengthArray<QQuickItem *, 20> changed;
            QQuickItem *oldSubFocusItem = QQuickItemPrivate::get(scope)->subFocusItem;
            if (oldSubFocusItem) {
                QQuickItemPrivate::get(oldSubFocusItem)->updateSubFocusItem(scope, false);
                QQuickItemPrivate::get(oldSubFocusItem)->focus = false;
                changed << oldSubFocusItem;
            } else if (!scope->isFocusScope() && scope->hasFocus()) {
                QQuickItemPrivate::get(scope)->focus = false;
                changed << scope;
            }
            d->updateSubFocusItem(scope, focus);

            d->focus = focus;
            changed << this;
            notifyListeners = true;
            emit focusChanged(focus);

            QQuickDeliveryAgentPrivate::notifyFocusChangesRecur(changed.data(), changed.size() - 1, reason);
        }
    } else {
        QVarLengthArray<QQuickItem *, 20> changed;
        QQuickItem *oldSubFocusItem = d->subFocusItem;
        if (!isFocusScope() && oldSubFocusItem) {
            QQuickItemPrivate::get(oldSubFocusItem)->updateSubFocusItem(this, false);
            QQuickItemPrivate::get(oldSubFocusItem)->focus = false;
            changed << oldSubFocusItem;
        }

        d->focus = focus;
        changed << this;
        notifyListeners = true;
        emit focusChanged(focus);

        QQuickDeliveryAgentPrivate::notifyFocusChangesRecur(changed.data(), changed.size() - 1, reason);
    }
    if (notifyListeners)
        d->notifyChangeListeners(QQuickItemPrivate::Focus, &QQuickItemChangeListener::itemFocusChanged, this, reason);
}

/*!
    Returns true if this item is a focus scope, and false otherwise.
  */
bool QQuickItem::isFocusScope() const
{
    return flags() & ItemIsFocusScope;
}

/*!
    If this item is a focus scope, this returns the item in its focus chain
    that currently has focus.

    Returns \nullptr if this item is not a focus scope.
  */
QQuickItem *QQuickItem::scopedFocusItem() const
{
    Q_D(const QQuickItem);
    if (!isFocusScope())
        return nullptr;
    else
        return d->subFocusItem;
}

/*!
    Returns \c true if this item is an ancestor of \a child (i.e., if this item
    is \a child's parent, or one of \a child's parent's ancestors).

    \since 5.7

    \sa parentItem()
  */
bool QQuickItem::isAncestorOf(const QQuickItem *child) const
{
    if (!child || child == this)
        return false;
    const QQuickItem *ancestor = child;
    while ((ancestor = ancestor->parentItem())) {
        if (ancestor == this)
            return true;
    }
    return false;
}

/*!
    Returns the mouse buttons accepted by this item.

    The default value is Qt::NoButton; that is, no mouse buttons are accepted.

    If an item does not accept the mouse button for a particular mouse event,
    the mouse event will not be delivered to the item and will be delivered
    to the next item in the item hierarchy instead.

    \sa acceptTouchEvents()
*/
Qt::MouseButtons QQuickItem::acceptedMouseButtons() const
{
    Q_D(const QQuickItem);
    return d->acceptedMouseButtons();
}

/*!
    Sets the mouse buttons accepted by this item to \a buttons.

    \note In Qt 5, calling setAcceptedMouseButtons() implicitly caused
    an item to receive touch events as well as mouse events; but it was
    recommended to call setAcceptTouchEvents() to subscribe for them.
    In Qt 6, it is necessary to call setAcceptTouchEvents() to continue
    to receive them.
*/
void QQuickItem::setAcceptedMouseButtons(Qt::MouseButtons buttons)
{
    Q_D(QQuickItem);
    d->extra.setTag(d->extra.tag().setFlag(QQuickItemPrivate::LeftMouseButtonAccepted, buttons & Qt::LeftButton));

    buttons &= ~Qt::LeftButton;
    if (buttons || d->extra.isAllocated()) {
        d->extra.value().acceptedMouseButtonsWithoutHandlers = buttons;
        d->extra.value().acceptedMouseButtons = d->extra->pointerHandlers.isEmpty() ? buttons : Qt::AllButtons;
    }
}

/*!
    Returns whether pointer events intended for this item's children should be
    filtered through this item.

    If both this item and a child item have acceptTouchEvents() \c true, then
    when a touch interaction occurs, this item will filter the touch event.
    But if either this item or the child cannot handle touch events,
    childMouseEventFilter() will be called with a synthesized mouse event.

    \sa setFiltersChildMouseEvents(), childMouseEventFilter()
  */
bool QQuickItem::filtersChildMouseEvents() const
{
    Q_D(const QQuickItem);
    return d->filtersChildMouseEvents;
}

/*!
    Sets whether pointer events intended for this item's children should be
    filtered through this item.

    If \a filter is true, childMouseEventFilter() will be called when
    a pointer event is triggered for a child item.

    \sa filtersChildMouseEvents()
  */
void QQuickItem::setFiltersChildMouseEvents(bool filter)
{
    Q_D(QQuickItem);
    d->filtersChildMouseEvents = filter;
}

/*!
    \internal
  */
bool QQuickItem::isUnderMouse() const
{
    Q_D(const QQuickItem);
    if (!d->window)
        return false;

    // QQuickWindow handles QEvent::Leave to reset the lastMousePosition
    // FIXME: Using QPointF() as the reset value means an item will not be
    // under the mouse if the mouse is at 0,0 of the window.
    if (const_cast<QQuickItemPrivate *>(d)->deliveryAgentPrivate()->lastMousePosition == QPointF())
        return false;

    QPointF cursorPos = QGuiApplicationPrivate::lastCursorPosition;
    return contains(mapFromScene(d->window->mapFromGlobal(cursorPos)));
}

/*!
    Returns whether hover events are accepted by this item.

    The default value is false.

    If this is false, then the item will not receive any hover events through
    the hoverEnterEvent(), hoverMoveEvent() and hoverLeaveEvent() functions.
*/
bool QQuickItem::acceptHoverEvents() const
{
    Q_D(const QQuickItem);
    return d->hoverEnabled;
}

/*!
    If \a enabled is true, this sets the item to accept hover events;
    otherwise, hover events are not accepted by this item.

    \sa acceptHoverEvents()
*/
void QQuickItem::setAcceptHoverEvents(bool enabled)
{
    Q_D(QQuickItem);
    d->hoverEnabled = enabled;
    d->setHasHoverInChild(enabled);
    // The DA needs to resolve which items and handlers should now be hovered or unhovered.
    // Marking this item dirty ensures that flushFrameSynchronousEvents() will be called from the render loop,
    // even if this change is not in response to a mouse event and no item has already marked itself dirty.
    d->dirty(QQuickItemPrivate::Content);
}

/*!
    Returns whether touch events are accepted by this item.

    The default value is \c false.

    If this is \c false, then the item will not receive any touch events through
    the touchEvent() function.

    \since 5.10
*/
bool QQuickItem::acceptTouchEvents() const
{
    Q_D(const QQuickItem);
    return d->touchEnabled;
}

/*!
    If \a enabled is true, this sets the item to accept touch events;
    otherwise, touch events are not accepted by this item.

    \since 5.10

    \sa acceptTouchEvents()
*/
void QQuickItem::setAcceptTouchEvents(bool enabled)
{
    Q_D(QQuickItem);
    d->touchEnabled = enabled;
}

void QQuickItemPrivate::setHasCursorInChild(bool hc)
{
#if QT_CONFIG(cursor)
    Q_Q(QQuickItem);

    // if we're asked to turn it off (because of an unsetcursor call, or a node
    // removal) then we should make sure it's really ok to turn it off.
    if (!hc && subtreeCursorEnabled) {
        if (hasCursor)
            return; // nope! sorry, I have a cursor myself
        for (QQuickItem *otherChild : std::as_const(childItems)) {
            QQuickItemPrivate *otherChildPrivate = QQuickItemPrivate::get(otherChild);
            if (otherChildPrivate->subtreeCursorEnabled || otherChildPrivate->hasCursor)
                return; // nope! sorry, something else wants it kept on.
        }
    }

    subtreeCursorEnabled = hc;
    QQuickItem *parent = q->parentItem();
    if (parent) {
        QQuickItemPrivate *parentPrivate = QQuickItemPrivate::get(parent);
        parentPrivate->setHasCursorInChild(hc);
    }
#else
    Q_UNUSED(hc);
#endif
}

void QQuickItemPrivate::setHasHoverInChild(bool hasHover)
{
    Q_Q(QQuickItem);

    // if we're asked to turn it off (because of a setAcceptHoverEvents call, or a node
    // removal) then we should make sure it's really ok to turn it off.
    if (!hasHover && subtreeHoverEnabled) {
        if (hoverEnabled)
            return; // nope! sorry, I need hover myself
        if (hasEnabledHoverHandlers())
            return; // nope! sorry, this item has enabled HoverHandlers

        for (QQuickItem *otherChild : std::as_const(childItems)) {
            QQuickItemPrivate *otherChildPrivate = QQuickItemPrivate::get(otherChild);
            if (otherChildPrivate->subtreeHoverEnabled || otherChildPrivate->hoverEnabled)
                return; // nope! sorry, something else wants it kept on.
            if (otherChildPrivate->hasEnabledHoverHandlers())
                return; // nope! sorry, we have pointer handlers which are interested.
        }
    }

    qCDebug(lcHoverTrace) << q << subtreeHoverEnabled << "->" << hasHover;
    subtreeHoverEnabled = hasHover;
    QQuickItem *parent = q->parentItem();
    if (parent) {
        QQuickItemPrivate *parentPrivate = QQuickItemPrivate::get(parent);
        parentPrivate->setHasHoverInChild(hasHover);
    }
}

#if QT_CONFIG(cursor)

/*!
    Returns the cursor shape for this item.

    The mouse cursor will assume this shape when it is over this
    item, unless an override cursor is set.
    See the \l{Qt::CursorShape}{list of predefined cursor objects} for a
    range of useful shapes.

    If no cursor shape has been set this returns a cursor with the Qt::ArrowCursor shape, however
    another cursor shape may be displayed if an overlapping item has a valid cursor.

    \sa setCursor(), unsetCursor()
*/

QCursor QQuickItem::cursor() const
{
    Q_D(const QQuickItem);
    return d->extra.isAllocated()
            ? d->extra->cursor
            : QCursor();
}

/*!
    Sets the \a cursor shape for this item.

    \sa cursor(), unsetCursor()
*/

void QQuickItem::setCursor(const QCursor &cursor)
{
    Q_D(QQuickItem);

    Qt::CursorShape oldShape = d->extra.isAllocated() ? d->extra->cursor.shape() : Qt::ArrowCursor;
    qCDebug(lcHoverTrace) << oldShape << "->" << cursor.shape();

    if (oldShape != cursor.shape() || oldShape >= Qt::LastCursor || cursor.shape() >= Qt::LastCursor) {
        d->extra.value().cursor = cursor;
        if (d->window) {
            QWindow *renderWindow = QQuickRenderControl::renderWindowFor(d->window);
            QWindow *window = renderWindow ? renderWindow : d->window; // this may not be a QQuickWindow
            if (QQuickWindowPrivate::get(d->window)->cursorItem == this)
                window->setCursor(cursor);
        }
    }

    QPointF updateCursorPos;
    if (!d->hasCursor) {
        d->hasCursor = true;
        if (d->window) {
            QWindow *renderWindow = QQuickRenderControl::renderWindowFor(d->window);
            QWindow *window = renderWindow ? renderWindow : d->window;
            QPointF pos = window->mapFromGlobal(QGuiApplicationPrivate::lastCursorPosition);
            if (contains(mapFromScene(pos)))
                updateCursorPos = pos;
        }
    }
    d->setHasCursorInChild(d->hasCursor || d->hasCursorHandler);
    if (!updateCursorPos.isNull())
        QQuickWindowPrivate::get(d->window)->updateCursor(updateCursorPos);
}

/*!
    Clears the cursor shape for this item.

    \sa cursor(), setCursor()
*/

void QQuickItem::unsetCursor()
{
    Q_D(QQuickItem);
    qCDebug(lcHoverTrace) << "clearing cursor";
    if (!d->hasCursor)
        return;
    d->hasCursor = false;
    d->setHasCursorInChild(d->hasCursorHandler);
    if (d->extra.isAllocated())
        d->extra->cursor = QCursor();

    if (d->window) {
        QQuickWindowPrivate *windowPrivate = QQuickWindowPrivate::get(d->window);
        if (windowPrivate->cursorItem == this) {
            QPointF pos = d->window->mapFromGlobal(QGuiApplicationPrivate::lastCursorPosition);
            windowPrivate->updateCursor(pos);
        }
    }
}

/*!
    \internal
    Returns the cursor that should actually be shown, allowing the given
    \a handler to override the Item cursor if it is active or hovered.

    \sa cursor(), setCursor(), QtQuick::PointerHandler::cursor
*/
QCursor QQuickItemPrivate::effectiveCursor(const QQuickPointerHandler *handler) const
{
    Q_Q(const QQuickItem);
    if (!handler)
        return q->cursor();
    bool hoverCursorSet = false;
    QCursor hoverCursor;
    bool activeCursorSet = false;
    QCursor activeCursor;
    if (const QQuickHoverHandler *hoverHandler = qobject_cast<const QQuickHoverHandler *>(handler)) {
        hoverCursorSet = hoverHandler->isCursorShapeExplicitlySet();
        hoverCursor = hoverHandler->cursorShape();
    } else if (handler->active()) {
        activeCursorSet = handler->isCursorShapeExplicitlySet();
        activeCursor = handler->cursorShape();
    }
    if (activeCursorSet)
        return activeCursor;
    if (hoverCursorSet)
        return hoverCursor;
    return q->cursor();
}

/*!
    \internal
    Returns the Pointer Handler that is currently attempting to set the cursor shape,
    or null if there is no such handler.

    If there are multiple handlers attempting to set the cursor:
    \list
    \li an active handler has the highest priority (e.g. a DragHandler being dragged)
    \li any HoverHandler that is reacting to a non-mouse device has priority for
        kCursorOverrideTimeout ms (a tablet stylus is jittery so that's enough)
    \li otherwise a HoverHandler that is reacting to the mouse, if any
    \endlist

    Within each category, if there are multiple handlers, the last-added one wins
    (the one that is declared at the bottom wins, because users may intuitively
    think it's "on top" even though there is no Z-order; or, one that is added
    in a specific use case overrides an imported component).

    \sa QtQuick::PointerHandler::cursor
*/
QQuickPointerHandler *QQuickItemPrivate::effectiveCursorHandler() const
{
    if (!hasPointerHandlers())
        return nullptr;
    QQuickPointerHandler* activeHandler = nullptr;
    QQuickPointerHandler* mouseHandler = nullptr;
    QQuickPointerHandler* nonMouseHandler = nullptr;
    for (QQuickPointerHandler *h : extra->pointerHandlers) {
        if (!h->isCursorShapeExplicitlySet())
            continue;
        QQuickHoverHandler *hoverHandler = qmlobject_cast<QQuickHoverHandler *>(h);
        // Prioritize any HoverHandler that is reacting to a non-mouse device.
        // Otherwise, choose the first hovered handler that is found.
        // TODO maybe: there was an idea to add QPointerDevice* as argument to this function
        // and check the device type, but why? HoverHandler already does that.
        if (!activeHandler && hoverHandler && hoverHandler->isHovered()) {
            qCDebug(lcHoverTrace) << hoverHandler << hoverHandler->acceptedDevices() << "wants to set cursor" << hoverHandler->cursorShape();
            if (hoverHandler->acceptedDevices().testFlag(QPointingDevice::DeviceType::Mouse)) {
                // If there's a conflict, the last-added HoverHandler wins.  Maybe the user is overriding a default...
                if (mouseHandler && mouseHandler->cursorShape() != hoverHandler->cursorShape()) {
                    qCDebug(lcHoverTrace) << "mouse cursor conflict:" << mouseHandler << "wants" << mouseHandler->cursorShape()
                                          << "but" << hoverHandler << "wants" << hoverHandler->cursorShape();
                }
                mouseHandler = hoverHandler;
            } else {
                // If there's a conflict, the last-added HoverHandler wins.
                if (nonMouseHandler && nonMouseHandler->cursorShape() != hoverHandler->cursorShape()) {
                    qCDebug(lcHoverTrace) << "non-mouse cursor conflict:" << nonMouseHandler << "wants" << nonMouseHandler->cursorShape()
                                          << "but" << hoverHandler << "wants" << hoverHandler->cursorShape();
                }
                nonMouseHandler = hoverHandler;
            }
        }
        if (!hoverHandler && h->active())
            activeHandler = h;
    }
    if (activeHandler) {
        qCDebug(lcHoverTrace) << "active handler choosing cursor" << activeHandler << activeHandler->cursorShape();
        return activeHandler;
    }
    // Mouse events are often synthetic; so if a HoverHandler for a non-mouse device wanted to set the cursor,
    // let it win, unless more than kCursorOverrideTimeout ms have passed
    // since the last time the non-mouse handler actually reacted to an event.
    // We could miss the fact that a tablet stylus has left proximity, because we don't deliver proximity events to windows.
    if (nonMouseHandler) {
        if (mouseHandler) {
            const bool beforeTimeout =
                QQuickPointerHandlerPrivate::get(mouseHandler)->lastEventTime <
                QQuickPointerHandlerPrivate::get(nonMouseHandler)->lastEventTime + kCursorOverrideTimeout;
            QQuickPointerHandler *winner = (beforeTimeout ? nonMouseHandler : mouseHandler);
            qCDebug(lcHoverTrace) << "non-mouse handler reacted last time:" << QQuickPointerHandlerPrivate::get(nonMouseHandler)->lastEventTime
                                  << "and mouse handler reacted at time:" << QQuickPointerHandlerPrivate::get(mouseHandler)->lastEventTime
                                  << "choosing cursor according to" << winner << winner->cursorShape();
            return winner;
        }
        qCDebug(lcHoverTrace) << "non-mouse handler choosing cursor" << nonMouseHandler << nonMouseHandler->cursorShape();
        return nonMouseHandler;
    }
    if (mouseHandler)
        qCDebug(lcHoverTrace) << "mouse handler choosing cursor" << mouseHandler << mouseHandler->cursorShape();
    return mouseHandler;
}

#endif

/*!
    \deprecated Use QPointerEvent::setExclusiveGrabber().

    Grabs the mouse input.

    This item will receive all mouse events until ungrabMouse() is called.
    Usually this function should not be called, since accepting for example
    a mouse press event makes sure that the following events are delivered
    to the item.
    If an item wants to take over mouse events from the current receiver,
    it needs to call this function.

    \warning This function should be used with caution.
  */
void QQuickItem::grabMouse()
{
    Q_D(QQuickItem);
    if (!d->window)
        return;
    auto da = d->deliveryAgentPrivate();
    Q_ASSERT(da);
    auto eventInDelivery = da->eventInDelivery();
    if (!eventInDelivery) {
        qWarning() << "cannot grab mouse: no event is currently being delivered";
        return;
    }
    auto epd = da->mousePointData();
    eventInDelivery->setExclusiveGrabber(epd->eventPoint, this);
}

/*!
    \deprecated Use QPointerEvent::setExclusiveGrabber().

    Releases the mouse grab following a call to grabMouse().

    Note that this function should only be called when the item wants
    to stop handling further events. There is no need to call this function
    after a release or cancel event since no future events will be received
    in any case. No move or release events will be delivered after this
    function was called.
*/
void QQuickItem::ungrabMouse()
{
    Q_D(QQuickItem);
    if (!d->window)
        return;
    auto da = d->deliveryAgentPrivate();
    Q_ASSERT(da);
    auto eventInDelivery = da->eventInDelivery();
    if (!eventInDelivery) {
        // do it the expensive way
        da->removeGrabber(this);
        return;
    }
    const auto &eventPoint = da->mousePointData()->eventPoint;
    if (eventInDelivery->exclusiveGrabber(eventPoint) == this)
        eventInDelivery->setExclusiveGrabber(eventPoint, nullptr);
}

/*!
    Returns whether mouse input should exclusively remain with this item.

    \sa setKeepMouseGrab()
 */
bool QQuickItem::keepMouseGrab() const
{
    Q_D(const QQuickItem);
    return d->keepMouse;
}

/*!
  Sets whether the mouse input should remain exclusively with this item.

  This is useful for items that wish to grab and keep mouse
  interaction following a predefined gesture.  For example,
  an item that is interested in horizontal mouse movement
  may set keepMouseGrab to true once a threshold has been
  exceeded.  Once keepMouseGrab has been set to true, filtering
  items will not react to mouse events.

  If \a keep is false, a filtering item may steal the grab. For example,
  \l Flickable may attempt to steal a mouse grab if it detects that the
  user has begun to move the viewport.

  \sa keepMouseGrab()
 */
void QQuickItem::setKeepMouseGrab(bool keep)
{
    Q_D(QQuickItem);
    d->keepMouse = keep;
}

/*!
    \deprecated Use QPointerEvent::setExclusiveGrabber().
    Grabs the touch points specified by \a ids.

    These touch points will be owned by the item until
    they are released. Alternatively, the grab can be stolen
    by a filtering item like Flickable. Use setKeepTouchGrab()
    to prevent the grab from being stolen.
*/
void QQuickItem::grabTouchPoints(const QList<int> &ids)
{
    Q_D(QQuickItem);
    auto event = d->deliveryAgentPrivate()->eventInDelivery();
    if (Q_UNLIKELY(!event)) {
        qWarning() << "cannot grab: no event is currently being delivered";
        return;
    }
    for (auto pt : event->points()) {
        if (ids.contains(pt.id()))
            event->setExclusiveGrabber(pt, this);
    }
}

/*!
    \deprecated Use QEventPoint::setExclusiveGrabber() instead.
    Ungrabs the touch points owned by this item.
*/
void QQuickItem::ungrabTouchPoints()
{
    Q_D(QQuickItem);
    if (!d->window)
        return;
    d->deliveryAgentPrivate()->removeGrabber(this, false, true);
}

/*!
    Returns whether the touch points grabbed by this item should exclusively
    remain with this item.

    \sa setKeepTouchGrab(), keepMouseGrab()
*/
bool QQuickItem::keepTouchGrab() const
{
    Q_D(const QQuickItem);
    return d->keepTouch;
}

/*!
  Sets whether the touch points grabbed by this item should remain
  exclusively with this item.

  This is useful for items that wish to grab and keep specific touch
  points following a predefined gesture.  For example,
  an item that is interested in horizontal touch point movement
  may set setKeepTouchGrab to true once a threshold has been
  exceeded.  Once setKeepTouchGrab has been set to true, filtering
  items will not react to the relevant touch points.

  If \a keep is false, a filtering item may steal the grab. For example,
  \l Flickable may attempt to steal a touch point grab if it detects that the
  user has begun to move the viewport.

  \sa keepTouchGrab(), setKeepMouseGrab()
 */
void QQuickItem::setKeepTouchGrab(bool keep)
{
    Q_D(QQuickItem);
    d->keepTouch = keep;
}

/*!
    \qmlmethod bool QtQuick::Item::contains(point point)

    Returns \c true if this item contains \a point, which is in local coordinates;
    returns \c false otherwise.  This is the same check that is used for
    hit-testing a QEventPoint during event delivery, and is affected by
    \l containmentMask if it is set.
*/
/*!
    Returns \c true if this item contains \a point, which is in local coordinates;
    returns \c false otherwise.

    This function can be overridden in order to handle point collisions in items
    with custom shapes. The default implementation checks whether the point is inside
    \l containmentMask() if it is set, or inside the bounding box otherwise.

    \note This method is used for hit-testing each QEventPoint during event
    delivery, so the implementation should be kept as lightweight as possible.
*/
bool QQuickItem::contains(const QPointF &point) const
{
    Q_D(const QQuickItem);
    if (d->extra.isAllocated() && d->extra->mask) {
        if (auto quickMask = qobject_cast<QQuickItem *>(d->extra->mask))
            return quickMask->contains(point - quickMask->position());

        bool res = false;
        QMetaMethod maskContains = d->extra->mask->metaObject()->method(d->extra->maskContainsIndex);
        maskContains.invoke(d->extra->mask,
                      Qt::DirectConnection,
                      Q_RETURN_ARG(bool, res),
                      Q_ARG(QPointF, point));
        return res;
    }

    qreal x = point.x();
    qreal y = point.y();
    return x >= 0 && y >= 0 && x < d->width && y < d->height;
}

/*!
    \qmlproperty QObject* QtQuick::Item::containmentMask
    \since 5.11
    This property holds an optional mask for the Item to be used in the
    \l contains() method. Its main use is currently to determine
    whether a \l {QPointerEvent}{pointer event} has landed into the item or not.

    By default the \c contains() method will return true for any point
    within the Item's bounding box. \c containmentMask allows for
    more fine-grained control. For example, if a custom C++
    QQuickItem subclass with a specialized contains() method
    is used as containmentMask:

    \code
    Item { id: item; containmentMask: AnotherItem { id: anotherItem } }
    \endcode

    \e{item}'s contains method would then return \c true only if
    \e{anotherItem}'s contains() implementation returns \c true.

    A \l Shape can be used as a mask, to make an item react to
    \l {QPointerEvent}{pointer events} only within a non-rectangular region:

    \table
    \row
    \li \image containmentMask-shape.gif
    \li \snippet qml/item/containmentMask-shape.qml 0
    \endtable

    It is also possible to define the contains method in QML. For example,
    to create a circular item that only responds to events within its
    actual bounds:

    \table
    \row
    \li \image containmentMask-circle.gif
    \li \snippet qml/item/containmentMask-circle-js.qml 0
    \endtable

    \sa {Qt Quick Examples - Shapes}
*/
/*!
    \property QQuickItem::containmentMask
    \since 5.11
    This property holds an optional mask to be used in the contains() method,
    which is mainly used for hit-testing each \l QPointerEvent.

    By default, \l contains() will return \c true for any point
    within the Item's bounding box. But any QQuickItem, or any QObject
    that implements a function of the form
    \code
    Q_INVOKABLE bool contains(const QPointF &point) const;
    \endcode
    can be used as a mask, to defer hit-testing to that object.

    \note contains() is called frequently during event delivery.
    Deferring hit-testing to another object slows it down somewhat.
    containmentMask() can cause performance problems if that object's
    contains() method is not efficient. If you implement a custom
    QQuickItem subclass, you can alternatively override contains().

    \sa contains()
*/
QObject *QQuickItem::containmentMask() const
{
    Q_D(const QQuickItem);
    if (!d->extra.isAllocated())
        return nullptr;
    return d->extra->mask.data();
}

void QQuickItem::setContainmentMask(QObject *mask)
{
    Q_D(QQuickItem);
    const bool extraDataExists = d->extra.isAllocated();
    // an Item can't mask itself (to prevent infinite loop in contains())
    if (mask == static_cast<QObject *>(this))
        return;
    // mask is null, and we had no mask
    if (!extraDataExists && !mask)
        return;
    // mask is non-null and the same
    if (extraDataExists && d->extra->mask == mask)
        return;

    QQuickItem *quickMask = d->extra.isAllocated() ? qobject_cast<QQuickItem *>(d->extra->mask)
                                                   : nullptr;
    if (quickMask) {
        QQuickItemPrivate *maskPrivate = QQuickItemPrivate::get(quickMask);
        maskPrivate->registerAsContainmentMask(this, false); // removed from use as my mask
    }

    if (!extraDataExists)
        d->extra.value(); // ensure extra exists
    if (mask) {
        int methodIndex = mask->metaObject()->indexOfMethod(QByteArrayLiteral("contains(QPointF)"));
        if (methodIndex < 0) {
            qmlWarning(this) << QStringLiteral("QQuickItem: Object set as mask does not have an invokable contains method, ignoring it.");
            return;
        }
        d->extra->maskContainsIndex = methodIndex;
    }
    d->extra->mask = mask;
    quickMask = qobject_cast<QQuickItem *>(mask);
    if (quickMask) {
        QQuickItemPrivate *maskPrivate = QQuickItemPrivate::get(quickMask);
        maskPrivate->registerAsContainmentMask(this, true); // telling maskPrivate that "this" is using it as mask
    }
    emit containmentMaskChanged();
}

/*!
    Maps the given \a point in this item's coordinate system to the equivalent
    point within \a item's coordinate system, and returns the mapped
    coordinate.

    \input item.qdocinc mapping

    If \a item is \nullptr, this maps \a point to the coordinate system of the
    scene.

    \sa {Concepts - Visual Coordinates in Qt Quick}
*/
QPointF QQuickItem::mapToItem(const QQuickItem *item, const QPointF &point) const
{
    QPointF p = mapToScene(point);
    if (item)
        p = item->mapFromScene(p);
    return p;
}

/*!
    Maps the given \a point in this item's coordinate system to the equivalent
    point within the scene's coordinate system, and returns the mapped
    coordinate.

    \input item.qdocinc mapping

    \sa {Concepts - Visual Coordinates in Qt Quick}
*/
QPointF QQuickItem::mapToScene(const QPointF &point) const
{
    Q_D(const QQuickItem);
    return d->itemToWindowTransform().map(point);
}

/*!
    Maps the given \a point in this item's coordinate system to the equivalent
    point within global screen coordinate system, and returns the mapped
    coordinate.

    \input item.qdocinc mapping

    For example, this may be helpful to add a popup to a Qt Quick component.

    \note Window positioning is done by the window manager and this value is
    treated only as a hint. So, the resulting window position may differ from
    what is expected.

    \since 5.7

    \sa {Concepts - Visual Coordinates in Qt Quick}
*/
QPointF QQuickItem::mapToGlobal(const QPointF &point) const
{
    Q_D(const QQuickItem);
    return d->windowToGlobalTransform().map(mapToScene(point));
}

/*!
    Maps the given \a rect in this item's coordinate system to the equivalent
    rectangular area within \a item's coordinate system, and returns the mapped
    rectangle value.

    \input item.qdocinc mapping

    If \a item is \nullptr, this maps \a rect to the coordinate system of the
    scene.

    \sa {Concepts - Visual Coordinates in Qt Quick}
*/
QRectF QQuickItem::mapRectToItem(const QQuickItem *item, const QRectF &rect) const
{
    Q_D(const QQuickItem);
    QTransform t = d->itemToWindowTransform();
    if (item)
        t *= QQuickItemPrivate::get(item)->windowToItemTransform();
    return t.mapRect(rect);
}

/*!
    Maps the given \a rect in this item's coordinate system to the equivalent
    rectangular area within the scene's coordinate system, and returns the mapped
    rectangle value.

    \input item.qdocinc mapping

    \sa {Concepts - Visual Coordinates in Qt Quick}
*/
QRectF QQuickItem::mapRectToScene(const QRectF &rect) const
{
    Q_D(const QQuickItem);
    return d->itemToWindowTransform().mapRect(rect);
}

/*!
    Maps the given \a point in \a item's coordinate system to the equivalent
    point within this item's coordinate system, and returns the mapped
    coordinate.

    \input item.qdocinc mapping

    If \a item is \nullptr, this maps \a point from the coordinate system of the
    scene.

    \sa {Concepts - Visual Coordinates in Qt Quick}
*/
QPointF QQuickItem::mapFromItem(const QQuickItem *item, const QPointF &point) const
{
    QPointF p = item?item->mapToScene(point):point;
    return mapFromScene(p);
}

/*!
    Maps the given \a point in the scene's coordinate system to the equivalent
    point within this item's coordinate system, and returns the mapped
    coordinate.

    \input item.qdocinc mapping

    \sa {Concepts - Visual Coordinates in Qt Quick}
*/
QPointF QQuickItem::mapFromScene(const QPointF &point) const
{
    Q_D(const QQuickItem);
    return d->windowToItemTransform().map(point);
}

/*!
    Maps the given \a point in the global screen coordinate system to the
    equivalent point within this item's coordinate system, and returns the
    mapped coordinate.

    \input item.qdocinc mapping

    For example, this may be helpful to add a popup to a Qt Quick component.

    \note Window positioning is done by the window manager and this value is
    treated only as a hint. So, the resulting window position may differ from
    what is expected.

    \note If this item is in a subscene, e.g. mapped onto a 3D
    \l [QtQuick3D QML] {Model}{Model} object, the UV mapping is incorporated
    into this transformation, so that it really goes from screen coordinates to
    this item's coordinates, as long as \a point is actually within this item's bounds.
    The other mapping functions do not yet work that way.

    \since 5.7

    \sa {Concepts - Visual Coordinates in Qt Quick}
*/
QPointF QQuickItem::mapFromGlobal(const QPointF &point) const
{
    Q_D(const QQuickItem);
    QPointF scenePoint = d->globalToWindowTransform().map(point);
    if (auto da = QQuickDeliveryAgentPrivate::currentOrItemDeliveryAgent(this)) {
        if (auto sceneTransform = da->sceneTransform())
            scenePoint = sceneTransform->map(scenePoint);
    }
    return mapFromScene(scenePoint);
}

/*!
    Maps the given \a rect in \a item's coordinate system to the equivalent
    rectangular area within this item's coordinate system, and returns the mapped
    rectangle value.

    \input item.qdocinc mapping

    If \a item is \nullptr, this maps \a rect from the coordinate system of the
    scene.

    \sa {Concepts - Visual Coordinates in Qt Quick}
*/
QRectF QQuickItem::mapRectFromItem(const QQuickItem *item, const QRectF &rect) const
{
    Q_D(const QQuickItem);
    QTransform t = item?QQuickItemPrivate::get(item)->itemToWindowTransform():QTransform();
    t *= d->windowToItemTransform();
    return t.mapRect(rect);
}

/*!
    Maps the given \a rect in the scene's coordinate system to the equivalent
    rectangular area within this item's coordinate system, and returns the mapped
    rectangle value.

    \input item.qdocinc mapping

    \sa {Concepts - Visual Coordinates in Qt Quick}
*/
QRectF QQuickItem::mapRectFromScene(const QRectF &rect) const
{
    Q_D(const QQuickItem);
    return d->windowToItemTransform().mapRect(rect);
}

/*!
  \property QQuickItem::anchors
  \internal
*/

/*!
  \property QQuickItem::left
  \internal
*/

/*!
  \property QQuickItem::right
  \internal
*/

/*!
  \property QQuickItem::horizontalCenter
  \internal
*/

/*!
  \property QQuickItem::top
  \internal
*/

/*!
  \property QQuickItem::bottom
  \internal
*/

/*!
  \property QQuickItem::verticalCenter
  \internal
*/

/*!
  \property QQuickItem::baseline
  \internal
*/

/*!
  \property QQuickItem::data
  \internal
*/

/*!
  \property QQuickItem::resources
  \internal
*/

/*!
  \reimp
  */
bool QQuickItem::event(QEvent *ev)
{
    Q_D(QQuickItem);

    switch (ev->type()) {
#if QT_CONFIG(im)
    case QEvent::InputMethodQuery: {
        QInputMethodQueryEvent *query = static_cast<QInputMethodQueryEvent *>(ev);
        Qt::InputMethodQueries queries = query->queries();
        for (uint i = 0; i < 32; ++i) {
            Qt::InputMethodQuery q = (Qt::InputMethodQuery)(int)(queries & (1<<i));
            if (q) {
                QVariant v = inputMethodQuery(q);
                query->setValue(q, v);
            }
        }
        query->accept();
        break;
    }
    case QEvent::InputMethod:
        inputMethodEvent(static_cast<QInputMethodEvent *>(ev));
        break;
#endif // im
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
        touchEvent(static_cast<QTouchEvent*>(ev));
        break;
    case QEvent::StyleAnimationUpdate:
        if (isVisible()) {
            ev->accept();
            update();
        }
        break;
    case QEvent::HoverEnter:
        hoverEnterEvent(static_cast<QHoverEvent*>(ev));
        break;
    case QEvent::HoverLeave:
        hoverLeaveEvent(static_cast<QHoverEvent*>(ev));
        break;
    case QEvent::HoverMove:
        hoverMoveEvent(static_cast<QHoverEvent*>(ev));
        break;
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
        d->deliverKeyEvent(static_cast<QKeyEvent*>(ev));
        break;
    case QEvent::ShortcutOverride:
        d->deliverShortcutOverrideEvent(static_cast<QKeyEvent*>(ev));
        break;
    case QEvent::FocusIn:
        focusInEvent(static_cast<QFocusEvent*>(ev));
        break;
    case QEvent::FocusOut:
        focusOutEvent(static_cast<QFocusEvent*>(ev));
        break;
    case QEvent::MouseMove:
        mouseMoveEvent(static_cast<QMouseEvent*>(ev));
        break;
    case QEvent::MouseButtonPress:
        mousePressEvent(static_cast<QMouseEvent*>(ev));
        break;
    case QEvent::MouseButtonRelease:
        mouseReleaseEvent(static_cast<QMouseEvent*>(ev));
        break;
    case QEvent::MouseButtonDblClick:
        mouseDoubleClickEvent(static_cast<QMouseEvent*>(ev));
        break;
#if QT_CONFIG(wheelevent)
    case QEvent::Wheel:
        wheelEvent(static_cast<QWheelEvent*>(ev));
        break;
#endif
#if QT_CONFIG(quick_draganddrop)
    case QEvent::DragEnter:
        dragEnterEvent(static_cast<QDragEnterEvent*>(ev));
        break;
    case QEvent::DragLeave:
        dragLeaveEvent(static_cast<QDragLeaveEvent*>(ev));
        break;
    case QEvent::DragMove:
        dragMoveEvent(static_cast<QDragMoveEvent*>(ev));
        break;
    case QEvent::Drop:
        dropEvent(static_cast<QDropEvent*>(ev));
        break;
#endif // quick_draganddrop
#if QT_CONFIG(gestures)
    case QEvent::NativeGesture:
        ev->ignore();
        break;
#endif // gestures
    case QEvent::LanguageChange:
    case QEvent::LocaleChange:
        for (QQuickItem *item : std::as_const(d->childItems))
            QCoreApplication::sendEvent(item, ev);
        break;
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
        if (d->providesPalette())
            d->setCurrentColorGroup();
        for (QQuickItem *item : std::as_const(d->childItems))
            QCoreApplication::sendEvent(item, ev);
        break;
    case QEvent::ApplicationPaletteChange:
        for (QQuickItem *item : std::as_const(d->childItems))
            QCoreApplication::sendEvent(item, ev);
        break;
    default:
        return QObject::event(ev);
    }

    return true;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug,
#if QT_VERSION >= QT_VERSION_CHECK(7, 0, 0)
                  const
#endif
                  QQuickItem *item)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    if (!item) {
        debug << "QQuickItem(nullptr)";
        return debug;
    }

    const QRectF rect(item->position(), QSizeF(item->width(), item->height()));

    debug << item->metaObject()->className() << '(' << static_cast<void *>(item);

    // Deferred properties will cause recursion when calling nameForObject
    // before the component is completed, so guard against this situation.
    if (item->isComponentComplete()) {
        if (QQmlContext *context = qmlContext(item)) {
            const auto objectId = context->nameForObject(item);
            if (!objectId.isEmpty())
                debug << ", id=" << objectId;
        }
    }
    if (!item->objectName().isEmpty())
        debug << ", name=" << item->objectName();
    debug << ", parent=" << static_cast<void *>(item->parentItem())
          << ", geometry=";
    QtDebugUtils::formatQRect(debug, rect);
    if (const qreal z = item->z())
        debug << ", z=" << z;
    if (item->flags().testFlag(QQuickItem::ItemIsViewport))
        debug << " \U0001f5bc"; // frame with picture
    if (item->flags().testFlag(QQuickItem::ItemObservesViewport))
        debug << " \u23ff"; // observer eye
    debug << ')';
    return debug;
}
#endif // QT_NO_DEBUG_STREAM

/*!
    \fn bool QQuickItem::isTextureProvider() const

    Returns true if this item is a texture provider. The default
    implementation returns false.

    This function can be called from any thread.
 */

bool QQuickItem::isTextureProvider() const
{
#if QT_CONFIG(quick_shadereffect)
    Q_D(const QQuickItem);
    return d->extra.isAllocated() && d->extra->layer && d->extra->layer->effectSource() ?
           d->extra->layer->effectSource()->isTextureProvider() : false;
#else
    return false;
#endif
}

/*!
    \fn QSGTextureProvider *QQuickItem::textureProvider() const

    Returns the texture provider for an item. The default implementation
    returns \nullptr.

    This function may only be called on the rendering thread.
 */

QSGTextureProvider *QQuickItem::textureProvider() const
{
#if QT_CONFIG(quick_shadereffect)
    Q_D(const QQuickItem);
    return d->extra.isAllocated() && d->extra->layer && d->extra->layer->effectSource() ?
           d->extra->layer->effectSource()->textureProvider() : nullptr;
#else
    return 0;
#endif
}

/*!
    \since 6.0
    \qmlproperty Palette QtQuick::Item::palette

    This property holds the palette currently set for the item.

    This property describes the item's requested palette. The palette is used by the item's style
    when rendering all controls, and is available as a means to ensure that custom controls can
    maintain consistency with the native platform's native look and feel. It's common that
    different platforms, or different styles, define different palettes for an application.

    The default palette depends on the system environment. ApplicationWindow maintains a
    system/theme palette which serves as a default for all controls. There may also be special
    palette defaults for certain types of controls. You can also set the default palette for
    controls by either:

    \list
    \li passing a custom palette to QGuiApplication::setPalette(), before loading any QML; or
    \li specifying the colors in the \l {Qt Quick Controls 2 Configuration File}
        {qtquickcontrols2.conf file}.
    \endlist

    Items propagate explicit palette properties from parents to children. If you change a specific
    property on a items's palette, that property propagates to all of the item's children,
    overriding any system defaults for that property.

    \code
    Item {
        palette {
            buttonText: "maroon"
            button: "lavender"
        }

        Button {
            text: "Click Me"
        }
    }
    \endcode

    \sa Window::palette, Popup::palette, ColorGroup, Palette, SystemPalette
*/

#if QT_CONFIG(quick_shadereffect)
/*!
    \property QQuickItem::layer
    \internal
  */
QQuickItemLayer *QQuickItemPrivate::layer() const
{
    if (!extra.isAllocated() || !extra->layer) {
        extra.value().layer = new QQuickItemLayer(const_cast<QQuickItem *>(q_func()));
        if (!componentComplete)
            extra->layer->classBegin();
    }
    return extra->layer;
}
#endif

/*!
    \internal
    Create a modified copy of the given \a event intended for delivery to this
    item, containing pointers to only the QEventPoint instances that are
    relevant to this item, and transforming their positions to this item's
    coordinate system.

    Returns an invalid event with type \l QEvent::None if all points are
    stationary; or there are no points inside the item; or none of the points
    were pressed inside, neither the item nor any of its handlers is grabbing
    any of them, and \a isFiltering is false.

    When \a isFiltering is true, it is assumed that the item cares about all
    points which are inside its bounds, because most filtering items need to
    monitor eventpoint movements until a drag threshold is exceeded or the
    requirements for a gesture to be recognized are met in some other way.
*/
void QQuickItemPrivate::localizedTouchEvent(const QTouchEvent *event, bool isFiltering, QMutableTouchEvent *localized)
{
    Q_Q(QQuickItem);
    QList<QEventPoint> touchPoints;
    QEventPoint::States eventStates;

    bool anyPressOrReleaseInside = false;
    bool anyGrabber = false;
    for (auto &p : event->points()) {
        if (p.isAccepted())
            continue;

        // include points where item is the grabber, or if any of its handlers is the grabber while some parent is filtering
        auto pointGrabber = event->exclusiveGrabber(p);
        bool isGrabber = (pointGrabber == q);
        if (!isGrabber && pointGrabber && isFiltering) {
            auto handlerGrabber = qmlobject_cast<QQuickPointerHandler *>(pointGrabber);
            if (handlerGrabber && handlerGrabber->parentItem() == q)
                isGrabber = true;
        }
        if (isGrabber)
            anyGrabber = true;

        // include points inside the bounds if no other item is the grabber or if the item is filtering
        const auto localPos = q->mapFromScene(p.scenePosition());
        bool isInside = q->contains(localPos);
        bool hasAnotherGrabber = pointGrabber && pointGrabber != q;
        // if there's no exclusive grabber, look for passive grabbers during filtering
        if (isFiltering && !pointGrabber) {
            auto pg = event->passiveGrabbers(p);
            if (!pg.isEmpty()) {
                // It seems unlikely to have multiple passive grabbers of one eventpoint with different grandparents.
                // So hopefully if we start from one passive grabber and go up the parent chain from there,
                // we will find any filtering parent items that exist.
                auto handler = qmlobject_cast<QQuickPointerHandler *>(pg.first());
                if (handler)
                    pointGrabber = handler->parentItem();
            }
        }

        // filtering: (childMouseEventFilter) include points that are grabbed by children of the target item
        bool grabberIsChild = false;
        auto parent = qobject_cast<QQuickItem*>(pointGrabber);
        while (isFiltering && parent) {
            if (parent == q) {
                grabberIsChild = true;
                break;
            }
            parent = parent->parentItem();
        }

        bool filterRelevant = isFiltering && grabberIsChild;
        if (!(isGrabber || (isInside && (!hasAnotherGrabber || isFiltering)) || filterRelevant))
            continue;
        if ((p.state() == QEventPoint::State::Pressed || p.state() == QEventPoint::State::Released) && isInside)
            anyPressOrReleaseInside = true;
        QEventPoint pCopy(p);
        eventStates |= p.state();
        if (p.state() == QEventPoint::State::Released)
            QMutableEventPoint::detach(pCopy);
        QMutableEventPoint::setPosition(pCopy, localPos);
        touchPoints.append(std::move(pCopy));
    }

    // Now touchPoints will have only points which are inside the item.
    // But if none of them were just pressed inside, and the item has no other reason to care, ignore them anyway.
    if (touchPoints.isEmpty() || (!anyPressOrReleaseInside && !anyGrabber && !isFiltering)) {
        *localized = QMutableTouchEvent(QEvent::None);
        return;
    }

    // if all points have the same state, set the event type accordingly
    QEvent::Type eventType = event->type();
    switch (eventStates) {
    case QEventPoint::State::Pressed:
        eventType = QEvent::TouchBegin;
        break;
    case QEventPoint::State::Released:
        eventType = QEvent::TouchEnd;
        break;
    default:
        eventType = QEvent::TouchUpdate;
        break;
    }

    QMutableTouchEvent ret(eventType, event->pointingDevice(), event->modifiers(), touchPoints);
    ret.setTarget(q);
    ret.setTimestamp(event->timestamp());
    ret.accept();
    *localized = ret;
}

bool QQuickItemPrivate::hasPointerHandlers() const
{
    return extra.isAllocated() && !extra->pointerHandlers.isEmpty();
}

bool QQuickItemPrivate::hasEnabledHoverHandlers() const
{
    if (!hasPointerHandlers())
        return false;
    for (QQuickPointerHandler *h : extra->pointerHandlers)
        if (auto *hh = qmlobject_cast<QQuickHoverHandler *>(h); hh && hh->enabled())
            return true;
    return false;
}

void QQuickItemPrivate::addPointerHandler(QQuickPointerHandler *h)
{
    Q_ASSERT(h);
    Q_Q(QQuickItem);
    // Accept all buttons, and leave filtering to pointerEvent() and/or user JS,
    // because there can be multiple handlers...
    extra.value().acceptedMouseButtons = Qt::AllButtons;
    auto &handlers = extra.value().pointerHandlers;
    if (!handlers.contains(h))
        handlers.prepend(h);
    auto &res = extra.value().resourcesList;
    if (!res.contains(h)) {
        res.append(h);
        QObject::connect(h, &QObject::destroyed, q, [this](QObject *o) {
            _q_resourceObjectDeleted(o);
        });
    }
}

void QQuickItemPrivate::removePointerHandler(QQuickPointerHandler *h)
{
    Q_ASSERT(h);
    Q_Q(QQuickItem);
    auto &handlers = extra.value().pointerHandlers;
    handlers.removeOne(h);
    auto &res = extra.value().resourcesList;
    res.removeOne(h);
    QObject::disconnect(h, &QObject::destroyed, q, nullptr);
    if (handlers.isEmpty())
        extra.value().acceptedMouseButtons = extra.value().acceptedMouseButtonsWithoutHandlers;
}

#if QT_CONFIG(quick_shadereffect)
QQuickItemLayer::QQuickItemLayer(QQuickItem *item)
    : m_item(item)
    , m_enabled(false)
    , m_mipmap(false)
    , m_smooth(false)
    , m_live(true)
    , m_componentComplete(true)
    , m_wrapMode(QQuickShaderEffectSource::ClampToEdge)
    , m_format(QQuickShaderEffectSource::RGBA8)
    , m_name("source")
    , m_effectComponent(nullptr)
    , m_effect(nullptr)
    , m_effectSource(nullptr)
    , m_textureMirroring(QQuickShaderEffectSource::MirrorVertically)
    , m_samples(0)
{
}

QQuickItemLayer::~QQuickItemLayer()
{
    delete m_effectSource;
    delete m_effect;
}

/*!
    \qmlproperty bool QtQuick::Item::layer.enabled

    Holds whether the item is layered or not. Layering is disabled by default.

    A layered item is rendered into an offscreen surface and cached until
    it is changed. Enabling layering for complex QML item hierarchies can
    sometimes be an optimization.

    None of the other layer properties have any effect when the layer
    is disabled.

    \sa {Item Layers}
 */
void QQuickItemLayer::setEnabled(bool e)
{
    if (e == m_enabled)
        return;
    m_enabled = e;
    if (m_componentComplete) {
        if (m_enabled)
            activate();
        else
            deactivate();
    }

    emit enabledChanged(e);
}

void QQuickItemLayer::classBegin()
{
    Q_ASSERT(!m_effectSource);
    Q_ASSERT(!m_effect);
    m_componentComplete = false;
}

void QQuickItemLayer::componentComplete()
{
    Q_ASSERT(!m_componentComplete);
    m_componentComplete = true;
    if (m_enabled)
        activate();
}

void QQuickItemLayer::activate()
{
    Q_ASSERT(!m_effectSource);
    m_effectSource = new QQuickShaderEffectSource();
    QQuickItemPrivate::get(m_effectSource)->setTransparentForPositioner(true);

    QQuickItem *parentItem = m_item->parentItem();
    if (parentItem) {
        m_effectSource->setParentItem(parentItem);
        m_effectSource->stackAfter(m_item);
    }

    m_effectSource->setSourceItem(m_item);
    m_effectSource->setHideSource(true);
    m_effectSource->setSmooth(m_smooth);
    m_effectSource->setLive(m_live);
    m_effectSource->setTextureSize(m_size);
    m_effectSource->setSourceRect(m_sourceRect);
    m_effectSource->setMipmap(m_mipmap);
    m_effectSource->setWrapMode(m_wrapMode);
    m_effectSource->setFormat(m_format);
    m_effectSource->setTextureMirroring(m_textureMirroring);
    m_effectSource->setSamples(m_samples);

    if (m_effectComponent)
        activateEffect();

    m_effectSource->setVisible(m_item->isVisible() && !m_effect);

    updateZ();
    updateGeometry();
    updateOpacity();
    updateMatrix();

    QQuickItemPrivate *id = QQuickItemPrivate::get(m_item);
    id->addItemChangeListener(this, QQuickItemPrivate::Geometry | QQuickItemPrivate::Opacity | QQuickItemPrivate::Parent | QQuickItemPrivate::Visibility | QQuickItemPrivate::SiblingOrder);
}

void QQuickItemLayer::deactivate()
{
    Q_ASSERT(m_effectSource);

    if (m_effectComponent)
        deactivateEffect();

    delete m_effectSource;
    m_effectSource = nullptr;

    QQuickItemPrivate *id = QQuickItemPrivate::get(m_item);
    id->removeItemChangeListener(this,  QQuickItemPrivate::Geometry | QQuickItemPrivate::Opacity | QQuickItemPrivate::Parent | QQuickItemPrivate::Visibility | QQuickItemPrivate::SiblingOrder);
}

void QQuickItemLayer::activateEffect()
{
    Q_ASSERT(m_effectSource);
    Q_ASSERT(m_effectComponent);
    Q_ASSERT(!m_effect);

    QObject *created = m_effectComponent->beginCreate(m_effectComponent->creationContext());
    m_effect = qobject_cast<QQuickItem *>(created);
    if (!m_effect) {
        qWarning("Item: layer.effect is not a QML Item.");
        m_effectComponent->completeCreate();
        delete created;
        return;
    }
    QQuickItem *parentItem = m_item->parentItem();
    if (parentItem) {
        m_effect->setParentItem(parentItem);
        m_effect->stackAfter(m_effectSource);
    }
    m_effect->setVisible(m_item->isVisible());
    m_effect->setProperty(m_name, QVariant::fromValue<QObject *>(m_effectSource));
    QQuickItemPrivate::get(m_effect)->setTransparentForPositioner(true);
    m_effectComponent->completeCreate();
}

void QQuickItemLayer::deactivateEffect()
{
    Q_ASSERT(m_effectSource);
    Q_ASSERT(m_effectComponent);

    delete m_effect;
    m_effect = nullptr;
}


/*!
    \qmlproperty Component QtQuick::Item::layer.effect

    Holds the effect that is applied to this layer.

    The effect is typically a \l ShaderEffect component, although any \l Item component can be
    assigned. The effect should have a source texture property with a name matching \l layer.samplerName.

    \sa layer.samplerName, {Item Layers}
 */

void QQuickItemLayer::setEffect(QQmlComponent *component)
{
    if (component == m_effectComponent)
        return;

    bool updateNeeded = false;
    if (m_effectSource && m_effectComponent) {
        deactivateEffect();
        updateNeeded = true;
    }

    m_effectComponent = component;

    if (m_effectSource && m_effectComponent) {
        activateEffect();
        updateNeeded = true;
    }

    if (updateNeeded) {
        updateZ();
        updateGeometry();
        updateOpacity();
        updateMatrix();
        m_effectSource->setVisible(m_item->isVisible() && !m_effect);
    }

    emit effectChanged(component);
}


/*!
    \qmlproperty bool QtQuick::Item::layer.mipmap

    If this property is true, mipmaps are generated for the texture.

    \note Some OpenGL ES 2 implementations do not support mipmapping of
    non-power-of-two textures.

    \sa {Item Layers}
 */

void QQuickItemLayer::setMipmap(bool mipmap)
{
    if (mipmap == m_mipmap)
        return;
    m_mipmap = mipmap;

    if (m_effectSource)
        m_effectSource->setMipmap(m_mipmap);

    emit mipmapChanged(mipmap);
}


/*!
    \qmlproperty enumeration QtQuick::Item::layer.format

    This property defines the format of the backing texture.
    Modifying this property makes most sense when the \a layer.effect is also
    specified.

    \value ShaderEffectSource.RGBA8
    \value ShaderEffectSource.RGBA16F
    \value ShaderEffectSource.RGBA32F
    \value ShaderEffectSource.Alpha     Starting with Qt 6.0, this value is not in use and has the same effect as \c RGBA8 in practice.
    \value ShaderEffectSource.RGB       Starting with Qt 6.0, this value is not in use and has the same effect as \c RGBA8 in practice.
    \value ShaderEffectSource.RGBA      Starting with Qt 6.0, this value is not in use and has the same effect as \c RGBA8 in practice.

    \sa {Item Layers}
 */

void QQuickItemLayer::setFormat(QQuickShaderEffectSource::Format f)
{
    if (f == m_format)
        return;
    m_format = f;

    if (m_effectSource)
        m_effectSource->setFormat(m_format);

    emit formatChanged(m_format);
}


/*!
    \qmlproperty rect QtQuick::Item::layer.sourceRect

    This property defines the rectangular area of the item that should be
    rendered into the texture. The source rectangle can be larger than
    the item itself. If the rectangle is null, which is the default,
    then the whole item is rendered to the texture.

    \sa {Item Layers}
 */

void QQuickItemLayer::setSourceRect(const QRectF &sourceRect)
{
    if (sourceRect == m_sourceRect)
        return;
    m_sourceRect = sourceRect;

    if (m_effectSource)
        m_effectSource->setSourceRect(m_sourceRect);

    emit sourceRectChanged(sourceRect);
}

/*!
    \qmlproperty bool QtQuick::Item::layer.smooth

    Holds whether the layer is smoothly transformed. When enabled, sampling the
    layer's texture is performed using \c linear interpolation, while
    non-smooth results in using the \c nearest filtering mode.

    By default, this property is set to \c false.

    \sa {Item Layers}
 */

void QQuickItemLayer::setSmooth(bool s)
{
    if (m_smooth == s)
        return;
    m_smooth = s;

    if (m_effectSource)
        m_effectSource->setSmooth(m_smooth);

    emit smoothChanged(s);
}

/*!
    \qmlproperty bool QtQuick::Item::layer.live
    \since 6.5

    When this property is true the layer texture is updated whenever the
    item updates. Otherwise it will always be a frozen image.

    By default, this property is set to \c true.

    \sa {Item Layers}
 */

void QQuickItemLayer::setLive(bool live)
{
    if (m_live == live)
        return;
    m_live = live;

    if (m_effectSource)
        m_effectSource->setLive(m_live);

    emit liveChanged(live);
}

/*!
    \qmlproperty size QtQuick::Item::layer.textureSize

    This property holds the requested pixel size of the layers texture. If it is empty,
    which is the default, the size of the item is used.

    \note Some platforms have a limit on how small framebuffer objects can be,
    which means the actual texture size might be larger than the requested
    size.

    \sa {Item Layers}
 */

void QQuickItemLayer::setSize(const QSize &size)
{
    if (size == m_size)
        return;
    m_size = size;

    if (m_effectSource)
        m_effectSource->setTextureSize(size);

    emit sizeChanged(size);
}

/*!
    \qmlproperty enumeration QtQuick::Item::layer.wrapMode

    This property defines the wrap modes associated with the texture.
    Modifying this property makes most sense when the \a layer.effect is
    specified.

    \value ShaderEffectSource.ClampToEdge       GL_CLAMP_TO_EDGE both horizontally and vertically
    \value ShaderEffectSource.RepeatHorizontally GL_REPEAT horizontally, GL_CLAMP_TO_EDGE vertically
    \value ShaderEffectSource.RepeatVertically  GL_CLAMP_TO_EDGE horizontally, GL_REPEAT vertically
    \value ShaderEffectSource.Repeat            GL_REPEAT both horizontally and vertically

    \note Some OpenGL ES 2 implementations do not support the GL_REPEAT
    wrap mode with non-power-of-two textures.

    \sa {Item Layers}
 */

void QQuickItemLayer::setWrapMode(QQuickShaderEffectSource::WrapMode mode)
{
    if (mode == m_wrapMode)
        return;
    m_wrapMode = mode;

    if (m_effectSource)
        m_effectSource->setWrapMode(m_wrapMode);

    emit wrapModeChanged(mode);
}

/*!
    \qmlproperty enumeration QtQuick::Item::layer.textureMirroring
    \since 5.6

    This property defines how the generated texture should be mirrored.
    The default value is \c{ShaderEffectSource.MirrorVertically}.
    Custom mirroring can be useful if the generated texture is directly accessed by custom shaders,
    such as those specified by ShaderEffect. If no effect is specified for the layered
    item, mirroring has no effect on the UI representation of the item.

    \value ShaderEffectSource.NoMirroring           No mirroring
    \value ShaderEffectSource.MirrorHorizontally    The generated texture is flipped along X-axis.
    \value ShaderEffectSource.MirrorVertically      The generated texture is flipped along Y-axis.
 */

void QQuickItemLayer::setTextureMirroring(QQuickShaderEffectSource::TextureMirroring mirroring)
{
    if (mirroring == m_textureMirroring)
        return;
    m_textureMirroring = mirroring;

    if (m_effectSource)
        m_effectSource->setTextureMirroring(m_textureMirroring);

    emit textureMirroringChanged(mirroring);
}

/*!
    \qmlproperty enumeration QtQuick::Item::layer.samples
    \since 5.10

    This property allows requesting multisampled rendering in the layer.

    By default multisampling is enabled whenever multisampling is
    enabled for the entire window, assuming the scenegraph renderer in
    use and the underlying graphics API supports this.

    By setting the value to 2, 4, etc. multisampled rendering can be requested
    for a part of the scene without enabling multisampling for the entire
    scene. This way multisampling is applied only to a given subtree, which can
    lead to significant performance gains since multisampling is not applied to
    other parts of the scene.

    \note Enabling multisampling can be potentially expensive regardless of the
    layer's size, as it incurs a hardware and driver dependent performance and
    memory cost.

    \note This property is only functional when support for multisample
    renderbuffers and framebuffer blits is available. Otherwise the value is
    silently ignored.
 */

void QQuickItemLayer::setSamples(int count)
{
    if (m_samples == count)
        return;

    m_samples = count;

    if (m_effectSource)
        m_effectSource->setSamples(m_samples);

    emit samplesChanged(count);
}

/*!
    \qmlproperty string QtQuick::Item::layer.samplerName

    Holds the name of the effect's source texture property.

    This value must match the name of the effect's source texture property
    so that the Item can pass the layer's offscreen surface to the effect correctly.

    \sa layer.effect, ShaderEffect, {Item Layers}
 */

void QQuickItemLayer::setName(const QByteArray &name) {
    if (m_name == name)
        return;
    if (m_effect) {
        m_effect->setProperty(m_name, QVariant());
        m_effect->setProperty(name, QVariant::fromValue<QObject *>(m_effectSource));
    }
    m_name = name;
    emit nameChanged(name);
}

void QQuickItemLayer::itemOpacityChanged(QQuickItem *item)
{
    Q_UNUSED(item);
    updateOpacity();
}

void QQuickItemLayer::itemGeometryChanged(QQuickItem *, QQuickGeometryChange, const QRectF &)
{
    updateGeometry();
}

void QQuickItemLayer::itemParentChanged(QQuickItem *item, QQuickItem *parent)
{
    Q_UNUSED(item);
    Q_ASSERT(item == m_item);
    Q_ASSERT(parent != m_effectSource);
    Q_ASSERT(parent == nullptr || parent != m_effect);

    m_effectSource->setParentItem(parent);
    if (parent)
        m_effectSource->stackAfter(m_item);

    if (m_effect) {
        m_effect->setParentItem(parent);
        if (parent)
            m_effect->stackAfter(m_effectSource);
    }
}

void QQuickItemLayer::itemSiblingOrderChanged(QQuickItem *)
{
    m_effectSource->stackAfter(m_item);
    if (m_effect)
        m_effect->stackAfter(m_effectSource);
}

void QQuickItemLayer::itemVisibilityChanged(QQuickItem *)
{
    QQuickItem *l = m_effect ? (QQuickItem *) m_effect : (QQuickItem *) m_effectSource;
    if (!l)
        return;
    l->setVisible(m_item->isVisible());
}

void QQuickItemLayer::updateZ()
{
    if (!m_componentComplete || !m_enabled)
        return;
    QQuickItem *l = m_effect ? (QQuickItem *) m_effect : (QQuickItem *) m_effectSource;
    if (!l)
        return;
    l->setZ(m_item->z());
}

void QQuickItemLayer::updateOpacity()
{
    QQuickItem *l = m_effect ? (QQuickItem *) m_effect : (QQuickItem *) m_effectSource;
    if (!l)
        return;
    l->setOpacity(m_item->opacity());
}

void QQuickItemLayer::updateGeometry()
{
    QQuickItem *l = m_effect ? (QQuickItem *) m_effect : (QQuickItem *) m_effectSource;
    if (!l)
        return;
    // Avoid calling QQuickImage::boundingRect() or other overrides
    // which may not be up-to-date at this time (QTBUG-104442, 104536)
    QRectF bounds = m_item->QQuickItem::boundingRect();
    l->setSize(bounds.size());
    l->setPosition(bounds.topLeft() + m_item->position());
}

void QQuickItemLayer::updateMatrix()
{
    // Called directly from transformChanged(), so needs some extra
    // checks.
    if (!m_componentComplete || !m_enabled)
        return;
    QQuickItem *l = m_effect ? (QQuickItem *) m_effect : (QQuickItem *) m_effectSource;
    if (!l)
        return;
    QQuickItemPrivate *ld = QQuickItemPrivate::get(l);
    l->setScale(m_item->scale());
    l->setRotation(m_item->rotation());
    ld->transforms = QQuickItemPrivate::get(m_item)->transforms;
    if (ld->origin() != QQuickItemPrivate::get(m_item)->origin())
        ld->extra.value().origin = QQuickItemPrivate::get(m_item)->origin();
    ld->dirty(QQuickItemPrivate::Transform);
}
#endif // quick_shadereffect

QQuickItemPrivate::ExtraData::ExtraData()
: z(0), scale(1), rotation(0), opacity(1),
  contents(nullptr), screenAttached(nullptr), layoutDirectionAttached(nullptr),
  enterKeyAttached(nullptr),
  keyHandler(nullptr),
#if QT_CONFIG(quick_shadereffect)
  layer(nullptr),
#endif
  effectRefCount(0), hideRefCount(0),
  recursiveEffectRefCount(0),
  opacityNode(nullptr), clipNode(nullptr), rootNode(nullptr),
  origin(QQuickItem::Center),
  transparentForPositioner(false)
{
}


#if QT_CONFIG(accessibility)
QAccessible::Role QQuickItemPrivate::effectiveAccessibleRole() const
{
    Q_Q(const QQuickItem);
    auto *attached = qmlAttachedPropertiesObject<QQuickAccessibleAttached>(q, false);
    auto role = QAccessible::NoRole;
    if (auto *accessibleAttached = qobject_cast<QQuickAccessibleAttached *>(attached))
        role = accessibleAttached->role();
    if (role == QAccessible::NoRole)
        role = accessibleRole();
    return role;
}

QAccessible::Role QQuickItemPrivate::accessibleRole() const
{
    return QAccessible::NoRole;
}
#endif

// helper code to let a visual parent mark its visual children for the garbage collector

namespace QV4 {
namespace Heap {
struct QQuickItemWrapper : public QObjectWrapper {
    static void markObjects(QV4::Heap::Base *that, QV4::MarkStack *markStack);
};
}
}

struct QQuickItemWrapper : public QV4::QObjectWrapper {
    V4_OBJECT2(QQuickItemWrapper, QV4::QObjectWrapper)
};

DEFINE_OBJECT_VTABLE(QQuickItemWrapper);

void QV4::Heap::QQuickItemWrapper::markObjects(QV4::Heap::Base *that, QV4::MarkStack *markStack)
{
    QObjectWrapper *This = static_cast<QObjectWrapper *>(that);
    if (QQuickItem *item = static_cast<QQuickItem*>(This->object())) {
        for (QQuickItem *child : std::as_const(QQuickItemPrivate::get(item)->childItems))
            QV4::QObjectWrapper::markWrapper(child, markStack);
    }
    QObjectWrapper::markObjects(that, markStack);
}

quint64 QQuickItemPrivate::_q_createJSWrapper(QV4::ExecutionEngine *engine)
{
    return (engine->memoryManager->allocate<QQuickItemWrapper>(q_func()))->asReturnedValue();
}

QDebug operator<<(QDebug debug, const QQuickItemPrivate::ChangeListener &listener)
{
   QDebugStateSaver stateSaver(debug);
   debug.nospace() << "ChangeListener listener=" << listener.listener << " types=" << listener.types;
   return debug;
}

//! \internal
QPointF QQuickItem::mapFromItem(const QQuickItem *item, qreal x, qreal y)
{ return mapFromItem(item, QPointF(x, y) ); }

//! \internal
QRectF QQuickItem::mapFromItem(const QQuickItem *item, const QRectF &rect) const
{ return mapRectFromItem(item, rect); }

//! \internal
QRectF QQuickItem::mapFromItem(const QQuickItem *item, qreal x, qreal y, qreal width, qreal height) const
{ return mapFromItem(item, QRectF(x, y, width, height)); }

//! \internal
QPointF QQuickItem::mapToItem(const QQuickItem *item, qreal x, qreal y)
{ return mapToItem(item, QPoint(x, y)); }

//! \internal
QRectF QQuickItem::mapToItem(const QQuickItem *item, const QRectF &rect) const
{ return mapRectToItem(item, rect); }

//! \internal
QRectF QQuickItem::mapToItem(const QQuickItem *item, qreal x, qreal y, qreal width, qreal height) const
{ return mapToItem(item, QRectF(x, y, width, height)); }

//! \internal
QPointF QQuickItem::mapToGlobal(qreal x, qreal y) const
{ return mapToGlobal(QPointF(x, y)); }

//! \internal
QPointF QQuickItem::mapFromGlobal(qreal x, qreal y) const
{ return mapFromGlobal(QPointF(x, y)); }

//! \internal
QQuickItemChangeListener::~QQuickItemChangeListener() = default;

QT_END_NAMESPACE

#include <moc_qquickitem.cpp>

#include "moc_qquickitem_p.cpp"
