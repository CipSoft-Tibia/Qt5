// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mainwindow.h"
#include "changeproperties.h"
#include "invokemethod.h"
#include "ambientproperties.h"
#include "controlinfo.h"
#include "docuwindow.h"

#include <QtWidgets/QMdiArea>
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMessageBox>
#include <QtGui/QCloseEvent>
#include <QtGui/QPixmap>
#include <QtCore/QDebug>
#include <QtCore/QLibraryInfo>
#include <QtCore/qt_windows.h>
#include <QtAxContainer/QAxScriptManager>
#include <QtAxContainer/QAxWidget>
#include <QtAxContainer/private/qaxbase_p.h>
#include "sandboxing.h"

QT_BEGIN_NAMESPACE

QT_END_NAMESPACE

QT_USE_NAMESPACE

struct ScriptLanguage {
    const char *name;
    const char *suffix;
};

static const ScriptLanguage scriptLanguages[] = {
    {"PerlScript", ".pl"},
    {"Python", ".py"}
};

MainWindow *MainWindow::m_instance = nullptr;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(this);
    MainWindow::m_instance = this; // Logging handler needs the UI

    setObjectName(QLatin1String("MainWindow"));

    for (auto scriptLanguage : scriptLanguages) {
        const QString name = QLatin1String(scriptLanguage.name);
        const QString suffix = QLatin1String(scriptLanguage.suffix);
        if (!QAxScriptManager::registerEngine(name, suffix))
            qWarning().noquote().nospace() << "Failed to register \"" << name
                << "\" (*" << suffix << ") with QAxScriptManager.";
    }

    QHBoxLayout *layout = new QHBoxLayout(Workbase);
    m_mdiArea = new QMdiArea(Workbase);
    layout->addWidget(m_mdiArea);
    layout->setContentsMargins(0, 0, 0, 0);

    connect(m_mdiArea, &QMdiArea::subWindowActivated, this, &MainWindow::updateGUI);
    connect(actionFileExit, &QAction::triggered, QCoreApplication::quit);
}

MainWindow::~MainWindow()
{
    MainWindow::m_instance = nullptr;
}

QAxWidget *MainWindow::activeAxWidget() const
{
    if (const QMdiSubWindow *activeSubWindow = m_mdiArea->currentSubWindow())
        return qobject_cast<QAxWidget*>(activeSubWindow->widget());
    return nullptr;
}

QList<QAxWidget *> MainWindow::axWidgets() const
{
    QList<QAxWidget *> result;
    const auto mdiSubWindows = m_mdiArea->subWindowList();
    for (const QMdiSubWindow *subWindow : mdiSubWindows)
        if (QAxWidget *axWidget = qobject_cast<QAxWidget *>(subWindow->widget()))
            result.push_back(axWidget);
    return result;
}

bool MainWindow::addControlFromClsid(const QString &clsid, QAxSelect::SandboxingLevel sandboxing)
{
    QAxWidget *container = new QAxWidget;

    bool result = false;
    {
        // RAII object for impersonating sandboxing on current thread
        std::unique_ptr<Sandboxing> sandbox_impl;

        switch (sandboxing) {
        case QAxSelect::SandboxingNone:
            break; // sandboxing disabled
        case QAxSelect::SandboxingProcess:
            // require out-of-process
            container->setClassContext(CLSCTX_LOCAL_SERVER);
            break;
        default:
            // impersonate desired sandboxing
            sandbox_impl = Sandboxing::Create(sandboxing, clsid);
            // require out-of-process and activate impersonation
            container->setClassContext(CLSCTX_LOCAL_SERVER | CLSCTX_ENABLE_CLOAKING);
            break;
        }

        result = container->setControl(clsid);
    }

    if (result) {
        container->setObjectName(container->windowTitle());
        m_mdiArea->addSubWindow(container);
        container->show();
        updateGUI();
    } else {
        delete container;
        logTabWidget->setCurrentIndex(logTabWidget->count() - 1);
        const QString message =
            tr("The control \"%1\" could not be loaded."
               " See the \"Debug log\" tab for details.").arg(clsid);
        QMessageBox::information(this, tr("Error Loading Control"), message);
    }
    return result;
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    // Controls using the same version of Qt may set this to false, causing hangs.
    QGuiApplication::setQuitOnLastWindowClosed(true);
    m_mdiArea->closeAllSubWindows();
    e->accept();
}

void MainWindow::appendLogText(const QString &message)
{
    logDebug->append(message);
}

void MainWindow::on_actionFileNew_triggered()
{
    QAxSelect select(this);
    while (select.exec() && !addControlFromClsid(select.clsid(), select.sandboxingLevel())) {
    }
}

void MainWindow::on_actionFileLoad_triggered()
{
    while (true) {
        const QString fname = QFileDialog::getOpenFileName(this, tr("Load"), QString(), QLatin1String("*.qax"));
        if (fname.isEmpty() || addControlFromFile(fname))
            break;
    }
}

bool MainWindow::addControlFromFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::information(this,
                                 tr("Error Loading File"),
                                 tr("The file could not be opened for reading.\n%1\n%2")
                                 .arg(QDir::toNativeSeparators(fileName), file.errorString()));
        return false;
    }

    QAxWidget *container = new QAxWidget(m_mdiArea);
    container->setObjectName(container->windowTitle());

    QDataStream d(&file);
    d >> *container;

    m_mdiArea->addSubWindow(container);
    container->show();

    updateGUI();
    return true;
}

void MainWindow::on_actionFileSave_triggered()
{
    QAxWidget *container = activeAxWidget();
    if (!container)
        return;

    QString fname = QFileDialog::getSaveFileName(this, tr("Save"), QString(), QLatin1String("*.qax"));
    if (fname.isEmpty())
        return;

    QFile file(fname);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::information(this, tr("Error Saving File"), tr("The file could not be opened for writing.\n%1").arg(fname));
        return;
    }
    QDataStream d(&file);
    d << *container;
}


void MainWindow::on_actionContainerSet_triggered()
{
    QAxWidget *container = activeAxWidget();
    if (!container)
        return;

    QAxSelect select(this);
    if (select.exec())
        container->setControl(select.clsid());
    updateGUI();
}

void MainWindow::on_actionContainerClear_triggered()
{
    QAxWidget *container = activeAxWidget();
    if (container)
        container->clear();
    updateGUI();
}

void MainWindow::on_actionContainerProperties_triggered()
{
    if (!m_dlgAmbient) {
        m_dlgAmbient = new AmbientProperties(this);
        m_dlgAmbient->setControl(m_mdiArea);
    }
    m_dlgAmbient->show();
}


void MainWindow::on_actionControlInfo_triggered()
{
    QAxWidget *container = activeAxWidget();
    if (!container)
        return;

    ControlInfo info(this);
    info.setControl(container);
    info.exec();
}

void MainWindow::on_actionControlProperties_triggered()
{
    QAxWidget *container = activeAxWidget();
    if (!container)
        return;

    if (!m_dlgProperties) {
        m_dlgProperties = new ChangeProperties(this);
        connect(container, SIGNAL(propertyChanged(QString)), m_dlgProperties, SLOT(updateProperties()));
    }
    m_dlgProperties->setControl(container);
    m_dlgProperties->show();
}

void MainWindow::on_actionControlMethods_triggered()
{
    QAxWidget *container = activeAxWidget();
    if (!container)
        return;

    if (!m_dlgInvoke)
        m_dlgInvoke = new InvokeMethod(this);
    m_dlgInvoke->setControl(container);
    m_dlgInvoke->show();
}

void MainWindow::on_VerbMenu_aboutToShow()
{
    VerbMenu->clear();

    QAxWidget *container = activeAxWidget();
    if (!container)
        return;

    QStringList verbs = container->verbs();
    for (qsizetype i = 0; i < verbs.size(); ++i) {
        VerbMenu->addAction(verbs.at(i));
    }

    if (verbs.isEmpty()) { // no verbs?
        VerbMenu->addAction(tr("-- Object does not support any verbs --"))->setEnabled(false);
    }
}

void MainWindow::on_VerbMenu_triggered(QAction *action)
{
    QAxWidget *container = activeAxWidget();
    if (!container)
        return;

    container->doVerb(action->text());
}

void MainWindow::on_actionControlDocumentation_triggered()
{
    QAxWidget *container = activeAxWidget();
    if (!container)
        return;

    const QString docu = container->generateDocumentation();
    if (docu.isEmpty())
        return;

    DocuWindow *docwindow = new DocuWindow(docu);
    QMdiSubWindow *subWindow = m_mdiArea->addSubWindow(docwindow);
    subWindow->setWindowTitle(DocuWindow::tr("%1 - Documentation").arg(container->windowTitle()));
    docwindow->show();
}

void MainWindow::on_actionControlPixmap_triggered()
{
    QAxWidget *container = activeAxWidget();
    if (!container)
        return;

    QLabel *label = new QLabel;
    label->setPixmap(container->grab());
    QMdiSubWindow *subWindow = m_mdiArea->addSubWindow(label);
    subWindow->setWindowTitle(tr("%1 - Pixmap").arg(container->windowTitle()));
    label->show();
}

void MainWindow::on_actionScriptingRun_triggered()
{
#ifndef QT_NO_QAXSCRIPT
    if (!m_scripts)
        return;

    // If we have only one script loaded we can use the cool dialog
    QStringList scriptList = m_scripts->scriptNames();
    if (scriptList.size() == 1) {
        InvokeMethod scriptInvoke(this);
        scriptInvoke.setWindowTitle(tr("Execute Script Function"));
        scriptInvoke.setControl(m_scripts->script(scriptList[0])->scriptEngine());
        scriptInvoke.exec();
        return;
    }

    bool ok = false;
    QStringList macroList = m_scripts->functions(QAxScript::FunctionNames);
    QString macro = QInputDialog::getItem(this, tr("Select Macro"), tr("Macro:"), macroList, 0, true, &ok);

    if (!ok)
        return;

    QVariant result = m_scripts->call(macro);
    if (result.isValid())
        logMacros->append(tr("Return value of %1: %2").arg(macro, result.toString()));
#endif
}

void MainWindow::on_actionFreeUnusedDLLs_triggered()
{
    // Explicitly unload unused DLLs with no remaining references.
    // This is also done automatically after 10min and in low memory situations.

    // must call twice due to DllCanUnloadNow implementation in qaxserverdll
    CoFreeUnusedLibrariesEx(0, 0);
    CoFreeUnusedLibrariesEx(0, 0);
}

#ifdef QT_NO_QAXSCRIPT
static inline void noScriptMessage(QWidget *parent)
{
    QMessageBox::information(parent, MainWindow::tr("Function not available"),
                             MainWindow::tr("QAxScript functionality is not available with this compiler."));
}
#endif // !QT_NO_QAXSCRIPT

void MainWindow::on_actionScriptingLoad_triggered()
{
#ifndef QT_NO_QAXSCRIPT
    QString file = QFileDialog::getOpenFileName(this, tr("Open Script"), QString(), QAxScriptManager::scriptFileFilter());

    if (!file.isEmpty())
        loadScript(file);
#else // !QT_NO_QAXSCRIPT
    noScriptMessage(this);
#endif
}

bool MainWindow::loadScript(const QString &file)
{
#ifndef QT_NO_QAXSCRIPT
    if (!m_scripts) {
        m_scripts = new QAxScriptManager(this);
        m_scripts->addObject(this);
    }

    const auto axw = axWidgets();
    for (QAxWidget *axWidget : axw) {
        QAxBase *ax = axWidget;
        m_scripts->addObject(ax);
    }

    QAxScript *script = m_scripts->load(file, file);
    if (script) {
        connect(script, &QAxScript::error, this, &MainWindow::logMacro);
        actionScriptingRun->setEnabled(true);
    }
    return script;
#else // !QT_NO_QAXSCRIPT
    Q_UNUSED(file);
    noScriptMessage(this);
    return false;
#endif
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    QApplication::aboutQt();
}

class VersionDialog : public QDialog
{
public:
    explicit VersionDialog(QWidget *parent = nullptr);
};

const char aboutTextFormat[] = QT_TRANSLATE_NOOP("MainWindow",
"<h3>Testcon - An ActiveX Test Container</h3>\nVersion: %1<br/><br/>\n"
"This application implements a generic test container for ActiveX controls."
"<br/><br/>Copyright (C) %2 The Qt Company Ltd.");

VersionDialog::VersionDialog(QWidget *parent) : QDialog(parent)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("About Testcon"));
    QGridLayout *layout = new QGridLayout(this);
    QLabel *logoLabel = new QLabel;
    logoLabel->setPixmap(QStringLiteral(":/qt-project.org/qmessagebox/images/qtlogo-64.png"));
    const QString aboutText =
        tr(aboutTextFormat).arg(QLatin1String(QLibraryInfo::build()),
                                QStringLiteral("2017"));
    QLabel *aboutLabel = new QLabel(aboutText);
    aboutLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    aboutLabel->setWordWrap(true);
    aboutLabel->setOpenExternalLinks(true);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox , &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(logoLabel, 0, 0, 1, 1);
    layout->addWidget(aboutLabel, 0, 1, 4, 4);
    layout->addWidget(buttonBox, 4, 2, 1, 1);
}

void MainWindow::on_actionAbout_Testcon_triggered()
{
    VersionDialog versionDialog(this);
    versionDialog.exec();
}

void MainWindow::updateGUI()
{
    QAxWidget *container = activeAxWidget();

    bool hasControl = container && !container->isNull();
    actionFileNew->setEnabled(true);
    actionFileLoad->setEnabled(true);
    actionFileSave->setEnabled(hasControl);
    actionContainerSet->setEnabled(container != nullptr);
    actionContainerClear->setEnabled(hasControl);
    actionControlProperties->setEnabled(hasControl);
    actionControlMethods->setEnabled(hasControl);
    actionControlInfo->setEnabled(hasControl);
    actionControlDocumentation->setEnabled(hasControl);
    actionControlPixmap->setEnabled(hasControl);
    VerbMenu->setEnabled(hasControl);
    if (m_dlgInvoke)
        m_dlgInvoke->setControl(hasControl ? container : nullptr);
    if (m_dlgProperties)
        m_dlgProperties->setControl(hasControl ? container : nullptr);

    const auto axw = axWidgets();
    for (QAxWidget *container : axw) {
        disconnect(container, &QAxWidget::signal, this, nullptr);
        if (actionLogSignals->isChecked())
            connect(container, &QAxWidget::signal, this, &MainWindow::logSignal);
        disconnect(container, &QAxWidget::exception, this, nullptr);
        connect(container, &QAxWidget::exception, this, &MainWindow::logException);

        disconnect(container, &QAxWidget::propertyChanged, this, nullptr);
        if (actionLogProperties->isChecked())
            connect(container, &QAxWidget::propertyChanged, this, &MainWindow::logPropertyChanged);
        container->blockSignals(actionFreezeEvents->isChecked());
    }
}

void MainWindow::logPropertyChanged(const QString &prop)
{
    QAxWidget *container = activeAxWidget();
    if (!container)
        return;

    QVariant var = container->property(prop.toLatin1());
    logProperties->append(tr("%1: Property Change: %2 - { %3 }").arg(container->windowTitle(), prop, var.toString()));
}

void MainWindow::logSignal(const QString &signal, int argc, void *argv)
{
    QAxWidget *container = activeAxWidget();
    if (!container)
        return;

    QString paramlist = QLatin1String(" - {");
    auto params = static_cast<const VARIANT *>(argv);
    for (int a = argc-1; a >= 0; --a) {
        paramlist += QLatin1Char(' ');
        paramlist += QAxBasePrivate::VARIANTToQVariant(params[a], nullptr).toString();
        paramlist += a > 0 ? QLatin1Char(',') : QLatin1Char(' ');
    }
    if (argc)
        paramlist += QLatin1Char('}');
    logSignals->append(container->windowTitle() + QLatin1String(": ") + signal + paramlist);
}

void MainWindow::logException(int code, const QString&source, const QString&desc, const QString&help)
{
    Q_UNUSED(desc);
    QAxWidget *container = qobject_cast<QAxWidget*>(sender());
    if (!container)
        return;

    QString str = tr("%1: Exception code %2 thrown by %3").
        arg(container->windowTitle()).arg(code).arg(source);
    logDebug->append(str);
    logDebug->append(tr("\tDescription: %1").arg(desc));

    if (!help.isEmpty())
        logDebug->append(tr("\tHelp available at %1").arg(help));
    else
        logDebug->append(tr("\tNo help available."));
}

void MainWindow::logMacro(int code, const QString &description, int sourcePosition, const QString &sourceText)
{
    /* FIXME This needs to be rewritten to not use string concatentation, such
     * that it can be translated in a sane way. */
    QString message = tr("Script: ");
    if (code)
        message += QString::number(code) + QLatin1Char(' ');

    const QChar singleQuote = QLatin1Char('\'');
    message += singleQuote + description + singleQuote;
    if (sourcePosition)
        message += tr(" at position ") + QString::number(sourcePosition);
    if (!sourceText.isEmpty())
        message += QLatin1String(" '") + sourceText + singleQuote;

    logMacros->append(message);
}
