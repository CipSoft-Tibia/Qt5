// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qhelpenginecore.h"
#include "qhelpfilterengine.h"
#include "qhelpsearchindexreader_default_p.h"

#include <QtCore/QSet>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

QT_BEGIN_NAMESPACE

namespace fulltextsearch {
namespace qt {

void Reader::setIndexPath(const QString &path)
{
    m_indexPath = path;
    m_namespaceAttributes.clear();
    m_filterEngineNamespaceList.clear();
    m_useFilterEngine = false;
}

void Reader::addNamespaceAttributes(const QString &namespaceName, const QStringList &attributes)
{
    m_namespaceAttributes.insert(namespaceName, attributes);
}

void Reader::setFilterEngineNamespaceList(const QStringList &namespaceList)
{
    m_useFilterEngine = true;
    m_filterEngineNamespaceList = namespaceList;
}

static QString namespacePlaceholders(const QMultiMap<QString, QStringList> &namespaces)
{
    QString placeholders;
    const auto &namespaceList = namespaces.uniqueKeys();
    bool firstNS = true;
    for (const QString &ns : namespaceList) {
        if (firstNS)
            firstNS = false;
        else
            placeholders += QLatin1String(" OR ");
        placeholders += QLatin1String("(namespace = ?");

        const QList<QStringList> &attributeSets = namespaces.values(ns);
        bool firstAS = true;
        for (const QStringList &attributeSet : attributeSets) {
            if (!attributeSet.isEmpty()) {
                if (firstAS) {
                    firstAS = false;
                    placeholders += QLatin1String(" AND (");
                } else {
                    placeholders += QLatin1String(" OR ");
                }
                placeholders += QLatin1String("attributes = ?");
            }
        }
        if (!firstAS)
            placeholders += QLatin1Char(')'); // close "AND ("
        placeholders += QLatin1Char(')');
    }
    return placeholders;
}

static void bindNamespacesAndAttributes(QSqlQuery *query, const QMultiMap<QString, QStringList> &namespaces)
{
    const auto &namespaceList = namespaces.uniqueKeys();
    for (const QString &ns : namespaceList) {
        query->addBindValue(ns);

        const QList<QStringList> &attributeSets = namespaces.values(ns);
        for (const QStringList &attributeSet : attributeSets) {
            if (!attributeSet.isEmpty())
                query->addBindValue(attributeSet.join(QLatin1Char('|')));
        }
    }
}

static QString namespacePlaceholders(const QStringList &namespaceList)
{
    QString placeholders;
    bool firstNS = true;
    for (int i = namespaceList.size(); i; --i) {
        if (firstNS)
            firstNS = false;
        else
            placeholders += QLatin1String(" OR ");
        placeholders += QLatin1String("namespace = ?");
    }
    return placeholders;
}

static void bindNamespacesAndAttributes(QSqlQuery *query, const QStringList &namespaceList)
{
    for (const QString &ns : namespaceList)
        query->addBindValue(ns);
}

QList<QHelpSearchResult> Reader::queryTable(const QSqlDatabase &db,
                                         const QString &tableName,
                                         const QString &searchInput) const
{
    const QString nsPlaceholders = m_useFilterEngine
            ? namespacePlaceholders(m_filterEngineNamespaceList)
            : namespacePlaceholders(m_namespaceAttributes);
    QSqlQuery query(db);
    query.prepare(QLatin1String("SELECT url, title, snippet(") + tableName +
                  QLatin1String(", -1, '<b>', '</b>', '...', '10') FROM ") + tableName +
                  QLatin1String(" WHERE (") + nsPlaceholders +
                  QLatin1String(") AND ") + tableName +
                  QLatin1String(" MATCH ? ORDER BY rank"));
    m_useFilterEngine
            ? bindNamespacesAndAttributes(&query, m_filterEngineNamespaceList)
            : bindNamespacesAndAttributes(&query, m_namespaceAttributes);
    query.addBindValue(searchInput);
    query.exec();

    QList<QHelpSearchResult> results;

    while (query.next()) {
        const QString &url = query.value(QLatin1String("url")).toString();
        const QString &title = query.value(QLatin1String("title")).toString();
        const QString &snippet = query.value(2).toString();
        results.append(QHelpSearchResult(url, title, snippet));
    }

    return results;
}

void Reader::searchInDB(const QString &searchInput)
{
    const QString &uniqueId = QHelpGlobal::uniquifyConnectionName(QLatin1String("QHelpReader"), this);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QLatin1String("QSQLITE"), uniqueId);
        db.setConnectOptions(QLatin1String("QSQLITE_OPEN_READONLY"));
        db.setDatabaseName(m_indexPath + QLatin1String("/fts"));

        if (db.open()) {
            const QList<QHelpSearchResult> titleResults = queryTable(db,
                                             QLatin1String("titles"), searchInput);
            const QList<QHelpSearchResult> contentResults = queryTable(db,
                                             QLatin1String("contents"), searchInput);

            // merge results form title and contents searches
            m_searchResults = QList<QHelpSearchResult>();

            QSet<QUrl> urls;

            for (const QHelpSearchResult &result : titleResults) {
                const QUrl &url = result.url();
                if (!urls.contains(url)) {
                    urls.insert(url);
                    m_searchResults.append(result);
                }
            }

            for (const QHelpSearchResult &result : contentResults) {
                const QUrl &url = result.url();
                if (!urls.contains(url)) {
                    urls.insert(url);
                    m_searchResults.append(result);
                }
            }
        }
    }
    QSqlDatabase::removeDatabase(uniqueId);
}

QList<QHelpSearchResult> Reader::searchResults() const
{
    return m_searchResults;
}

static bool attributesMatchFilter(const QStringList &attributes,
                                  const QStringList &filter)
{
    for (const QString &attribute : filter) {
        if (!attributes.contains(attribute, Qt::CaseInsensitive))
            return false;
    }

    return true;
}

void QHelpSearchIndexReaderDefault::run()
{
    QMutexLocker lock(&m_mutex);

    if (m_cancel)
        return;

    const QString searchInput = m_searchInput;
    const QString collectionFile = m_collectionFile;
    const QString indexPath = m_indexFilesFolder;
    const bool usesFilterEngine = m_usesFilterEngine;

    lock.unlock();

    QHelpEngineCore engine(collectionFile, nullptr);
    if (!engine.setupData())
        return;

    emit searchingStarted();

    // setup the reader
    m_reader.setIndexPath(indexPath);

    if (usesFilterEngine) {
        m_reader.setFilterEngineNamespaceList(
                    engine.filterEngine()->namespacesForFilter(
                        engine.filterEngine()->activeFilter()));
    } else {
        const QStringList &registeredDocs = engine.registeredDocumentations();
        const QStringList &currentFilter = engine.filterAttributes(engine.currentFilter());

        for (const QString &namespaceName : registeredDocs) {
            const QList<QStringList> &attributeSets =
                    engine.filterAttributeSets(namespaceName);

            for (const QStringList &attributes : attributeSets) {
                if (attributesMatchFilter(attributes, currentFilter)) {
                    m_reader.addNamespaceAttributes(namespaceName, attributes);
                }
            }
        }
    }

    lock.relock();
    if (m_cancel) {
        emit searchingFinished(0);   // TODO: check this, speed issue while locking???
        return;
    }
    lock.unlock();

    m_searchResults.clear();
    m_reader.searchInDB(searchInput);    // TODO: should this be interruptible as well ???

    lock.relock();
    m_searchResults = m_reader.searchResults();
    lock.unlock();

    emit searchingFinished(m_searchResults.size());
}

}   // namespace std
}   // namespace fulltextsearch

QT_END_NAMESPACE
