// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CONTAINERWIDGER_TASKMENU_H
#define CONTAINERWIDGER_TASKMENU_H

#include <qdesigner_taskmenu_p.h>
#include <shared_enums_p.h>

#include <extensionfactory_p.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QDesignerFormWindowInterface;
class QDesignerFormEditorInterface;
class QDesignerContainerExtension;
class QAction;
class QMdiArea;
class QMenu;
class QWizard;

namespace qdesigner_internal {

class PromotionTaskMenu;

// ContainerWidgetTaskMenu: Task menu for containers with extension

class ContainerWidgetTaskMenu: public QDesignerTaskMenu
{
    Q_OBJECT
public:
    explicit ContainerWidgetTaskMenu(QWidget *widget, ContainerType type, QObject *parent = nullptr);
    ~ContainerWidgetTaskMenu() override;

    QAction *preferredEditAction() const override;
    QList<QAction*> taskActions() const override;

private slots:
    void removeCurrentPage();
    void addPage();
    void addPageAfter();

protected:
    QDesignerContainerExtension *containerExtension() const;
    QList<QAction*> &containerActions() { return m_taskActions; }
    int pageCount() const;

private:
    QDesignerFormWindowInterface *formWindow() const;

private:
    static QString pageMenuText(ContainerType ct, int index, int count);
    bool canDeletePage() const;

    const ContainerType m_type;
    QWidget *m_containerWidget;
    QDesignerFormEditorInterface *m_core;
    PromotionTaskMenu *m_pagePromotionTaskMenu;
    QAction *m_pageMenuAction;
    QMenu *m_pageMenu;
    QList<QAction*> m_taskActions;
    QAction *m_actionInsertPageAfter;
    QAction *m_actionInsertPage;
    QAction *m_actionDeletePage;
};

// WizardContainerWidgetTaskMenu: Provide next/back since QWizard
// has modes in which the "Back" button is not visible.

class WizardContainerWidgetTaskMenu : public ContainerWidgetTaskMenu {
    Q_OBJECT
public:
    explicit WizardContainerWidgetTaskMenu(QWizard *w, QObject *parent = nullptr);

    QList<QAction*> taskActions() const override;

private:
    QAction *m_nextAction;
    QAction *m_previousAction;
};


// MdiContainerWidgetTaskMenu: Provide tile/cascade for MDI containers in addition

class MdiContainerWidgetTaskMenu : public ContainerWidgetTaskMenu {
    Q_OBJECT
public:
    explicit MdiContainerWidgetTaskMenu(QMdiArea *m, QObject *parent = nullptr);

    QList<QAction*> taskActions() const override;
private:
    void initializeActions();

    QAction *m_nextAction = nullptr;
    QAction *m_previousAction = nullptr;
    QAction *m_tileAction = nullptr;
    QAction *m_cascadeAction = nullptr;
};

class ContainerWidgetTaskMenuFactory: public QExtensionFactory
{
    Q_OBJECT
public:
    explicit ContainerWidgetTaskMenuFactory(QDesignerFormEditorInterface *core, QExtensionManager *extensionManager = nullptr);

protected:
    QObject *createExtension(QObject *object, const QString &iid, QObject *parent) const override;

private:
    QDesignerFormEditorInterface *m_core;
};

}  // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // CONTAINERWIDGER_TASKMENU_H
