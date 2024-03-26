// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHELPSEARCHENGINE_H
#define QHELPSEARCHENGINE_H

#include <QtHelp/qhelp_global.h>

#include <QtCore/QMap>
#include <QtCore/QUrl>
#include <QtCore/QObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE

class QHelpEngineCore;
class QHelpSearchQueryWidget;
class QHelpSearchEnginePrivate;
class QHelpSearchResultData;
class QHelpSearchResultWidget;

class QHELP_EXPORT QHelpSearchQuery
{
public:
    enum FieldName { DEFAULT = 0, FUZZY, WITHOUT, PHRASE, ALL, ATLEAST };

    QHelpSearchQuery()
        : fieldName(DEFAULT) { wordList.clear(); }
    QHelpSearchQuery(FieldName field, const QStringList &wordList_)
        : fieldName(field), wordList(wordList_) {}

    FieldName fieldName;
    QStringList wordList;
};

class QHELP_EXPORT QHelpSearchResult
{
public:
    QHelpSearchResult();
    QHelpSearchResult(const QHelpSearchResult &other);
    QHelpSearchResult(const QUrl &url, const QString &title, const QString &snippet);
    ~QHelpSearchResult();

    QHelpSearchResult &operator=(const QHelpSearchResult &other);

    QString title() const;
    QUrl url() const;
    QString snippet() const;

private:
    QSharedDataPointer<QHelpSearchResultData> d;
};

class QHELP_EXPORT QHelpSearchEngine : public QObject
{
    Q_OBJECT

public:
    explicit QHelpSearchEngine(QHelpEngineCore *helpEngine, QObject *parent = nullptr);
    ~QHelpSearchEngine();

    QHelpSearchQueryWidget* queryWidget();
    QHelpSearchResultWidget* resultWidget();

#if QT_DEPRECATED_SINCE(5, 9)
    typedef QPair<QString, QString> SearchHit;

    QT_DEPRECATED int hitsCount() const;
    QT_DEPRECATED int hitCount() const;
    QT_DEPRECATED QList<SearchHit> hits(int start, int end) const;
    QT_DEPRECATED QList<QHelpSearchQuery> query() const;
#endif

    int searchResultCount() const;
    QList<QHelpSearchResult> searchResults(int start, int end) const;
    QString searchInput() const;

public Q_SLOTS:
    void reindexDocumentation();
    void cancelIndexing();

#if QT_DEPRECATED_SINCE(5, 9)
    QT_DEPRECATED void search(const QList<QHelpSearchQuery> &queryList);
#endif

    void search(const QString &searchInput);
    void cancelSearching();

    void scheduleIndexDocumentation();

Q_SIGNALS:
    void indexingStarted();
    void indexingFinished();

    void searchingStarted();
    void searchingFinished(int searchResultCount);

private Q_SLOTS:
    void indexDocumentation();

private:
    QHelpSearchEnginePrivate *d;
};

QT_END_NAMESPACE

#endif  // QHELPSEARCHENGINE_H
