// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qhelpsearchindexwriter_default_p.h"
#include "qhelp_global.h"
#include "qhelpenginecore.h"
#include "qhelpdbreader_p.h"

#include <QtCore/QDataStream>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QStringDecoder>
#include <QtCore/QTextStream>
#include <QtCore/QSet>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlDriver>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include <QTextDocument>

QT_BEGIN_NAMESPACE

namespace fulltextsearch {
namespace qt {

const char FTS_DB_NAME[] = "fts";

Writer::Writer(const QString &path)
    : m_dbDir(path)
{
    clearLegacyIndex();
    QDir().mkpath(m_dbDir);
    m_uniqueId = QHelpGlobal::uniquifyConnectionName(QLatin1String("QHelpWriter"), this);
    m_db = new QSqlDatabase();
    *m_db = QSqlDatabase::addDatabase(QLatin1String("QSQLITE"), m_uniqueId);
    const QString dbPath = m_dbDir + QLatin1Char('/') + QLatin1String(FTS_DB_NAME);
    m_db->setDatabaseName(dbPath);
    if (!m_db->open()) {
        const QString &error = QHelpSearchIndexWriter::tr("Cannot open database \"%1\" using connection \"%2\": %3")
                .arg(dbPath, m_uniqueId, m_db->lastError().text());
        qWarning("%s", qUtf8Printable(error));
        delete m_db;
        m_db = nullptr;
        QSqlDatabase::removeDatabase(m_uniqueId);
        m_uniqueId = QString();
    } else {
        startTransaction();
    }
}

bool Writer::tryInit(bool reindex)
{
    if (!m_db)
        return true;

    QSqlQuery query(*m_db);
    // HACK: we try to perform any modifying command just to check if
    // we don't get SQLITE_BUSY code (SQLITE_BUSY is defined to 5 in sqlite driver)
    if (!query.exec(QLatin1String("CREATE TABLE foo ();"))) {
        if (query.lastError().nativeErrorCode() == QLatin1String("5")) // db is locked
            return false;
    }
    // HACK: clear what we have created
    query.exec(QLatin1String("DROP TABLE foo;"));

    init(reindex);
    return true;
}

bool Writer::hasDB()
{
    if (!m_db)
        return false;

    QSqlQuery query(*m_db);

    query.prepare(QLatin1String("SELECT id FROM info LIMIT 1"));
    query.exec();

    return query.next();
}

void Writer::clearLegacyIndex()
{
    // Clear old legacy clucene index.
    // More important in case of Creator, since
    // the index folder is common for all Creator versions
    QDir dir(m_dbDir);
    if (!dir.exists())
        return;

    const QStringList &list = dir.entryList(QDir::Files | QDir::Hidden);
    if (!list.contains(QLatin1String(FTS_DB_NAME))) {
        for (const QString &item : list)
            dir.remove(item);
    }
}

void Writer::init(bool reindex)
{
    if (!m_db)
        return;

    QSqlQuery query(*m_db);

    if (reindex && hasDB()) {
        m_needOptimize = true;

        query.exec(QLatin1String("DROP TABLE titles;"));
        query.exec(QLatin1String("DROP TABLE contents;"));
        query.exec(QLatin1String("DROP TABLE info;"));
    }

    query.exec(QLatin1String("CREATE TABLE info (id INTEGER PRIMARY KEY, namespace, attributes, url, title, data);"));

    query.exec(QLatin1String("CREATE VIRTUAL TABLE titles USING fts5("
                             "namespace UNINDEXED, attributes UNINDEXED, "
                             "url UNINDEXED, title, "
                             "tokenize = 'porter unicode61', content = 'info', content_rowid='id');"));
    query.exec(QLatin1String("CREATE TRIGGER titles_insert AFTER INSERT ON info BEGIN "
                             "INSERT INTO titles(rowid, namespace, attributes, url, title) "
                             "VALUES(new.id, new.namespace, new.attributes, new.url, new.title); "
                             "END;"));
    query.exec(QLatin1String("CREATE TRIGGER titles_delete AFTER DELETE ON info BEGIN "
                             "INSERT INTO titles(titles, rowid, namespace, attributes, url, title) "
                             "VALUES('delete', old.id, old.namespace, old.attributes, old.url, old.title); "
                             "END;"));
    query.exec(QLatin1String("CREATE TRIGGER titles_update AFTER UPDATE ON info BEGIN "
                             "INSERT INTO titles(titles, rowid, namespace, attributes, url, title) "
                             "VALUES('delete', old.id, old.namespace, old.attributes, old.url, old.title); "
                             "INSERT INTO titles(rowid, namespace, attributes, url, title) "
                             "VALUES(new.id, new.namespace, new.attributes, new.url, new.title); "
                             "END;"));

    query.exec(QLatin1String("CREATE VIRTUAL TABLE contents USING fts5("
                             "namespace UNINDEXED, attributes UNINDEXED, "
                             "url UNINDEXED, title, data, "
                             "tokenize = 'porter unicode61', content = 'info', content_rowid='id');"));
    query.exec(QLatin1String("CREATE TRIGGER contents_insert AFTER INSERT ON info BEGIN "
                             "INSERT INTO contents(rowid, namespace, attributes, url, title, data) "
                             "VALUES(new.id, new.namespace, new.attributes, new.url, new.title, new.data); "
                             "END;"));
    query.exec(QLatin1String("CREATE TRIGGER contents_delete AFTER DELETE ON info BEGIN "
                             "INSERT INTO contents(contents, rowid, namespace, attributes, url, title, data) "
                             "VALUES('delete', old.id, old.namespace, old.attributes, old.url, old.title, old.data); "
                             "END;"));
    query.exec(QLatin1String("CREATE TRIGGER contents_update AFTER UPDATE ON info BEGIN "
                             "INSERT INTO contents(contents, rowid, namespace, attributes, url, title, data) "
                             "VALUES('delete', old.id, old.namespace, old.attributes, old.url, old.title, old.data); "
                             "INSERT INTO contents(rowid, namespace, attributes, url, title, data) "
                             "VALUES(new.id, new.namespace, new.attributes, new.url, new.title, new.data); "
                             "END;"));
}

Writer::~Writer()
{
    if (m_db) {
        m_db->close();
        delete m_db;
    }

    if (!m_uniqueId.isEmpty())
        QSqlDatabase::removeDatabase(m_uniqueId);
}

void Writer::flush()
{
    if (!m_db)
        return;

    QSqlQuery query(*m_db);

    query.prepare(QLatin1String("INSERT INTO info (namespace, attributes, url, title, data) VALUES (?, ?, ?, ?, ?)"));
    query.addBindValue(m_namespaces);
    query.addBindValue(m_attributes);
    query.addBindValue(m_urls);
    query.addBindValue(m_titles);
    query.addBindValue(m_contents);
    query.execBatch();

    m_namespaces = QVariantList();
    m_attributes = QVariantList();
    m_urls = QVariantList();
    m_titles = QVariantList();
    m_contents = QVariantList();
}

void Writer::removeNamespace(const QString &namespaceName)
{
    if (!m_db)
        return;

    if (!hasNamespace(namespaceName))
        return; // no data to delete

    m_needOptimize = true;

    QSqlQuery query(*m_db);

    query.prepare(QLatin1String("DELETE FROM info WHERE namespace = ?"));
    query.addBindValue(namespaceName);
    query.exec();
}

bool Writer::hasNamespace(const QString &namespaceName)
{
    if (!m_db)
        return false;

    QSqlQuery query(*m_db);

    query.prepare(QLatin1String("SELECT id FROM info WHERE namespace = ? LIMIT 1"));
    query.addBindValue(namespaceName);
    query.exec();

    return query.next();
}

void Writer::insertDoc(const QString &namespaceName,
                       const QString &attributes,
                       const QString &url,
                       const QString &title,
                       const QString &contents)
{
    m_namespaces.append(namespaceName);
    m_attributes.append(attributes);
    m_urls.append(url);
    m_titles.append(title);
    m_contents.append(contents);
}

void Writer::startTransaction()
{
    if (!m_db)
        return;

    m_needOptimize = false;
    if (m_db && m_db->driver()->hasFeature(QSqlDriver::Transactions))
        m_db->transaction();
}

void Writer::endTransaction()
{
    if (!m_db)
        return;

    QSqlQuery query(*m_db);

    if (m_needOptimize) {
        query.exec(QLatin1String("INSERT INTO titles(titles) VALUES('rebuild')"));
        query.exec(QLatin1String("INSERT INTO contents(contents) VALUES('rebuild')"));
    }

    if (m_db && m_db->driver()->hasFeature(QSqlDriver::Transactions))
        m_db->commit();

    if (m_needOptimize)
        query.exec(QLatin1String("VACUUM"));
}

QHelpSearchIndexWriter::QHelpSearchIndexWriter()
    : QThread()
    , m_cancel(false)
{
}

QHelpSearchIndexWriter::~QHelpSearchIndexWriter()
{
    m_mutex.lock();
    this->m_cancel = true;
    m_mutex.unlock();

    wait();
}

void QHelpSearchIndexWriter::cancelIndexing()
{
    QMutexLocker lock(&m_mutex);
    m_cancel = true;
}

void QHelpSearchIndexWriter::updateIndex(const QString &collectionFile,
                                         const QString &indexFilesFolder,
                                         bool reindex)
{
    wait();
    QMutexLocker lock(&m_mutex);

    m_cancel = false;
    m_reindex = reindex;
    m_collectionFile = collectionFile;
    m_indexFilesFolder = indexFilesFolder;

    lock.unlock();

    start(QThread::LowestPriority);
}

static const char IndexedNamespacesKey[] = "FTS5IndexedNamespaces";

static QMap<QString, QDateTime> readIndexMap(const QHelpEngineCore &engine)
{
    QMap<QString, QDateTime> indexMap;
    QDataStream dataStream(engine.customValue(
                QLatin1String(IndexedNamespacesKey)).toByteArray());
    dataStream >> indexMap;
    return indexMap;
}

static bool writeIndexMap(QHelpEngineCore *engine,
    const QMap<QString, QDateTime> &indexMap)
{
    QByteArray data;

    QDataStream dataStream(&data, QIODevice::ReadWrite);
    dataStream << indexMap;

    return engine->setCustomValue(
                QLatin1String(IndexedNamespacesKey), data);
}

static bool clearIndexMap(QHelpEngineCore *engine)
{
    return engine->removeCustomValue(QLatin1String(IndexedNamespacesKey));
}

void QHelpSearchIndexWriter::run()
{
    QMutexLocker lock(&m_mutex);

    if (m_cancel)
        return;

    const bool reindex(m_reindex);
    const QString collectionFile(m_collectionFile);
    const QString indexPath(m_indexFilesFolder);

    lock.unlock();

    QHelpEngineCore engine(collectionFile, nullptr);
    if (!engine.setupData())
        return;

    if (reindex)
        clearIndexMap(&engine);

    emit indexingStarted();

    Writer writer(indexPath);

    while (!writer.tryInit(reindex))
        sleep(1);

    const QStringList &registeredDocs = engine.registeredDocumentations();
    QMap<QString, QDateTime> indexMap = readIndexMap(engine);

    if (!reindex) {
        for (const QString &namespaceName : registeredDocs) {
            if (indexMap.contains(namespaceName)) {
                const QString path = engine.documentationFileName(namespaceName);
                if (indexMap.value(namespaceName) < QFileInfo(path).lastModified()) {
                    // Remove some outdated indexed stuff
                    indexMap.remove(namespaceName);
                    writer.removeNamespace(namespaceName);
                } else if (!writer.hasNamespace(namespaceName)) {
                    // No data in fts db for namespace.
                    // The namespace could have been removed from fts db
                    // or the whole fts db have been removed
                    // without removing it from indexMap.
                    indexMap.remove(namespaceName);
                }
            } else {
                // Needed in case namespaceName was removed from indexMap
                // without removing it from fts db.
                // May happen when e.g. qch file was removed manually
                // without removing fts db.
                writer.removeNamespace(namespaceName);
            }
        // TODO: we may also detect if there are any other data
        // and remove it
        }
    } else {
        indexMap.clear();
    }

    for (const QString &namespaceName : indexMap.keys()) {
        if (!registeredDocs.contains(namespaceName)) {
            indexMap.remove(namespaceName);
            writer.removeNamespace(namespaceName);
        }
    }

    for (const QString &namespaceName : registeredDocs) {
        lock.relock();
        if (m_cancel) {
            // store what we have done so far
            writeIndexMap(&engine, indexMap);
            writer.endTransaction();
            emit indexingFinished();
            return;
        }
        lock.unlock();

        // if indexed, continue
        if (indexMap.contains(namespaceName))
            continue;

        const QString fileName = engine.documentationFileName(namespaceName);
        QHelpDBReader reader(fileName, QHelpGlobal::uniquifyConnectionName(
                                 fileName, this), nullptr);
        if (!reader.init())
            continue;

        const QString virtualFolder = reader.virtualFolder();

        const QList<QStringList> &attributeSets =
            engine.filterAttributeSets(namespaceName);

        for (const QStringList &attributes : attributeSets) {
            const QString &attributesString = attributes.join(QLatin1Char('|'));

            const QMultiMap<QString, QByteArray> htmlFiles =
                    reader.filesData(attributes, QLatin1String("html"));
            const QMultiMap<QString, QByteArray> htmFiles =
                    reader.filesData(attributes, QLatin1String("htm"));
            const QMultiMap<QString, QByteArray> txtFiles =
                    reader.filesData(attributes, QLatin1String("txt"));

            QMultiMap<QString, QByteArray> files = htmlFiles;
            files.unite(htmFiles);
            files.unite(txtFiles);

            for (auto it = files.cbegin(), end = files.cend(); it != end ; ++it) {
                lock.relock();
                if (m_cancel) {
                    // store what we have done so far
                    writeIndexMap(&engine, indexMap);
                    writer.endTransaction();
                    emit indexingFinished();
                    return;
                }
                lock.unlock();

                const QString &file = it.key();
                const QByteArray &data = it.value();

                if (data.isEmpty())
                    continue;

                QUrl url;
                url.setScheme(QLatin1String("qthelp"));
                url.setAuthority(namespaceName);
                url.setPath(QLatin1Char('/') + virtualFolder + QLatin1Char('/') + file);

                if (url.hasFragment())
                    url.setFragment(QString());

                const QString &fullFileName = url.toString();
                if (!fullFileName.endsWith(QLatin1String(".html"))
                        && !fullFileName.endsWith(QLatin1String(".htm"))
                        && !fullFileName.endsWith(QLatin1String(".txt"))) {
                    continue;
                }

                QTextStream s(data);
                auto encoding = QStringDecoder::encodingForHtml(data);
                if (encoding)
                    s.setEncoding(*encoding);

                const QString &text = s.readAll();
                if (text.isEmpty())
                    continue;

                QString title;
                QString contents;
                if (fullFileName.endsWith(QLatin1String(".txt"))) {
                    title = fullFileName.mid(fullFileName.lastIndexOf(QLatin1Char('/')) + 1);
                    contents = text.toHtmlEscaped();
                } else {
                    QTextDocument doc;
                    doc.setHtml(text);

                    title = doc.metaInformation(QTextDocument::DocumentTitle).toHtmlEscaped();
                    contents = doc.toPlainText().toHtmlEscaped();
                }

                writer.insertDoc(namespaceName, attributesString, fullFileName, title, contents);
            }
        }
        writer.flush();
        const QString &path = engine.documentationFileName(namespaceName);
        indexMap.insert(namespaceName, QFileInfo(path).lastModified());
    }

    writeIndexMap(&engine, indexMap);

    writer.endTransaction();
    emit indexingFinished();
}

}   // namespace std
}   // namespace fulltextsearch

QT_END_NAMESPACE
