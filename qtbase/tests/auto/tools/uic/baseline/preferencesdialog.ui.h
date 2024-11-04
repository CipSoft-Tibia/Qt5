/********************************************************************************
** Form generated from reading UI file 'preferencesdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <previewconfigurationwidget_p.h>
#include "fontpanel.h"
#include "gridpanel_p.h"

QT_BEGIN_NAMESPACE

class Ui_PreferencesDialog
{
public:
    QVBoxLayout *vboxLayout;
    QHBoxLayout *hboxLayout;
    QVBoxLayout *vboxLayout1;
    QGroupBox *m_uiModeGroupBox;
    QVBoxLayout *vboxLayout2;
    QComboBox *m_uiModeCombo;
    FontPanel *m_fontPanel;
    qdesigner_internal::PreviewConfigurationWidget *m_previewConfigurationWidget;
    QVBoxLayout *vboxLayout3;
    QGroupBox *m_templatePathGroupBox;
    QGridLayout *gridLayout;
    QListWidget *m_templatePathListWidget;
    QToolButton *m_addTemplatePathButton;
    QToolButton *m_removeTemplatePathButton;
    QSpacerItem *spacerItem;
    qdesigner_internal::GridPanel *m_gridPanel;
    QFrame *line;
    QDialogButtonBox *m_dialogButtonBox;

    void setupUi(QDialog *PreferencesDialog)
    {
        if (PreferencesDialog->objectName().isEmpty())
            PreferencesDialog->setObjectName("PreferencesDialog");
        PreferencesDialog->resize(455, 359);
        PreferencesDialog->setModal(true);
        vboxLayout = new QVBoxLayout(PreferencesDialog);
        vboxLayout->setObjectName("vboxLayout");
        hboxLayout = new QHBoxLayout();
        hboxLayout->setObjectName("hboxLayout");
        vboxLayout1 = new QVBoxLayout();
        vboxLayout1->setObjectName("vboxLayout1");
        m_uiModeGroupBox = new QGroupBox(PreferencesDialog);
        m_uiModeGroupBox->setObjectName("m_uiModeGroupBox");
        vboxLayout2 = new QVBoxLayout(m_uiModeGroupBox);
        vboxLayout2->setObjectName("vboxLayout2");
        m_uiModeCombo = new QComboBox(m_uiModeGroupBox);
        m_uiModeCombo->setObjectName("m_uiModeCombo");

        vboxLayout2->addWidget(m_uiModeCombo);


        vboxLayout1->addWidget(m_uiModeGroupBox);

        m_fontPanel = new FontPanel(PreferencesDialog);
        m_fontPanel->setObjectName("m_fontPanel");

        vboxLayout1->addWidget(m_fontPanel);

        m_previewConfigurationWidget = new qdesigner_internal::PreviewConfigurationWidget(PreferencesDialog);
        m_previewConfigurationWidget->setObjectName("m_previewConfigurationWidget");

        vboxLayout1->addWidget(m_previewConfigurationWidget);


        hboxLayout->addLayout(vboxLayout1);

        vboxLayout3 = new QVBoxLayout();
        vboxLayout3->setObjectName("vboxLayout3");
        m_templatePathGroupBox = new QGroupBox(PreferencesDialog);
        m_templatePathGroupBox->setObjectName("m_templatePathGroupBox");
        gridLayout = new QGridLayout(m_templatePathGroupBox);
        gridLayout->setObjectName("gridLayout");
        m_templatePathListWidget = new QListWidget(m_templatePathGroupBox);
        m_templatePathListWidget->setObjectName("m_templatePathListWidget");

        gridLayout->addWidget(m_templatePathListWidget, 0, 0, 1, 3);

        m_addTemplatePathButton = new QToolButton(m_templatePathGroupBox);
        m_addTemplatePathButton->setObjectName("m_addTemplatePathButton");

        gridLayout->addWidget(m_addTemplatePathButton, 1, 0, 1, 1);

        m_removeTemplatePathButton = new QToolButton(m_templatePathGroupBox);
        m_removeTemplatePathButton->setObjectName("m_removeTemplatePathButton");

        gridLayout->addWidget(m_removeTemplatePathButton, 1, 1, 1, 1);

        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(spacerItem, 1, 2, 1, 1);


        vboxLayout3->addWidget(m_templatePathGroupBox);

        m_gridPanel = new qdesigner_internal::GridPanel(PreferencesDialog);
        m_gridPanel->setObjectName("m_gridPanel");

        vboxLayout3->addWidget(m_gridPanel);


        hboxLayout->addLayout(vboxLayout3);


        vboxLayout->addLayout(hboxLayout);

        line = new QFrame(PreferencesDialog);
        line->setObjectName("line");
        line->setFrameShape(QFrame::Shape::HLine);
        line->setFrameShadow(QFrame::Shadow::Sunken);

        vboxLayout->addWidget(line);

        m_dialogButtonBox = new QDialogButtonBox(PreferencesDialog);
        m_dialogButtonBox->setObjectName("m_dialogButtonBox");
        m_dialogButtonBox->setOrientation(Qt::Horizontal);
        m_dialogButtonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        vboxLayout->addWidget(m_dialogButtonBox);


        retranslateUi(PreferencesDialog);
        QObject::connect(m_dialogButtonBox, &QDialogButtonBox::accepted, PreferencesDialog, qOverload<>(&QDialog::accept));
        QObject::connect(m_dialogButtonBox, &QDialogButtonBox::rejected, PreferencesDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(PreferencesDialog);
    } // setupUi

    void retranslateUi(QDialog *PreferencesDialog)
    {
        PreferencesDialog->setWindowTitle(QCoreApplication::translate("PreferencesDialog", "Preferences", nullptr));
        m_uiModeGroupBox->setTitle(QCoreApplication::translate("PreferencesDialog", "User Interface Mode", nullptr));
        m_templatePathGroupBox->setTitle(QCoreApplication::translate("PreferencesDialog", "Additional Template Paths", nullptr));
        m_addTemplatePathButton->setText(QCoreApplication::translate("PreferencesDialog", "...", nullptr));
        m_removeTemplatePathButton->setText(QCoreApplication::translate("PreferencesDialog", "...", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PreferencesDialog: public Ui_PreferencesDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PREFERENCESDIALOG_H
