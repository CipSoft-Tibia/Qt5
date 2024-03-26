// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "tracer.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QLibraryInfo>
#include <QtCore/QLocale>
#include <QtCore/QScopedPointer>
#include <QtCore/QStringList>
#include <QtCore/QTranslator>
#include <QtCore/QUrl>

#include <QtWidgets/QApplication>
#include <QtGui/QDesktopServices>

#include <QtHelp/QHelpEngine>
#include <QtHelp/QHelpSearchEngine>

#include <QtSql/QSqlDatabase>

#if defined(BROWSER_QTWEBKIT)
#include <QtGui/QFont>
#include <QWebSettings>
#endif

#include "../shared/collectionconfiguration.h"
#include "helpenginewrapper.h"
#include "mainwindow.h"
#include "cmdlineparser.h"

// #define TRACING_REQUESTED

QT_USE_NAMESPACE

namespace {

void
updateLastPagesOnUnregister(QHelpEngineCore& helpEngine, const QString& nsName)
{
    TRACE_OBJ
    int lastPage = CollectionConfiguration::lastTabPage(helpEngine);
    QStringList currentPages = CollectionConfiguration::lastShownPages(helpEngine);
    if (!currentPages.isEmpty()) {
        QStringList zoomList = CollectionConfiguration::lastZoomFactors(helpEngine);
        while (zoomList.size() < currentPages.size())
            zoomList.append(CollectionConfiguration::DefaultZoomFactor);

        for (int i = currentPages.size(); --i >= 0;) {
            if (QUrl(currentPages.at(i)).host() == nsName) {
                zoomList.removeAt(i);
                currentPages.removeAt(i);
                lastPage = (lastPage == (i + 1)) ? 1 : lastPage;
            }
        }

        CollectionConfiguration::setLastShownPages(helpEngine, currentPages);
        CollectionConfiguration::setLastTabPage(helpEngine, lastPage);
        CollectionConfiguration::setLastZoomFactors(helpEngine, zoomList);
    }
}

void stripNonexistingDocs(QHelpEngineCore& collection)
{
    TRACE_OBJ
    const QStringList &namespaces = collection.registeredDocumentations();
    for (const QString &ns : namespaces) {
        QFileInfo fi(collection.documentationFileName(ns));
        if (!fi.exists() || !fi.isFile())
            collection.unregisterDocumentation(ns);
    }
}

QString indexFilesFolder(const QString &collectionFile)
{
    TRACE_OBJ
    QString indexFilesFolder = QLatin1String(".fulltextsearch");
    if (!collectionFile.isEmpty()) {
        QFileInfo fi(collectionFile);
        indexFilesFolder = QLatin1Char('.') +
            fi.fileName().left(fi.fileName().lastIndexOf(QLatin1String(".qhc")));
    }
    return indexFilesFolder;
}

/*
 * Returns the expected absolute file path of the cached collection file
 * correspondinging to the given collection's file.
 * It may or may not exist yet.
 */
QString constructCachedCollectionFilePath(const QHelpEngineCore &collection)
{
    TRACE_OBJ
    const QString &filePath = collection.collectionFile();
    const QString &fileName = QFileInfo(filePath).fileName();
    const QString &cacheDir = CollectionConfiguration::cacheDir(collection);
    const QString &dir = !cacheDir.isEmpty()
        && CollectionConfiguration::cacheDirIsRelativeToCollection(collection)
            ? QFileInfo(filePath).dir().absolutePath()
                + QDir::separator() + cacheDir
            : MainWindow::collectionFileDirectory(false, cacheDir);
    return dir + QDir::separator() + fileName;
}

bool synchronizeDocs(QHelpEngineCore &collection,
                     QHelpEngineCore &cachedCollection,
                     CmdLineParser &cmd)
{
    TRACE_OBJ
    const QDateTime &lastCollectionRegisterTime =
        CollectionConfiguration::lastRegisterTime(collection);
    if (!lastCollectionRegisterTime.isValid() || lastCollectionRegisterTime
        < CollectionConfiguration::lastRegisterTime(cachedCollection))
        return true;

    const QStringList &docs = collection.registeredDocumentations();
    const QStringList &cachedDocs = cachedCollection.registeredDocumentations();

    /*
     * Ensure that the cached collection contains all docs that
     * the collection contains.
     */
    for (const QString &doc : docs) {
        if (!cachedDocs.contains(doc)) {
            const QString &docFile = collection.documentationFileName(doc);
            if (!cachedCollection.registerDocumentation(docFile)) {
                cmd.showMessage(QCoreApplication::translate("Assistant",
                                    "Error registering documentation file '%1': %2").
                                arg(docFile).arg(cachedCollection.error()), true);
                return false;
            }
        }
    }

    CollectionConfiguration::updateLastRegisterTime(cachedCollection);

    return true;
}

bool removeSearchIndex(const QString &collectionFile)
{
    TRACE_OBJ
    QString path = QFileInfo(collectionFile).path();
    path += QLatin1Char('/') + indexFilesFolder(collectionFile);

    QDir dir(path);
    if (!dir.exists())
        return false;

    const QStringList &list = dir.entryList(QDir::Files | QDir::Hidden);
    for (const QString &item : list)
        dir.remove(item);
    return true;
}

QCoreApplication* createApplication(int &argc, char *argv[])
{
    TRACE_OBJ
#ifndef Q_OS_WIN
    // Look for arguments that imply command-line mode.
    const char * cmdModeArgs[] = {
        "-help", "-register", "-unregister", "-remove-search-index",
        "-rebuild-search-index"
    };
    for (int i = 1; i < argc; ++i) {
        for (size_t j = 0; j < sizeof cmdModeArgs/sizeof *cmdModeArgs; ++j) {
            if (strcmp(argv[i], cmdModeArgs[j]) == 0)
                return new QCoreApplication(argc, argv);
        }
    }
#endif
    QApplication *app = new QApplication(argc, argv);
    app->connect(app, &QGuiApplication::lastWindowClosed,
                 &QCoreApplication::quit);
    return app;
}

bool registerDocumentation(QHelpEngineCore &collection, CmdLineParser &cmd,
                           bool printSuccess)
{
    TRACE_OBJ
    if (!collection.registerDocumentation(cmd.helpFile())) {
        cmd.showMessage(QCoreApplication::translate("Assistant",
                     "Could not register documentation file\n%1\n\nReason:\n%2")
                     .arg(cmd.helpFile()).arg(collection.error()), true);
        return false;
    }
    if (printSuccess)
        cmd.showMessage(QCoreApplication::translate("Assistant",
                            "Documentation successfully registered."),
                        false);
    CollectionConfiguration::updateLastRegisterTime(collection);
    return true;
}

bool unregisterDocumentation(QHelpEngineCore &collection,
    const QString &namespaceName, CmdLineParser &cmd, bool printSuccess)
{
    TRACE_OBJ
    if (!collection.unregisterDocumentation(namespaceName)) {
        cmd.showMessage(QCoreApplication::translate("Assistant",
                             "Could not unregister documentation"
                             " file\n%1\n\nReason:\n%2").
                        arg(cmd.helpFile()).arg(collection.error()), true);
        return false;
    }
    updateLastPagesOnUnregister(collection, namespaceName);
    if (printSuccess)
        cmd.showMessage(QCoreApplication::translate("Assistant",
                            "Documentation successfully unregistered."),
                        false);
    return true;
}

void setupTranslation(const QString &fileName, const QString &dir)
{
    QTranslator *translator = new QTranslator(QCoreApplication::instance());
    if (translator->load(QLocale(), fileName, QLatin1String("_"), dir))
        QCoreApplication::installTranslator(translator);
}

void setupTranslations()
{
    TRACE_OBJ
    const QString &resourceDir
        = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
    setupTranslation(QLatin1String("assistant"), resourceDir);
    setupTranslation(QLatin1String("qt"), resourceDir);
    setupTranslation(QLatin1String("qt_help"), resourceDir);
}

} // Anonymous namespace.

enum ExitStatus {
    ExitSuccess = 0,
    ExitFailure,
    NoExit
};

static ExitStatus preliminarySetup(CmdLineParser *cmd)
{
    /*
     * Create the collection objects that we need. We always have the
     * cached collection file. Depending on whether the user specified
     * one, we also may have an input collection file.
     */
    const QString collectionFile = cmd->collectionFile();
    const bool collectionFileGiven = !collectionFile.isEmpty();
    QScopedPointer<QHelpEngineCore> collection;
    if (collectionFileGiven) {
        collection.reset(new QHelpEngineCore(collectionFile));
        if (!collection->setupData()) {
            cmd->showMessage(QCoreApplication::translate("Assistant",
                             "Error reading collection file '%1': %2.")
                            .arg(collectionFile).arg(collection->error()), true);
            return ExitFailure;
        }
    }
    const QString &cachedCollectionFile = collectionFileGiven
            ? constructCachedCollectionFilePath(*collection)
            : MainWindow::defaultHelpCollectionFileName();
    if (collectionFileGiven && !QFileInfo(cachedCollectionFile).exists()
            && !collection->copyCollectionFile(cachedCollectionFile)) {
        cmd->showMessage(QCoreApplication::translate("Assistant",
                         "Error creating collection file '%1': %2.")
                        .arg(cachedCollectionFile).arg(collection->error()), true);
        return ExitFailure;
    }
    QHelpEngineCore cachedCollection(cachedCollectionFile);
    if (!cachedCollection.setupData()) {
        cmd->showMessage(QCoreApplication::translate("Assistant",
                         "Error reading collection file '%1': %2.")
                        .arg(cachedCollectionFile)
                        .arg(cachedCollection.error()), true);
        return ExitFailure;
    }

    stripNonexistingDocs(cachedCollection);
    if (collectionFileGiven) {
        if (CollectionConfiguration::isNewer(*collection, cachedCollection))
            CollectionConfiguration::copyConfiguration(*collection,
                                                       cachedCollection);
        if (!synchronizeDocs(*collection, cachedCollection, *cmd))
            return ExitFailure;
    }

    if (cmd->registerRequest() != CmdLineParser::None) {
        const QStringList &cachedDocs =
                cachedCollection.registeredDocumentations();
        const QString &namespaceName =
                QHelpEngineCore::namespaceName(cmd->helpFile());
        if (cmd->registerRequest() == CmdLineParser::Register) {
            if (collectionFileGiven
                    && !registerDocumentation(*collection, *cmd, true))
                return ExitFailure;
            if (!cachedDocs.contains(namespaceName)
                    && !registerDocumentation(cachedCollection, *cmd, !collectionFileGiven))
                return ExitFailure;
            return ExitSuccess;
        }
        if (cmd->registerRequest() == CmdLineParser::Unregister) {
            if (collectionFileGiven
                    && !unregisterDocumentation(*collection, namespaceName, *cmd, true))
                return ExitFailure;
            if (cachedDocs.contains(namespaceName)
                    && !unregisterDocumentation(cachedCollection, namespaceName,
                                                *cmd, !collectionFileGiven))
                return ExitFailure;
            return ExitSuccess;
        }
    }

    if (cmd->removeSearchIndex()) {
        return removeSearchIndex(cachedCollectionFile)
                ? ExitSuccess : ExitFailure;
    }

    if (!QSqlDatabase::isDriverAvailable(QLatin1String("QSQLITE"))) {
        cmd->showMessage(QCoreApplication::translate("Assistant",
                         "Cannot load sqlite database driver!"),
                         true);
        return ExitFailure;
    }

    if (!cmd->currentFilter().isEmpty()) {
        if (collectionFileGiven)
            collection->setCurrentFilter(cmd->currentFilter());
        cachedCollection.setCurrentFilter(cmd->currentFilter());
    }

    if (collectionFileGiven)
        cmd->setCollectionFile(cachedCollectionFile);

    return NoExit;
}

int main(int argc, char *argv[])
{
    TRACE_OBJ
    QScopedPointer<QCoreApplication> a(createApplication(argc, argv));
#if QT_CONFIG(library)
    a->addLibraryPath(a->applicationDirPath() + QLatin1String("/plugins"));
#endif
    setupTranslations();

#if defined(BROWSER_QTWEBKIT)
    if (qobject_cast<QApplication *>(a.data())) {
        QFont f;
        f.setStyleHint(QFont::SansSerif);
        QWebSettings::globalSettings()->setFontFamily(QWebSettings::StandardFont, f.defaultFamily());
    }
#endif // BROWSER_QTWEBKIT

    // Parse arguments.
    CmdLineParser cmd(a->arguments());
    CmdLineParser::Result res = cmd.parse();
    if (res == CmdLineParser::Help)
        return 0;
    else if (res == CmdLineParser::Error)
        return -1;

    const ExitStatus status = preliminarySetup(&cmd);
    switch (status) {
        case ExitFailure: return EXIT_FAILURE;
        case ExitSuccess: return EXIT_SUCCESS;
        default: break;
    }

    MainWindow *w = new MainWindow(&cmd);
    w->show();

    /*
     * We need to be careful here: The main window has to be deleted before
     * the help engine wrapper, which has to be deleted before the
     * QApplication.
     */
    const int retval = a->exec();
    delete w;
    HelpEngineWrapper::removeInstance();
    return retval;
}
