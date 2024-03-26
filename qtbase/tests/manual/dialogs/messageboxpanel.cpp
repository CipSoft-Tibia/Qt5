// Copyright (C) 2013 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "messageboxpanel.h"

#include <QGroupBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QDebug>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QRegularExpressionValidator>
#include <QRegularExpression>

MessageBoxPanel::MessageBoxPanel(QWidget *parent) : QWidget(parent)
,m_iconComboBox(new QComboBox)
,m_textInMsgBox(new QLineEdit)
,m_informativeText(new QLineEdit)
,m_detailedtext(new QLineEdit)
,m_buttonsMask(new QLineEdit)
,m_btnExec(new QPushButton)
,m_btnShowApply(new QPushButton)
,m_resultLabel(new QLabel)
,m_chkReallocMsgBox(new QCheckBox(QString::fromLatin1("Reallocate Message Box")))
,m_checkboxText(new QLineEdit)
,m_checkBoxResult(new QLabel)
,m_msgbox(new QMessageBox)
{
    // --- Options ---
    QGroupBox *optionsGroupBox = new QGroupBox(tr("Options"), this);
    QVBoxLayout *optionsLayout = new QVBoxLayout(optionsGroupBox);

    // text
    optionsLayout->addWidget(new QLabel(QString::fromLatin1("Message box text")));
    m_textInMsgBox->setText(QString::fromLatin1("This is a simple test with a text that is not long"));
    optionsLayout->addWidget(m_textInMsgBox);

    // informative text
    optionsLayout->addWidget(new QLabel(QString::fromLatin1("Informative Text")));
    optionsLayout->addWidget(m_informativeText);

    // detailed text
    optionsLayout->addWidget(new QLabel(QString::fromLatin1("detailed Text")));
    optionsLayout->addWidget(m_detailedtext);

    // icon
    QStringList items;
    items << "NoIcon" << "Information" << "Warning" << "Critical" << "Question";
    m_iconComboBox->addItems(items);
    optionsLayout->addWidget(new QLabel(QString::fromLatin1("Message box icon")));
    optionsLayout->addWidget(m_iconComboBox);

    // buttons mask
    optionsLayout->addWidget(new QLabel(QString::fromLatin1("Message box button mask (in hex)")));
    m_validator = new QRegularExpressionValidator(QRegularExpression("0[xX]?[0-9a-fA-F]+"), this);
    m_buttonsMask->setMaxLength(10);
    m_buttonsMask->setValidator(m_validator);
    m_buttonsMask->setText(QString::fromLatin1("0x00300400"));
    optionsLayout->addWidget(m_buttonsMask);

    // check box check
    optionsLayout->addWidget(new QLabel(QString::fromLatin1("Checkbox text ("" => no chkbox)")));
    optionsLayout->addWidget(m_checkboxText);

    // reallocate
    optionsLayout->addWidget(m_chkReallocMsgBox);
    optionsLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Expanding));

    // Exec/Show
    QGroupBox *execGroupBox = new QGroupBox(tr("Exec"));
    QVBoxLayout *execLayout = new QVBoxLayout(execGroupBox);
    m_btnExec->setText(QString::fromLatin1("Exec message box"));
    connect(m_btnExec, SIGNAL(clicked()), this, SLOT(doExec()));
    execLayout->addWidget(m_btnExec);

    m_btnShowApply->setText(QString::fromLatin1("Show / apply"));
    connect(m_btnShowApply, SIGNAL(clicked()), this, SLOT(doShowApply()));
    execLayout->addWidget(m_btnShowApply);

    // result label
    execLayout->addWidget(m_resultLabel);
    execLayout->addWidget(m_checkBoxResult);

    execLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Expanding));
    execGroupBox->setLayout(execLayout);

    // Main layout
    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainLayout->addWidget(optionsGroupBox);
    mainLayout->addWidget(execGroupBox);

    setLayout(mainLayout);
}

void MessageBoxPanel::setupMessageBox(QMessageBox &box)
{
    m_resultLabel->setText(QString());
    m_checkBoxResult->setText(QString());
    box.setText(m_textInMsgBox->text());
    box.setInformativeText(m_informativeText->text());
    box.setDetailedText(m_detailedtext->text());

    QString btnHexText = m_buttonsMask->text();
    btnHexText = btnHexText.replace(QString::fromLatin1("0x"), QString(), Qt::CaseInsensitive);
    bool ok;
    QMessageBox::StandardButtons btns = (QMessageBox::StandardButtons) btnHexText.toUInt(&ok, 16);
    box.setStandardButtons((QMessageBox::StandardButtons) btns);
    if (box.standardButtons() == QMessageBox::StandardButtons())
        box.setStandardButtons(QMessageBox::Ok); // just to have something.

    box.setCheckBox(0);
    if (m_checkboxText->text().length() > 0)
        box.setCheckBox(new QCheckBox(m_checkboxText->text()));

    box.setIcon((QMessageBox::Icon) m_iconComboBox->currentIndex());
}

MessageBoxPanel::~MessageBoxPanel()
{
    if (m_msgbox)
        m_msgbox->deleteLater();
}

void MessageBoxPanel::doExec()
{
    if (!m_msgbox || m_chkReallocMsgBox->isChecked()) {
        if (m_msgbox)
            m_msgbox->deleteLater();
        m_msgbox = new QMessageBox;
    }
    setupMessageBox(*m_msgbox);
    m_msgbox->setWindowModality(Qt::NonModal);

    int res = m_msgbox->exec();
    QString sres;
    sres.setNum(res, 16);
    m_resultLabel->setText(QString::fromLatin1("Return value (hex): %1").arg(sres));
    if (m_msgbox->checkBox()) {
        if (m_msgbox->checkBox()->isChecked())
            m_checkBoxResult->setText(QString::fromLatin1("Checkbox was checked"));
        else
            m_checkBoxResult->setText(QString::fromLatin1("Checkbox was not checked"));
    }
}

void MessageBoxPanel::doShowApply()
{
    if (!m_msgbox || m_chkReallocMsgBox->isChecked()) {
        if (m_msgbox)
            m_msgbox->deleteLater();
        m_msgbox = new QMessageBox;
    }
    setupMessageBox(*m_msgbox);
    if (!m_msgbox->isVisible()) {
        m_msgbox->setWindowModality(Qt::NonModal);
        m_msgbox->show();
    }
}
