/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Assistant of the Qt Toolkit.
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

#include "qhelpenginecore.h"
#include "qhelpengine_p.h"
#include "qhelpdbreader_p.h"
#include "qhelpcollectionhandler_p.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QPluginLoader>
#include <QtCore/QFileInfo>
#include <QtCore/QThread>
#include <QtWidgets/QApplication>
#include <QtSql/QSqlQuery>

QT_BEGIN_NAMESPACE

void QHelpEngineCorePrivate::init(const QString &collectionFile,
                                  QHelpEngineCore *helpEngineCore)
{
    q = helpEngineCore;
    collectionHandler = new QHelpCollectionHandler(collectionFile, helpEngineCore);
    connect(collectionHandler, &QHelpCollectionHandler::error,
            this, &QHelpEngineCorePrivate::errorReceived);
    needsSetup = true;
}

QHelpEngineCorePrivate::~QHelpEngineCorePrivate()
{
    delete collectionHandler;
    emitReadersAboutToBeInvalidated();
}

void QHelpEngineCorePrivate::emitReadersAboutToBeInvalidated()
{
    emit q->readersAboutToBeInvalidated();
}

bool QHelpEngineCorePrivate::setup()
{
    error.clear();
    if (!needsSetup)
        return true;

    needsSetup = false;
    emit q->setupStarted();
    emitReadersAboutToBeInvalidated();

    const QVariant readOnlyVariant = q->property("_q_readonly");
    const bool readOnly = readOnlyVariant.isValid()
            ? readOnlyVariant.toBool() : false;
    collectionHandler->setReadOnly(readOnly);
    const bool opened = collectionHandler->openCollectionFile();
    if (opened)
        q->currentFilter();

    emit q->setupFinished();

    return opened;
}

void QHelpEngineCorePrivate::errorReceived(const QString &msg)
{
    error = msg;
}

/*!
    \class QHelpEngineCore
    \since 4.4
    \inmodule QtHelp
    \brief The QHelpEngineCore class provides the core functionality
    of the help system.

    Before the help engine can be used, it must be initialized by
    calling setupData(). At the beginning of the setup process the
    signal setupStarted() is emitted. From this point on until
    the signal setupFinished() is emitted, is the help data in an
    undefined meaning unusable state.

    The core help engine can be used to perform different tasks.
    By calling linksForIdentifier() the engine returns
    URLs specifying the file locations inside the help system. The
    actual file data can then be retrived by calling fileData(). In
    contrast to all other functions in this class, linksForIdentifier()
    depends on the currently set custom filter. Depending on the filter,
    the function may return different results.

    Every help engine can contain any number of custom filters. A custom
    filter is defined by a name and set of filter attributes and can be
    added to the help engine by calling addCustomFilter(). Analogous,
    it is removed by calling removeCustomFilter(). customFilters() returns
    all defined filters.

    The help engine also offers the possibility to set and read values
    in a persistant way comparable to ini files or Windows registry
    entries. For more information see setValue() or value().

    This class does not offer any GUI components or functionality for
    indices or contents. If you need one of those use QHelpEngine
    instead.

    When creating a custom help viewer the viewer can be
    configured by writing a custom collection file which could contain various
    keywords to be used to configure the help engine. These keywords and values
    and their meaning can be found in the help information for
    \l{assistant-custom-help-viewer.html#creating-a-custom-help-collection-file}
    {creating a custom help collection file} for Assistant.
*/

/*!
    \fn void QHelpEngineCore::setupStarted()

    This signal is emitted when setup is started.
*/

/*!
    \fn void QHelpEngineCore::setupFinished()

    This signal is emitted when the setup is complete.
*/

/*!
    \fn void QHelpEngineCore::currentFilterChanged(const QString &newFilter)

    This signal is emitted when the current filter is changed to
    \a newFilter.
*/

/*!
    \fn void QHelpEngineCore::warning(const QString &msg)

    This signal is emitted when a non critical error occurs.
    The warning message is stored in \a msg.
*/

/*!
    Constructs a new core help engine with a \a parent. The help engine
    uses the information stored in the \a collectionFile to provide help.
    If the collection file does not exist yet, it'll be created.
*/
QHelpEngineCore::QHelpEngineCore(const QString &collectionFile, QObject *parent)
    : QObject(parent)
{
    d = new QHelpEngineCorePrivate();
    d->init(collectionFile, this);
}

/*!
    \internal
*/
QHelpEngineCore::QHelpEngineCore(QHelpEngineCorePrivate *helpEngineCorePrivate,
                                 QObject *parent)
    : QObject(parent)
{
    d = helpEngineCorePrivate;
}

/*!
    Destructs the help engine.
*/
QHelpEngineCore::~QHelpEngineCore()
{
    delete d;
}

/*!
    \property QHelpEngineCore::collectionFile
    \brief the absolute file name of the collection file currently used.
    \since 4.5

    Setting this property leaves the help engine in an invalid state. It is
    important to invoke setupData() or any getter function in order to setup
    the help engine again.
*/
QString QHelpEngineCore::collectionFile() const
{
    return d->collectionHandler->collectionFile();
}

void QHelpEngineCore::setCollectionFile(const QString &fileName)
{
    if (fileName == collectionFile())
        return;

    if (d->collectionHandler) {
        delete d->collectionHandler;
        d->collectionHandler = nullptr;
        d->emitReadersAboutToBeInvalidated();
    }
    d->init(fileName, this);
    d->needsSetup = true;
}

/*!
    Sets up the help engine by processing the information found
    in the collection file and returns true if successful; otherwise
    returns false.

    By calling the function, the help
    engine is forced to initialize itself immediately. Most of
    the times, this function does not have to be called
    explicitly because getter functions which depend on a correctly
    set up help engine do that themselves.

    \note \c{qsqlite4.dll} needs to be deployed with the application as the
    help system uses the sqlite driver when loading help collections.
*/
bool QHelpEngineCore::setupData()
{
    d->needsSetup = true;
    return d->setup();
}

/*!
    Creates the file \a fileName and copies all contents from
    the current collection file into the newly created file,
    and returns true if successful; otherwise returns false.

    The copying process makes sure that file references to Qt
    Collection files (\c{.qch}) files are updated accordingly.
*/
bool QHelpEngineCore::copyCollectionFile(const QString &fileName)
{
    if (!d->setup())
        return false;
    return d->collectionHandler->copyCollectionFile(fileName);
}

/*!
    Returns the namespace name defined for the Qt compressed help file (.qch)
    specified by its \a documentationFileName. If the file is not valid, an
    empty string is returned.

    \sa documentationFileName()
*/
QString QHelpEngineCore::namespaceName(const QString &documentationFileName)
{
    QHelpDBReader reader(documentationFileName,
        QHelpGlobal::uniquifyConnectionName(QLatin1String("GetNamespaceName"),
        QThread::currentThread()), nullptr);
    if (reader.init())
        return reader.namespaceName();
    return QString();
}

/*!
    Registers the Qt compressed help file (.qch) contained in the file
    \a documentationFileName. One compressed help file, uniquely
    identified by its namespace can only be registered once.
    True is returned if the registration was successful, otherwise
    false.

    \sa unregisterDocumentation(), error()
*/
bool QHelpEngineCore::registerDocumentation(const QString &documentationFileName)
{
    d->error.clear();
    d->needsSetup = true;
    return d->collectionHandler->registerDocumentation(documentationFileName);
}

/*!
    Unregisters the Qt compressed help file (.qch) identified by its
    \a namespaceName from the help collection. Returns true
    on success, otherwise false.

    \sa registerDocumentation(), error()
*/
bool QHelpEngineCore::unregisterDocumentation(const QString &namespaceName)
{
    d->error.clear();
    d->needsSetup = true;
    return d->collectionHandler->unregisterDocumentation(namespaceName);
}

/*!
    Returns the absolute file name of the Qt compressed help file (.qch)
    identified by the \a namespaceName. If there is no Qt compressed help file
    with the specified namespace registered, an empty string is returned.

    \sa namespaceName()
*/
QString QHelpEngineCore::documentationFileName(const QString &namespaceName)
{
    if (!d->setup())
        return QString();

    const QHelpCollectionHandler::FileInfo fileInfo =
            d->collectionHandler->registeredDocumentation(namespaceName);

    if (fileInfo.namespaceName.isEmpty())
        return QString();

    if (QDir::isAbsolutePath(fileInfo.fileName))
        return fileInfo.fileName;

    return QFileInfo(QFileInfo(d->collectionHandler->collectionFile()).absolutePath()
                     + QLatin1Char('/') + fileInfo.fileName).absoluteFilePath();
}

/*!
    Returns a list of all registered Qt compressed help files of the current collection file.
    The returned names are the namespaces of the registered Qt compressed help files (.qch).
*/
QStringList QHelpEngineCore::registeredDocumentations() const
{
    QStringList list;
    if (!d->setup())
        return list;
    const QHelpCollectionHandler::FileInfoList &docList
            = d->collectionHandler->registeredDocumentations();
    for (const QHelpCollectionHandler::FileInfo &info : docList)
        list.append(info.namespaceName);
    return list;
}

/*!
    Returns a list of custom filters.

    \sa addCustomFilter(), removeCustomFilter()
*/
QStringList QHelpEngineCore::customFilters() const
{
    if (!d->setup())
        return QStringList();
    return d->collectionHandler->customFilters();
}

/*!
    Adds the new custom filter \a filterName. The filter attributes
    are specified by \a attributes. If the filter already exists,
    its attribute set is replaced. The function returns true if
    the operation succeeded, otherwise it returns false.

    \sa customFilters(), removeCustomFilter()
*/
bool QHelpEngineCore::addCustomFilter(const QString &filterName,
                                      const QStringList &attributes)
{
    d->error.clear();
    d->needsSetup = true;
    return d->collectionHandler->addCustomFilter(filterName, attributes);
}

/*!
    Returns true if the filter \a filterName was removed successfully,
    otherwise false.

    \sa addCustomFilter(), customFilters()
*/
bool QHelpEngineCore::removeCustomFilter(const QString &filterName)
{
    d->error.clear();
    d->needsSetup = true;
    return d->collectionHandler->removeCustomFilter(filterName);
}

/*!
    Returns a list of all defined filter attributes.
*/
QStringList QHelpEngineCore::filterAttributes() const
{
    if (!d->setup())
        return QStringList();
    return d->collectionHandler->filterAttributes();
}

/*!
    Returns a list of filter attributes used by the custom
    filter \a filterName.
*/
QStringList QHelpEngineCore::filterAttributes(const QString &filterName) const
{
    if (!d->setup())
        return QStringList();
    return d->collectionHandler->filterAttributes(filterName);
}

/*!
    \property QHelpEngineCore::currentFilter
    \brief the name of the custom filter currently applied.
    \since 4.5

    Setting this property will save the new custom filter permanently in the
    help collection file. To set a custom filter without saving it
    permanently, disable the auto save filter mode.

    \sa autoSaveFilter()
*/
QString QHelpEngineCore::currentFilter() const
{
    if (!d->setup())
        return QString();

    if (d->currentFilter.isEmpty()) {
        const QString &filter =
            d->collectionHandler->customValue(QLatin1String("CurrentFilter"),
                QString()).toString();
        if (!filter.isEmpty()
            && d->collectionHandler->customFilters().contains(filter))
            d->currentFilter = filter;
    }
    return d->currentFilter;
}

void QHelpEngineCore::setCurrentFilter(const QString &filterName)
{
    if (!d->setup() || filterName == d->currentFilter)
        return;
    d->currentFilter = filterName;
    if (d->autoSaveFilter) {
        d->collectionHandler->setCustomValue(QLatin1String("CurrentFilter"),
            d->currentFilter);
    }
    emit currentFilterChanged(d->currentFilter);
}

/*!
    Returns a list of filter attributes for the different filter sections
    defined in the Qt compressed help file with the given namespace
    \a namespaceName.
*/
QList<QStringList> QHelpEngineCore::filterAttributeSets(const QString &namespaceName) const
{
    if (!d->setup())
        return QList<QStringList>();

    return d->collectionHandler->filterAttributeSets(namespaceName);
}

/*!
    Returns a list of files contained in the Qt compressed help file \a
    namespaceName. The files can be filtered by \a filterAttributes as
    well as by their extension \a extensionFilter (e.g. 'html').
*/
QList<QUrl> QHelpEngineCore::files(const QString namespaceName,
                                   const QStringList &filterAttributes,
                                   const QString &extensionFilter)
{
    QList<QUrl> res;
    if (!d->setup())
        return res;

    QUrl url;
    url.setScheme(QLatin1String("qthelp"));
    url.setAuthority(namespaceName);

    const QStringList &files = d->collectionHandler->files(
                namespaceName, filterAttributes, extensionFilter);
    for (const QString &file : files) {
        url.setPath(QLatin1String("/") + file);
        res.append(url);
    }
    return res;
}

/*!
    Returns an invalid URL if the file \a url cannot be found.
    If the file exists, either the same url is returned or a
    different url if the file is located in a different namespace
    which is merged via a common virtual folder.
*/
QUrl QHelpEngineCore::findFile(const QUrl &url) const
{
    if (!d->setup())
        return url;

    const QStringList &attributes = filterAttributes(currentFilter());
    QUrl result = d->collectionHandler->findFile(url, attributes);
    if (!result.isEmpty())
        return result;

    result = d->collectionHandler->findFile(url);
    if (!result.isEmpty())
        return result;

    return url;
}

/*!
    Returns the data of the file specified by \a url. If the
    file does not exist, an empty QByteArray is returned.

    \sa findFile()
*/
QByteArray QHelpEngineCore::fileData(const QUrl &url) const
{
    if (!d->setup())
        return QByteArray();

    return d->collectionHandler->fileData(url);
}

/*!
    Returns documents found for the \a id. The map contains the
    document titles and their URLs.
    The returned map contents depends on the current filter, meaning only the keywords
    registered for the current filter will be returned.
*/
QMap<QString, QUrl> QHelpEngineCore::linksForIdentifier(const QString &id) const
{
    if (!d->setup())
        return QMap<QString, QUrl>();

    return d->collectionHandler->linksForIdentifier(id, filterAttributes(d->currentFilter));
}

/*!
    \since 4.5

    Returns all documents found for the \a keyword. The returned map consists of the
    document titles and their URLs.
*/
QMap<QString, QUrl> QHelpEngineCore::linksForKeyword(const QString &keyword) const
{
    if (!d->setup())
        return QMap<QString, QUrl>();

    return d->collectionHandler->linksForKeyword(keyword, filterAttributes(d->currentFilter));
}

/*!
    Removes the \a key from the settings section in the
    collection file. Returns true if the value was removed
    successfully, otherwise false.

    \sa customValue(), setCustomValue()
*/
bool QHelpEngineCore::removeCustomValue(const QString &key)
{
    d->error.clear();
    return d->collectionHandler->removeCustomValue(key);
}

/*!
    Returns the value assigned to the \a key. If the requested
    key does not exist, the specified \a defaultValue is
    returned.

    \sa setCustomValue(), removeCustomValue()
*/
QVariant QHelpEngineCore::customValue(const QString &key, const QVariant &defaultValue) const
{
    if (!d->setup())
        return QVariant();
    return d->collectionHandler->customValue(key, defaultValue);
}

/*!
    Save the \a value under the \a key. If the key already exist,
    the value will be overwritten. Returns true if the value was
    saved successfully, otherwise false.

    \sa customValue(), removeCustomValue()
*/
bool QHelpEngineCore::setCustomValue(const QString &key, const QVariant &value)
{
    d->error.clear();
    return d->collectionHandler->setCustomValue(key, value);
}

/*!
    Returns the meta data for the Qt compressed help file \a
    documentationFileName. If there is no data available for
    \a name, an invalid QVariant() is returned. The meta
    data is defined when creating the Qt compressed help file and
    cannot be modified later. Common meta data includes e.g.
    the author of the documentation.
*/
QVariant QHelpEngineCore::metaData(const QString &documentationFileName,
                                   const QString &name)
{
    QHelpDBReader reader(documentationFileName, QLatin1String("GetMetaData"), nullptr);

    if (reader.init())
        return reader.metaData(name);
    return QVariant();
}

/*!
    Returns a description of the last error that occurred.
*/
QString QHelpEngineCore::error() const
{
    return d->error;
}

/*!
    \property QHelpEngineCore::autoSaveFilter
    \brief whether QHelpEngineCore is in auto save filter mode or not.
    \since 4.5

    If QHelpEngineCore is in auto save filter mode, the current filter is
    automatically saved when it is changed by the setCurrentFilter()
    function. The filter is saved persistently in the help collection file.

    By default, this mode is on.
*/
void QHelpEngineCore::setAutoSaveFilter(bool save)
{
    d->autoSaveFilter = save;
}

bool QHelpEngineCore::autoSaveFilter() const
{
    return d->autoSaveFilter;
}

QT_END_NAMESPACE
