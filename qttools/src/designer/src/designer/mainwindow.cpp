/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Designer of the Qt Toolkit.
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

#include "mainwindow.h"
#include "qdesigner.h"
#include "qdesigner_actions.h"
#include "qdesigner_workbench.h"
#include "qdesigner_formwindow.h"
#include "qdesigner_toolwindow.h"
#include "qdesigner_settings.h"
#include "qttoolbardialog.h"

#include <QtDesigner/abstractformwindow.h>

#include <QtWidgets/qaction.h>
#include <QtGui/qevent.h>
#include <QtWidgets/qtoolbar.h>
#include <QtWidgets/qmdisubwindow.h>
#include <QtWidgets/qstatusbar.h>
#include <QtWidgets/qmenu.h>
#include <QtWidgets/qlayout.h>
#include <QtWidgets/qdockwidget.h>

#include <QtCore/qurl.h>
#include <QtCore/qdebug.h>
#include <QtCore/qmimedata.h>

static const char *uriListMimeFormatC = "text/uri-list";

QT_BEGIN_NAMESPACE

typedef QList<QAction *> ActionList;

// Helpers for creating toolbars and menu

static void addActionsToToolBar(const ActionList &actions, QToolBar *t)
{
    for (QAction *action : actions) {
        if (action->property(QDesignerActions::defaultToolbarPropertyName).toBool())
            t->addAction(action);
    }
}
static QToolBar *createToolBar(const QString &title, const QString &objectName, const ActionList &actions)
{
    QToolBar *rc =  new QToolBar;
    rc->setObjectName(objectName);
    rc->setWindowTitle(title);
    addActionsToToolBar(actions, rc);
    return rc;
}

// ---------------- MainWindowBase

MainWindowBase::MainWindowBase(QWidget *parent, Qt::WindowFlags flags) :
    QMainWindow(parent, flags),
    m_policy(AcceptCloseEvents)
{
#ifndef Q_OS_MACOS
    setWindowIcon(qDesigner->windowIcon());
#endif
}

void MainWindowBase::closeEvent(QCloseEvent *e)
{
    switch (m_policy) {
    case AcceptCloseEvents:
        QMainWindow::closeEvent(e);
        break;
      case EmitCloseEventSignal:
        emit closeEventReceived(e);
        break;
    }
}

QVector<QToolBar *> MainWindowBase::createToolBars(const QDesignerActions *actions, bool singleToolBar)
{
    // Note that whenever you want to add a new tool bar here, you also have to update the default
    // action groups added to the toolbar manager in the mainwindow constructor
    QVector<QToolBar *> rc;
    if (singleToolBar) {
        //: Not currently used (main tool bar)
        QToolBar *main = createToolBar(tr("Main"), QStringLiteral("mainToolBar"), actions->fileActions()->actions());
        addActionsToToolBar(actions->editActions()->actions(), main);
        addActionsToToolBar(actions->toolActions()->actions(), main);
        addActionsToToolBar(actions->formActions()->actions(), main);
        rc.push_back(main);
    } else {
        rc.push_back(createToolBar(tr("File"), QStringLiteral("fileToolBar"), actions->fileActions()->actions()));
        rc.push_back(createToolBar(tr("Edit"), QStringLiteral("editToolBar"),  actions->editActions()->actions()));
        rc.push_back(createToolBar(tr("Tools"), QStringLiteral("toolsToolBar"), actions->toolActions()->actions()));
        rc.push_back(createToolBar(tr("Form"), QStringLiteral("formToolBar"), actions->formActions()->actions()));
    }
    return rc;
}

QString MainWindowBase::mainWindowTitle()
{
    return tr("Qt Designer");
}

// Use the minor Qt version as settings versions to avoid conflicts
int MainWindowBase::settingsVersion()
{
    const int version = QT_VERSION;
    return (version & 0x00FF00) >> 8;
}

// ----------------- DockedMdiArea

DockedMdiArea::DockedMdiArea(const QString &extension, QWidget *parent) :
    QMdiArea(parent),
    m_extension(extension)
{
    setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    setLineWidth(1);
    setAcceptDrops(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

QStringList DockedMdiArea::uiFiles(const QMimeData *d) const
{
    // Extract dropped UI files from Mime data.
    QStringList rc;
    if (!d->hasFormat(QLatin1String(uriListMimeFormatC)))
        return rc;
    const QList<QUrl> urls = d->urls();
    if (urls.empty())
        return rc;
    const QList<QUrl>::const_iterator cend = urls.constEnd();
    for (QList<QUrl>::const_iterator it = urls.constBegin(); it != cend; ++it) {
        const QString fileName = it->toLocalFile();
        if (!fileName.isEmpty() && fileName.endsWith(m_extension))
            rc.push_back(fileName);
    }
    return rc;
}

bool DockedMdiArea::event(QEvent *event)
{
    // Listen for desktop file manager drop and emit a signal once a file is
    // dropped.
    switch (event->type()) {
    case QEvent::DragEnter: {
        QDragEnterEvent *e = static_cast<QDragEnterEvent*>(event);
        if (!uiFiles(e->mimeData()).empty()) {
            e->acceptProposedAction();
            return true;
        }
    }
        break;
    case QEvent::Drop: {
        QDropEvent *e = static_cast<QDropEvent*>(event);
        const QStringList files = uiFiles(e->mimeData());
        const QStringList::const_iterator cend = files.constEnd();
        for (QStringList::const_iterator it = files.constBegin(); it != cend; ++it) {
            emit fileDropped(*it);
        }
        e->acceptProposedAction();
        return true;
    }
        break;
    default:
        break;
    }
    return QMdiArea::event(event);
}

// ------------- ToolBarManager:

static void addActionsToToolBarManager(const ActionList &al, const QString &title, QtToolBarManager *tbm)
{
    for (QAction *action : al)
        tbm->addAction(action, title);
}

ToolBarManager::ToolBarManager(QMainWindow *configureableMainWindow,
                                         QWidget *parent,
                                         QMenu *toolBarMenu,
                                         const QDesignerActions *actions,
                                         const QVector<QToolBar *> &toolbars,
                                         const QVector<QDesignerToolWindow *> &toolWindows) :
    QObject(parent),
    m_configureableMainWindow(configureableMainWindow),
    m_parent(parent),
    m_toolBarMenu(toolBarMenu),
    m_manager(new QtToolBarManager(this)),
    m_configureAction(new QAction(tr("Configure Toolbars..."), this)),
    m_toolbars(toolbars)
{
    m_configureAction->setMenuRole(QAction::NoRole);
    m_configureAction->setObjectName(QStringLiteral("__qt_configure_tool_bars_action"));
    connect(m_configureAction, &QAction::triggered, this, &ToolBarManager::configureToolBars);

    m_manager->setMainWindow(configureableMainWindow);

    for (QToolBar *tb : qAsConst(m_toolbars)) {
        const QString title = tb->windowTitle();
        m_manager->addToolBar(tb, title);
        addActionsToToolBarManager(tb->actions(), title, m_manager);
    }

    addActionsToToolBarManager(actions->windowActions()->actions(), tr("Window"), m_manager);
    addActionsToToolBarManager(actions->helpActions()->actions(), tr("Help"), m_manager);

    // Filter out the device profile preview actions which have int data().
    ActionList previewActions = actions->styleActions()->actions();
    ActionList::iterator it = previewActions.begin();
    for ( ; (*it)->isSeparator() || (*it)->data().type() == QVariant::Int; ++it) ;
    previewActions.erase(previewActions.begin(), it);
    addActionsToToolBarManager(previewActions, tr("Style"), m_manager);

    const QString dockTitle = tr("Dock views");
    for (QDesignerToolWindow *tw : toolWindows) {
        if (QAction *action = tw->action())
            m_manager->addAction(action, dockTitle);
    }

    addActionsToToolBarManager(actions->fileActions()->actions(), tr("File"), m_manager);
    addActionsToToolBarManager(actions->editActions()->actions(), tr("Edit"), m_manager);
    addActionsToToolBarManager(actions->toolActions()->actions(), tr("Tools"), m_manager);
    addActionsToToolBarManager(actions->formActions()->actions(), tr("Form"), m_manager);

    m_manager->addAction(m_configureAction, tr("Toolbars"));
    updateToolBarMenu();
}

// sort function for sorting tool bars alphabetically by title [non-static since called from template]

bool toolBarTitleLessThan(const QToolBar *t1, const QToolBar *t2)
{
    return t1->windowTitle() < t2->windowTitle();
}

void ToolBarManager::updateToolBarMenu()
{
    // Sort tool bars alphabetically by title
    qStableSort(m_toolbars.begin(), m_toolbars.end(), toolBarTitleLessThan);
    // add to menu
    m_toolBarMenu->clear();
    for (QToolBar *tb : qAsConst(m_toolbars))
        m_toolBarMenu->addAction(tb->toggleViewAction());
    m_toolBarMenu->addAction(m_configureAction);
}

void ToolBarManager::configureToolBars()
{
    QtToolBarDialog dlg(m_parent);
    dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    dlg.setToolBarManager(m_manager);
    dlg.exec();
    updateToolBarMenu();
}

QByteArray ToolBarManager::saveState(int version) const
{
    return m_manager->saveState(version);
}

bool ToolBarManager::restoreState(const QByteArray &state, int version)
{
    return m_manager->restoreState(state, version);
}

// ---------- DockedMainWindow

DockedMainWindow::DockedMainWindow(QDesignerWorkbench *wb,
                                   QMenu *toolBarMenu,
                                   const QVector<QDesignerToolWindow *> &toolWindows) :
    m_toolBarManager(0)
{
    setObjectName(QStringLiteral("MDIWindow"));
    setWindowTitle(mainWindowTitle());

    const QVector<QToolBar *> toolbars = createToolBars(wb->actionManager(), false);
    for (QToolBar *tb : toolbars)
        addToolBar(tb);
    DockedMdiArea *dma = new DockedMdiArea(wb->actionManager()->uiExtension());
    connect(dma, &DockedMdiArea::fileDropped,
            this, &DockedMainWindow::fileDropped);
    connect(dma, &QMdiArea::subWindowActivated,
            this, &DockedMainWindow::slotSubWindowActivated);
    setCentralWidget(dma);

    QStatusBar *sb = statusBar();
    Q_UNUSED(sb)

    m_toolBarManager = new ToolBarManager(this, this, toolBarMenu, wb->actionManager(), toolbars, toolWindows);
}

QMdiArea *DockedMainWindow::mdiArea() const
{
    return static_cast<QMdiArea *>(centralWidget());
}

void DockedMainWindow::slotSubWindowActivated(QMdiSubWindow* subWindow)
{
    if (subWindow) {
        QWidget *widget = subWindow->widget();
        if (QDesignerFormWindow *fw = qobject_cast<QDesignerFormWindow*>(widget)) {
            emit formWindowActivated(fw);
            mdiArea()->setActiveSubWindow(subWindow);
        }
    }
}

// Create a MDI subwindow for the form.
QMdiSubWindow *DockedMainWindow::createMdiSubWindow(QWidget *fw, Qt::WindowFlags f, const QKeySequence &designerCloseActionShortCut)
{
    QMdiSubWindow *rc = mdiArea()->addSubWindow(fw, f);
    // Make action shortcuts respond only if focused to avoid conflicts with
    // designer menu actions
    if (designerCloseActionShortCut == QKeySequence(QKeySequence::Close)) {
        const ActionList systemMenuActions = rc->systemMenu()->actions();
        if (!systemMenuActions.empty()) {
            const ActionList::const_iterator cend = systemMenuActions.constEnd();
            for (ActionList::const_iterator it = systemMenuActions.constBegin(); it != cend; ++it) {
                if ( (*it)->shortcut() == designerCloseActionShortCut) {
                    (*it)->setShortcutContext(Qt::WidgetShortcut);
                    break;
                }
            }
        }
    }
    return rc;
}

DockedMainWindow::DockWidgetList DockedMainWindow::addToolWindows(const DesignerToolWindowList &tls)
{
    DockWidgetList rc;
    for (QDesignerToolWindow *tw : tls) {
        QDockWidget *dockWidget = new QDockWidget;
        dockWidget->setObjectName(tw->objectName() + QStringLiteral("_dock"));
        dockWidget->setWindowTitle(tw->windowTitle());
        addDockWidget(tw->dockWidgetAreaHint(), dockWidget);
        dockWidget->setWidget(tw);
        rc.push_back(dockWidget);
    }
    return rc;
}

// Settings consist of MainWindow state and tool bar manager state
void DockedMainWindow::restoreSettings(const QDesignerSettings &s, const DockWidgetList &dws, const QRect &desktopArea)
{
    const int version = settingsVersion();
    m_toolBarManager->restoreState(s.toolBarsState(DockedMode), version);

    // If there are no old geometry settings, show the window maximized
    s.restoreGeometry(this, QRect(desktopArea.topLeft(), QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX)));

    const QByteArray mainWindowState = s.mainWindowState(DockedMode);
    const bool restored = !mainWindowState.isEmpty() && restoreState(mainWindowState, version);
    if (!restored) {
        // Default: Tabify less relevant windows bottom/right.
        tabifyDockWidget(dws.at(QDesignerToolWindow::SignalSlotEditor),
                         dws.at(QDesignerToolWindow::ActionEditor));
        tabifyDockWidget(dws.at(QDesignerToolWindow::ActionEditor),
                         dws.at(QDesignerToolWindow::ResourceEditor));
    }
}

void DockedMainWindow::saveSettings(QDesignerSettings &s) const
{
    const int version = settingsVersion();
    s.setToolBarsState(DockedMode, m_toolBarManager->saveState(version));
    s.saveGeometryFor(this);
    s.setMainWindowState(DockedMode, saveState(version));
}

QT_END_NAMESPACE
