// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QDESIGNER_ACTIONS_H
#define QDESIGNER_ACTIONS_H

#include "assistantclient.h"
#include "qdesigner_settings.h"

#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QDesignerWorkbench;

class QDir;
class QTimer;
class QAction;
class QActionGroup;
class QDesignerFormEditorInterface;
class QDesignerFormWindowInterface;
class AppFontDialog;

class QRect;
class QWidget;
class QPixmap;
class QPrinter;
class QMenu;

namespace qdesigner_internal {
    class PreviewConfiguration;
    class PreviewManager;
    enum class UicLanguage;
}

class QDesignerActions: public QObject
{
    Q_OBJECT
public:
    explicit QDesignerActions(QDesignerWorkbench *mainWindow);
    ~QDesignerActions() override;

    QDesignerWorkbench *workbench() const;
    QDesignerFormEditorInterface *core() const;

    bool saveForm(QDesignerFormWindowInterface *fw);
    bool readInForm(const QString &fileName);
    bool writeOutForm(QDesignerFormWindowInterface *formWindow, const QString &fileName, bool check = true);

    QActionGroup *fileActions() const;
    QActionGroup *recentFilesActions() const;
    QActionGroup *editActions() const;
    QActionGroup *formActions() const;
    QActionGroup *settingsActions() const;
    QActionGroup *windowActions() const;
    QActionGroup *toolActions() const;
    QActionGroup *helpActions() const;
    QActionGroup *uiMode() const;
    QActionGroup *styleActions() const;
    // file actions
    QAction *openFormAction() const;
    QAction *closeFormAction() const;
    // window actions
    QAction *minimizeAction() const;
    // edit mode actions
    QAction *editWidgets() const;
    // form actions
    QAction *previewFormAction() const;
    QAction *viewCodeAction() const;

    void setBringAllToFrontVisible(bool visible);
    void setWindowListSeparatorVisible(bool visible);

    bool openForm(QWidget *parent);

    QString uiExtension() const;

    // Boolean dynamic property set on actions to
    // show them in the default toolbar layout
    static const char *defaultToolbarPropertyName;

public slots:
    void activeFormWindowChanged(QDesignerFormWindowInterface *formWindow);
    void createForm();
    void slotOpenForm();
    void helpRequested(const QString &manual, const QString &document);

signals:
    void useBigIcons(bool);

private slots:
    void saveForm();
    void saveFormAs();
    void saveAllForms();
    void saveFormAsTemplate();
    void notImplementedYet();
    void shutdown();
    void editWidgetsSlot();
    void openRecentForm();
    void clearRecentFiles();
    void closeForm();
    void showDesignerHelp();
    void aboutDesigner();
    void showWidgetSpecificHelp();
    void backupForms();
    void showNewFormDialog(const QString &fileName);
    void showPreferencesDialog();
    void showAppFontDialog();
    void savePreviewImage();
    void printPreviewImage();
    void updateCloseAction();
    void formWindowCountChanged();
    void formWindowSettingsChanged(QDesignerFormWindowInterface *fw);

private:
    QAction *createRecentFilesMenu();
    bool saveFormAs(QDesignerFormWindowInterface *fw);
    void updateRecentFileActions();
    void addRecentFile(const QString &fileName);
    void showHelp(const QString &help);
    void closePreview();
    QRect fixDialogRect(const QRect &rect) const;
    QString fixResourceFileBackupPath(QDesignerFormWindowInterface *fwi, const QDir& backupDir);
    void showStatusBarMessage(const QString &message) const;
    QActionGroup *createHelpActions();
    bool ensureBackupDirectories();
    QPixmap createPreviewPixmap(QDesignerFormWindowInterface *fw);
    qdesigner_internal::PreviewConfiguration previewConfiguration();
    void viewCode(qdesigner_internal::UicLanguage language);

    enum { MaxRecentFiles = 10 };
    QDesignerWorkbench *m_workbench;
    QDesignerFormEditorInterface *m_core;
    QDesignerSettings m_settings;
    AssistantClient m_assistantClient;
    QString m_openDirectory;
    QString m_saveDirectory;


    QString m_backupPath;
    QString m_backupTmpPath;

    QTimer* m_backupTimer;

    QActionGroup *m_fileActions;
    QActionGroup *m_recentFilesActions;
    QActionGroup *m_editActions;
    QActionGroup *m_formActions;
    QActionGroup *m_settingsActions;
    QActionGroup *m_windowActions;
    QActionGroup *m_toolActions;
    QActionGroup *m_helpActions = nullptr;
    QActionGroup *m_styleActions = nullptr;

    QAction *m_editWidgetsAction;

    QAction *m_newFormAction;
    QAction *m_openFormAction;
    QAction *m_saveFormAction;
    QAction *m_saveFormAsAction;
    QAction *m_saveAllFormsAction;
    QAction *m_saveFormAsTemplateAction;
    QAction *m_closeFormAction;
    QAction *m_savePreviewImageAction;
    QAction *m_printPreviewAction;

    QAction *m_quitAction;

    QAction *m_previewFormAction = nullptr;
    QAction *m_viewCppCodeAction;
    QAction *m_viewPythonCodeAction;

    QAction *m_minimizeAction;
    QAction *m_bringAllToFrontSeparator;
    QAction *m_bringAllToFrontAction;
    QAction *m_windowListSeparatorAction;

    QAction *m_preferencesAction;
    QAction *m_appFontAction;

    QPointer<AppFontDialog> m_appFontDialog;

    QPrinter *m_printer = nullptr;

    qdesigner_internal::PreviewManager *m_previewManager = nullptr;

    std::unique_ptr<QMenu> m_recentMenu;
};

QT_END_NAMESPACE

#endif // QDESIGNER_ACTIONS_H
