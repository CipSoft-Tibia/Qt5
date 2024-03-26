// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bookmarkitem.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

BookmarkItem::BookmarkItem(const DataVector &data, BookmarkItem *parent)
    : m_data(data)
    , m_parent(parent)
{
}

BookmarkItem::~BookmarkItem()
{
    qDeleteAll(m_children);
}

BookmarkItem*
BookmarkItem::parent() const
{
    return m_parent;
}

void
BookmarkItem::setParent(BookmarkItem *parent)
{
    m_parent = parent;
}

void
BookmarkItem::addChild(BookmarkItem *child)
{
    child->setParent(this);
    m_children.append(child);
}

BookmarkItem*
BookmarkItem::child(int number) const
{
    if (number >= 0 && number < m_children.size())
        return m_children[number];
    return nullptr;
}

int BookmarkItem::childCount() const
{
    return m_children.size();
}

int BookmarkItem::childNumber() const
{
     if (m_parent)
         return m_parent->m_children.indexOf(const_cast<BookmarkItem*>(this));
     return 0;
}

QVariant
BookmarkItem::data(int column) const
{
    if (column == 0)
        return m_data[0];

    if (column == 1 || column == UserRoleUrl)
        return m_data[1];

    if (column == UserRoleFolder)
        return m_data[1].toString() == QLatin1String("Folder");

    if (column == UserRoleExpanded)
        return m_data[2];

    return QVariant();
}

void
BookmarkItem::setData(const DataVector &data)
{
    m_data = data;
}

bool
BookmarkItem::setData(int column, const QVariant &newValue)
{
    int index = -1;
    if (column == 0 || column == 1)
        index = column;

    if (column == UserRoleUrl || column == UserRoleFolder)
        index = 1;

    if (column == UserRoleExpanded)
        index = 2;

    if (index < 0)
        return false;

    m_data[index] = newValue;
    return true;
}

bool
BookmarkItem::insertChildren(bool isFolder, int position, int count)
{
    if (position < 0 || position > m_children.size())
        return false;

    for (int row = 0; row < count; ++row) {
        m_children.insert(position, new BookmarkItem(DataVector()
            << (isFolder
                ? QCoreApplication::translate("BookmarkItem", "New Folder")
                : QCoreApplication::translate("BookmarkItem", "Untitled"))
            << (isFolder ? "Folder" : "about:blank") << false, this));
    }

    return true;
}

bool
BookmarkItem::removeChildren(int position, int count)
{
    if (position < 0 || position > m_children.size())
        return false;

    for (int row = 0; row < count; ++row)
        delete m_children.takeAt(position);

    return true;
}

void
BookmarkItem::dumpTree(int indent) const
{
    const QString tree(indent, ' ');
    qDebug() << tree + (data(UserRoleFolder).toBool() ? "Folder" : "Bookmark")
        << "Label:" << data(0).toString() << "parent:" << m_parent << "this:"
        << this;

    for (BookmarkItem *item : m_children)
        item->dumpTree(indent + 4);
}

QT_END_NAMESPACE
