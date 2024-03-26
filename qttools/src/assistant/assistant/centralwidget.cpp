// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "centralwidget.h"

#include "findwidget.h"
#include "helpenginewrapper.h"
#include "helpviewer.h"
#include "openpagesmanager.h"
#include "tracer.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QTimer>

#include <QtGui/QKeyEvent>
#include <QtWidgets/QMenu>
#ifndef QT_NO_PRINTER
#include <QtPrintSupport/QPageSetupDialog>
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrintPreviewDialog>
#include <QtPrintSupport/QPrinter>
#endif
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QVBoxLayout>

#include <QtHelp/QHelpSearchEngine>

QT_BEGIN_NAMESPACE

namespace {
    CentralWidget *staticCentralWidget = nullptr;
}

// -- TabBar

TabBar::TabBar(QWidget *parent)
    : QTabBar(parent)
{
    TRACE_OBJ
#ifdef Q_OS_MAC
    setDocumentMode(true);
#endif
    setMovable(true);
    setShape(QTabBar::RoundedNorth);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred,
        QSizePolicy::TabWidget));
    connect(this, &QTabBar::currentChanged,
            this, &TabBar::slotCurrentChanged);
    connect(this, &QTabBar::tabCloseRequested,
            this, &TabBar::slotTabCloseRequested);
    connect(this, &QWidget::customContextMenuRequested,
            this, &TabBar::slotCustomContextMenuRequested);
}

TabBar::~TabBar()
{
    TRACE_OBJ
}

int TabBar::addNewTab(const QString &title)
{
    TRACE_OBJ
    const int index = addTab(title);
    setTabsClosable(count() > 1);
    return index;
}

void TabBar::setCurrent(HelpViewer *viewer)
{
    TRACE_OBJ
    for (int i = 0; i < count(); ++i) {
        HelpViewer *data = tabData(i).value<HelpViewer*>();
        if (data == viewer) {
            setCurrentIndex(i);
            break;
        }
    }
}

void TabBar::removeTabAt(HelpViewer *viewer)
{
    TRACE_OBJ
    for (int i = 0; i < count(); ++i) {
        HelpViewer *data = tabData(i).value<HelpViewer*>();
        if (data == viewer) {
            removeTab(i);
            break;
        }
    }
    setTabsClosable(count() > 1);
}

void TabBar::titleChanged()
{
    TRACE_OBJ
    for (int i = 0; i < count(); ++i) {
        HelpViewer *data = tabData(i).value<HelpViewer*>();
        QString title = data->title();
        title.replace(QLatin1Char('&'), QLatin1String("&&"));
        setTabText(i, title.isEmpty() ? tr("(Untitled)") : title);
    }
}

void TabBar::slotCurrentChanged(int index)
{
    TRACE_OBJ
    emit currentTabChanged(tabData(index).value<HelpViewer*>());
}

void TabBar::slotTabCloseRequested(int index)
{
    TRACE_OBJ
    OpenPagesManager::instance()->closePage(tabData(index).value<HelpViewer*>());
}

void TabBar::slotCustomContextMenuRequested(const QPoint &pos)
{
    TRACE_OBJ
    const int tab = tabAt(pos);
    if (tab < 0)
        return;

    QMenu menu(QString(), this);
    menu.addAction(tr("New &Tab"), OpenPagesManager::instance(),
                   &OpenPagesManager::createBlankPage);

    const bool enableAction = count() > 1;
    QAction *closePage = menu.addAction(tr("&Close Tab"));
    closePage->setEnabled(enableAction);

    QAction *closePages = menu.addAction(tr("Close Other Tabs"));
    closePages->setEnabled(enableAction);

    menu.addSeparator();

    HelpViewer *viewer = tabData(tab).value<HelpViewer*>();
    QAction *newBookmark = menu.addAction(tr("Add Bookmark for this Page..."));
    const QString &url = viewer->source().toString();
    if (url.isEmpty() || url == QLatin1String("about:blank"))
        newBookmark->setEnabled(false);

    QAction *pickedAction = menu.exec(mapToGlobal(pos));
    if (pickedAction == closePage)
        slotTabCloseRequested(tab);
    else if (pickedAction == closePages) {
        for (int i = count() - 1; i >= 0; --i) {
            if (i != tab)
                slotTabCloseRequested(i);
        }
    } else if (pickedAction == newBookmark)
        emit addBookmark(viewer->title(), url);
}

// -- CentralWidget

CentralWidget::CentralWidget(QWidget *parent)
    : QWidget(parent)
#ifndef QT_NO_PRINTER
    , m_printer(nullptr)
#endif
    , m_findWidget(new FindWidget(this))
    , m_stackedWidget(new QStackedWidget(this))
    , m_tabBar(new TabBar(this))
{
    TRACE_OBJ
    staticCentralWidget = this;
    QVBoxLayout *vboxLayout = new QVBoxLayout(this);

    vboxLayout->setContentsMargins(QMargins());
    vboxLayout->setSpacing(0);
    vboxLayout->addWidget(m_tabBar);
    m_tabBar->setVisible(HelpEngineWrapper::instance().showTabs());
    vboxLayout->addWidget(m_stackedWidget);
    vboxLayout->addWidget(m_findWidget);
    m_findWidget->hide();

    connect(m_findWidget, &FindWidget::findNext, this, &CentralWidget::findNext);
    connect(m_findWidget, &FindWidget::findPrevious, this, &CentralWidget::findPrevious);
    connect(m_findWidget, &FindWidget::find, this, &CentralWidget::find);
    connect(m_findWidget, &FindWidget::escapePressed, this, &CentralWidget::activateTab);
    connect(m_tabBar, &TabBar::addBookmark, this, &CentralWidget::addBookmark);
}

CentralWidget::~CentralWidget()
{
    TRACE_OBJ
    QStringList zoomFactors;
    QStringList currentPages;
    for (int i = 0; i < m_stackedWidget->count(); ++i) {
        const HelpViewer * const viewer = viewerAt(i);
        const QUrl &source = viewer->source();
        if (source.isValid()) {
            currentPages << source.toString();
            zoomFactors << QString::number(viewer->scale());
        }
    }

    HelpEngineWrapper &helpEngine = HelpEngineWrapper::instance();
    helpEngine.setLastShownPages(currentPages);
    helpEngine.setLastZoomFactors(zoomFactors);
    helpEngine.setLastTabPage(m_stackedWidget->currentIndex());

#ifndef QT_NO_PRINTER
    delete m_printer;
#endif
}

CentralWidget *CentralWidget::instance()
{
    TRACE_OBJ
    return staticCentralWidget;
}

QUrl CentralWidget::currentSource() const
{
    TRACE_OBJ
    return currentHelpViewer()->source();
}

QString CentralWidget::currentTitle() const
{
    TRACE_OBJ
    return currentHelpViewer()->title();
}

bool CentralWidget::hasSelection() const
{
    TRACE_OBJ
    return !currentHelpViewer()->selectedText().isEmpty();
}

bool CentralWidget::isForwardAvailable() const
{
    TRACE_OBJ
    return currentHelpViewer()->isForwardAvailable();
}

bool CentralWidget::isBackwardAvailable() const
{
    TRACE_OBJ
    return currentHelpViewer()->isBackwardAvailable();
}

HelpViewer* CentralWidget::viewerAt(int index) const
{
    TRACE_OBJ
    return static_cast<HelpViewer*>(m_stackedWidget->widget(index));
}

HelpViewer* CentralWidget::currentHelpViewer() const
{
    TRACE_OBJ
    return static_cast<HelpViewer *>(m_stackedWidget->currentWidget());
}

void CentralWidget::addPage(HelpViewer *page, bool fromSearch)
{
    TRACE_OBJ
    page->installEventFilter(this);
    page->setFocus(Qt::OtherFocusReason);
    connectSignals(page);
    const int index = m_stackedWidget->addWidget(page);
    m_tabBar->setTabData(m_tabBar->addNewTab(page->title()),
        QVariant::fromValue(viewerAt(index)));
    connect(page, &HelpViewer::titleChanged, m_tabBar, &TabBar::titleChanged);

    if (fromSearch) {
        connect(currentHelpViewer(), &HelpViewer::loadFinished,
                this, &CentralWidget::highlightSearchTerms);
    }
}

void CentralWidget::removePage(int index)
{
    TRACE_OBJ
    const bool currentChanged = index == currentIndex();
    m_tabBar->removeTabAt(viewerAt(index));
    m_stackedWidget->removeWidget(m_stackedWidget->widget(index));
    if (currentChanged)
        emit currentViewerChanged();
}

int CentralWidget::currentIndex() const
{
    TRACE_OBJ
    return  m_stackedWidget->currentIndex();
}

void CentralWidget::setCurrentPage(HelpViewer *page)
{
    TRACE_OBJ
    m_tabBar->setCurrent(page);
    m_stackedWidget->setCurrentWidget(page);
    emit currentViewerChanged();
}

void CentralWidget::connectTabBar()
{
    TRACE_OBJ
    connect(m_tabBar, &TabBar::currentTabChanged, OpenPagesManager::instance(),
            QOverload<HelpViewer *>::of(&OpenPagesManager::setCurrentPage));
}

// -- public slots

#if QT_CONFIG(clipboard)
void CentralWidget::copy()
{
    TRACE_OBJ
    currentHelpViewer()->copy();
}
#endif

void CentralWidget::home()
{
    TRACE_OBJ
    currentHelpViewer()->home();
}

void CentralWidget::zoomIn()
{
    TRACE_OBJ
    currentHelpViewer()->scaleUp();
}

void CentralWidget::zoomOut()
{
    TRACE_OBJ
    currentHelpViewer()->scaleDown();
}

void CentralWidget::resetZoom()
{
    TRACE_OBJ
    currentHelpViewer()->resetScale();
}

void CentralWidget::forward()
{
    TRACE_OBJ
    currentHelpViewer()->forward();
}

void CentralWidget::nextPage()
{
    TRACE_OBJ
    m_stackedWidget->setCurrentIndex((m_stackedWidget->currentIndex() + 1)
        % m_stackedWidget->count());
}

void CentralWidget::backward()
{
    TRACE_OBJ
    currentHelpViewer()->backward();
}

void CentralWidget::previousPage()
{
    TRACE_OBJ
    m_stackedWidget->setCurrentIndex((m_stackedWidget->currentIndex() - 1)
        % m_stackedWidget->count());
}

void CentralWidget::print()
{
    TRACE_OBJ
#if !defined(QT_NO_PRINTER) && !defined(QT_NO_PRINTDIALOG)
    initPrinter();
    QPrintDialog dlg(m_printer, this);

    if (!currentHelpViewer()->selectedText().isEmpty())
        dlg.setOption(QAbstractPrintDialog::PrintSelection);
    dlg.setOption(QAbstractPrintDialog::PrintPageRange);
    dlg.setOption(QAbstractPrintDialog::PrintCollateCopies);
    dlg.setWindowTitle(tr("Print Document"));
    if (dlg.exec() == QDialog::Accepted)
        currentHelpViewer()->print(m_printer);
#endif
}

void CentralWidget::pageSetup()
{
    TRACE_OBJ
#if !defined(QT_NO_PRINTER) && !defined(QT_NO_PRINTDIALOG)
    initPrinter();
    QPageSetupDialog dlg(m_printer);
    dlg.exec();
#endif
}

void CentralWidget::printPreview()
{
    TRACE_OBJ
#if !defined(QT_NO_PRINTER) && !defined(QT_NO_PRINTDIALOG)
    initPrinter();
    QPrintPreviewDialog preview(m_printer, this);
    connect(&preview, &QPrintPreviewDialog::paintRequested,
            this, &CentralWidget::printPreviewToPrinter);
    preview.exec();
#endif
}

void CentralWidget::setSource(const QUrl &url)
{
    TRACE_OBJ
    HelpViewer *viewer = currentHelpViewer();
    viewer->setSource(url);
    viewer->setFocus(Qt::OtherFocusReason);
}

void CentralWidget::setSourceFromSearch(const QUrl &url)
{
    TRACE_OBJ
    connect(currentHelpViewer(), &HelpViewer::loadFinished,
            this, &CentralWidget::highlightSearchTerms);
    currentHelpViewer()->setSource(url);
    currentHelpViewer()->setFocus(Qt::OtherFocusReason);
}

void CentralWidget::findNext()
{
    TRACE_OBJ
    find(m_findWidget->text(), true, false);
}

void CentralWidget::findPrevious()
{
    TRACE_OBJ
    find(m_findWidget->text(), false, false);
}

void CentralWidget::find(const QString &ttf, bool forward, bool incremental)
{
    TRACE_OBJ
    bool found = false;
    if (HelpViewer *viewer = currentHelpViewer()) {
        HelpViewer::FindFlags flags;
        if (!forward)
            flags |= HelpViewer::FindBackward;
        if (m_findWidget->caseSensitive())
            flags |= HelpViewer::FindCaseSensitively;
        found = viewer->findText(ttf, flags, incremental, false);
    }

    if (!found && ttf.isEmpty())
        found = true;   // the line edit is empty, no need to mark it red...

    if (!m_findWidget->isVisible())
        m_findWidget->show();
    m_findWidget->setPalette(found);
}

void CentralWidget::activateTab()
{
    TRACE_OBJ
    currentHelpViewer()->setFocus();
}

void CentralWidget::showTextSearch()
{
    TRACE_OBJ
    m_findWidget->show();
}

void CentralWidget::updateBrowserFont()
{
    TRACE_OBJ
    const int count = m_stackedWidget->count();
    const QFont &font = viewerAt(count - 1)->viewerFont();
    for (int i = 0; i < count; ++i)
        viewerAt(i)->setViewerFont(font);
}

void CentralWidget::updateUserInterface()
{
    m_tabBar->setVisible(HelpEngineWrapper::instance().showTabs());
}

// -- protected

void CentralWidget::keyPressEvent(QKeyEvent *e)
{
    TRACE_OBJ
    const QString &text = e->text();
    if (text.startsWith(QLatin1Char('/'))) {
        if (!m_findWidget->isVisible()) {
            m_findWidget->showAndClear();
        } else {
            m_findWidget->show();
        }
    } else {
        QWidget::keyPressEvent(e);
    }
}

void CentralWidget::focusInEvent(QFocusEvent * /* event */)
{
    TRACE_OBJ
    // If we have a current help viewer then this is the 'focus proxy',
    // otherwise it's the central widget. This is needed, so an embedding
    // program can just set the focus to the central widget and it does
    // The Right Thing(TM)
    QWidget *receiver = m_stackedWidget;
    if (HelpViewer *viewer = currentHelpViewer())
        receiver = viewer;
    QTimer::singleShot(1, receiver,
                       QOverload<>::of(&QWidget::setFocus));
}

// -- private slots

void CentralWidget::highlightSearchTerms()
{
    TRACE_OBJ
    QHelpSearchEngine *searchEngine =
        HelpEngineWrapper::instance().searchEngine();
    const QString searchInput = searchEngine->searchInput();
    const bool wholePhrase = searchInput.startsWith(QLatin1Char('"')) &&
                             searchInput.endsWith(QLatin1Char('"'));
    const QStringList &words = wholePhrase ? QStringList(searchInput.mid(1, searchInput.size() - 2)) :
                                searchInput.split(QRegularExpression("\\W+"), Qt::SkipEmptyParts);
    HelpViewer *viewer = currentHelpViewer();
    for (const QString &word : words)
        viewer->findText(word, {}, false, true);
    disconnect(viewer, &HelpViewer::loadFinished,
               this, &CentralWidget::highlightSearchTerms);
}

void CentralWidget::printPreviewToPrinter(QPrinter *p)
{
    TRACE_OBJ
#ifndef QT_NO_PRINTER
    currentHelpViewer()->print(p);
#endif
}

void CentralWidget::handleSourceChanged(const QUrl &url)
{
    TRACE_OBJ
    if (sender() == currentHelpViewer())
        emit sourceChanged(url);
}

void CentralWidget::slotHighlighted(const QUrl &link)
{
    TRACE_OBJ
    QUrl resolvedLink = m_resolvedLinks.value(link);
    if (!link.isEmpty() && resolvedLink.isEmpty()) {
        resolvedLink = HelpEngineWrapper::instance().findFile(link);
        m_resolvedLinks.insert(link, resolvedLink);
    }
    emit highlighted(resolvedLink);
}

// -- private

void CentralWidget::initPrinter()
{
    TRACE_OBJ
#ifndef QT_NO_PRINTER
    if (!m_printer)
        m_printer = new QPrinter(QPrinter::HighResolution);
#endif
}

void CentralWidget::connectSignals(HelpViewer *page)
{
    TRACE_OBJ
#if defined(BROWSER_QTWEBKIT)
    connect(page, &HelpViewer::printRequested,
            this, &CentralWidget::print);
#endif
#if QT_CONFIG(clipboard)
    connect(page, &HelpViewer::copyAvailable,
            this, &CentralWidget::copyAvailable);
#endif
    connect(page, &HelpViewer::forwardAvailable,
            this, &CentralWidget::forwardAvailable);
    connect(page, &HelpViewer::backwardAvailable,
            this, &CentralWidget::backwardAvailable);
    connect(page, &HelpViewer::sourceChanged,
            this, &CentralWidget::handleSourceChanged);
    connect(page, QOverload<const QUrl &>::of(&HelpViewer::highlighted),
            this, &CentralWidget::slotHighlighted);
}

bool CentralWidget::eventFilter(QObject *object, QEvent *e)
{
    TRACE_OBJ
    if (e->type() != QEvent::KeyPress)
        return QWidget::eventFilter(object, e);

    HelpViewer *viewer = currentHelpViewer();
    QKeyEvent *keyEvent = static_cast<QKeyEvent*> (e);
    if (viewer == object && keyEvent->key() == Qt::Key_Backspace) {
        if (viewer->isBackwardAvailable()) {
#if defined(BROWSER_QTWEBKIT)
            // this helps in case there is an html <input> field
            if (!viewer->hasFocus())
#endif // BROWSER_QTWEBKIT
                viewer->backward();
        }
    }
    return QWidget::eventFilter(object, e);
}

QT_END_NAMESPACE
