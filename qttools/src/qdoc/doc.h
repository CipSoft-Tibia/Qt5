/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
  doc.h
*/

#ifndef DOC_H
#define DOC_H

#include <qset.h>
#include <qstring.h>
#include <qmap.h>

#include "location.h"

QT_BEGIN_NAMESPACE

class Atom;
class CodeMarker;
class Config;
class DocPrivate;
class Quoter;
class Text;
class DitaRef;

typedef QPair<QString, Location> ArgLocPair;
typedef QList<ArgLocPair> ArgList;
typedef QMap<QString, QString> QStringMap;
typedef QMultiMap<QString, QString> QStringMultiMap;

struct Topic
{
    QString topic;
    QString args;
    Topic() { }
    Topic(QString& t, const QString &a) : topic(t), args(a) { }
    bool isEmpty() const { return topic.isEmpty(); }
    void clear() { topic.clear(); args.clear(); }
};
typedef QList<Topic> TopicList;

typedef QList<DitaRef*> DitaRefList;

class DitaRef
{
public:
    DitaRef() { }
    virtual ~DitaRef() { }

    const QString& navtitle() const { return navtitle_; }
    const QString& href() const { return href_; }
    void setNavtitle(const QString& t) { navtitle_ = t; }
    void setHref(const QString& t) { href_ = t; }
    virtual bool isMapRef() const = 0;
    virtual const DitaRefList* subrefs() const { return 0; }
    virtual void appendSubref(DitaRef* ) { }

private:
    QString navtitle_;
    QString href_;
};

class TopicRef : public DitaRef
{
public:
    TopicRef() { }
    ~TopicRef();

    bool isMapRef() const override { return false; }
    const DitaRefList* subrefs() const override { return &subrefs_; }
    void appendSubref(DitaRef* t) override { subrefs_.append(t); }

private:
    DitaRefList subrefs_;
};

class MapRef : public DitaRef
{
public:
    MapRef() { }
    ~MapRef() { }

    bool isMapRef() const override { return true; }
};

class Doc
{
    Q_DECLARE_TR_FUNCTIONS(QDoc::Doc)

public:
    // the order is important
    enum Sections {
        NoSection = -2,
        Part = -1,
        Chapter = 1,
        Section1 = 1,
        Section2 = 2,
        Section3 = 3,
        Section4 = 4
    };

    Doc() : priv(0) {}
    Doc(const Location& start_loc,
        const Location& end_loc,
        const QString& source,
        const QSet<QString>& metaCommandSet,
        const QSet<QString>& topics);
    Doc(const Doc &doc);
    ~Doc();

    Doc& operator=( const Doc& doc );

    void renameParameters(const QStringList &oldNames,
                          const QStringList &newNames);
    void simplifyEnumDoc();
    void setBody(const Text &body);
    const DitaRefList& ditamap() const;

    const Location &location() const;
    const Location& startLocation() const;
    const Location& endLocation() const;
    bool isEmpty() const;
    const QString& source() const;
    const Text& body() const;
    Text briefText(bool inclusive = false) const;
    Text trimmedBriefText(const QString &className) const;
    Text legaleseText() const;
    Sections granularity() const;
    const QSet<QString> &parameterNames() const;
    const QStringList &enumItemNames() const;
    const QStringList &omitEnumItemNames() const;
    const QSet<QString> &metaCommandsUsed() const;
    const TopicList& topicsUsed() const;
    ArgList metaCommandArgs(const QString& metaCommand) const;
    const QList<Text> &alsoList() const;
    bool hasTableOfContents() const;
    bool hasKeywords() const;
    bool hasTargets() const;
    const QList<Atom *> &tableOfContents() const;
    const QVector<int> &tableOfContentsLevels() const;
    const QList<Atom *> &keywords() const;
    const QList<Atom *> &targets() const;
    const QStringMultiMap &metaTagMap() const;

    static void initialize( const Config &config );
    static void terminate();
    static QString alias( const QString &english );
    static void trimCStyleComment( Location& location, QString& str );
    static QString resolveFile(const Location &location,const QString &fileName,
                               QString *userFriendlyFilePath = nullptr);
    static CodeMarker *quoteFromFile(const Location &location,
                                     Quoter &quoter,
                                     const QString &fileName);
    static QString canonicalTitle(const QString &title);
    static const Config* config() { return config_; }

private:
    void detach();
    DocPrivate *priv;
    static const Config* config_;
};
Q_DECLARE_TYPEINFO(Doc, Q_MOVABLE_TYPE);
typedef QList<Doc> DocList;

QT_END_NAMESPACE

#endif
