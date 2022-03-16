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

#include "qdesigner_workbench.h"
#include "qdesigner.h"
#include "qdesigner_actions.h"
#include "qdesigner_appearanceoptions.h"
#include "qdesigner_settings.h"
#include "qdesigner_toolwindow.h"
#include "qdesigner_formwindow.h"
#include "appfontdialog.h"

#include <QtDesigner/abstractformeditor.h>
#include <QtDesigner/abstractformwindow.h>
#include <QtDesigner/abstractformwindowmanager.h>
#include <QtDesigner/abstractformeditorplugin.h>
#include <QtDesigner/abstractwidgetbox.h>
#include <QtDesigner/abstractmetadatabase.h>

#include <QtDesigner/QDesignerComponents>
#include <QtDesigner/abstractintegration.h>
#include <QtDesigner/private/pluginmanager_p.h>
#include <QtDesigner/private/formwindowbase_p.h>
#include <QtDesigner/private/actioneditor_p.h>

#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qurl.h>
#include <QtCore/qtimer.h>
#include <QtCore/qpluginloader.h>
#include <QtCore/qdebug.h>

#include <QtWidgets/qactiongroup.h>
#include <QtGui/qevent.h>
#include <QtWidgets/qdesktopwidget.h>
#include <QtWidgets/qdockwidget.h>
#include <QtWidgets/qmenu.h>
#include <QtWidgets/qmenubar.h>
#include <QtWidgets/qmessagebox.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qtoolbar.h>
#include <QtWidgets/qmdiarea.h>
#include <QtWidgets/qmdisubwindow.h>
#include <QtWidgets/qlayout.h>

QT_BEGIN_NAMESPACE

static const char *appFontPrefixC = "AppFonts";

typedef QList<QAction *> ActionList;

static QMdiSubWindow *mdiSubWindowOf(const QWidget *w)
{
    QMdiSubWindow *rc = qobject_cast<QMdiSubWindow *>(w->parentWidget());
    Q_ASSERT(rc);
    return rc;
}

static QDockWidget *dockWidgetOf(const QWidget *w)
{
    for (QWidget *parentWidget = w->parentWidget(); parentWidget ; parentWidget = parentWidget->parentWidget()) {
        if (QDockWidget *dw = qobject_cast<QDockWidget *>(parentWidget)) {
            return dw;
        }
    }
    Q_ASSERT("Dock widget not found");
    return 0;
}

// ------------ QDesignerWorkbench::Position
QDesignerWorkbench::Position::Position(const QMdiSubWindow *mdiSubWindow, const QPoint &mdiAreaOffset) :
    m_minimized(mdiSubWindow->isShaded()),
    m_position(mdiSubWindow->pos() + mdiAreaOffset)
{
}

QDesignerWorkbench::Position::Position(const QDockWidget *dockWidget) :
    m_minimized(dockWidget->isMinimized()),
    m_position(dockWidget->pos())
{
}

QDesignerWorkbench::Position::Position(const QWidget *topLevelWindow, const QPoint &desktopTopLeft)
{
    const QWidget *window =topLevelWindow->window ();
    Q_ASSERT(window);
    m_minimized = window->isMinimized();
    m_position = window->pos() - desktopTopLeft;
}

void QDesignerWorkbench::Position::applyTo(QMdiSubWindow *mdiSubWindow,
                                           const QPoint &mdiAreaOffset) const
{
    // QMdiSubWindow attempts to resize its children to sizeHint() when switching user interface modes.
    // Restore old size
    const QPoint mdiAreaPos =  QPoint(qMax(0, m_position.x() - mdiAreaOffset.x()),
                                      qMax(0, m_position.y() - mdiAreaOffset.y()));
    mdiSubWindow->move(mdiAreaPos);
    const QSize decorationSize = mdiSubWindow->size() - mdiSubWindow->contentsRect().size();
    mdiSubWindow->resize(mdiSubWindow->widget()->size() + decorationSize);
    mdiSubWindow->show();
    if (m_minimized) {
        mdiSubWindow->showShaded();
    }
}

void QDesignerWorkbench::Position::applyTo(QWidget *topLevelWindow, const QPoint &desktopTopLeft) const
{
    QWidget *window = topLevelWindow->window ();
    const QPoint newPos = m_position + desktopTopLeft;
    window->move(newPos);
    if ( m_minimized) {
        topLevelWindow->showMinimized();
    } else {
        topLevelWindow->show();
    }
}

void QDesignerWorkbench::Position::applyTo(QDockWidget *dockWidget) const
{
    dockWidget->widget()->setVisible(true);
    dockWidget->setVisible(!m_minimized);
}

static inline void addActionsToMenu(QMenu *m, const ActionList &al)
{
    const ActionList::const_iterator cend = al.constEnd();
    for (ActionList::const_iterator it = al.constBegin(); it != cend; ++it)
        m->addAction(*it);
}

static inline QMenu *addMenu(QMenuBar *mb, const QString &title, const ActionList &al)
{
    QMenu *rc = mb->addMenu(title);
    addActionsToMenu(rc, al);
    return rc;
}

// -------- QDesignerWorkbench

QDesignerWorkbench::QDesignerWorkbench()  :
    m_core(QDesignerComponents::createFormEditor(this)),
    m_windowActions(new QActionGroup(this)),
    m_globalMenuBar(new QMenuBar),
    m_mode(NeutralMode),
    m_dockedMainWindow(0),
    m_state(StateInitializing),
    m_uiSettingsChanged(false)
{
    QDesignerSettings settings(m_core);

    (void) QDesignerComponents::createTaskMenu(core(), this);

    initializeCorePlugins();
    QDesignerComponents::initializePlugins(core());
    m_actionManager = new QDesignerActions(this); // accesses plugin components

    m_windowActions->setExclusive(true);
    connect(m_windowActions, &QActionGroup::triggered,
            this, &QDesignerWorkbench::formWindowActionTriggered);

    // Build main menu bar
    addMenu(m_globalMenuBar, tr("&File"), m_actionManager->fileActions()->actions());

    QMenu *editMenu = addMenu(m_globalMenuBar, tr("&Edit"), m_actionManager->editActions()->actions());
    editMenu->addSeparator();
    addActionsToMenu(editMenu, m_actionManager->toolActions()->actions());

    QMenu *formMenu = addMenu(m_globalMenuBar, tr("F&orm"),  m_actionManager->formActions()->actions());
    QMenu *previewSubMenu = new QMenu(tr("Preview in"), formMenu);
    formMenu->insertMenu(m_actionManager->previewFormAction(), previewSubMenu);
    addActionsToMenu(previewSubMenu, m_actionManager->styleActions()->actions());

    QMenu *viewMenu = m_globalMenuBar->addMenu(tr("&View"));

    addMenu(m_globalMenuBar, tr("&Settings"), m_actionManager->settingsActions()->actions());

    m_windowMenu = addMenu(m_globalMenuBar, tr("&Window"), m_actionManager->windowActions()->actions());

    addMenu(m_globalMenuBar, tr("&Help"), m_actionManager->helpActions()->actions());

    //  Add the tools in view menu order
    QActionGroup *viewActions = new QActionGroup(this);
    viewActions->setExclusive(false);

    for (int i = 0; i < QDesignerToolWindow::StandardToolWindowCount; i++) {
        QDesignerToolWindow *toolWindow = QDesignerToolWindow::createStandardToolWindow(static_cast< QDesignerToolWindow::StandardToolWindow>(i), this);
        m_toolWindows.push_back(toolWindow);
        if (QAction *action = toolWindow->action()) {
            viewMenu->addAction(action);
            viewActions->addAction(action);
        }
        // The widget box becomes the main window in top level mode
        if (i == QDesignerToolWindow::WidgetBox) {
            connect(toolWindow, &QDesignerToolWindow::closeEventReceived,
                    this, &QDesignerWorkbench::handleCloseEvent);
        }
    }
    // Integration
    m_integration = new QDesignerIntegration(m_core, this);
    connect(m_integration, &QDesignerIntegration::helpRequested,
            m_actionManager, &QDesignerActions::helpRequested);

    // remaining view options (config toolbars)
    viewMenu->addSeparator();
    m_toolbarMenu = viewMenu->addMenu(tr("Toolbars"));

    emit initialized();

    connect(m_core->formWindowManager(), &QDesignerFormWindowManagerInterface::activeFormWindowChanged,
            this, &QDesignerWorkbench::updateWindowMenu);


    { // Add application specific options pages
        QDesignerAppearanceOptionsPage *appearanceOptions = new QDesignerAppearanceOptionsPage(m_core);
        connect(appearanceOptions, &QDesignerAppearanceOptionsPage::settingsChanged, this, &QDesignerWorkbench::notifyUISettingsChanged);
        QList<QDesignerOptionsPageInterface*> optionsPages = m_core->optionsPages();
        optionsPages.push_front(appearanceOptions);
        m_core->setOptionsPages(optionsPages);
    }

    restoreUISettings();
    AppFontWidget::restore(m_core->settingsManager(), QLatin1String(appFontPrefixC));
    m_state = StateUp;
}

QDesignerWorkbench::~QDesignerWorkbench()
{
    switch (m_mode) {
    case NeutralMode:
    case DockedMode:
        qDeleteAll(m_toolWindows);
        break;
    case TopLevelMode: // Everything parented here
        delete widgetBoxToolWindow();
        break;
    }
}

void QDesignerWorkbench::saveGeometriesForModeChange()
{
    m_Positions.clear();
    switch (m_mode) {
    case NeutralMode:
        break;
    case TopLevelMode: {
        const QPoint desktopOffset = QApplication::desktop()->availableGeometry().topLeft();
        for (QDesignerToolWindow *tw : qAsConst(m_toolWindows))
            m_Positions.insert(tw, Position(tw, desktopOffset));
        for (QDesignerFormWindow *fw : qAsConst(m_formWindows))
            m_Positions.insert(fw,  Position(fw, desktopOffset));
    }
        break;
    case DockedMode: {
        const QPoint mdiAreaOffset = m_dockedMainWindow->mdiArea()->pos();
        for (QDesignerToolWindow *tw : qAsConst(m_toolWindows))
            m_Positions.insert(tw, Position(dockWidgetOf(tw)));
        for (QDesignerFormWindow *fw : qAsConst(m_formWindows))
            m_Positions.insert(fw, Position(mdiSubWindowOf(fw), mdiAreaOffset));
    }
        break;
    }
}

UIMode QDesignerWorkbench::mode() const
{
    return m_mode;
}

void QDesignerWorkbench::addFormWindow(QDesignerFormWindow *formWindow)
{
    // ### Q_ASSERT(formWindow->windowTitle().isEmpty() == false);

    m_formWindows.append(formWindow);


    m_actionManager->setWindowListSeparatorVisible(true);

    if (QAction *action = formWindow->action()) {
        m_windowActions->addAction(action);
        m_windowMenu->addAction(action);
        action->setChecked(true);
    }

    m_actionManager->minimizeAction()->setEnabled(true);
    m_actionManager->minimizeAction()->setChecked(false);
    connect(formWindow, &QDesignerFormWindow::minimizationStateChanged,
            this, &QDesignerWorkbench::minimizationStateChanged);

    m_actionManager->editWidgets()->trigger();
}

Qt::WindowFlags QDesignerWorkbench::magicalWindowFlags(const QWidget *widgetForFlags) const
{
    switch (m_mode) {
        case TopLevelMode: {
#ifdef Q_OS_MACOS
            if (qobject_cast<const QDesignerToolWindow *>(widgetForFlags))
                return Qt::Tool;
#else
            Q_UNUSED(widgetForFlags);
#endif
            return Qt::Window;
        }
        case DockedMode:
            return Qt::Window | Qt::WindowShadeButtonHint | Qt::WindowSystemMenuHint | Qt::WindowTitleHint;
        case NeutralMode:
            return Qt::Window;
        default:
            Q_ASSERT(0);
            return 0;
    }
}

QWidget *QDesignerWorkbench::magicalParent(const QWidget *w) const
{
    switch (m_mode) {
        case TopLevelMode: {
            // Use widget box as parent for all windows except self. This will
            // result in having just one entry in the MS Windows task bar.
            QWidget *widgetBoxWrapper = widgetBoxToolWindow();
            return w == widgetBoxWrapper ? 0 : widgetBoxWrapper;
        }
        case DockedMode:
            return m_dockedMainWindow->mdiArea();
        case NeutralMode:
            return 0;
        default:
            Q_ASSERT(0);
            return 0;
    }
}

void QDesignerWorkbench::switchToNeutralMode()
{
    QDesignerSettings settings(m_core);
    saveGeometries(settings);
    saveGeometriesForModeChange();

    if (m_mode == TopLevelMode) {
        delete m_topLevelData.toolbarManager;
        m_topLevelData.toolbarManager = 0;
        qDeleteAll(m_topLevelData.toolbars);
        m_topLevelData.toolbars.clear();
    }

    m_mode = NeutralMode;

    for (QDesignerToolWindow *tw : qAsConst(m_toolWindows)) {
        tw->setCloseEventPolicy(MainWindowBase::AcceptCloseEvents);
        tw->setParent(0);
    }

    for (QDesignerFormWindow *fw : qAsConst(m_formWindows)) {
        fw->setParent(0);
        fw->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    }

#ifndef Q_OS_MACOS
    m_globalMenuBar->setParent(0);
#endif

    m_core->setTopLevel(0);
    qDesigner->setMainWindow(0);

    delete m_dockedMainWindow;
    m_dockedMainWindow = 0;
}

void QDesignerWorkbench::switchToDockedMode()
{
    if (m_mode == DockedMode)
        return;

    switchToNeutralMode();

#if !defined(Q_OS_MACOS)
#    if defined(Q_OS_UNIX)
    QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, false);
#    endif // Q_OS_UNIX
    QDesignerToolWindow *widgetBoxWrapper = widgetBoxToolWindow();
    widgetBoxWrapper->action()->setVisible(true);
    widgetBoxWrapper->setWindowTitle(tr("Widget Box"));
#endif // !Q_OS_MACOS

    m_mode = DockedMode;
    const QDesignerSettings settings(m_core);
    m_dockedMainWindow = new DockedMainWindow(this, m_toolbarMenu, m_toolWindows);
    m_dockedMainWindow->setUnifiedTitleAndToolBarOnMac(true);
    m_dockedMainWindow->setCloseEventPolicy(MainWindowBase::EmitCloseEventSignal);
    connect(m_dockedMainWindow, &DockedMainWindow::closeEventReceived,
            this, &QDesignerWorkbench::handleCloseEvent);
    connect(m_dockedMainWindow, &DockedMainWindow::fileDropped,
            this, &QDesignerWorkbench::slotFileDropped);
    connect(m_dockedMainWindow, &DockedMainWindow::formWindowActivated,
            this, &QDesignerWorkbench::slotFormWindowActivated);
    m_dockedMainWindow->restoreSettings(settings, m_dockedMainWindow->addToolWindows(m_toolWindows), desktopGeometry());

    m_core->setTopLevel(m_dockedMainWindow);

#ifndef Q_OS_MACOS
    m_dockedMainWindow->setMenuBar(m_globalMenuBar);
    m_globalMenuBar->show();
#endif
    qDesigner->setMainWindow(m_dockedMainWindow);

    for (QDesignerFormWindow *fw : qAsConst(m_formWindows)) {
        QMdiSubWindow *subwin = m_dockedMainWindow->createMdiSubWindow(fw, magicalWindowFlags(fw),
                                                                       m_actionManager->closeFormAction()->shortcut());
        subwin->hide();
        if (QWidget *mainContainer = fw->editor()->mainContainer())
            resizeForm(fw, mainContainer);
    }

    m_actionManager->setBringAllToFrontVisible(false);
    m_dockedMainWindow->show();
    // Trigger adjustMDIFormPositions() delayed as viewport size is not yet known.

    if (m_state != StateInitializing)
        QMetaObject::invokeMethod(this, "adjustMDIFormPositions", Qt::QueuedConnection);
}

void QDesignerWorkbench::adjustMDIFormPositions()
{
    const QPoint mdiAreaOffset = m_dockedMainWindow->mdiArea()->pos();

    for (QDesignerFormWindow *fw : qAsConst(m_formWindows)) {
        const PositionMap::const_iterator pit = m_Positions.constFind(fw);
        if (pit != m_Positions.constEnd())
            pit->applyTo(mdiSubWindowOf(fw), mdiAreaOffset);
    }
}

void QDesignerWorkbench::switchToTopLevelMode()
{
    if (m_mode == TopLevelMode)
        return;

    // make sure that the widgetbox is visible if it is different from neutral.
    QDesignerToolWindow *widgetBoxWrapper = widgetBoxToolWindow();
    Q_ASSERT(widgetBoxWrapper);

    switchToNeutralMode();
    const QPoint desktopOffset = desktopGeometry().topLeft();
    m_mode = TopLevelMode;

    // The widget box is special, it gets the menubar and gets to be the main widget.

    m_core->setTopLevel(widgetBoxWrapper);
#if !defined(Q_OS_MACOS)
#    if defined(Q_OS_UNIX)
    // For now the appmenu protocol does not make it possible to associate a
    // menubar with all application windows. This means in top level mode you
    // can only reach the menubar when the widgetbox window is active. Since
    // this is quite inconvenient, better not use the native menubar in this
    // configuration and keep the menubar in the widgetbox window.
    QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, true);
#    endif // Q_OS_UNIX
    widgetBoxWrapper->setMenuBar(m_globalMenuBar);
    widgetBoxWrapper->action()->setVisible(false);
    widgetBoxWrapper->setCloseEventPolicy(MainWindowBase::EmitCloseEventSignal);
    qDesigner->setMainWindow(widgetBoxWrapper);
    widgetBoxWrapper->setWindowTitle(MainWindowBase::mainWindowTitle());
#endif // !Q_OS_MACOS

    const QDesignerSettings settings(m_core);
    m_topLevelData.toolbars = MainWindowBase::createToolBars(m_actionManager, false);
    m_topLevelData.toolbarManager = new ToolBarManager(widgetBoxWrapper, widgetBoxWrapper,
                                                       m_toolbarMenu, m_actionManager,
                                                       m_topLevelData.toolbars, m_toolWindows);
    const int toolBarCount = m_topLevelData.toolbars.size();
    for (int i = 0; i < toolBarCount; i++) {
        widgetBoxWrapper->addToolBar(m_topLevelData.toolbars.at(i));
        if (i == 3)
            widgetBoxWrapper->insertToolBarBreak(m_topLevelData.toolbars.at(i));
    }
    m_topLevelData.toolbarManager->restoreState(settings.toolBarsState(m_mode), MainWindowBase::settingsVersion());
    widgetBoxWrapper->restoreState(settings.mainWindowState(m_mode), MainWindowBase::settingsVersion());

    bool found_visible_window = false;
    for (QDesignerToolWindow *tw : qAsConst(m_toolWindows)) {
        tw->setParent(magicalParent(tw), magicalWindowFlags(tw));
        settings.restoreGeometry(tw, tw->geometryHint());
        tw->action()->setChecked(tw->isVisible());
        found_visible_window |= tw->isVisible();
    }

    if (!m_toolWindows.isEmpty() && !found_visible_window)
        m_toolWindows.first()->show();

    m_actionManager->setBringAllToFrontVisible(true);

    for (QDesignerFormWindow *fw : qAsConst(m_formWindows)) {
        fw->setParent(magicalParent(fw), magicalWindowFlags(fw));
        fw->setAttribute(Qt::WA_DeleteOnClose, true);
        const PositionMap::const_iterator pit = m_Positions.constFind(fw);
        if (pit != m_Positions.constEnd()) pit->applyTo(fw, desktopOffset);
        // Force an activate in order to refresh minimumSize, otherwise it will not be respected
        if (QLayout *layout = fw->layout())
            layout->invalidate();
        if (QWidget *mainContainer = fw->editor()->mainContainer())
            resizeForm(fw, mainContainer);
    }
}

QDesignerFormWindowManagerInterface *QDesignerWorkbench::formWindowManager() const
{
    return m_core->formWindowManager();
}

QDesignerFormEditorInterface *QDesignerWorkbench::core() const
{
    return m_core;
}

int QDesignerWorkbench::toolWindowCount() const
{
    return m_toolWindows.count();
}

QDesignerToolWindow *QDesignerWorkbench::toolWindow(int index) const
{
    return m_toolWindows.at(index);
}

int QDesignerWorkbench::formWindowCount() const
{
    return m_formWindows.count();
}

QDesignerFormWindow *QDesignerWorkbench::formWindow(int index) const
{
    return m_formWindows.at(index);
}

QRect QDesignerWorkbench::desktopGeometry() const
{
    // Return geometry of the desktop designer is running in.
    QWidget *widget = 0;
    switch (m_mode) {
    case DockedMode:
        widget = m_dockedMainWindow;
        break;
    case TopLevelMode:
        widget = widgetBoxToolWindow();
        break;
    case NeutralMode:
        break;
    }
    const QDesktopWidget *desktop = qApp->desktop();
    const int screenNumber = widget ? desktop->screenNumber(widget) : 0;
    return desktop->availableGeometry(screenNumber);
}

QRect QDesignerWorkbench::availableGeometry() const
{
    if (m_mode == DockedMode)
        return m_dockedMainWindow->mdiArea()->geometry();

    const QDesktopWidget *desktop = qDesigner->desktop();
    return desktop->availableGeometry(desktop->screenNumber(widgetBoxToolWindow()));
}

int QDesignerWorkbench::marginHint() const
{    return 20;
}

void QDesignerWorkbench::slotFormWindowActivated(QDesignerFormWindow* fw)
{
    core()->formWindowManager()->setActiveFormWindow(fw->editor());
}

void QDesignerWorkbench::removeFormWindow(QDesignerFormWindow *formWindow)
{
    QDesignerFormWindowInterface *editor = formWindow->editor();
    const bool loadOk = editor->mainContainer();
    updateBackup(editor);
    const int index = m_formWindows.indexOf(formWindow);
    if (index != -1) {
        m_formWindows.removeAt(index);
    }

    if (QAction *action = formWindow->action()) {
        m_windowActions->removeAction(action);
        m_windowMenu->removeAction(action);
    }

    if (m_formWindows.empty()) {
        m_actionManager->setWindowListSeparatorVisible(false);
        // Show up new form dialog unless closing
        if (loadOk && m_state == StateUp
            && QDesignerSettings(m_core).showNewFormOnStartup()) {
            QTimer::singleShot(200, m_actionManager, &QDesignerActions::createForm);
        }
    }
}

void QDesignerWorkbench::initializeCorePlugins()
{
    QObjectList plugins = QPluginLoader::staticInstances();
    plugins += core()->pluginManager()->instances();

    for (QObject *plugin : qAsConst(plugins)) {
        if (QDesignerFormEditorPluginInterface *formEditorPlugin = qobject_cast<QDesignerFormEditorPluginInterface*>(plugin)) {
            if (!formEditorPlugin->isInitialized())
                formEditorPlugin->initialize(core());
        }
    }
}

void QDesignerWorkbench::saveSettings() const
{
    QDesignerSettings settings(m_core);
    settings.clearBackup();
    saveGeometries(settings);
    AppFontWidget::save(m_core->settingsManager(), QLatin1String(appFontPrefixC));
}

void QDesignerWorkbench::saveGeometries(QDesignerSettings &settings) const
{
    switch (m_mode) {
    case DockedMode:
        m_dockedMainWindow->saveSettings(settings);
        break;
    case TopLevelMode:
        settings.setToolBarsState(m_mode, m_topLevelData.toolbarManager->saveState(MainWindowBase::settingsVersion()));
        settings.setMainWindowState(m_mode, widgetBoxToolWindow()->saveState(MainWindowBase::settingsVersion()));
        for (const QDesignerToolWindow *tw : m_toolWindows)
            settings.saveGeometryFor(tw);
        break;
    case NeutralMode:
        break;
    }
}

void QDesignerWorkbench::slotFileDropped(const QString &f)
{
    readInForm(f);
}

bool QDesignerWorkbench::readInForm(const QString &fileName) const
{
    return m_actionManager->readInForm(fileName);
}

bool QDesignerWorkbench::writeOutForm(QDesignerFormWindowInterface *formWindow, const QString &fileName) const
{
    return m_actionManager->writeOutForm(formWindow, fileName);
}

bool QDesignerWorkbench::saveForm(QDesignerFormWindowInterface *frm)
{
    return m_actionManager->saveForm(frm);
}

QDesignerFormWindow *QDesignerWorkbench::findFormWindow(QWidget *widget) const
{
    for (QDesignerFormWindow *formWindow : m_formWindows) {
        if (formWindow->editor() == widget)
            return formWindow;
    }

    return 0;
}

bool QDesignerWorkbench::handleClose()
{
    m_state = StateClosing;
    QList<QDesignerFormWindow *> dirtyForms;
    for (QDesignerFormWindow *w : qAsConst(m_formWindows)) {
        if (w->editor()->isDirty())
            dirtyForms << w;
    }

    const int count = dirtyForms.size();
    if (count == 1) {
        if (!dirtyForms.at(0)->close()) {
            m_state = StateUp;
            return false;
        }
    } else if (count > 1) {
        QMessageBox box(QMessageBox::Warning, tr("Save Forms?"),
                        tr("There are %n forms with unsaved changes."
                           " Do you want to review these changes before quitting?", "", count),
                        QMessageBox::Cancel | QMessageBox::Discard | QMessageBox::Save);
        box.setInformativeText(tr("If you do not review your documents, all your changes will be lost."));
        box.button(QMessageBox::Discard)->setText(tr("Discard Changes"));
        QPushButton *save = static_cast<QPushButton *>(box.button(QMessageBox::Save));
        save->setText(tr("Review Changes"));
        box.setDefaultButton(save);
        switch (box.exec()) {
        case QMessageBox::Cancel:
            m_state = StateUp;
            return false;
        case QMessageBox::Save:
            for (QDesignerFormWindow *fw : qAsConst(dirtyForms)) {
                fw->show();
                fw->raise();
                if (!fw->close()) {
                    m_state = StateUp;
                    return false;
                }
            }
            break;
        case QMessageBox::Discard:
            for (QDesignerFormWindow *fw : qAsConst(dirtyForms)) {
                fw->editor()->setDirty(false);
                fw->setWindowModified(false);
            }
            break;
        }
    }

    for (QDesignerFormWindow *fw : qAsConst(m_formWindows))
        fw->close();

    saveSettings();
    return true;
}

QDesignerActions *QDesignerWorkbench::actionManager() const
{
    return m_actionManager;
}

void QDesignerWorkbench::updateWindowMenu(QDesignerFormWindowInterface *fwi)
{
    bool minimizeChecked = false;
    bool minimizeEnabled = false;
    QDesignerFormWindow *activeFormWindow = 0;
    do {
        if (!fwi)
            break;
        activeFormWindow = qobject_cast<QDesignerFormWindow *>(fwi->parentWidget());
        if (!activeFormWindow)
            break;

        minimizeEnabled = true;
        minimizeChecked = isFormWindowMinimized(activeFormWindow);
    } while (false) ;

    m_actionManager->minimizeAction()->setEnabled(minimizeEnabled);
    m_actionManager->minimizeAction()->setChecked(minimizeChecked);

    for (QDesignerFormWindow *fw : qAsConst(m_formWindows))
        fw->action()->setChecked(fw == activeFormWindow);
}

void QDesignerWorkbench::formWindowActionTriggered(QAction *a)
{
    QDesignerFormWindow *fw = qobject_cast<QDesignerFormWindow *>(a->parentWidget());
    Q_ASSERT(fw);

    if (isFormWindowMinimized(fw))
        setFormWindowMinimized(fw, false);

    if (m_mode == DockedMode) {
        if (QMdiSubWindow *subWindow = qobject_cast<QMdiSubWindow *>(fw->parent())) {
            m_dockedMainWindow->mdiArea()->setActiveSubWindow(subWindow);
        }
    } else {
        fw->activateWindow();
        fw->raise();
    }
}

void QDesignerWorkbench::closeAllToolWindows()
{
    for (QDesignerToolWindow *tw : qAsConst(m_toolWindows))
        tw->hide();
}

bool QDesignerWorkbench::readInBackup()
{
    const QMap<QString, QString> backupFileMap = QDesignerSettings(m_core).backup();
    if (backupFileMap.isEmpty())
        return false;

    const  QMessageBox::StandardButton answer =
        QMessageBox::question(0, tr("Backup Information"),
                                 tr("The last session of Designer was not terminated correctly. "
                                       "Backup files were left behind. Do you want to load them?"),
                                    QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes);
    if (answer == QMessageBox::No)
        return false;

    const QString modifiedPlaceHolder = QStringLiteral("[*]");
    for (auto it = backupFileMap.cbegin(), end = backupFileMap.cend(); it != end; ++it) {
        QString fileName = it.key();
        fileName.remove(modifiedPlaceHolder);

        if(m_actionManager->readInForm(it.value()))
            formWindowManager()->activeFormWindow()->setFileName(fileName);

    }
    return true;
}

void QDesignerWorkbench::updateBackup(QDesignerFormWindowInterface* fwi)
{
    QString fwn = QDir::toNativeSeparators(fwi->fileName());
    if (fwn.isEmpty())
        fwn = fwi->parentWidget()->windowTitle();

    QDesignerSettings settings(m_core);
    QMap<QString, QString> map = settings.backup();
    map.remove(fwn);
    settings.setBackup(map);
}

namespace {
    void raiseWindow(QWidget *w) {
        if (w->isMinimized())
            w->setWindowState(w->windowState() & ~Qt::WindowMinimized);
        w->raise();
    }
}

void QDesignerWorkbench::bringAllToFront()
{
    if (m_mode !=  TopLevelMode)
        return;
    for (QDesignerToolWindow *tw : qAsConst(m_toolWindows))
        raiseWindow(tw);
    for (QDesignerFormWindow *dfw : qAsConst(m_formWindows))
        raiseWindow(dfw);
}

// Resize a form window taking MDI decorations into account
// Apply maximum size as there is no layout connection between
// the form's main container and the integration's outer
// container due to the tool widget stack.

void QDesignerWorkbench::resizeForm(QDesignerFormWindow *fw, const QWidget *mainContainer) const
{
    const QSize containerSize = mainContainer->size();
    const QSize containerMaximumSize = mainContainer->maximumSize();
    if (m_mode != DockedMode) {
        fw->resize(containerSize);
        fw->setMaximumSize(containerMaximumSize);
        return;
    }
    // get decorations and resize MDI
    QMdiSubWindow *mdiSubWindow = qobject_cast<QMdiSubWindow *>(fw->parent());
    Q_ASSERT(mdiSubWindow);
    const QSize decorationSize = mdiSubWindow->geometry().size() - mdiSubWindow->contentsRect().size();
    mdiSubWindow->resize(containerSize + decorationSize);
    // In Qt::RightToLeft mode, the window can grow to be partially hidden by the right border.
    const int mdiAreaWidth = m_dockedMainWindow->mdiArea()->width();
    if (qApp->layoutDirection() == Qt::RightToLeft && mdiSubWindow->geometry().right() >= mdiAreaWidth)
        mdiSubWindow->move(mdiAreaWidth - mdiSubWindow->width(), mdiSubWindow->pos().y());

    if (containerMaximumSize == QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX)) {
        mdiSubWindow->setMaximumSize(containerMaximumSize);
    } else {
        mdiSubWindow->setMaximumSize(containerMaximumSize + decorationSize);
    }
}


// Load a form or return 0 and do cleanup. file name and editor file
// name in case of loading a template file.

QDesignerFormWindow * QDesignerWorkbench::loadForm(const QString &fileName,
                                                   bool detectLineTermiantorMode,
                                                   QString *errorMessage)
{
    QFile file(fileName);

    qdesigner_internal::FormWindowBase::LineTerminatorMode mode = qdesigner_internal::FormWindowBase::NativeLineTerminator;

    if (detectLineTermiantorMode) {
        if (file.open(QFile::ReadOnly)) {
            const QString text = QString::fromUtf8(file.readLine());
            file.close();

            const int lf = text.indexOf(QLatin1Char('\n'));
            if (lf > 0 && text.at(lf-1) == QLatin1Char('\r')) {
                mode = qdesigner_internal::FormWindowBase::CRLFLineTerminator;
            } else if (lf >= 0) {
                mode = qdesigner_internal::FormWindowBase::LFLineTerminator;
            }
        }
    }

    if (!file.open(QFile::ReadOnly|QFile::Text)) {
        *errorMessage = tr("The file <b>%1</b> could not be opened: %2").arg(file.fileName(), file.errorString());
        return 0;
    }

    // Create a form
    QDesignerFormWindowManagerInterface *formWindowManager = m_core->formWindowManager();

    QDesignerFormWindow *formWindow = new QDesignerFormWindow(/*formWindow=*/ 0, this);
    addFormWindow(formWindow);
    QDesignerFormWindowInterface *editor = formWindow->editor();
    Q_ASSERT(editor);

    // Temporarily set the file name. It is needed when converting a UIC 3 file.
    // In this case, the file name will we be cleared on return to force a save box.
    editor->setFileName(fileName);

    if (!editor->setContents(&file, errorMessage)) {
        removeFormWindow(formWindow);
        formWindowManager->removeFormWindow(editor);
        m_core->metaDataBase()->remove(editor);
        return 0;
    }

    if (qdesigner_internal::FormWindowBase *fwb = qobject_cast<qdesigner_internal::FormWindowBase *>(editor))
        fwb->setLineTerminatorMode(mode);

    switch (m_mode) {
    case DockedMode: {
        // below code must be after above call to setContents(), because setContents() may popup warning dialogs which would cause
        // mdi sub window activation (because of dialogs internal call to  processEvent or such)
        // That activation could have worse consequences, e.g. NULL resource set for active form) before the form is loaded
        QMdiSubWindow *subWin = m_dockedMainWindow->createMdiSubWindow(formWindow, magicalWindowFlags(formWindow), m_actionManager->closeFormAction()->shortcut());
        m_dockedMainWindow->mdiArea()->setActiveSubWindow(subWin);
    }
        break;
    case TopLevelMode: {
        const QRect formWindowGeometryHint = formWindow->geometryHint();
        formWindow->setAttribute(Qt::WA_DeleteOnClose, true);
        formWindow->setParent(magicalParent(formWindow), magicalWindowFlags(formWindow));
        formWindow->resize(formWindowGeometryHint.size());
        formWindow->move(availableGeometry().center() - formWindowGeometryHint.center());
    }
        break;
    case NeutralMode:
        break;
    }

    // Did user specify another (missing) resource path -> set dirty.
    const bool dirty = editor->property("_q_resourcepathchanged").toBool();
    editor->setDirty(dirty);
    resizeForm(formWindow, editor->mainContainer());
    formWindowManager->setActiveFormWindow(editor);
    return formWindow;
}


QDesignerFormWindow * QDesignerWorkbench::openForm(const QString &fileName, QString *errorMessage)
{
    QDesignerFormWindow *rc = loadForm(fileName, true, errorMessage);
    if (!rc)
        return 0;
    rc->editor()->setFileName(fileName);
    rc->firstShow();
    return rc;
}

QDesignerFormWindow * QDesignerWorkbench::openTemplate(const QString &templateFileName,
                                                       const QString &editorFileName,
                                                       QString *errorMessage)
{
    QDesignerFormWindow *rc = loadForm(templateFileName, false, errorMessage);
    if (!rc)
        return 0;

    rc->editor()->setFileName(editorFileName);
    rc->firstShow();
    return rc;
}

void QDesignerWorkbench::minimizationStateChanged(QDesignerFormWindowInterface *formWindow, bool minimized)
{
    // refresh the minimize action state
    if (core()->formWindowManager()->activeFormWindow() == formWindow) {
        m_actionManager->minimizeAction()->setChecked(minimized);
    }
}

void QDesignerWorkbench::toggleFormMinimizationState()
{
    QDesignerFormWindowInterface *fwi = core()->formWindowManager()->activeFormWindow();
    if (!fwi || m_mode == NeutralMode)
        return;
    QDesignerFormWindow *fw = qobject_cast<QDesignerFormWindow *>(fwi->parentWidget());
    Q_ASSERT(fw);
    setFormWindowMinimized(fw, !isFormWindowMinimized(fw));
}

bool QDesignerWorkbench::isFormWindowMinimized(const QDesignerFormWindow *fw)
{
    switch (m_mode) {
    case DockedMode:
        return mdiSubWindowOf(fw)->isShaded();
    case TopLevelMode:
        return fw->window()->isMinimized();
    default:
        break;
    }
    return fw->isMinimized();
}

void QDesignerWorkbench::setFormWindowMinimized(QDesignerFormWindow *fw, bool minimized)
{
    switch (m_mode) {
    case DockedMode: {
        QMdiSubWindow *mdiSubWindow = mdiSubWindowOf(fw);
        if (minimized) {
            mdiSubWindow->showShaded();
        } else {
            mdiSubWindow->setWindowState(mdiSubWindow->windowState() & ~Qt::WindowMinimized);
        }
    }
        break;
    case TopLevelMode:        {
        QWidget *window = fw->window();
        if (window->isMinimized()) {
            window->setWindowState(window->windowState() & ~Qt::WindowMinimized);
        } else {
            window->showMinimized();
        }
    }
        break;
    default:
        break;
    }
}

/* Applies UI mode changes using Timer-0 delayed signal
 * signal to make sure the preferences dialog is closed and destroyed
 * before a possible switch from docked mode to top-level mode happens.
 * (The switch deletes the main window, which the preference dialog is
 * a child of -> BOOM) */

void QDesignerWorkbench::applyUiSettings()
{
    if (m_uiSettingsChanged) {
        m_uiSettingsChanged = false;
        QTimer::singleShot(0, this, &QDesignerWorkbench::restoreUISettings);
    }
}

void QDesignerWorkbench::notifyUISettingsChanged()
{
    m_uiSettingsChanged = true;
}

void QDesignerWorkbench::restoreUISettings()
{
    UIMode mode = QDesignerSettings(m_core).uiMode();
    switch (mode) {
        case TopLevelMode:
            switchToTopLevelMode();
            break;
        case DockedMode:
            switchToDockedMode();
            break;

        default: Q_ASSERT(0);
    }

    ToolWindowFontSettings fontSettings = QDesignerSettings(m_core).toolWindowFont();
    const QFont &font = fontSettings.m_useFont ? fontSettings.m_font : qApp->font();

    if (font == m_toolWindows.front()->font())
        return;

    for (QDesignerToolWindow *tw : qAsConst(m_toolWindows))
        tw->setFont(font);
}

void QDesignerWorkbench::handleCloseEvent(QCloseEvent *ev)
{
    ev->setAccepted(handleClose());
    if (ev->isAccepted())
        QMetaObject::invokeMethod(qDesigner, "quit", Qt::QueuedConnection);  // We're going down!
}

QDesignerToolWindow *QDesignerWorkbench::widgetBoxToolWindow() const
{
    return m_toolWindows.at(QDesignerToolWindow::WidgetBox);
}

QT_END_NAMESPACE
