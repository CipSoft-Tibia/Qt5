/****************************************************************************
**
** Copyright (C) 2017 Crimson AS <info@crimson.no>
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlimport_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtQml/qqmlfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qpluginloader.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qreadwritelock.h>
#include <QtQml/qqmlextensioninterface.h>
#include <QtQml/qqmlextensionplugin.h>
#include <private/qqmlextensionplugin_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qqmltypenamecache_p.h>
#include <private/qqmlengine_p.h>
#include <private/qfieldlist_p.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonarray.h>

#include <algorithm>
#include <functional>

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(qmlImportTrace, QML_IMPORT_TRACE)
DEFINE_BOOL_CONFIG_OPTION(qmlCheckTypes, QML_CHECK_TYPES)

static const QLatin1Char Dot('.');
static const QLatin1Char Slash('/');
static const QLatin1Char Backslash('\\');
static const QLatin1Char Colon(':');
static const QLatin1String Slash_qmldir("/qmldir");
static const QLatin1String String_qmldir("qmldir");
static const QString dotqml_string(QStringLiteral(".qml"));
static const QString dotuidotqml_string(QStringLiteral(".ui.qml"));
static bool designerSupportRequired = false;

namespace {

QString resolveLocalUrl(const QString &url, const QString &relative)
{
    if (relative.contains(Colon)) {
        // contains a host name
        return QUrl(url).resolved(QUrl(relative)).toString();
    } else if (relative.isEmpty()) {
        return url;
    } else if (relative.at(0) == Slash || !url.contains(Slash)) {
        return relative;
    } else {
        const QStringRef baseRef = url.leftRef(url.lastIndexOf(Slash) + 1);
        if (relative == QLatin1String("."))
            return baseRef.toString();

        QString base = baseRef + relative;

        // Remove any relative directory elements in the path
        int length = base.length();
        int index = 0;
        while ((index = base.indexOf(QLatin1String("/."), index)) != -1) {
            if ((length > (index + 2)) && (base.at(index + 2) == Dot) &&
                (length == (index + 3) || (base.at(index + 3) == Slash))) {
                // Either "/../" or "/..<END>"
                int previous = base.lastIndexOf(Slash, index - 1);
                if (previous == -1)
                    break;

                int removeLength = (index - previous) + 3;
                base.remove(previous + 1, removeLength);
                length -= removeLength;
                index = previous;
            } else if ((length == (index + 2)) || (base.at(index + 2) == Slash)) {
                // Either "/./" or "/.<END>"
                base.remove(index, 2);
                length -= 2;
            } else {
                ++index;
            }
        }

        return base;
    }
}

bool isPathAbsolute(const QString &path)
{
#if defined(Q_OS_UNIX)
    return (path.at(0) == Slash);
#else
    QFileInfo fi(path);
    return fi.isAbsolute();
#endif
}

/*
    \internal

    Fetches the QQmlType instance registered for \a urlString, creating a
    registration for it if it is not already registered, using the associated
    \a typeName, \a isCompositeSingleton, \a majorVersion and \a minorVersion
    details.

    Errors (if there are any) are placed into \a errors, if it is nonzero. Note
    that errors are treated as fatal if \a errors is not set.
*/
QQmlType fetchOrCreateTypeForUrl(const QString &urlString, const QHashedStringRef& typeName,
                        bool isCompositeSingleton, QList<QQmlError> *errors,
                        int majorVersion=-1, int minorVersion=-1)
{
    QUrl url(urlString); // ### unfortunate (costly) conversion
    QQmlType ret = QQmlMetaType::qmlType(url);
    if (ret.isValid())
        return ret;

    int dot = typeName.indexOf(QLatin1Char('.'));
    QHashedStringRef unqualifiedtype = dot < 0 ? typeName : QHashedStringRef(typeName.constData() + dot + 1, typeName.length() - dot - 1);

    // We need a pointer, but we were passed a string. Take a copy so we
    // can guarentee it will live long enough to reach qmlregister.
    QByteArray buf(unqualifiedtype.toString().toUtf8());

    QQmlMetaTypeRegistrationFailureRecorder failureRecorder;

    // Register the type. Note that the URI parameters here are empty; for
    // file type imports, we do not place them in a URI as we don't
    // necessarily have a good and unique one (picture a library import,
    // which may be found in multiple plugin locations on disk), but there
    // are other reasons for this too.
    //
    // By not putting them in a URI, we prevent the types from being
    // registered on a QQmlTypeModule; this is important, as once types are
    // placed on there, they cannot be easily removed, meaning if the
    // developer subsequently loads a different import (meaning different
    // types) with the same URI (using, say, a different plugin path), it is
    // very undesirable that we continue to associate the types from the
    // "old" URI with that new module.
    //
    // Not having URIs also means that the types cannot be found by name
    // etc, the only way to look them up is through QQmlImports -- for
    // better or worse.
    if (isCompositeSingleton) {
        QQmlPrivate::RegisterCompositeSingletonType reg = {
            url,
            "", // uri
            majorVersion,
            minorVersion,
            buf.constData()
        };
        ret = QQmlMetaType::registerCompositeSingletonType(reg);
    } else {
        QQmlPrivate::RegisterCompositeType reg = {
            url,
            "", // uri
            majorVersion,
            minorVersion,
            buf.constData()
        };
        ret = QQmlMetaType::registerCompositeType(reg);
    }

    // This means that the type couldn't be found by URL, but could not be
    // registered either, meaning we most likely were passed some kind of bad
    // data.
    if (!ret.isValid()) {
        if (!errors) // Cannot list errors properly, just quit
            qFatal("%s", failureRecorder.failures().join('\n').toLatin1().constData());
        QQmlError error;
        error.setDescription(failureRecorder.failures().join('\n'));
        errors->prepend(error);
    }
    return ret;
}

} // namespace

struct RegisteredPlugin {
    QString uri;
    QPluginLoader* loader;
};

struct StringRegisteredPluginMap : public QMap<QString, RegisteredPlugin> {
    QMutex mutex;
};

Q_GLOBAL_STATIC(StringRegisteredPluginMap, qmlEnginePluginsWithRegisteredTypes); // stores the uri and the PluginLoaders

void qmlClearEnginePlugins()
{
    StringRegisteredPluginMap *plugins = qmlEnginePluginsWithRegisteredTypes();
    QMutexLocker lock(&plugins->mutex);
#if QT_CONFIG(library)
    for (auto &plugin : qAsConst(*plugins)) {
        QPluginLoader* loader = plugin.loader;
        if (loader && !loader->unload())
            qWarning("Unloading %s failed: %s", qPrintable(plugin.uri), qPrintable(loader->errorString()));
        delete loader;
    }
#endif
    plugins->clear();
}

typedef QPair<QStaticPlugin, QJsonArray> StaticPluginPair;

/*!
    \internal
    \class QQmlImportInstance

    A QQmlImportType represents a single import of a document, held within a
    namespace.

    \note The uri here may not necessarily be unique (e.g. for file imports).

    \note Version numbers may be -1 for file imports: this means that no
    version was specified as part of the import. Type resolution will be
    responsible for attempting to find the "best" possible version.
*/

/*!
    \internal
    \class QQmlImportNamespace

    A QQmlImportNamespace is a way of seperating imports into a local namespace.

    Within a QML document, there is at least one namespace (the
    "unqualified set") where imports without a qualifier are placed, i.e:

        import QtQuick 2.6

    will have a single namespace (the unqualified set) containing a single import
    for QtQuick 2.6. However, there may be others if an import statement gives
    a qualifier, i.e the following will result in an additional new
    QQmlImportNamespace in the qualified set:

        import MyFoo 1.0 as Foo
*/

class QQmlImportsPrivate
{
public:
    QQmlImportsPrivate(QQmlTypeLoader *loader);
    ~QQmlImportsPrivate();

    QQmlImportNamespace *importNamespace(const QString &prefix) const;

    bool addLibraryImport(const QString& uri, const QString &prefix,
                          int vmaj, int vmin, const QString &qmldirIdentifier, const QString &qmldirUrl, bool incomplete,
                          QQmlImportDatabase *database,
                          QList<QQmlError> *errors);

    bool addFileImport(const QString &uri, const QString &prefix,
                       int vmaj, int vmin,
                       bool isImplicitImport, bool incomplete, QQmlImportDatabase *database,
                       QList<QQmlError> *errors);

    bool updateQmldirContent(const QString &uri, const QString &prefix,
                             const QString &qmldirIdentifier, const QString& qmldirUrl,
                             QQmlImportDatabase *database,
                             QList<QQmlError> *errors);

    bool resolveType(const QHashedStringRef &type, int *vmajor, int *vminor,
                     QQmlType *type_return, QList<QQmlError> *errors,
                     QQmlType::RegistrationType registrationType,
                     QQmlImport::RecursionRestriction recursionRestriction
                     = QQmlImport::PreventRecursion);

    QUrl baseUrl;
    QString base;
    int ref;

    // storage of data related to imports without a namespace
    mutable QQmlImportNamespace unqualifiedset;

    QQmlImportNamespace *findQualifiedNamespace(const QHashedStringRef &) const;

    // storage of data related to imports with a namespace
    mutable QFieldList<QQmlImportNamespace, &QQmlImportNamespace::nextNamespace> qualifiedSets;

    QQmlTypeLoader *typeLoader;

    static bool locateQmldir(const QString &uri, int vmaj, int vmin,
                             QQmlImportDatabase *database,
                             QString *outQmldirFilePath, QString *outUrl);

    static bool validateQmldirVersion(const QQmlTypeLoaderQmldirContent &qmldir, const QString &uri, int vmaj, int vmin,
                                      QList<QQmlError> *errors);

    bool importExtension(const QString &absoluteFilePath, const QString &uri,
                         int vmaj, int vmin,
                         QQmlImportDatabase *database,
                         const QQmlTypeLoaderQmldirContent &qmldir,
                         QList<QQmlError> *errors);

    bool getQmldirContent(const QString &qmldirIdentifier, const QString &uri,
                          QQmlTypeLoaderQmldirContent *qmldir, QList<QQmlError> *errors);

    QString resolvedUri(const QString &dir_arg, QQmlImportDatabase *database);

    QQmlImportInstance *addImportToNamespace(QQmlImportNamespace *nameSpace,
                                                      const QString &uri, const QString &url,
                                                      int vmaj, int vmin, QV4::CompiledData::Import::ImportType type,
                                                      QList<QQmlError> *errors, bool lowPrecedence = false);

    bool populatePluginPairVector(QVector<StaticPluginPair> &result, const QString &uri, const QStringList &versionUris,
                                     const QString &qmldirPath, QList<QQmlError> *errors);
};

/*!
\class QQmlImports
\brief The QQmlImports class encapsulates one QML document's import statements.
\internal
*/
QQmlImports::QQmlImports(const QQmlImports &copy)
: d(copy.d)
{
    ++d->ref;
}

QQmlImports &
QQmlImports::operator =(const QQmlImports &copy)
{
    ++copy.d->ref;
    if (--d->ref == 0)
        delete d;
    d = copy.d;
    return *this;
}

QQmlImports::QQmlImports(QQmlTypeLoader *typeLoader)
    : d(new QQmlImportsPrivate(typeLoader))
{
}

QQmlImports::~QQmlImports()
{
    if (--d->ref == 0)
        delete d;
}

/*!
  Sets the base URL to be used for all relative file imports added.
*/
void QQmlImports::setBaseUrl(const QUrl& url, const QString &urlString)
{
    d->baseUrl = url;

    if (urlString.isEmpty()) {
        d->base = url.toString();
    } else {
        //Q_ASSERT(url.toString() == urlString);
        d->base = urlString;
    }
}

/*!
  Returns the base URL to be used for all relative file imports added.
*/
QUrl QQmlImports::baseUrl() const
{
    return d->baseUrl;
}

/*
    \internal

    This method is responsible for populating data of all types visible in this
    document's imports into the \a cache for resolution elsewhere (e.g. in JS,
    or when loading additional types).

    \note This is for C++ types only. Composite types are handled separately,
    as they do not have a QQmlTypeModule.
*/
void QQmlImports::populateCache(QQmlTypeNameCache *cache) const
{
    const QQmlImportNamespace &set = d->unqualifiedset;

    for (int ii = set.imports.count() - 1; ii >= 0; --ii) {
        const QQmlImportInstance *import = set.imports.at(ii);
        QQmlTypeModule *module = QQmlMetaType::typeModule(import->uri, import->majversion);
        if (module) {
            cache->m_anonymousImports.append(QQmlTypeModuleVersion(module, import->minversion));
        }
    }

    for (QQmlImportNamespace *ns = d->qualifiedSets.first(); ns; ns = d->qualifiedSets.next(ns)) {

        const QQmlImportNamespace &set = *ns;

        // positioning is important; we must create the namespace even if there is no module.
        QQmlImportRef &typeimport = cache->m_namedImports[set.prefix];
        typeimport.m_qualifier = set.prefix;

        for (int ii = set.imports.count() - 1; ii >= 0; --ii) {
            const QQmlImportInstance *import = set.imports.at(ii);
            QQmlTypeModule *module = QQmlMetaType::typeModule(import->uri, import->majversion);
            if (module) {
                QQmlImportRef &typeimport = cache->m_namedImports[set.prefix];
                typeimport.modules.append(QQmlTypeModuleVersion(module, import->minversion));
            }
        }
    }
}

// We need to exclude the entry for the current baseUrl. This can happen for example
// when handling qmldir files on the remote dir case and the current type is marked as
// singleton.
bool excludeBaseUrl(const QString &importUrl, const QString &fileName, const QString &baseUrl)
{
    if (importUrl.isEmpty())
        return false;

    if (baseUrl.startsWith(importUrl))
    {
        if (fileName == baseUrl.midRef(importUrl.size()))
            return false;
    }

    return true;
}

void findCompositeSingletons(const QQmlImportNamespace &set, QList<QQmlImports::CompositeSingletonReference> &resultList, const QUrl &baseUrl)
{
    typedef QQmlDirComponents::const_iterator ConstIterator;

    for (int ii = set.imports.count() - 1; ii >= 0; --ii) {
        const QQmlImportInstance *import = set.imports.at(ii);

        const QQmlDirComponents &components = import->qmlDirComponents;

        const int importMajorVersion = import->majversion;
        const int importMinorVersion = import->minversion;
        auto shouldSkipSingleton = [importMajorVersion, importMinorVersion](int singletonMajorVersion, int singletonMinorVersion) -> bool {
            return importMajorVersion != -1 &&
                    (singletonMajorVersion > importMajorVersion || (singletonMajorVersion == importMajorVersion && singletonMinorVersion > importMinorVersion));
        };

        ConstIterator cend = components.constEnd();
        for (ConstIterator cit = components.constBegin(); cit != cend; ++cit) {
            if (cit->singleton && excludeBaseUrl(import->url, cit->fileName, baseUrl.toString())) {
                if (shouldSkipSingleton(cit->majorVersion, cit->minorVersion))
                    continue;
                QQmlImports::CompositeSingletonReference ref;
                ref.typeName = cit->typeName;
                ref.prefix = set.prefix;
                ref.majorVersion = cit->majorVersion;
                ref.minorVersion = cit->minorVersion;
                resultList.append(ref);
            }
        }

        if (QQmlTypeModule *module = QQmlMetaType::typeModule(import->uri, import->majversion)) {
            module->walkCompositeSingletons([&resultList, &set, &shouldSkipSingleton](const QQmlType &singleton) {
                if (shouldSkipSingleton(singleton.majorVersion(), singleton.minorVersion()))
                    return;
                QQmlImports::CompositeSingletonReference ref;
                ref.typeName = singleton.elementName();
                ref.prefix = set.prefix;
                ref.majorVersion = singleton.majorVersion();
                ref.minorVersion = singleton.minorVersion();
                resultList.append(ref);
            });
        }
    }
}

/*
    \internal

    Returns a list of all composite singletons present in this document's
    imports.

    This information is used by QQmlTypeLoader to ensure that composite singletons
    are marked as dependencies during type loading.
*/
QList<QQmlImports::CompositeSingletonReference> QQmlImports::resolvedCompositeSingletons() const
{
    QList<QQmlImports::CompositeSingletonReference> compositeSingletons;

    const QQmlImportNamespace &set = d->unqualifiedset;
    findCompositeSingletons(set, compositeSingletons, baseUrl());

    for (QQmlImportNamespace *ns = d->qualifiedSets.first(); ns; ns = d->qualifiedSets.next(ns)) {
        const QQmlImportNamespace &set = *ns;
        findCompositeSingletons(set, compositeSingletons, baseUrl());
    }

    std::stable_sort(compositeSingletons.begin(), compositeSingletons.end(),
                     [](const QQmlImports::CompositeSingletonReference &lhs,
                        const QQmlImports::CompositeSingletonReference &rhs) {
        if (lhs.prefix != rhs.prefix)
            return lhs.prefix < rhs.prefix;

        if (lhs.typeName != rhs.typeName)
            return lhs.typeName < rhs.typeName;

        return lhs.majorVersion != rhs.majorVersion
            ? lhs.majorVersion < rhs.majorVersion
            : lhs.minorVersion < rhs.minorVersion;
    });

    return compositeSingletons;
}

/*
    \internal

    Returns a list of scripts imported by this document. This is used by
    QQmlTypeLoader to properly handle dependencies on imported scripts.
*/
QList<QQmlImports::ScriptReference> QQmlImports::resolvedScripts() const
{
    QList<QQmlImports::ScriptReference> scripts;

    const QQmlImportNamespace &set = d->unqualifiedset;

    for (int ii = set.imports.count() - 1; ii >= 0; --ii) {
        const QQmlImportInstance *import = set.imports.at(ii);

        for (const QQmlDirParser::Script &script : import->qmlDirScripts) {
            ScriptReference ref;
            ref.nameSpace = script.nameSpace;
            ref.location = QUrl(import->url).resolved(QUrl(script.fileName));
            scripts.append(ref);
        }
    }

    for (QQmlImportNamespace *ns = d->qualifiedSets.first(); ns; ns = d->qualifiedSets.next(ns)) {
        const QQmlImportNamespace &set = *ns;

        for (int ii = set.imports.count() - 1; ii >= 0; --ii) {
            const QQmlImportInstance *import = set.imports.at(ii);

            for (const QQmlDirParser::Script &script : import->qmlDirScripts) {
                ScriptReference ref;
                ref.nameSpace = script.nameSpace;
                ref.qualifier = set.prefix;
                ref.location = QUrl(import->url).resolved(QUrl(script.fileName));
                scripts.append(ref);
            }
        }
    }

    return scripts;
}

static QString joinStringRefs(const QVector<QStringRef> &refs, const QChar &sep)
{
    QString str;
    for (auto it = refs.cbegin(); it != refs.cend(); ++it) {
        if (it != refs.cbegin())
            str += sep;
        str += *it;
    }
    return str;
}

/*!
    Forms complete paths to a qmldir file, from a base URL, a module URI and version specification.

    For example, QtQml.Models 2.0:
    - base/QtQml/Models.2.0/qmldir
    - base/QtQml.2.0/Models/qmldir
    - base/QtQml/Models.2/qmldir
    - base/QtQml.2/Models/qmldir
    - base/QtQml/Models/qmldir
*/
QStringList QQmlImports::completeQmldirPaths(const QString &uri, const QStringList &basePaths, int vmaj, int vmin)
{
    const QVector<QStringRef> parts = uri.splitRef(Dot, QString::SkipEmptyParts);

    QStringList qmlDirPathsPaths;
    // fully & partially versioned parts + 1 unversioned for each base path
    qmlDirPathsPaths.reserve(basePaths.count() * (2 * parts.count() + 1));

    for (int version = FullyVersioned; version <= Unversioned; ++version) {
        const QString ver = versionString(vmaj, vmin, static_cast<QQmlImports::ImportVersion>(version));

        for (const QString &path : basePaths) {
            QString dir = path;
            if (!dir.endsWith(Slash) && !dir.endsWith(Backslash))
                dir += Slash;

            // append to the end
            qmlDirPathsPaths += dir + joinStringRefs(parts, Slash) + ver + Slash_qmldir;

            if (version != Unversioned) {
                // insert in the middle
                for (int index = parts.count() - 2; index >= 0; --index) {
                    qmlDirPathsPaths += dir + joinStringRefs(parts.mid(0, index + 1), Slash)
                                            + ver + Slash
                                            + joinStringRefs(parts.mid(index + 1), Slash) + Slash_qmldir;
                }
            }
        }
    }

    return qmlDirPathsPaths;
}

QString QQmlImports::versionString(int vmaj, int vmin, ImportVersion version)
{
    if (version == QQmlImports::FullyVersioned) {
        // extension with fully encoded version number (eg. MyModule.3.2)
        return QString::asprintf(".%d.%d", vmaj, vmin);
    } else if (version == QQmlImports::PartiallyVersioned) {
        // extension with encoded version major (eg. MyModule.3)
        return QString::asprintf(".%d", vmaj);
    } // else extension without version number (eg. MyModule)
    return QString();
}

/*!
  \internal

  The given (namespace qualified) \a type is resolved to either
  \list
  \li a QQmlImportNamespace stored at \a ns_return, or
  \li a QQmlType stored at \a type_return,
  \endlist

  If any return pointer is 0, the corresponding search is not done.

  \sa addFileImport(), addLibraryImport
*/
bool QQmlImports::resolveType(const QHashedStringRef &type,
                              QQmlType *type_return, int *vmaj, int *vmin,
                              QQmlImportNamespace** ns_return, QList<QQmlError> *errors,
                              QQmlType::RegistrationType registrationType,
                              QQmlImport::RecursionRestriction recursionRestriction) const
{
    QQmlImportNamespace* ns = d->findQualifiedNamespace(type);
    if (ns) {
        if (ns_return)
            *ns_return = ns;
        return true;
    }
    if (type_return) {
        if (d->resolveType(type, vmaj, vmin, type_return, errors, registrationType,
                           recursionRestriction)) {
            if (qmlImportTrace()) {
#define RESOLVE_TYPE_DEBUG qDebug().nospace() << "QQmlImports(" << qPrintable(baseUrl().toString()) \
                                              << ')' << "::resolveType: " << type.toString() << " => "

                if (type_return && type_return->isValid()) {
                    if (type_return->isCompositeSingleton())
                        RESOLVE_TYPE_DEBUG << type_return->typeName() << ' ' << type_return->sourceUrl() << " TYPE/URL-SINGLETON";
                    else if (type_return->isComposite())
                        RESOLVE_TYPE_DEBUG << type_return->typeName() << ' ' << type_return->sourceUrl() << " TYPE/URL";
                    else
                        RESOLVE_TYPE_DEBUG << type_return->typeName() << " TYPE";
                }
#undef RESOLVE_TYPE_DEBUG
            }
            return true;
        }
    }
    return false;
}

bool QQmlImportInstance::setQmldirContent(const QString &resolvedUrl, const QQmlTypeLoaderQmldirContent &qmldir, QQmlImportNamespace *nameSpace, QList<QQmlError> *errors)
{
    Q_ASSERT(resolvedUrl.endsWith(Slash));
    url = resolvedUrl;
    localDirectoryPath = QQmlFile::urlToLocalFileOrQrc(url);

    qmlDirComponents = qmldir.components();

    const QQmlDirScripts &scripts = qmldir.scripts();
    if (!scripts.isEmpty()) {
        // Verify that we haven't imported these scripts already
        for (QList<QQmlImportInstance *>::const_iterator it = nameSpace->imports.constBegin();
             it != nameSpace->imports.constEnd(); ++it) {
            if ((*it != this) && ((*it)->uri == uri)) {
                QQmlError error;
                error.setDescription(QQmlImportDatabase::tr("\"%1\" is ambiguous. Found in %2 and in %3").arg(uri).arg(url).arg((*it)->url));
                errors->prepend(error);
                return false;
            }
        }

        qmlDirScripts = getVersionedScripts(scripts, majversion, minversion);
    }

    return true;
}

QQmlDirScripts QQmlImportInstance::getVersionedScripts(const QQmlDirScripts &qmldirscripts, int vmaj, int vmin)
{
    QMap<QString, QQmlDirParser::Script> versioned;

    for (QList<QQmlDirParser::Script>::const_iterator sit = qmldirscripts.constBegin();
         sit != qmldirscripts.constEnd(); ++sit) {
        // Only include scripts that match our requested version
        if (((vmaj == -1) || (sit->majorVersion == vmaj)) &&
            ((vmin == -1) || (sit->minorVersion <= vmin))) {
            // Load the highest version that matches
            QMap<QString, QQmlDirParser::Script>::iterator vit = versioned.find(sit->nameSpace);
            if (vit == versioned.end() || (vit->minorVersion < sit->minorVersion)) {
                versioned.insert(sit->nameSpace, *sit);
            }
        }
    }

    return versioned.values();
}

/*!
  \internal

  Searching \e only in the namespace \a ns (previously returned in a call to
  resolveType(), \a type is found and returned to
  a QQmlType stored at \a type_return. If the type is from a QML file, the returned
  type will be a CompositeType.

  If the return pointer is 0, the corresponding search is not done.
*/
bool QQmlImports::resolveType(QQmlImportNamespace *ns, const QHashedStringRef &type,
                              QQmlType *type_return, int *vmaj, int *vmin,
                              QQmlType::RegistrationType registrationType) const
{
    return ns->resolveType(d->typeLoader, type, vmaj, vmin, type_return, nullptr, nullptr, registrationType);
}

bool QQmlImportInstance::resolveType(QQmlTypeLoader *typeLoader, const QHashedStringRef& type,
                                     int *vmajor, int *vminor, QQmlType *type_return, QString *base,
                                     bool *typeRecursionDetected,
                                     QQmlType::RegistrationType registrationType,
                                     QQmlImport::RecursionRestriction recursionRestriction,
                                     QList<QQmlError> *errors) const
{
    if (majversion >= 0 && minversion >= 0) {
        QQmlType t = QQmlMetaType::qmlType(type, uri, majversion, minversion);
        if (t.isValid()) {
            if (vmajor)
                *vmajor = majversion;
            if (vminor)
                *vminor = minversion;
            if (type_return)
                *type_return = t;
            return true;
        }
    }

    const QString typeStr = type.toString();
    QQmlDirComponents::ConstIterator it = qmlDirComponents.find(typeStr), end = qmlDirComponents.end();
    if (it != end) {
        QString componentUrl;
        bool isCompositeSingleton = false;
        QQmlDirComponents::ConstIterator candidate = end;
        for ( ; it != end && it.key() == typeStr; ++it) {
            const QQmlDirParser::Component &c = *it;
            switch (registrationType) {
            case QQmlType::AnyRegistrationType:
                break;
            case QQmlType::CompositeSingletonType:
                if (!c.singleton)
                    continue;
                break;
            default:
                if (c.singleton)
                    continue;
                break;
            }

            // importing version -1 means import ALL versions
            if ((majversion == -1) ||
                (implicitlyImported && c.internal) || // allow the implicit import of internal types
                (c.majorVersion == majversion && c.minorVersion <= minversion)) {
                // Is this better than the previous candidate?
                if ((candidate == end) ||
                    (c.majorVersion > candidate->majorVersion) ||
                    ((c.majorVersion == candidate->majorVersion) && (c.minorVersion > candidate->minorVersion))) {
                    if (base) {
                        componentUrl = resolveLocalUrl(QString(url + c.typeName + dotqml_string), c.fileName);
                        if (c.internal) {
                            if (resolveLocalUrl(*base, c.fileName) != componentUrl)
                                continue; // failed attempt to access an internal type
                        }
                        if (recursionRestriction == QQmlImport::PreventRecursion && *base == componentUrl) {
                            if (typeRecursionDetected)
                                *typeRecursionDetected = true;
                            continue; // no recursion
                        }
                    }

                    // This is our best candidate so far
                    candidate = it;
                    isCompositeSingleton = c.singleton;
                }
            }
        }

        if (candidate != end) {
            if (!base) // ensure we have a componentUrl
                componentUrl = resolveLocalUrl(QString(url + candidate->typeName + dotqml_string), candidate->fileName);
            QQmlType returnType = fetchOrCreateTypeForUrl(componentUrl, type, isCompositeSingleton,
                                                          nullptr, candidate->majorVersion,
                                                          candidate->minorVersion);
            if (vmajor)
                *vmajor = candidate->majorVersion;
            if (vminor)
                *vminor = candidate->minorVersion;
            if (type_return)
                *type_return = returnType;
            return returnType.isValid();
        }
    } else if (!isLibrary && !localDirectoryPath.isEmpty()) {
        QString qmlUrl;
        bool exists = false;

        const QString urlsToTry[2] = {
            typeStr + dotqml_string, // Type -> Type.qml
            typeStr + dotuidotqml_string // Type -> Type.ui.qml
        };
        for (uint i = 0; i < sizeof(urlsToTry) / sizeof(urlsToTry[0]); ++i) {
            exists = typeLoader->fileExists(localDirectoryPath, urlsToTry[i]);
            if (exists) {
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
                // don't let function.qml confuse the use of "new Function(...)" for example.
                if (!QQml_isFileCaseCorrect(localDirectoryPath + urlsToTry[i])) {
                    exists = false;
                    if (errors) {
                        QQmlError caseError;
                        caseError.setDescription(QLatin1String("File name case mismatch"));
                        errors->append(caseError);
                    }
                    break;
                }
#else
                Q_UNUSED(errors);
#endif
                qmlUrl = url + urlsToTry[i];
                break;
            }
        }

        if (exists) {
            if (recursionRestriction == QQmlImport::PreventRecursion && base && (*base == qmlUrl)) { // no recursion
                if (typeRecursionDetected)
                    *typeRecursionDetected = true;
            } else {
                QQmlType returnType = fetchOrCreateTypeForUrl(
                            qmlUrl, type, registrationType == QQmlType::CompositeSingletonType, errors);
                if (type_return)
                    *type_return = returnType;
                return returnType.isValid();
            }
        }
    }

    return false;
}

bool QQmlImportsPrivate::resolveType(const QHashedStringRef& type, int *vmajor, int *vminor,
                                     QQmlType *type_return, QList<QQmlError> *errors,
                                     QQmlType::RegistrationType registrationType,
                                     QQmlImport::RecursionRestriction recursionRestriction)
{
    QQmlImportNamespace *s = nullptr;
    int dot = type.indexOf(Dot);
    if (dot >= 0) {
        QHashedStringRef namespaceName(type.constData(), dot);
        s = findQualifiedNamespace(namespaceName);
        if (!s) {
            if (errors) {
                QQmlError error;
                error.setDescription(QQmlImportDatabase::tr("- %1 is not a namespace").arg(namespaceName.toString()));
                errors->prepend(error);
            }
            return false;
        }
        int ndot = type.indexOf(Dot,dot+1);
        if (ndot > 0) {
            if (errors) {
                QQmlError error;
                error.setDescription(QQmlImportDatabase::tr("- nested namespaces not allowed"));
                errors->prepend(error);
            }
            return false;
        }
    } else {
        s = &unqualifiedset;
    }
    QHashedStringRef unqualifiedtype = dot < 0 ? type : QHashedStringRef(type.constData()+dot+1, type.length()-dot-1);
    if (s) {
        if (s->resolveType(typeLoader, unqualifiedtype, vmajor, vminor, type_return, &base, errors,
                           registrationType, recursionRestriction))
            return true;
        if (s->imports.count() == 1 && !s->imports.at(0)->isLibrary && type_return && s != &unqualifiedset) {
            // qualified, and only 1 url
            *type_return = fetchOrCreateTypeForUrl(resolveLocalUrl(s->imports.at(0)->url, unqualifiedtype.toString() + QLatin1String(".qml")), type, false, errors);
            return type_return->isValid();
        }
    }

    return false;
}

QQmlImportInstance *QQmlImportNamespace::findImport(const QString &uri) const
{
    for (QQmlImportInstance *import : imports) {
        if (import->uri == uri)
            return import;
    }
    return nullptr;
}

bool QQmlImportNamespace::resolveType(QQmlTypeLoader *typeLoader, const QHashedStringRef &type,
                                      int *vmajor, int *vminor, QQmlType *type_return,
                                      QString *base, QList<QQmlError> *errors,
                                      QQmlType::RegistrationType registrationType,
                                      QQmlImport::RecursionRestriction recursionRestriction)
{
    bool typeRecursionDetected = false;
    for (int i=0; i<imports.count(); ++i) {
        const QQmlImportInstance *import = imports.at(i);
        if (import->resolveType(typeLoader, type, vmajor, vminor, type_return, base,
                                &typeRecursionDetected, registrationType, recursionRestriction, errors)) {
            if (qmlCheckTypes()) {
                // check for type clashes
                for (int j = i+1; j<imports.count(); ++j) {
                    const QQmlImportInstance *import2 = imports.at(j);
                    if (import2->resolveType(typeLoader, type, vmajor, vminor, nullptr, base,
                                             nullptr, registrationType)) {
                        if (errors) {
                            QString u1 = import->url;
                            QString u2 = import2->url;
                            if (base) {
                                QStringRef b(base);
                                int dot = b.lastIndexOf(Dot);
                                if (dot >= 0) {
                                    b = b.left(dot+1);
                                    QStringRef l = b.left(dot);
                                    if (u1.startsWith(b))
                                        u1 = u1.mid(b.count());
                                    else if (u1 == l)
                                        u1 = QQmlImportDatabase::tr("local directory");
                                    if (u2.startsWith(b))
                                        u2 = u2.mid(b.count());
                                    else if (u2 == l)
                                        u2 = QQmlImportDatabase::tr("local directory");
                                }
                            }

                            QQmlError error;
                            if (u1 != u2) {
                                error.setDescription(QQmlImportDatabase::tr("is ambiguous. Found in %1 and in %2").arg(u1).arg(u2));
                            } else {
                                error.setDescription(QQmlImportDatabase::tr("is ambiguous. Found in %1 in version %2.%3 and %4.%5")
                                                        .arg(u1)
                                                        .arg(import->majversion).arg(import->minversion)
                                                        .arg(import2->majversion).arg(import2->minversion));
                            }
                            errors->prepend(error);
                        }
                        return false;
                    }
                }
            }
            return true;
        }
    }
    if (errors) {
        QQmlError error;
        if (typeRecursionDetected)
            error.setDescription(QQmlImportDatabase::tr("is instantiated recursively"));
        else
            error.setDescription(QQmlImportDatabase::tr("is not a type"));
        errors->prepend(error);
    }
    return false;
}

QQmlImportsPrivate::QQmlImportsPrivate(QQmlTypeLoader *loader)
: ref(1), typeLoader(loader) {
}

QQmlImportsPrivate::~QQmlImportsPrivate()
{
    while (QQmlImportNamespace *ns = qualifiedSets.takeFirst())
        delete ns;
}

QQmlImportNamespace *QQmlImportsPrivate::findQualifiedNamespace(const QHashedStringRef &prefix) const
{
    for (QQmlImportNamespace *ns = qualifiedSets.first(); ns; ns = qualifiedSets.next(ns)) {
        if (prefix == ns->prefix)
            return ns;
    }
    return nullptr;
}

/*
    Returns the list of possible versioned URI combinations. For example, if \a uri is
    QtQml.Models, \a vmaj is 2, and \a vmin is 0, this method returns the following:
    [QtQml.Models.2.0, QtQml.2.0.Models, QtQml.Models.2, QtQml.2.Models, QtQml.Models]
 */
static QStringList versionUriList(const QString &uri, int vmaj, int vmin)
{
    QStringList result;
    for (int version = QQmlImports::FullyVersioned; version <= QQmlImports::Unversioned; ++version) {
        int index = uri.length();
        do {
            QString versionUri = uri;
            versionUri.insert(index, QQmlImports::versionString(vmaj, vmin, static_cast<QQmlImports::ImportVersion>(version)));
            result += versionUri;

            index = uri.lastIndexOf(Dot, index - 1);
        } while (index > 0 && version != QQmlImports::Unversioned);
    }
    return result;
}

static QVector<QStaticPlugin> makePlugins()
{
    QVector<QStaticPlugin> plugins;
    // To avoid traversing all static plugins for all imports, we cut down
    // the list the first time called to only contain QML plugins:
    const auto staticPlugins = QPluginLoader::staticPlugins();
    for (const QStaticPlugin &plugin : staticPlugins) {
        const QString iid = plugin.metaData().value(QLatin1String("IID")).toString();
        if (iid == QLatin1String(QQmlExtensionInterface_iid) || iid == QLatin1String(QQmlExtensionInterface_iid_old)) {
            plugins.append(plugin);
        }
    }
    return plugins;
}

/*
    Get all static plugins that are QML plugins and has a meta data URI that matches with one of
    \a versionUris, which is a list of all possible versioned URI combinations - see versionUriList()
    above.
 */
bool QQmlImportsPrivate::populatePluginPairVector(QVector<StaticPluginPair> &result, const QString &uri, const QStringList &versionUris,
                                                      const QString &qmldirPath, QList<QQmlError> *errors)
{
    static const QVector<QStaticPlugin> plugins = makePlugins();
    for (const QStaticPlugin &plugin : plugins) {
        // Since a module can list more than one plugin, we keep iterating even after we found a match.
        if (QQmlExtensionPlugin *instance = qobject_cast<QQmlExtensionPlugin *>(plugin.instance())) {
            const QJsonArray metaTagsUriList = plugin.metaData().value(QLatin1String("uri")).toArray();
            if (metaTagsUriList.isEmpty()) {
                if (errors) {
                    QQmlError error;
                    error.setDescription(QQmlImportDatabase::tr("static plugin for module \"%1\" with name \"%2\" has no metadata URI")
                                         .arg(uri).arg(QString::fromUtf8(instance->metaObject()->className())));
                    error.setUrl(QUrl::fromLocalFile(qmldirPath));
                    errors->prepend(error);
                }
                return false;
            }
            // A plugin can be set up to handle multiple URIs, so go through the list:
            for (const QJsonValue &metaTagUri : metaTagsUriList) {
                if (versionUris.contains(metaTagUri.toString())) {
                    result.append(qMakePair(plugin, metaTagsUriList));
                    break;
                }
            }
        }
    }
    return true;
}

#if defined(QT_SHARED) || !QT_CONFIG(library)
static inline QString msgCannotLoadPlugin(const QString &uri, const QString &why)
{
    return QQmlImportDatabase::tr("plugin cannot be loaded for module \"%1\": %2").arg(uri, why);
}
#endif

/*
Import an extension defined by a qmldir file.

\a qmldirFilePath is a raw file path.
*/
bool QQmlImportsPrivate::importExtension(const QString &qmldirFilePath,
                                         const QString &uri,
                                         int vmaj, int vmin,
                                         QQmlImportDatabase *database,
                                         const QQmlTypeLoaderQmldirContent &qmldir,
                                         QList<QQmlError> *errors)
{
    Q_ASSERT(qmldir.hasContent());

    if (qmlImportTrace())
        qDebug().nospace() << "QQmlImports(" << qPrintable(base) << ")::importExtension: "
                           << "loaded " << qmldirFilePath;

    if (designerSupportRequired && !qmldir.designerSupported()) {
        if (errors) {
            QQmlError error;
            error.setDescription(QQmlImportDatabase::tr("module does not support the designer \"%1\"").arg(qmldir.typeNamespace()));
            error.setUrl(QUrl::fromLocalFile(qmldirFilePath));
            errors->prepend(error);
        }
        return false;
    }

    int qmldirPluginCount = qmldir.plugins().count();
    if (qmldirPluginCount == 0)
        return true;

    if (!database->qmlDirFilesForWhichPluginsHaveBeenLoaded.contains(qmldirFilePath)) {
        // First search for listed qmldir plugins dynamically. If we cannot resolve them all, we continue
        // searching static plugins that has correct metadata uri. Note that since we only know the uri
        // for a static plugin, and not the filename, we cannot know which static plugin belongs to which
        // listed plugin inside qmldir. And for this reason, mixing dynamic and static plugins inside a
        // single module is not recommended.

        QString typeNamespace = qmldir.typeNamespace();
        QString qmldirPath = qmldirFilePath;
        int slash = qmldirPath.lastIndexOf(Slash);
        if (slash > 0)
            qmldirPath.truncate(slash);

        int dynamicPluginsFound = 0;
        int staticPluginsFound = 0;

#if defined(QT_SHARED)
        const auto qmldirPlugins = qmldir.plugins();
        for (const QQmlDirParser::Plugin &plugin : qmldirPlugins) {
            QString resolvedFilePath = database->resolvePlugin(typeLoader, qmldirPath, plugin.path, plugin.name);
            if (!resolvedFilePath.isEmpty()) {
                dynamicPluginsFound++;
                if (!database->importDynamicPlugin(resolvedFilePath, uri, typeNamespace, vmaj, errors)) {
                    if (errors) {
                        // XXX TODO: should we leave the import plugin error alone?
                        // Here, we pop it off the top and coalesce it into this error's message.
                        // The reason is that the lower level may add url and line/column numbering information.
                        QQmlError poppedError = errors->takeFirst();
                        QQmlError error;
                        error.setDescription(msgCannotLoadPlugin(uri, poppedError.description()));
                        error.setUrl(QUrl::fromLocalFile(qmldirFilePath));
                        errors->prepend(error);
                    }
                    return false;
                }
            }
        }
#endif // QT_SHARED

        if (dynamicPluginsFound < qmldirPluginCount) {
            // Check if the missing plugins can be resolved statically. We do this by looking at
            // the URIs embedded in a plugins meta data. Since those URIs can be anything from fully
            // versioned to unversioned, we need to compare with differnt version strings. If a module
            // has several plugins, they must all have the same version. Start by populating pluginPairs
            // with relevant plugins to cut the list short early on:
            const QStringList versionUris = versionUriList(uri, vmaj, vmin);
            QVector<StaticPluginPair> pluginPairs;
            if (!populatePluginPairVector(pluginPairs, uri, versionUris, qmldirFilePath, errors))
                return false;

            const QString basePath = QFileInfo(qmldirPath).absoluteFilePath();
            for (const QString &versionUri : versionUris) {
                for (const StaticPluginPair &pair : qAsConst(pluginPairs)) {
                    for (const QJsonValue &metaTagUri : pair.second) {
                        if (versionUri == metaTagUri.toString()) {
                            staticPluginsFound++;
                            QObject *instance = pair.first.instance();
                            if (!database->importStaticPlugin(instance, basePath, uri, typeNamespace, vmaj, errors)) {
                                if (errors) {
                                    QQmlError poppedError = errors->takeFirst();
                                    QQmlError error;
                                    error.setDescription(QQmlImportDatabase::tr("static plugin for module \"%1\" with name \"%2\" cannot be loaded: %3")
                                                         .arg(uri).arg(QString::fromUtf8(instance->metaObject()->className())).arg(poppedError.description()));
                                    error.setUrl(QUrl::fromLocalFile(qmldirFilePath));
                                    errors->prepend(error);
                                }
                                return false;
                            }
                            break;
                        }
                    }
                }
                if (staticPluginsFound > 0)
                    break;
            }
        }

        if ((dynamicPluginsFound + staticPluginsFound) < qmldirPluginCount) {
            if (errors) {
                QQmlError error;
                if (qmldirPluginCount > 1 && staticPluginsFound > 0)
                    error.setDescription(QQmlImportDatabase::tr("could not resolve all plugins for module \"%1\"").arg(uri));
                else
                    error.setDescription(QQmlImportDatabase::tr("module \"%1\" plugin \"%2\" not found").arg(uri).arg(qmldir.plugins()[dynamicPluginsFound].name));
                error.setUrl(QUrl::fromLocalFile(qmldirFilePath));
                errors->prepend(error);
            }
            return false;
        }

        database->qmlDirFilesForWhichPluginsHaveBeenLoaded.insert(qmldirFilePath);
    }
    return true;
}

bool QQmlImportsPrivate::getQmldirContent(const QString &qmldirIdentifier, const QString &uri,
                                          QQmlTypeLoaderQmldirContent *qmldir, QList<QQmlError> *errors)
{
    Q_ASSERT(errors);
    Q_ASSERT(qmldir);

    *qmldir = typeLoader->qmldirContent(qmldirIdentifier);
    if ((*qmldir).hasContent()) {
        // Ensure that parsing was successful
        if ((*qmldir).hasError()) {
            QUrl url = QUrl::fromLocalFile(qmldirIdentifier);
            const QList<QQmlError> qmldirErrors = (*qmldir).errors(uri);
            for (int i = 0; i < qmldirErrors.size(); ++i) {
                QQmlError error = qmldirErrors.at(i);
                error.setUrl(url);
                errors->append(error);
            }
            return false;
        }
    }

    return true;
}

QString QQmlImportsPrivate::resolvedUri(const QString &dir_arg, QQmlImportDatabase *database)
{
    QString dir = dir_arg;
    if (dir.endsWith(Slash) || dir.endsWith(Backslash))
        dir.chop(1);

    QStringList paths = database->fileImportPath;
    if (!paths.isEmpty())
        std::sort(paths.begin(), paths.end(), std::greater<QString>()); // Ensure subdirs preceed their parents.

    QString stableRelativePath = dir;
    for (const QString &path : qAsConst(paths)) {
        if (dir.startsWith(path)) {
            stableRelativePath = dir.mid(path.length()+1);
            break;
        }
    }

    stableRelativePath.replace(Backslash, Slash);

    // remove optional versioning in dot notation from uri
    int versionDot = stableRelativePath.lastIndexOf(Dot);
    if (versionDot >= 0) {
        int nextSlash = stableRelativePath.indexOf(Slash, versionDot);
        if (nextSlash >= 0)
            stableRelativePath.remove(versionDot, nextSlash - versionDot);
        else
            stableRelativePath = stableRelativePath.left(versionDot);
    }

    stableRelativePath.replace(Slash, Dot);

    return stableRelativePath;
}

/*
Locates the qmldir file for \a uri version \a vmaj.vmin.  Returns true if found,
and fills in outQmldirFilePath and outQmldirUrl appropriately.  Otherwise returns
false.
*/
bool QQmlImportsPrivate::locateQmldir(const QString &uri, int vmaj, int vmin, QQmlImportDatabase *database,
                                      QString *outQmldirFilePath, QString *outQmldirPathUrl)
{
    Q_ASSERT(vmaj >= 0 && vmin >= 0); // Versions are always specified for libraries

    // Check cache first

    QQmlImportDatabase::QmldirCache *cacheHead = nullptr;
    {
    QQmlImportDatabase::QmldirCache **cachePtr = database->qmldirCache.value(uri);
    if (cachePtr) {
        cacheHead = *cachePtr;
        QQmlImportDatabase::QmldirCache *cache = cacheHead;
        while (cache) {
            if (cache->versionMajor == vmaj && cache->versionMinor == vmin) {
                *outQmldirFilePath = cache->qmldirFilePath;
                *outQmldirPathUrl = cache->qmldirPathUrl;
                return !cache->qmldirFilePath.isEmpty();
            }
            cache = cache->next;
        }
    }
    }

    QQmlTypeLoader &typeLoader = QQmlEnginePrivate::get(database->engine)->typeLoader;

    // Interceptor might redirect remote files to local ones.
    QQmlAbstractUrlInterceptor *interceptor = typeLoader.engine()->urlInterceptor();
    QStringList localImportPaths = database->importPathList(
                interceptor ? QQmlImportDatabase::LocalOrRemote : QQmlImportDatabase::Local);

    // Search local import paths for a matching version
    const QStringList qmlDirPaths = QQmlImports::completeQmldirPaths(uri, localImportPaths, vmaj, vmin);
    for (QString qmldirPath : qmlDirPaths) {
        if (interceptor) {
            qmldirPath = QQmlFile::urlToLocalFileOrQrc(
                        interceptor->intercept(QQmlImports::urlFromLocalFileOrQrcOrUrl(qmldirPath),
                                               QQmlAbstractUrlInterceptor::QmldirFile));
        }

        QString absoluteFilePath = typeLoader.absoluteFilePath(qmldirPath);
        if (!absoluteFilePath.isEmpty()) {
            QString url;
            const QStringRef absolutePath = absoluteFilePath.leftRef(absoluteFilePath.lastIndexOf(Slash) + 1);
            if (absolutePath.at(0) == Colon)
                url = QLatin1String("qrc") + absolutePath;
            else
                url = QUrl::fromLocalFile(absolutePath.toString()).toString();

            QQmlImportDatabase::QmldirCache *cache = new QQmlImportDatabase::QmldirCache;
            cache->versionMajor = vmaj;
            cache->versionMinor = vmin;
            cache->qmldirFilePath = absoluteFilePath;
            cache->qmldirPathUrl = url;
            cache->next = cacheHead;
            database->qmldirCache.insert(uri, cache);

            *outQmldirFilePath = absoluteFilePath;
            *outQmldirPathUrl = url;

            return true;
        }
    }

    QQmlImportDatabase::QmldirCache *cache = new QQmlImportDatabase::QmldirCache;
    cache->versionMajor = vmaj;
    cache->versionMinor = vmin;
    cache->next = cacheHead;
    database->qmldirCache.insert(uri, cache);

    return false;
}

bool QQmlImportsPrivate::validateQmldirVersion(const QQmlTypeLoaderQmldirContent &qmldir, const QString &uri, int vmaj, int vmin,
                                               QList<QQmlError> *errors)
{
    int lowest_min = INT_MAX;
    int highest_min = INT_MIN;

    typedef QQmlDirComponents::const_iterator ConstIterator;
    const QQmlDirComponents &components = qmldir.components();

    ConstIterator cend = components.constEnd();
    for (ConstIterator cit = components.constBegin(); cit != cend; ++cit) {
        for (ConstIterator cit2 = components.constBegin(); cit2 != cit; ++cit2) {
            if ((cit2->typeName == cit->typeName) &&
                (cit2->majorVersion == cit->majorVersion) &&
                (cit2->minorVersion == cit->minorVersion)) {
                // This entry clashes with a predecessor
                QQmlError error;
                error.setDescription(QQmlImportDatabase::tr("\"%1\" version %2.%3 is defined more than once in module \"%4\"")
                                     .arg(cit->typeName).arg(cit->majorVersion).arg(cit->minorVersion).arg(uri));
                errors->prepend(error);
                return false;
            }
        }

        if (cit->majorVersion == vmaj) {
            lowest_min = qMin(lowest_min, cit->minorVersion);
            highest_min = qMax(highest_min, cit->minorVersion);
        }
    }

    typedef QList<QQmlDirParser::Script>::const_iterator SConstIterator;
    const QQmlDirScripts &scripts = qmldir.scripts();

    SConstIterator send = scripts.constEnd();
    for (SConstIterator sit = scripts.constBegin(); sit != send; ++sit) {
        for (SConstIterator sit2 = scripts.constBegin(); sit2 != sit; ++sit2) {
            if ((sit2->nameSpace == sit->nameSpace) &&
                (sit2->majorVersion == sit->majorVersion) &&
                (sit2->minorVersion == sit->minorVersion)) {
                // This entry clashes with a predecessor
                QQmlError error;
                error.setDescription(QQmlImportDatabase::tr("\"%1\" version %2.%3 is defined more than once in module \"%4\"")
                                     .arg(sit->nameSpace).arg(sit->majorVersion).arg(sit->minorVersion).arg(uri));
                errors->prepend(error);
                return false;
            }
        }

        if (sit->majorVersion == vmaj) {
            lowest_min = qMin(lowest_min, sit->minorVersion);
            highest_min = qMax(highest_min, sit->minorVersion);
        }
    }

    if (lowest_min > vmin || highest_min < vmin) {
        QQmlError error;
        error.setDescription(QQmlImportDatabase::tr("module \"%1\" version %2.%3 is not installed").arg(uri).arg(vmaj).arg(vmin));
        errors->prepend(error);
        return false;
    }

    return true;
}

QQmlImportNamespace *QQmlImportsPrivate::importNamespace(const QString &prefix) const
{
    QQmlImportNamespace *nameSpace = nullptr;

    if (prefix.isEmpty()) {
        nameSpace = &unqualifiedset;
    } else {
        nameSpace = findQualifiedNamespace(prefix);

        if (!nameSpace) {
            nameSpace = new QQmlImportNamespace;
            nameSpace->prefix = prefix;
            qualifiedSets.append(nameSpace);
        }
    }

    return nameSpace;
}

QQmlImportInstance *QQmlImportsPrivate::addImportToNamespace(QQmlImportNamespace *nameSpace,
                                                                      const QString &uri, const QString &url, int vmaj, int vmin,
                                                                      QV4::CompiledData::Import::ImportType type,
                                                                      QList<QQmlError> *errors, bool lowPrecedence)
{
    Q_ASSERT(nameSpace);
    Q_ASSERT(errors);
    Q_UNUSED(errors);
    Q_ASSERT(url.isEmpty() || url.endsWith(Slash));

    QQmlImportInstance *import = new QQmlImportInstance;
    import->uri = uri;
    import->url = url;
    import->localDirectoryPath = QQmlFile::urlToLocalFileOrQrc(url);
    import->majversion = vmaj;
    import->minversion = vmin;
    import->isLibrary = (type == QV4::CompiledData::Import::ImportLibrary);

    if (lowPrecedence)
        nameSpace->imports.append(import);
    else
        nameSpace->imports.prepend(import);

    return import;
}

bool QQmlImportsPrivate::addLibraryImport(const QString& uri, const QString &prefix,
                                          int vmaj, int vmin, const QString &qmldirIdentifier, const QString &qmldirUrl, bool incomplete,
                                          QQmlImportDatabase *database,
                                          QList<QQmlError> *errors)
{
    Q_ASSERT(database);
    Q_ASSERT(errors);

    QQmlImportNamespace *nameSpace = importNamespace(prefix);
    Q_ASSERT(nameSpace);

    QQmlImportInstance *inserted = addImportToNamespace(nameSpace, uri, qmldirUrl, vmaj, vmin, QV4::CompiledData::Import::ImportLibrary, errors);
    Q_ASSERT(inserted);

    if (!incomplete) {
        QQmlTypeLoaderQmldirContent qmldir;

        if (!qmldirIdentifier.isEmpty()) {
            if (!getQmldirContent(qmldirIdentifier, uri, &qmldir, errors))
                return false;

            if (qmldir.hasContent()) {
                if (!importExtension(qmldir.pluginLocation(), uri, vmaj, vmin, database, qmldir, errors))
                    return false;

                if (!inserted->setQmldirContent(qmldirUrl, qmldir, nameSpace, errors))
                    return false;
            }
        }

        // Ensure that we are actually providing something
        if ((vmaj < 0) || (vmin < 0) || !QQmlMetaType::isModule(uri, vmaj, vmin)) {
            if (inserted->qmlDirComponents.isEmpty() && inserted->qmlDirScripts.isEmpty()) {
                QQmlError error;
                if (QQmlMetaType::isAnyModule(uri))
                    error.setDescription(QQmlImportDatabase::tr("module \"%1\" version %2.%3 is not installed").arg(uri).arg(vmaj).arg(vmin));
                else
                    error.setDescription(QQmlImportDatabase::tr("module \"%1\" is not installed").arg(uri));
                errors->prepend(error);
                return false;
            } else if ((vmaj >= 0) && (vmin >= 0) && qmldir.hasContent()) {
                // Verify that the qmldir content is valid for this version
                if (!validateQmldirVersion(qmldir, uri, vmaj, vmin, errors))
                    return false;
            }
        }
    }

    return true;
}

bool QQmlImportsPrivate::addFileImport(const QString& uri, const QString &prefix,
                                       int vmaj, int vmin,
                                       bool isImplicitImport, bool incomplete, QQmlImportDatabase *database,
                                       QList<QQmlError> *errors)
{
    Q_ASSERT(errors);

    QQmlImportNamespace *nameSpace = importNamespace(prefix);
    Q_ASSERT(nameSpace);

    // The uri for this import.  For library imports this is the same as uri
    // specified by the user, but it may be different in the case of file imports.
    QString importUri = uri;
    QString qmldirUrl = resolveLocalUrl(base, importUri + (importUri.endsWith(Slash)
                                                           ? String_qmldir
                                                           : Slash_qmldir));
    if (QQmlAbstractUrlInterceptor *interceptor = typeLoader->engine()->urlInterceptor()) {
        qmldirUrl = interceptor->intercept(QUrl(qmldirUrl),
                                           QQmlAbstractUrlInterceptor::QmldirFile).toString();
    }
    QString qmldirIdentifier;

    if (QQmlFile::isLocalFile(qmldirUrl)) {

        QString localFileOrQrc = QQmlFile::urlToLocalFileOrQrc(qmldirUrl);
        Q_ASSERT(!localFileOrQrc.isEmpty());

        const QString dir = localFileOrQrc.left(localFileOrQrc.lastIndexOf(Slash) + 1);
        if (!typeLoader->directoryExists(dir)) {
            if (!isImplicitImport) {
                QQmlError error;
                error.setDescription(QQmlImportDatabase::tr("\"%1\": no such directory").arg(uri));
                error.setUrl(QUrl(qmldirUrl));
                errors->prepend(error);
            }
            return false;
        }

        // Transforms the (possible relative) uri into our best guess relative to the
        // import paths.
        importUri = resolvedUri(dir, database);
        if (importUri.endsWith(Slash))
            importUri.chop(1);

        if (!typeLoader->absoluteFilePath(localFileOrQrc).isEmpty())
            qmldirIdentifier = localFileOrQrc;

    } else if (nameSpace->prefix.isEmpty() && !incomplete) {

        if (!isImplicitImport) {
            QQmlError error;
            error.setDescription(QQmlImportDatabase::tr("import \"%1\" has no qmldir and no namespace").arg(importUri));
            error.setUrl(QUrl(qmldirUrl));
            errors->prepend(error);
        }

        return false;

    }

    // The url for the path containing files for this import
    QString url = resolveLocalUrl(base, uri);
    if (!url.endsWith(Slash) && !url.endsWith(Backslash))
        url += Slash;

    // ### For enum support, we are now adding the implicit import always (and earlier). Bail early
    //     if the implicit import has already been explicitly added, otherwise we can run into issues
    //     with duplicate imports. However remember that we attempted to add this as implicit import, to
    //     allow for the loading of internal types.
    if (isImplicitImport) {
        for (QList<QQmlImportInstance *>::const_iterator it = nameSpace->imports.constBegin();
             it != nameSpace->imports.constEnd(); ++it) {
            if ((*it)->url == url) {
                (*it)->implicitlyImported = true;
                return true;
            }
        }
    }

    QQmlImportInstance *inserted = addImportToNamespace(nameSpace, importUri, url, vmaj, vmin, QV4::CompiledData::Import::ImportFile, errors, isImplicitImport);
    Q_ASSERT(inserted);

    if (!incomplete && !qmldirIdentifier.isEmpty()) {
        QQmlTypeLoaderQmldirContent qmldir;
        if (!getQmldirContent(qmldirIdentifier, importUri, &qmldir, errors))
            return false;

        if (qmldir.hasContent()) {
            if (!importExtension(qmldir.pluginLocation(), importUri, vmaj, vmin, database, qmldir, errors))
                return false;

            if (!inserted->setQmldirContent(url, qmldir, nameSpace, errors))
                return false;
        }
    }

    return true;
}

bool QQmlImportsPrivate::updateQmldirContent(const QString &uri, const QString &prefix,
                                             const QString &qmldirIdentifier, const QString& qmldirUrl,
                                             QQmlImportDatabase *database, QList<QQmlError> *errors)
{
    QQmlImportNamespace *nameSpace = importNamespace(prefix);
    Q_ASSERT(nameSpace);

    if (QQmlImportInstance *import = nameSpace->findImport(uri)) {
        QQmlTypeLoaderQmldirContent qmldir;
        if (!getQmldirContent(qmldirIdentifier, uri, &qmldir, errors))
            return false;

        if (qmldir.hasContent()) {
            int vmaj = import->majversion;
            int vmin = import->minversion;
            if (!importExtension(qmldir.pluginLocation(), uri, vmaj, vmin, database, qmldir, errors))
                return false;

            if (import->setQmldirContent(qmldirUrl, qmldir, nameSpace, errors)) {
                if (import->qmlDirComponents.isEmpty() && import->qmlDirScripts.isEmpty()) {
                    // The implicit import qmldir can be empty, and plugins have no extra versions
                    if (uri != QLatin1String(".") && !QQmlMetaType::isModule(uri, vmaj, vmin)) {
                        QQmlError error;
                        if (QQmlMetaType::isAnyModule(uri))
                            error.setDescription(QQmlImportDatabase::tr("module \"%1\" version %2.%3 is not installed").arg(uri).arg(vmaj).arg(vmin));
                        else
                            error.setDescription(QQmlImportDatabase::tr("module \"%1\" is not installed").arg(uri));
                        errors->prepend(error);
                        return false;
                    }
                } else if ((vmaj >= 0) && (vmin >= 0)) {
                    // Verify that the qmldir content is valid for this version
                    if (!validateQmldirVersion(qmldir, uri, vmaj, vmin, errors))
                        return false;
                }
                return true;
            }
        }
    }

    if (errors->isEmpty()) {
        QQmlError error;
        error.setDescription(QQmlTypeLoader::tr("Cannot update qmldir content for '%1'").arg(uri));
        errors->prepend(error);
    }

    return false;
}

/*!
  \internal

  Adds an implicit "." file import.  This is equivalent to calling addFileImport(), but error
  messages related to the path or qmldir file not existing are suppressed.

  Additionally, this will add the import with lowest instead of highest precedence.
*/
bool QQmlImports::addImplicitImport(QQmlImportDatabase *importDb, QList<QQmlError> *errors)
{
    Q_ASSERT(errors);

    if (qmlImportTrace())
        qDebug().nospace() << "QQmlImports(" << qPrintable(baseUrl().toString())
                           << ")::addImplicitImport";

    bool incomplete = !isLocal(baseUrl());
    return d->addFileImport(QLatin1String("."), QString(), -1, -1, true, incomplete, importDb, errors);
}

/*!
  \internal

  Adds information to \a imports such that subsequent calls to resolveType()
  will resolve types qualified by \a prefix by considering types found at the given \a uri.

  The uri is either a directory (if importType is FileImport), or a URI resolved using paths
  added via addImportPath() (if importType is LibraryImport).

  The \a prefix may be empty, in which case the import location is considered for
  unqualified types.

  The base URL must already have been set with Import::setBaseUrl().

  Optionally, the url the import resolved to can be returned by providing the url parameter.
  Not all imports will result in an output url being generated, in which case the url will
  be set to an empty string.

  Returns true on success, and false on failure.  In case of failure, the errors array will
  filled appropriately.
*/
bool QQmlImports::addFileImport(QQmlImportDatabase *importDb,
                                const QString& uri, const QString& prefix, int vmaj, int vmin,
                                bool incomplete, QList<QQmlError> *errors)
{
    Q_ASSERT(importDb);
    Q_ASSERT(errors);

    if (qmlImportTrace())
        qDebug().nospace() << "QQmlImports(" << qPrintable(baseUrl().toString()) << ')' << "::addFileImport: "
                           << uri << ' ' << vmaj << '.' << vmin << " as " << prefix;

    return d->addFileImport(uri, prefix, vmaj, vmin, false, incomplete, importDb, errors);
}

bool QQmlImports::addLibraryImport(QQmlImportDatabase *importDb,
                                   const QString &uri, const QString &prefix, int vmaj, int vmin,
                                   const QString &qmldirIdentifier, const QString& qmldirUrl, bool incomplete, QList<QQmlError> *errors)
{
    Q_ASSERT(importDb);
    Q_ASSERT(errors);

    if (qmlImportTrace())
        qDebug().nospace() << "QQmlImports(" << qPrintable(baseUrl().toString()) << ')' << "::addLibraryImport: "
                           << uri << ' ' << vmaj << '.' << vmin << " as " << prefix;

    return d->addLibraryImport(uri, prefix, vmaj, vmin, qmldirIdentifier, qmldirUrl, incomplete, importDb, errors);
}

bool QQmlImports::updateQmldirContent(QQmlImportDatabase *importDb,
                                      const QString &uri, const QString &prefix,
                                      const QString &qmldirIdentifier, const QString& qmldirUrl, QList<QQmlError> *errors)
{
    Q_ASSERT(importDb);
    Q_ASSERT(errors);

    if (qmlImportTrace())
        qDebug().nospace() << "QQmlImports(" << qPrintable(baseUrl().toString()) << ')' << "::updateQmldirContent: "
                           << uri << " to " << qmldirUrl << " as " << prefix;

    return d->updateQmldirContent(uri, prefix, qmldirIdentifier, qmldirUrl, importDb, errors);
}

bool QQmlImports::locateQmldir(QQmlImportDatabase *importDb,
                               const QString& uri, int vmaj, int vmin,
                               QString *qmldirFilePath, QString *url)
{
    return d->locateQmldir(uri, vmaj, vmin, importDb, qmldirFilePath, url);
}

bool QQmlImports::isLocal(const QString &url)
{
    return !QQmlFile::urlToLocalFileOrQrc(url).isEmpty();
}

bool QQmlImports::isLocal(const QUrl &url)
{
    return !QQmlFile::urlToLocalFileOrQrc(url).isEmpty();
}

QUrl QQmlImports::urlFromLocalFileOrQrcOrUrl(const QString &file)
{
    QUrl url(QLatin1String(file.at(0) == Colon ? "qrc" : "") + file);

    // We don't support single character schemes as those conflict with windows drive letters.
    if (url.scheme().length() < 2)
        return QUrl::fromLocalFile(file);
    return url;
}

void QQmlImports::setDesignerSupportRequired(bool b)
{
    designerSupportRequired = b;
}


/*!
\class QQmlImportDatabase
\brief The QQmlImportDatabase class manages the QML imports for a QQmlEngine.
\internal
*/
QQmlImportDatabase::QQmlImportDatabase(QQmlEngine *e)
: engine(e)
{
    filePluginPath << QLatin1String(".");
    // Search order is applicationDirPath(), qrc:/qt-project.org/imports, $QML2_IMPORT_PATH, QLibraryInfo::Qml2ImportsPath

    QString installImportsPath = QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath);
    addImportPath(installImportsPath);

    // env import paths
    if (Q_UNLIKELY(!qEnvironmentVariableIsEmpty("QML2_IMPORT_PATH"))) {
        const QString envImportPath = qEnvironmentVariable("QML2_IMPORT_PATH");
#if defined(Q_OS_WIN)
        QLatin1Char pathSep(';');
#else
        QLatin1Char pathSep(':');
#endif
        QStringList paths = envImportPath.split(pathSep, QString::SkipEmptyParts);
        for (int ii = paths.count() - 1; ii >= 0; --ii)
            addImportPath(paths.at(ii));
    }

    addImportPath(QStringLiteral("qrc:/qt-project.org/imports"));
    addImportPath(QCoreApplication::applicationDirPath());
}

QQmlImportDatabase::~QQmlImportDatabase()
{
    clearDirCache();
}

/*!
  \internal

  Returns the result of the merge of \a baseName with \a path, \a suffixes, and \a prefix.
  The \a prefix must contain the dot.

  \a qmldirPath is the location of the qmldir file.
 */
QString QQmlImportDatabase::resolvePlugin(QQmlTypeLoader *typeLoader,
                                          const QString &qmldirPath,
                                          const QString &qmldirPluginPath,
                                          const QString &baseName, const QStringList &suffixes,
                                          const QString &prefix)
{
    QStringList searchPaths = filePluginPath;
    bool qmldirPluginPathIsRelative = QDir::isRelativePath(qmldirPluginPath);
    if (!qmldirPluginPathIsRelative)
        searchPaths.prepend(qmldirPluginPath);

    for (const QString &pluginPath : qAsConst(searchPaths)) {
        QString resolvedPath;
        if (pluginPath == QLatin1String(".")) {
            if (qmldirPluginPathIsRelative && !qmldirPluginPath.isEmpty() && qmldirPluginPath != QLatin1String("."))
                resolvedPath = QDir::cleanPath(qmldirPath + Slash + qmldirPluginPath);
            else
                resolvedPath = qmldirPath;
        } else {
            if (QDir::isRelativePath(pluginPath))
                resolvedPath = QDir::cleanPath(qmldirPath + Slash + pluginPath);
            else
                resolvedPath = pluginPath;
        }

        // hack for resources, should probably go away
        if (resolvedPath.startsWith(Colon))
            resolvedPath = QCoreApplication::applicationDirPath();

        if (!resolvedPath.endsWith(Slash))
            resolvedPath += Slash;

        resolvedPath += prefix + baseName;
        for (const QString &suffix : suffixes) {
            const QString absolutePath = typeLoader->absoluteFilePath(resolvedPath + suffix);
            if (!absolutePath.isEmpty())
                return absolutePath;
        }
    }

    if (qmlImportTrace())
        qDebug() << "QQmlImportDatabase::resolvePlugin: Could not resolve plugin" << baseName
                 << "in" << qmldirPath;

    return QString();
}

/*!
  \internal

  Returns the result of the merge of \a baseName with \a dir and the platform suffix.

  \table
  \header \li Platform \li Valid suffixes
  \row \li Windows     \li \c .dll
  \row \li Unix/Linux  \li \c .so
  \row \li \macos    \li \c .dylib, \c .bundle, \c .so
  \endtable

  Version number on unix are ignored.
*/
QString QQmlImportDatabase::resolvePlugin(QQmlTypeLoader *typeLoader,
                                                  const QString &qmldirPath, const QString &qmldirPluginPath,
                                                  const QString &baseName)
{
#if defined(Q_OS_WIN)
    static const QString prefix;
    static const QStringList suffixes = {
# ifdef QT_DEBUG
        QLatin1String("d.dll"), // try a qmake-style debug build first
# endif
        QLatin1String(".dll")
    };
#elif defined(Q_OS_DARWIN)
    static const QString prefix = QLatin1String("lib");
    static const QStringList suffixes = {
# ifdef QT_DEBUG
        QLatin1String("_debug.dylib"), // try a qmake-style debug build first
        QLatin1String(".dylib"),
# else
        QLatin1String(".dylib"),
        QLatin1String("_debug.dylib"), // try a qmake-style debug build after
# endif
        QLatin1String(".so"),
        QLatin1String(".bundle")
    };
# else  // Unix
    static const QString prefix = QLatin1String("lib");
    static const QStringList suffixes = { QLatin1String(".so") };
#endif

    return resolvePlugin(typeLoader, qmldirPath, qmldirPluginPath, baseName, suffixes, prefix);
}

/*!
    \internal
*/
QStringList QQmlImportDatabase::pluginPathList() const
{
    return filePluginPath;
}

/*!
    \internal
*/
void QQmlImportDatabase::setPluginPathList(const QStringList &paths)
{
    if (qmlImportTrace())
        qDebug().nospace() << "QQmlImportDatabase::setPluginPathList: " << paths;

    filePluginPath = paths;
}

/*!
    \internal
*/
void QQmlImportDatabase::addPluginPath(const QString& path)
{
    if (qmlImportTrace())
        qDebug().nospace() << "QQmlImportDatabase::addPluginPath: " << path;

    QUrl url = QUrl(path);
    if (url.isRelative() || url.scheme() == QLatin1String("file")
            || (url.scheme().length() == 1 && QFile::exists(path)) ) {  // windows path
        QDir dir = QDir(path);
        filePluginPath.prepend(dir.canonicalPath());
    } else {
        filePluginPath.prepend(path);
    }
}

/*!
    \internal
*/
void QQmlImportDatabase::addImportPath(const QString& path)
{
    if (qmlImportTrace())
        qDebug().nospace() << "QQmlImportDatabase::addImportPath: " << path;

    if (path.isEmpty())
        return;

    QUrl url = QUrl(path);
    QString cPath;

    if (url.scheme() == QLatin1String("file")) {
        cPath = QQmlFile::urlToLocalFileOrQrc(url);
    } else if (path.startsWith(QLatin1Char(':'))) {
        // qrc directory, e.g. :/foo
        // need to convert to a qrc url, e.g. qrc:/foo
        cPath = QLatin1String("qrc") + path;
        cPath.replace(Backslash, Slash);
    } else if (url.isRelative() ||
               (url.scheme().length() == 1 && QFile::exists(path)) ) {  // windows path
        QDir dir = QDir(path);
        cPath = dir.canonicalPath();
    } else {
        cPath = path;
        cPath.replace(Backslash, Slash);
    }

    if (!cPath.isEmpty()
        && !fileImportPath.contains(cPath))
        fileImportPath.prepend(cPath);
}

/*!
    \internal
*/
QStringList QQmlImportDatabase::importPathList(PathType type) const
{
    if (type == LocalOrRemote)
        return fileImportPath;

    QStringList list;
    for (const QString &path : fileImportPath) {
        bool localPath = isPathAbsolute(path) || QQmlFile::isLocalFile(path);
        if (localPath == (type == Local))
            list.append(path);
    }

    return list;
}

/*!
    \internal
*/
void QQmlImportDatabase::setImportPathList(const QStringList &paths)
{
    if (qmlImportTrace())
        qDebug().nospace() << "QQmlImportDatabase::setImportPathList: " << paths;

    fileImportPath = paths;

    // Our existing cached paths may have been invalidated
    clearDirCache();
}

/*!
    \internal
*/
bool QQmlImportDatabase::registerPluginTypes(QObject *instance, const QString &basePath,
                                      const QString &uri, const QString &typeNamespace, int vmaj, QList<QQmlError> *errors)
{
    if (qmlImportTrace())
        qDebug().nospace() << "QQmlImportDatabase::registerPluginTypes: " << uri << " from " << basePath;

    QQmlTypesExtensionInterface *iface = qobject_cast<QQmlTypesExtensionInterface *>(instance);
    if (!iface) {
        if (errors) {
            QQmlError error;
            error.setDescription(tr("Module loaded for URI '%1' does not implement QQmlTypesExtensionInterface").arg(typeNamespace));
            errors->prepend(error);
        }
        return false;
    }

    const QByteArray bytes = uri.toUtf8();
    const char *moduleId = bytes.constData();

    QQmlMetaTypeRegistrationFailureRecorder failureRecorder;
    {
        // Create a scope for QWriteLocker to keep it as narrow as possible, and
        // to ensure that we release it before the call to initalizeEngine below
        QMutexLocker lock(QQmlMetaType::typeRegistrationLock());

        if (!typeNamespace.isEmpty()) {
            // This is an 'identified' module
            if (typeNamespace != uri) {
                // The namespace for type registrations must match the URI for locating the module
                if (errors) {
                    QQmlError error;
                    error.setDescription(tr("Module namespace '%1' does not match import URI '%2'").arg(typeNamespace).arg(uri));
                    errors->prepend(error);
                }
                return false;
            }

            if (QQmlMetaType::namespaceContainsRegistrations(typeNamespace, vmaj)) {
                // Other modules have already installed to this namespace
                if (errors) {
                    QQmlError error;
                    error.setDescription(tr("Namespace '%1' has already been used for type registration").arg(typeNamespace));
                    errors->prepend(error);
                }
                return false;
            } else {
                QQmlMetaType::protectNamespace(typeNamespace);
            }
        } else {
            // This is not an identified module - provide a warning
            qWarning().nospace() << qPrintable(tr("Module '%1' does not contain a module identifier directive - it cannot be protected from external registrations.").arg(uri));
        }

        QQmlMetaType::setTypeRegistrationNamespace(typeNamespace);

        if (QQmlExtensionPlugin *plugin = qobject_cast<QQmlExtensionPlugin *>(instance)) {
            // basepath should point to the directory of the module, not the plugin file itself:
            QQmlExtensionPluginPrivate::get(plugin)->baseUrl = QQmlImports::urlFromLocalFileOrQrcOrUrl(basePath);
        }

        iface->registerTypes(moduleId);
        QQmlMetaType::setTypeRegistrationNamespace(QString());
    } // QWriteLocker lock(QQmlMetaType::typeRegistrationLock())

    if (!failureRecorder.failures().isEmpty()) {
        if (errors) {
            for (const QString &failure : failureRecorder.failures()) {
                QQmlError error;
                error.setDescription(failure);
                errors->prepend(error);
            }
        }
        return false;
    }

    return true;
}

/*!
    \internal
*/
bool QQmlImportDatabase::importStaticPlugin(QObject *instance, const QString &basePath,
                                      const QString &uri, const QString &typeNamespace, int vmaj, QList<QQmlError> *errors)
{
    // Dynamic plugins are differentiated by their filepath. For static plugins we
    // don't have that information so we use their address as key instead.
    const QString uniquePluginID = QString::asprintf("%p", instance);
    {
        StringRegisteredPluginMap *plugins = qmlEnginePluginsWithRegisteredTypes();
        QMutexLocker lock(&plugins->mutex);

        // Plugin types are global across all engines and should only be
        // registered once. But each engine still needs to be initialized.
        bool typesRegistered = plugins->contains(uniquePluginID);

        if (typesRegistered) {
            Q_ASSERT_X(plugins->value(uniquePluginID).uri == uri,
                       "QQmlImportDatabase::importStaticPlugin",
                       "Internal error: Static plugin imported previously with different uri");
        } else {
            RegisteredPlugin plugin;
            plugin.uri = uri;
            plugin.loader = nullptr;
            plugins->insert(uniquePluginID, plugin);

            if (!registerPluginTypes(instance, basePath, uri, typeNamespace, vmaj, errors))
                return false;
        }

        // Release the lock on plugins early as we're done with the global part. Releasing the lock
        // also allows other QML loader threads to acquire the lock while this thread is blocking
        // in the initializeEngine call to the gui thread (which in turn may be busy waiting for
        // other QML loader threads and thus not process the initializeEngine call).
    }

    // The plugin's per-engine initialization does not need lock protection, as this function is
    // only called from the engine specific loader thread and importDynamicPlugin as well as
    // importStaticPlugin are the only places of access.
    if (!initializedPlugins.contains(uniquePluginID)) {
        initializedPlugins.insert(uniquePluginID);

        if (QQmlExtensionInterface *eiface = qobject_cast<QQmlExtensionInterface *>(instance)) {
            QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);
            ep->typeLoader.initializeEngine(eiface, uri.toUtf8().constData());
        }
    }

    return true;
}

/*!
    \internal
*/
bool QQmlImportDatabase::importDynamicPlugin(const QString &filePath, const QString &uri,
                                             const QString &typeNamespace, int vmaj, QList<QQmlError> *errors)
{
#if QT_CONFIG(library)
    QFileInfo fileInfo(filePath);
    const QString absoluteFilePath = fileInfo.absoluteFilePath();

    QObject *instance = nullptr;
    bool engineInitialized = initializedPlugins.contains(absoluteFilePath);
    {
        StringRegisteredPluginMap *plugins = qmlEnginePluginsWithRegisteredTypes();
        QMutexLocker lock(&plugins->mutex);
        bool typesRegistered = plugins->contains(absoluteFilePath);

        if (typesRegistered) {
            Q_ASSERT_X(plugins->value(absoluteFilePath).uri == uri,
                       "QQmlImportDatabase::importDynamicPlugin",
                       "Internal error: Plugin imported previously with different uri");
        }

        if (!engineInitialized || !typesRegistered) {
            if (!QQml_isFileCaseCorrect(absoluteFilePath)) {
                if (errors) {
                    QQmlError error;
                    error.setDescription(tr("File name case mismatch for \"%1\"").arg(absoluteFilePath));
                    errors->prepend(error);
                }
                return false;
            }

            QPluginLoader* loader = nullptr;
            if (!typesRegistered) {
                loader = new QPluginLoader(absoluteFilePath);

                if (!loader->load()) {
                    if (errors) {
                        QQmlError error;
                        error.setDescription(loader->errorString());
                        errors->prepend(error);
                    }
                    delete loader;
                    return false;
                }
            } else {
                loader = plugins->value(absoluteFilePath).loader;
            }

            instance = loader->instance();

            if (!typesRegistered) {
                RegisteredPlugin plugin;
                plugin.uri = uri;
                plugin.loader = loader;
                plugins->insert(absoluteFilePath, plugin);

                // Continue with shared code path for dynamic and static plugins:
                if (!registerPluginTypes(instance, fileInfo.absolutePath(), uri, typeNamespace, vmaj, errors))
                    return false;
            }
        }

    // Release the lock on plugins early as we're done with the global part. Releasing the lock
    // also allows other QML loader threads to acquire the lock while this thread is blocking
    // in the initializeEngine call to the gui thread (which in turn may be busy waiting for
    // other QML loader threads and thus not process the initializeEngine call).
    }


    if (!engineInitialized) {
        // The plugin's per-engine initialization does not need lock protection, as this function is
        // only called from the engine specific loader thread and importDynamicPlugin as well as
        // importStaticPlugin are the only places of access.
        initializedPlugins.insert(absoluteFilePath);

        if (QQmlExtensionInterface *eiface = qobject_cast<QQmlExtensionInterface *>(instance)) {
            QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);
            ep->typeLoader.initializeEngine(eiface, uri.toUtf8().constData());
        }
    }

    return true;
#else
    Q_UNUSED(filePath);
    Q_UNUSED(uri);
    Q_UNUSED(typeNamespace);
    Q_UNUSED(vmaj);
    Q_UNUSED(errors);
    return false;
#endif
}

void QQmlImportDatabase::clearDirCache()
{
    QStringHash<QmldirCache *>::ConstIterator itr = qmldirCache.begin();
    while (itr != qmldirCache.end()) {
        QmldirCache *cache = *itr;
        do {
            QmldirCache *nextCache = cache->next;
            delete cache;
            cache = nextCache;
        } while (cache);

        ++itr;
    }
    qmldirCache.clear();
}

QT_END_NAMESPACE
