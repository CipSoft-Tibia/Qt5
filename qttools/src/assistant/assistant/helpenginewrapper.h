// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef HELPENGINEWRAPPER_H
#define HELPENGINEWRAPPER_H

#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtGui/QFont>
#include <QtGui/QFontDatabase>

#include "qhelplink.h"

QT_BEGIN_NAMESPACE

class QFileSystemWatcher;
class QHelpContentModel;
class QHelpContentWidget;
class QHelpIndexModel;
class QHelpIndexWidget;
class QHelpSearchEngine;
class QHelpFilterEngine;
class QHelpEngineCore;

enum {
    ShowHomePage = 0,
    ShowBlankPage = 1,
    ShowLastPages = 2
};

class HelpEngineWrapperPrivate;
class TimeoutForwarder;

class HelpEngineWrapper : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(HelpEngineWrapper)
    friend class TimeoutForwarder;
public:
    static HelpEngineWrapper &instance();
    static HelpEngineWrapper &instance(const QString &collectionFile);
    static void removeInstance();

    // Forwarded help engine member functions, possibly enriched.
    QHelpSearchEngine *searchEngine() const;
    QHelpContentModel *contentModel() const;
    QHelpIndexModel *indexModel() const;
    QHelpContentWidget *contentWidget();
    QHelpIndexWidget *indexWidget();
    bool setupData();
    const QStringList registeredDocumentations() const;
    QString documentationFileName(const QString &namespaceName) const;
    const QString collectionFile() const;
    bool registerDocumentation(const QString &docFile);
    bool unregisterDocumentation(const QString &namespaceName);
    QUrl findFile(const QUrl &url) const;
    QByteArray fileData(const QUrl &url) const;
    QList<QHelpLink> documentsForIdentifier(const QString &id) const;
    QString error() const;

    QHelpFilterEngine *filterEngine() const;

    /*
     * To be called after assistant has finished looking for new documentation.
     * This will mainly cause the search index to be updated, if necessary.
     */
    void initialDocSetupDone();

    const QStringList qtDocInfo(const QString &component) const;
    void setQtDocInfo(const QString &component, const QStringList &doc);

    const QString homePage() const;
    void setHomePage(const QString &page);
    const QString defaultHomePage() const;
    void setDefaultHomePage(const QString &page);

    int lastTabPage() const;
    void setLastTabPage(int lastPage);

    // TODO: Don't allow last pages and zoom factors to be set in isolation
    //       Perhaps also fill up missing elements automatically or assert.
    const QStringList lastShownPages() const;
    void setLastShownPages(const QStringList &lastShownPages);
    const QStringList lastZoomFactors() const;
    void setLastZoomFactors(const QStringList &lastZoomFactors);

    const QString cacheDir() const;
    bool cacheDirIsRelativeToCollection() const;
    void setCacheDir(const QString &cacheDir, bool relativeToCollection);

    bool filterFunctionalityEnabled() const;
    void setFilterFunctionalityEnabled(bool enabled);

    bool filterToolbarVisible() const;
    void setFilterToolbarVisible(bool visible);

    bool addressBarEnabled() const;
    void setAddressBarEnabled(bool enabled);

    bool addressBarVisible() const;
    void setAddressBarVisible(bool visible);

    bool documentationManagerEnabled() const;
    void setDocumentationManagerEnabled(bool enabled);

    const QByteArray aboutMenuTexts() const;
    void setAboutMenuTexts(const QByteArray &texts);
    const QByteArray aboutTexts() const;
    void setAboutTexts(const QByteArray &texts);
    const QByteArray aboutIcon() const;
    void setAboutIcon(const QByteArray &icon);
    const QByteArray aboutImages() const;
    void setAboutImages(const QByteArray &images);

    const QString windowTitle() const;
    void setWindowTitle(const QString &windowTitle);

    const QByteArray applicationIcon() const;
    void setApplicationIcon(const QByteArray &icon);

    const QByteArray mainWindow() const;
    void setMainWindow(const QByteArray &mainWindow);
    const QByteArray mainWindowGeometry() const;
    void setMainWindowGeometry(const QByteArray &geometry);

    const QByteArray bookmarks() const;
    void setBookmarks(const QByteArray &bookmarks);

    int startOption() const;
    void setStartOption(int option);

    bool hasFontSettings() const;
    bool usesAppFont() const;
    void setUseAppFont(bool useAppFont);
    bool usesBrowserFont() const;
    void setUseBrowserFont(bool useBrowserFont);
    const QFont appFont() const;
    void setAppFont(const QFont &font);
    QFontDatabase::WritingSystem appWritingSystem() const;
    void setAppWritingSystem(QFontDatabase::WritingSystem system);
    const QFont browserFont() const;
    void setBrowserFont(const QFont &font);
    QFontDatabase::WritingSystem browserWritingSystem() const;
    void setBrowserWritingSystem(QFontDatabase::WritingSystem system);

    bool showTabs() const;
    void setShowTabs(bool show);

    bool fullTextSearchFallbackEnabled() const;

    const QByteArray topicChooserGeometry() const;
    void setTopicChooserGeometry(const QByteArray &geometry);

    QHelpEngineCore *helpEngine() const;

signals:

    // For asynchronous doc updates triggered by external actions.
    void documentationRemoved(const QString &namespaceName);
    void documentationUpdated(const QString &namespaceName);

    // Forwarded from QHelpEngineCore.
    void setupFinished();

private:
    HelpEngineWrapper(const QString &collectionFile);
    ~HelpEngineWrapper();

    static HelpEngineWrapper *helpEngineWrapper;

    HelpEngineWrapperPrivate *d;
};

QT_END_NAMESPACE

#endif // HELPENGINEWRAPPER_H
