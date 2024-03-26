// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qhelpprojectdata_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStack>
#include <QtCore/QMap>
#include <QtCore/QRegularExpression>
#include <QtCore/QTextStream>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtCore/QXmlStreamReader>

QT_BEGIN_NAMESPACE

class QHelpProjectDataPrivate : public QXmlStreamReader
{
public:
    void readData(const QByteArray &contents);

    QString virtualFolder;
    QString namespaceName;
    QString fileName;
    QString rootPath;

    QList<QHelpDataCustomFilter> customFilterList;
    QList<QHelpDataFilterSection> filterSectionList;
    QMap<QString, QVariant> metaData;

    QString errorMsg;

private:
    void readProject();
    void readCustomFilter();
    void readFilterSection();
    void readTOC();
    void readKeywords();
    void readFiles();
    void skipUnknownToken();
    void addMatchingFiles(const QString &pattern);
    bool hasValidSyntax(const QString &nameSpace, const QString &vFolder) const;

    QMap<QString, QStringList> dirEntriesCache;
};

void QHelpProjectDataPrivate::skipUnknownToken()
{
    const QString message = QCoreApplication::translate("QHelpProject",
                  "Skipping unknown token <%1> in file \"%2\".")
                  .arg(name()).arg(fileName) + QLatin1Char('\n');
    fputs(qPrintable(message), stdout);

    skipCurrentElement();
}

void QHelpProjectDataPrivate::readData(const QByteArray &contents)
{
    addData(contents);
    while (!atEnd()) {
        readNext();
        if (isStartElement()) {
            if (name() == QLatin1String("QtHelpProject")
                    && attributes().value(QLatin1String("version"))
                    == QLatin1String("1.0")) {
                readProject();
            } else {
                raiseError(QCoreApplication::translate("QHelpProject",
                               "Unknown token. Expected \"QtHelpProject\"."));
            }
        }
    }

    if (hasError()) {
        raiseError(QCoreApplication::translate("QHelpProject",
                   "Error in line %1: %2").arg(lineNumber())
                   .arg(errorString()));
    }
}

void QHelpProjectDataPrivate::readProject()
{
    while (!atEnd()) {
        readNext();
        if (isStartElement()) {
            if (name() == QLatin1String("virtualFolder")) {
                virtualFolder = readElementText();
                if (!hasValidSyntax(QLatin1String("test"), virtualFolder))
                    raiseError(QCoreApplication::translate("QHelpProject",
                                   "Virtual folder has invalid syntax in file: \"%1\"").arg(fileName));
            } else if (name() == QLatin1String("namespace")) {
                namespaceName = readElementText();
                if (!hasValidSyntax(namespaceName, QLatin1String("test")))
                    raiseError(QCoreApplication::translate("QHelpProject",
                                   "Namespace \"%1\" has invalid syntax in file: \"%2\"").arg(namespaceName, fileName));
            } else if (name() == QLatin1String("customFilter")) {
                readCustomFilter();
            } else if (name() == QLatin1String("filterSection")) {
                readFilterSection();
            } else if (name() == QLatin1String("metaData")) {
                QString n = attributes().value(QLatin1String("name")).toString();
                if (!metaData.contains(n))
                    metaData[n]
                        = attributes().value(QLatin1String("value")).toString();
                else
                    metaData.insert(n, attributes().
                                    value(QLatin1String("value")).toString());
            } else {
                skipUnknownToken();
            }
        } else if (isEndElement() && name() == QLatin1String("QtHelpProject")) {
            if (namespaceName.isEmpty())
                raiseError(QCoreApplication::translate("QHelpProject",
                              "Missing namespace in QtHelpProject file: \"%1\"").arg(fileName));
            else if (virtualFolder.isEmpty())
                raiseError(QCoreApplication::translate("QHelpProject",
                               "Missing virtual folder in QtHelpProject file: \"%1\"").arg(fileName));
            break;
        }
    }
}

void QHelpProjectDataPrivate::readCustomFilter()
{
    QHelpDataCustomFilter filter;
    filter.name = attributes().value(QLatin1String("name")).toString();
    while (!atEnd()) {
        readNext();
        if (isStartElement()) {
            if (name() == QLatin1String("filterAttribute"))
                filter.filterAttributes.append(readElementText());
            else
                skipUnknownToken();
        } else if (isEndElement() && name() == QLatin1String("customFilter")) {
            break;
        }
    }
    customFilterList.append(filter);
}

void QHelpProjectDataPrivate::readFilterSection()
{
    filterSectionList.append(QHelpDataFilterSection());
    while (!atEnd()) {
        readNext();
        if (isStartElement()) {
            if (name() == QLatin1String("filterAttribute"))
                filterSectionList.last().addFilterAttribute(readElementText());
            else if (name() == QLatin1String("toc"))
                readTOC();
            else if (name() == QLatin1String("keywords"))
                readKeywords();
            else if (name() == QLatin1String("files"))
                readFiles();
            else
                skipUnknownToken();
        } else if (isEndElement() && name() == QLatin1String("filterSection")) {
            break;
        }
    }
}

void QHelpProjectDataPrivate::readTOC()
{
    QStack<QHelpDataContentItem*> contentStack;
    QHelpDataContentItem *itm = nullptr;
    while (!atEnd()) {
        readNext();
        if (isStartElement()) {
            if (name() == QLatin1String("section")) {
                const QString &title = attributes().value(QLatin1String("title")).toString();
                const QString &ref = attributes().value(QLatin1String("ref")).toString();
                if (contentStack.isEmpty()) {
                    itm = new QHelpDataContentItem(nullptr, title, ref);
                    filterSectionList.last().addContent(itm);
                } else {
                    itm = new QHelpDataContentItem(contentStack.top(), title, ref);
                }
                contentStack.push(itm);
            } else {
                skipUnknownToken();
            }
        } else if (isEndElement()) {
            if (name() == QLatin1String("section")) {
                contentStack.pop();
                continue;
            } else if (name() == QLatin1String("toc") && contentStack.isEmpty()) {
                return;
            } else {
                skipUnknownToken();
            }
        }
    }
}

static inline QString msgMissingAttribute(const QString &fileName, qint64 lineNumber, const QString &name)
{
    QString result;
    QTextStream str(&result);
    str << QDir::toNativeSeparators(fileName) << ':' << lineNumber
        << ": Missing attribute in <keyword";
    if (!name.isEmpty())
        str << " name=\"" << name << '"';
    str << ">.";
    return result;
}

void QHelpProjectDataPrivate::readKeywords()
{
    while (!atEnd()) {
        readNext();
        if (isStartElement()) {
            if (name() == QLatin1String("keyword")) {
                const QString &refAttribute = attributes().value(QStringLiteral("ref")).toString();
                const QString &nameAttribute = attributes().value(QStringLiteral("name")).toString();
                const QString &idAttribute = attributes().value(QStringLiteral("id")).toString();
                if (refAttribute.isEmpty() || (nameAttribute.isEmpty() && idAttribute.isEmpty())) {
                    qWarning("%s", qPrintable(msgMissingAttribute(fileName, lineNumber(), nameAttribute)));
                    continue;
                }
                filterSectionList.last()
                    .addIndex(QHelpDataIndexItem(nameAttribute, idAttribute, refAttribute));
            } else {
                skipUnknownToken();
            }
        } else if (isEndElement()) {
            if (name() == QLatin1String("keyword"))
                continue;
            else if (name() == QLatin1String("keywords"))
                return;
            else
                skipUnknownToken();
        }
    }
}

void QHelpProjectDataPrivate::readFiles()
{
    while (!atEnd()) {
        readNext();
        if (isStartElement()) {
            if (name() == QLatin1String("file"))
                addMatchingFiles(readElementText());
            else
                skipUnknownToken();
        } else if (isEndElement()) {
            if (name() == QLatin1String("file"))
                continue;
            else if (name() == QLatin1String("files"))
                return;
            else
                skipUnknownToken();
        }
    }
}

// Expand file pattern and add matches into list. If the pattern does not match
// any files, insert the pattern itself so the QHelpGenerator will emit a
// meaningful warning later.
void QHelpProjectDataPrivate::addMatchingFiles(const QString &pattern)
{
    // The pattern matching is expensive, so we skip it if no
    // wildcard symbols occur in the string.
    if (!pattern.contains(QLatin1Char('?')) && !pattern.contains(QLatin1Char('*'))
        && !pattern.contains(QLatin1Char('[')) && !pattern.contains(QLatin1Char(']'))) {
        filterSectionList.last().addFile(pattern);
        return;
    }

    const QFileInfo fileInfo(rootPath + QLatin1Char('/') + pattern);
    const QDir &dir = fileInfo.dir();
    const QString &path = dir.canonicalPath();

    // QDir::entryList() is expensive, so we cache the results.
    const auto &it = dirEntriesCache.constFind(path);
    const QStringList &entries = it != dirEntriesCache.cend() ?
                                 it.value() : dir.entryList(QDir::Files);
    if (it == dirEntriesCache.cend())
        dirEntriesCache.insert(path, entries);

    bool matchFound = false;
#ifdef Q_OS_WIN
    auto cs = QRegularExpression::CaseInsensitiveOption;
#else
    auto cs = QRegularExpression::NoPatternOption;
#endif
    const QRegularExpression regExp(QRegularExpression::wildcardToRegularExpression(fileInfo.fileName()), cs);
    for (const QString &file : entries) {
        auto match = regExp.match(file);
        if (match.hasMatch()) {
            matchFound = true;
            filterSectionList.last().
                addFile(QFileInfo(pattern).dir().path() + QLatin1Char('/') + file);
        }
    }
    if (!matchFound)
        filterSectionList.last().addFile(pattern);
}

bool QHelpProjectDataPrivate::hasValidSyntax(const QString &nameSpace,
                                             const QString &vFolder) const
{
    const QLatin1Char slash('/');
    if (nameSpace.contains(slash) || vFolder.contains(slash))
        return false;
    QUrl url;
    const QLatin1String scheme("qthelp");
    url.setScheme(scheme);
    const QString &canonicalNamespace = nameSpace.toLower();
    url.setHost(canonicalNamespace);
    url.setPath(slash + vFolder);

    const QString expectedUrl(scheme + QLatin1String("://")
        + canonicalNamespace + slash + vFolder);
    return url.isValid() && url.toString() == expectedUrl;
}

/*!
    \internal
    \class QHelpProjectData
    \since 4.4
    \brief The QHelpProjectData class stores all information found
    in a Qt help project file.

    The structure is filled with data by calling readData(). The
    specified file has to have the Qt help project file format in
    order to be read successfully. Possible reading errors can be
    retrieved by calling errorMessage().
*/

/*!
    Constructs a Qt help project data structure.
*/
QHelpProjectData::QHelpProjectData()
{
    d = new QHelpProjectDataPrivate;
}

/*!
    Destroys the help project data.
*/
QHelpProjectData::~QHelpProjectData()
{
    delete d;
}

/*!
    Reads the file \a fileName and stores the help data. The file has to
    have the Qt help project file format. Returns true if the file
    was successfully read, otherwise false.

    \sa errorMessage()
*/
bool QHelpProjectData::readData(const QString &fileName)
{
    d->fileName = fileName;
    d->rootPath = QFileInfo(fileName).absolutePath();
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        d->errorMsg = QCoreApplication::translate("QHelpProject",
                          "The input file %1 could not be opened.").arg(fileName);
        return false;
    }

    d->readData(file.readAll());
    return !d->hasError();
}

/*!
    Returns an error message if the reading of the Qt help project
    file failed. Otherwise, an empty QString is returned.

    \sa readData()
*/
QString QHelpProjectData::errorMessage() const
{
    if (d->hasError())
        return d->errorString();
    return d->errorMsg;
}

/*!
    \internal
*/
QString QHelpProjectData::namespaceName() const
{
    return d->namespaceName;
}

/*!
    \internal
*/
QString QHelpProjectData::virtualFolder() const
{
    return d->virtualFolder;
}

/*!
    \internal
*/
QList<QHelpDataCustomFilter> QHelpProjectData::customFilters() const
{
    return d->customFilterList;
}

/*!
    \internal
*/
QList<QHelpDataFilterSection> QHelpProjectData::filterSections() const
{
    return d->filterSectionList;
}

/*!
    \internal
*/
QMap<QString, QVariant> QHelpProjectData::metaData() const
{
    return d->metaData;
}

/*!
    \internal
*/
QString QHelpProjectData::rootPath() const
{
    return d->rootPath;
}

QT_END_NAMESPACE
