// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickwindowinspector.h"
#include "inspecttool.h"

#include <private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

namespace QmlJSDebugger {

/*
 * Returns the first visible item at the given position, or 0 when no such
 * child exists.
 */
static QQuickItem *itemAt(QQuickItem *item, const QPointF &pos,
                          QQuickItem *overlay)
{
    if (item == overlay)
        return nullptr;

    if (!item->isVisible() || item->opacity() == 0.0)
        return nullptr;

    if (item->flags() & QQuickItem::ItemClipsChildrenToShape) {
        if (!QRectF(0, 0, item->width(), item->height()).contains(pos))
            return nullptr;
    }

    QList<QQuickItem *> children = QQuickItemPrivate::get(item)->paintOrderChildItems();
    for (int i = children.size() - 1; i >= 0; --i) {
        QQuickItem *child = children.at(i);
        if (QQuickItem *betterCandidate = itemAt(child, item->mapToItem(child, pos),
                                                 overlay))
            return betterCandidate;
    }

    if (!(item->flags() & QQuickItem::ItemHasContents))
        return nullptr;

    if (!QRectF(0, 0, item->width(), item->height()).contains(pos))
        return nullptr;

    return item;
}

/*
 * Collects all the items at the given position, from top to bottom.
 */
static void collectItemsAt(QQuickItem *item, const QPointF &pos,
                           QQuickItem *overlay, QList<QQuickItem *> &resultList)
{
    if (item == overlay)
        return;

    if (item->flags() & QQuickItem::ItemClipsChildrenToShape) {
        if (!QRectF(0, 0, item->width(), item->height()).contains(pos))
            return;
    }

    QList<QQuickItem *> children = QQuickItemPrivate::get(item)->paintOrderChildItems();
    for (int i = children.size() - 1; i >= 0; --i) {
        QQuickItem *child = children.at(i);
        collectItemsAt(child, item->mapToItem(child, pos), overlay, resultList);
    }

    if (!QRectF(0, 0, item->width(), item->height()).contains(pos))
        return;

    resultList.append(item);
}

QQuickWindowInspector::QQuickWindowInspector(QQuickWindow *quickWindow, QObject *parent) :
    QObject(parent),
    m_overlay(new QQuickItem),
    m_window(quickWindow),
    m_parentWindow(nullptr),
    m_tool(nullptr)
{
    setParentWindow(quickWindow);

    // Try to make sure the overlay is always on top
    m_overlay->setZ(FLT_MAX);

    if (QQuickItem *root = m_window->contentItem())
        m_overlay->setParentItem(root);

    m_window->installEventFilter(this);
}

bool QQuickWindowInspector::eventFilter(QObject *obj, QEvent *event)
{
    if (!m_tool || obj != m_window)
        return QObject::eventFilter(obj, event);

    switch (event->type()) {
    case QEvent::Enter:
        m_tool->enterEvent(static_cast<QEnterEvent*>(event));
        return true;
    case QEvent::Leave:
        m_tool->leaveEvent(event);
        return true;
    case QEvent::MouseButtonPress:
        m_tool->mousePressEvent(static_cast<QMouseEvent*>(event));
        return true;
    case QEvent::MouseMove:
        m_tool->mouseMoveEvent(static_cast<QMouseEvent*>(event));
        return true;
    case QEvent::MouseButtonRelease:
        return true;
    case QEvent::KeyPress:
        m_tool->keyPressEvent(static_cast<QKeyEvent*>(event));
        return true;
    case QEvent::KeyRelease:
        return true;
    case QEvent::MouseButtonDblClick:
        m_tool->mouseDoubleClickEvent(static_cast<QMouseEvent*>(event));
        return true;
#if QT_CONFIG(wheelevent)
    case QEvent::Wheel:
        return true;
#endif
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
        m_tool->touchEvent(static_cast<QTouchEvent*>(event));
        return true;
    default:
        break;
    }

    return QObject::eventFilter(obj, event);
}

static Qt::WindowFlags fixFlags(Qt::WindowFlags flags)
{
    // If only the type flag is given, some other window flags are automatically assumed. When we
    // add a flag, we need to make those explicit.
    switch (flags) {
    case Qt::Window:
        return flags | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint
                | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint;
    case Qt::Dialog:
    case Qt::Tool:
        return flags | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint;
    default:
        return flags;
    }
}

void QQuickWindowInspector::setShowAppOnTop(bool appOnTop)
{
    if (!m_parentWindow)
        return;

    Qt::WindowFlags flags = m_parentWindow->flags();
    Qt::WindowFlags newFlags = appOnTop ? (fixFlags(flags) | Qt::WindowStaysOnTopHint) :
                                          (flags & ~Qt::WindowStaysOnTopHint);
    if (newFlags != flags)
        m_parentWindow->setFlags(newFlags);
}

bool QQuickWindowInspector::isEnabled() const
{
    return m_tool != nullptr;
}

void QQuickWindowInspector::setEnabled(bool enabled)
{
    if (enabled) {
        m_tool = new InspectTool(this, m_window);
    } else {
        delete m_tool;
        m_tool = nullptr;
    }
}

QQuickWindow *QQuickWindowInspector::quickWindow() const
{
    return m_window;
}

void QQuickWindowInspector::setParentWindow(QWindow *parentWindow)
{
    if (parentWindow) {
        while (QWindow *w = parentWindow->parent())
            parentWindow = w;
    }

    m_parentWindow = parentWindow;
}

QList<QQuickItem *> QQuickWindowInspector::itemsAt(const QPointF &pos) const
{
    QList<QQuickItem *> resultList;
    QQuickItem *root = m_window->contentItem();
    collectItemsAt(root, root->mapFromScene(pos), m_overlay,
                   resultList);
    return resultList;
}

QQuickItem *QQuickWindowInspector::topVisibleItemAt(const QPointF &pos) const
{
    QQuickItem *root = m_window->contentItem();
    return itemAt(root, root->mapFromScene(pos), m_overlay);
}


} // namespace QmlJSDebugger

QT_END_NAMESPACE

#include "moc_qquickwindowinspector.cpp"
