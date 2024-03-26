// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlpreviewblacklist.h"

QT_BEGIN_NAMESPACE

void QQmlPreviewBlacklist::blacklist(const QString &path)
{
    if (!path.isEmpty())
        m_root.insert(path, 0);
}

void QQmlPreviewBlacklist::whitelist(const QString &path)
{
    if (!path.isEmpty())
        m_root.remove(path, 0);
}

bool QQmlPreviewBlacklist::isBlacklisted(const QString &path) const
{
    return path.isEmpty() ? true : m_root.containedPrefixLeaf(path, 0) > 0;
}

void QQmlPreviewBlacklist::clear()
{
    m_root = Node();
}

QQmlPreviewBlacklist::Node::Node()
{
}

QQmlPreviewBlacklist::Node::Node(const QQmlPreviewBlacklist::Node &other) :
    m_mine(other.m_mine), m_isLeaf(other.m_isLeaf)
{
    for (auto it = other.m_next.begin(), end = other.m_next.end(); it != end; ++it)
        m_next.insert(it.key(), new Node(**it));
}

QQmlPreviewBlacklist::Node::Node(QQmlPreviewBlacklist::Node &&other) noexcept
{
    m_mine.swap(other.m_mine);
    m_next.swap(other.m_next);
    m_isLeaf = other.m_isLeaf;
}

QQmlPreviewBlacklist::Node::~Node()
{
    qDeleteAll(m_next);
}

QQmlPreviewBlacklist::Node &QQmlPreviewBlacklist::Node::operator=(
        const QQmlPreviewBlacklist::Node &other)
{
    if (&other != this) {
        m_mine = other.m_mine;
        for (auto it = other.m_next.begin(), end = other.m_next.end(); it != end; ++it)
            m_next.insert(it.key(), new Node(**it));
        m_isLeaf = other.m_isLeaf;
    }
    return *this;
}

QQmlPreviewBlacklist::Node &QQmlPreviewBlacklist::Node::operator=(
        QQmlPreviewBlacklist::Node &&other) noexcept
{
    if (&other != this) {
        m_mine.swap(other.m_mine);
        m_next.swap(other.m_next);
        m_isLeaf = other.m_isLeaf;
    }
    return *this;
}

void QQmlPreviewBlacklist::Node::split(QString::iterator it, QString::iterator end)
{
    QString existing;
    existing.resize(end - it - 1);
    std::copy(it + 1, end, existing.begin());

    Node *node = new Node(existing, m_next, m_isLeaf);
    m_next.clear();
    m_next.insert(*it, node);
    m_mine.resize(it - m_mine.begin());
    m_isLeaf = false;
}

void QQmlPreviewBlacklist::Node::insert(const QString &path, int offset)
{
    for (auto it = m_mine.begin(), end = m_mine.end(); it != end; ++it) {
        if (offset == path.size()) {
            split(it, end);
            m_isLeaf = true;
            return;
        }

        if (path.at(offset) != *it) {
            split(it, end);

            QString inserted;
            inserted.resize(path.size() - offset - 1);
            std::copy(path.begin() + offset + 1, path.end(), inserted.begin());
            m_next.insert(path.at(offset), new Node(inserted));
            return;
        }

        ++offset;
    }

    if (offset == path.size()) {
        m_isLeaf = true;
        return;
    }

    Node *&node = m_next[path.at(offset++)];
    if (node == nullptr) {
        QString inserted;
        inserted.resize(path.size() - offset);
        std::copy(path.begin() + offset, path.end(), inserted.begin());
        node = new Node(inserted);
    } else {
        node->insert(path, offset);
    }
}

void QQmlPreviewBlacklist::Node::remove(const QString &path, int offset)
{
    for (auto it = m_mine.begin(), end = m_mine.end(); it != end; ++it) {
        if (offset == path.size() || path.at(offset) != *it) {
            split(it, end);
            return;
        }
        ++offset;
    }

    m_isLeaf = false;
    if (offset == path.size())
        return;

    auto it = m_next.find(path.at(offset));
    if (it != m_next.end())
        (*it)->remove(path, ++offset);
}

int QQmlPreviewBlacklist::Node::containedPrefixLeaf(const QString &path, int offset) const
{
    if (offset == path.size())
        return (m_mine.isEmpty() && m_isLeaf) ? offset : -1;

    for (auto it = m_mine.begin(), end = m_mine.end(); it != end; ++it) {
        if (path.at(offset) != *it)
            return -1;

        if (++offset == path.size())
            return (++it == end && m_isLeaf) ? offset : -1;
    }

    const QChar c = path.at(offset);
    if (m_isLeaf && c == '/')
        return offset;

    auto it = m_next.find(c);
    if (it == m_next.end())
        return -1;

    return (*it)->containedPrefixLeaf(path, ++offset);
}

QQmlPreviewBlacklist::Node::Node(const QString &mine,
                                 const QHash<QChar, QQmlPreviewBlacklist::Node *> &next,
                                 bool isLeaf)
    : m_mine(mine), m_next(next), m_isLeaf(isLeaf)
{
}

QT_END_NAMESPACE
