/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Assistant module of the Qt Toolkit.
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

#include "openpagesmanager.h"

#include "centralwidget.h"
#include "helpenginewrapper.h"
#include "helpviewer.h"
#include "openpagesmodel.h"
#include "openpagesswitcher.h"
#include "openpageswidget.h"
#include "tracer.h"
#include "../shared/collectionconfiguration.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QTreeView>

QT_BEGIN_NAMESPACE

OpenPagesManager *OpenPagesManager::m_instance = nullptr;

OpenPagesManager *OpenPagesManager::createInstance(QObject *parent,
                      bool defaultCollection, const QUrl &cmdLineUrl)
{
    TRACE_OBJ
    Q_ASSERT(!m_instance);
    m_instance = new OpenPagesManager(parent, defaultCollection, cmdLineUrl);
    return m_instance;
}

OpenPagesManager *OpenPagesManager::instance()
{
    TRACE_OBJ
    Q_ASSERT(m_instance);
    return m_instance;
}

OpenPagesManager::OpenPagesManager(QObject *parent, bool defaultCollection,
                                   const QUrl &cmdLineUrl)
    : QObject(parent)
    , m_model(new OpenPagesModel(this))
{
    TRACE_OBJ
    m_openPagesWidget = new OpenPagesWidget(m_model);
    m_openPagesWidget->setFrameStyle(QFrame::NoFrame);
    connect(m_openPagesWidget, &OpenPagesWidget::setCurrentPage,
            this, QOverload<const QModelIndex &>::of(&OpenPagesManager::setCurrentPage));
    connect(m_openPagesWidget, &OpenPagesWidget::closePage,
            this, QOverload<const QModelIndex &>::of(&OpenPagesManager::closePage));
    connect(m_openPagesWidget, &OpenPagesWidget::closePagesExcept,
            this, &OpenPagesManager::closePagesExcept);

    m_openPagesSwitcher = new OpenPagesSwitcher(m_model);
    connect(m_openPagesSwitcher, &OpenPagesSwitcher::closePage,
            this, QOverload<const QModelIndex &>::of(&OpenPagesManager::closePage));
    connect(m_openPagesSwitcher, &OpenPagesSwitcher::setCurrentPage,
            this, QOverload<const QModelIndex &>::of(&OpenPagesManager::setCurrentPage));

    setupInitialPages(defaultCollection, cmdLineUrl);
}

OpenPagesManager ::~OpenPagesManager()
{
    TRACE_OBJ
    m_instance = nullptr;
    delete m_openPagesSwitcher;
}

int OpenPagesManager::pageCount() const
{
    TRACE_OBJ
    return m_model->rowCount();
}

void OpenPagesManager::setupInitialPages(bool defaultCollection,
            const QUrl &cmdLineUrl)
{
    TRACE_OBJ
    if (cmdLineUrl.isValid()) {
        createPage(cmdLineUrl);
        return;
    }

    HelpEngineWrapper &helpEngine = HelpEngineWrapper::instance();
    int initialPage = 0;
    switch (helpEngine.startOption()) {
    case ShowHomePage:
        m_model->addPage(helpEngine.homePage());
        break;
    case ShowBlankPage:
        m_model->addPage(QUrl(QLatin1String("about:blank")));
        break;
    case ShowLastPages: {
        const QStringList &lastShownPageList = helpEngine.lastShownPages();
        const int pageCount = lastShownPageList.count();
        if (pageCount == 0) {
            if (defaultCollection)
                m_model->addPage(QUrl(QLatin1String("help")));
            else
                m_model->addPage(QUrl(QLatin1String("about:blank")));
        } else {
            QStringList zoomFactors = helpEngine.lastZoomFactors();
            while (zoomFactors.count() < pageCount)
                zoomFactors.append(CollectionConfiguration::DefaultZoomFactor);
            initialPage = helpEngine.lastTabPage();
            if (initialPage >= pageCount) {
                qWarning("Initial page set to %d, maximum possible value is %d",
                         initialPage, pageCount - 1);
                initialPage = 0;
            }
            for (int curPage = 0; curPage < pageCount; ++curPage) {
                const QString &curFile = lastShownPageList.at(curPage);
                if (helpEngine.findFile(curFile).isValid()
                    || curFile == QLatin1String("about:blank")) {
                    m_model->addPage(curFile, zoomFactors.at(curPage).toFloat());
                } else if (curPage <= initialPage && initialPage > 0)
                    --initialPage;
            }
        }
        break;
    }
    default:
        Q_ASSERT(0);
    }

    if (m_model->rowCount() == 0)
        m_model->addPage(helpEngine.homePage());
    for (int i = 0; i < m_model->rowCount(); ++i)
        CentralWidget::instance()->addPage(m_model->pageAt(i));
    setCurrentPage((initialPage >= m_model->rowCount())
        ? m_model->rowCount() - 1 : initialPage);
    m_openPagesSwitcher->selectCurrentPage();
}

HelpViewer *OpenPagesManager::createBlankPage()
{
    TRACE_OBJ
    return createPage(QUrl(QLatin1String("about:blank")));
}

void OpenPagesManager::closeCurrentPage()
{
    TRACE_OBJ
    Q_ASSERT(m_model->rowCount() > 1);
    const QModelIndexList selectedIndexes
        = m_openPagesWidget->selectionModel()->selectedRows();
    if (selectedIndexes.isEmpty())
        return;
    Q_ASSERT(selectedIndexes.count() == 1);
    removePage(selectedIndexes.first().row());
}

HelpViewer *OpenPagesManager::createPage(const QUrl &url, bool fromSearch)
{
    TRACE_OBJ
    if (HelpViewer::launchWithExternalApp(url))
        return nullptr;

    emit aboutToAddPage();

    m_model->addPage(url);
    const int index = m_model->rowCount() - 1;
    HelpViewer * const page = m_model->pageAt(index);
    CentralWidget::instance()->addPage(page, fromSearch);
    setCurrentPage(index);

    emit pageAdded(index);
    return page;
}

HelpViewer *OpenPagesManager::createNewPageFromSearch(const QUrl &url)
{
    TRACE_OBJ
    return createPage(url, true);
}

void OpenPagesManager::closePage(HelpViewer *viewer)
{
    TRACE_OBJ
    for (int i = 0; i < m_model->rowCount(); ++i) {
        if (m_model->pageAt(i) == viewer) {
            removePage(i);
            break;
        }
    }
}

void OpenPagesManager::closePage(const QModelIndex &index)
{
    TRACE_OBJ
    if (index.isValid())
        removePage(index.row());
}

void OpenPagesManager::closePages(const QString &nameSpace)
{
    TRACE_OBJ
    closeOrReloadPages(nameSpace, false);
}

void OpenPagesManager::reloadPages(const QString &nameSpace)
{
    TRACE_OBJ
    closeOrReloadPages(nameSpace, true);
    m_openPagesWidget->selectCurrentPage();
}

void OpenPagesManager::closeOrReloadPages(const QString &nameSpace, bool tryReload)
{
    TRACE_OBJ
    for (int i = m_model->rowCount() - 1; i >= 0; --i) {
        HelpViewer *page = m_model->pageAt(i);
        if (page->source().host() != nameSpace)
            continue;
        if (tryReload  && HelpEngineWrapper::instance().findFile(page->source()).isValid())
            page->reload();
        else if (m_model->rowCount() == 1)
            page->setSource(QUrl(QLatin1String("about:blank")));
        else
            removePage(i);
    }
}

bool OpenPagesManager::pagesOpenForNamespace(const QString &nameSpace) const
{
    TRACE_OBJ
    for (int i = 0; i < m_model->rowCount(); ++i)
        if (m_model->pageAt(i)->source().host() == nameSpace)
            return true;
    return false;
}

void OpenPagesManager::setCurrentPage(const QModelIndex &index)
{
    TRACE_OBJ
    if (index.isValid())
        setCurrentPage(index.row());
}

void OpenPagesManager::setCurrentPage(int index)
{
    TRACE_OBJ
    setCurrentPage(m_model->pageAt(index));
}

void OpenPagesManager::setCurrentPage(HelpViewer *page)
{
    TRACE_OBJ
    CentralWidget::instance()->setCurrentPage(page);
    m_openPagesWidget->selectCurrentPage();
}

void OpenPagesManager::removePage(int index)
{
    TRACE_OBJ
    emit aboutToClosePage(index);

    CentralWidget::instance()->removePage(index);
    m_model->removePage(index);
    m_openPagesWidget->selectCurrentPage();

    emit pageClosed();
}


void OpenPagesManager::closePagesExcept(const QModelIndex &index)
{
    TRACE_OBJ
    if (!index.isValid())
        return;

    int i = 0;
    HelpViewer *viewer = m_model->pageAt(index.row());
    while (m_model->rowCount() > 1) {
        if (m_model->pageAt(i) != viewer)
            removePage(i);
        else
            ++i;
    }
}

QAbstractItemView *OpenPagesManager::openPagesWidget() const
{
    TRACE_OBJ
    return m_openPagesWidget;
}

void OpenPagesManager::nextPage()
{
    TRACE_OBJ
    nextOrPreviousPage(1);
}

void OpenPagesManager::nextPageWithSwitcher()
{
    TRACE_OBJ
    if (!m_openPagesSwitcher->isVisible()) {
        m_openPagesSwitcher->selectCurrentPage();
        m_openPagesSwitcher->gotoNextPage();
        showSwitcherOrSelectPage();
    } else {
        m_openPagesSwitcher->gotoNextPage();
    }
}

void OpenPagesManager::previousPage()
{
    TRACE_OBJ
    nextOrPreviousPage(-1);
}

void OpenPagesManager::previousPageWithSwitcher()
{
    TRACE_OBJ
    if (!m_openPagesSwitcher->isVisible()) {
        m_openPagesSwitcher->selectCurrentPage();
        m_openPagesSwitcher->gotoPreviousPage();
        showSwitcherOrSelectPage();
    } else {
        m_openPagesSwitcher->gotoPreviousPage();
    }
}

void OpenPagesManager::nextOrPreviousPage(int offset)
{
    TRACE_OBJ
    setCurrentPage((CentralWidget::instance()->currentIndex() + offset
        + m_model->rowCount()) % m_model->rowCount());
}

void OpenPagesManager::showSwitcherOrSelectPage() const
{
    TRACE_OBJ
    if (QApplication::keyboardModifiers() != Qt::NoModifier) {
        const int width = CentralWidget::instance()->width();
        const int height = CentralWidget::instance()->height();
        const QPoint p(CentralWidget::instance()->mapToGlobal(QPoint(0, 0)));
        m_openPagesSwitcher->move((width - m_openPagesSwitcher->width()) / 2 + p.x(),
            (height - m_openPagesSwitcher->height()) / 2 + p.y());
        m_openPagesSwitcher->setVisible(true);
    } else {
        m_openPagesSwitcher->selectAndHide();
    }
}

QT_END_NAMESPACE
