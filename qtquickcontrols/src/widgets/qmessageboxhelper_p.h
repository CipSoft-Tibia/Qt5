/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMESSAGEBOXHELPER_P_H
#define QMESSAGEBOXHELPER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QMessageBox>
#include "../dialogs/qquickabstractmessagedialog_p.h"

QT_BEGIN_NAMESPACE

class QCloseableMessageBox : public QMessageBox
{
public:
    QCloseableMessageBox(QWidget *parent = 0) : QMessageBox(parent) { }

    void closeEvent(QCloseEvent *e) {
        // QTBUG-36227: Bypass QMessageBox::closeEvent()
        QDialog::closeEvent(e);
    }

    void keyPressEvent(QKeyEvent *e) {
        QMessageBox::keyPressEvent(e);
        // QTBUG-36227: reject on escape or cmd-period even if there's no cancel button
        if ((isVisible() && e->key() == Qt::Key_Escape)
#ifdef Q_OS_MAC
            || (e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_Period)
#endif
                )
            reject();
    }
};

class QMessageBoxHelper : public QPlatformMessageDialogHelper
{
    Q_OBJECT
public:
    QMessageBoxHelper() {
        connect(&m_dialog, SIGNAL(accepted()), this, SIGNAL(accept()));
        connect(&m_dialog, SIGNAL(rejected()), this, SIGNAL(reject()));
        connect(&m_dialog, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
    }

    virtual void exec() { m_dialog.exec(); }

    virtual bool show(Qt::WindowFlags f, Qt::WindowModality m, QWindow *parent) {
        m_dialog.winId();
        QWindow *window = m_dialog.windowHandle();
        Q_ASSERT(window);
        window->setTransientParent(parent);
        window->setFlags(f);
        m_dialog.setWindowModality(m);
        m_dialog.setWindowTitle(QPlatformMessageDialogHelper::options()->windowTitle());
        m_dialog.setIcon(static_cast<QMessageBox::Icon>(QPlatformMessageDialogHelper::options()->icon()));
        if (!QPlatformMessageDialogHelper::options()->text().isNull())
            m_dialog.setText(QPlatformMessageDialogHelper::options()->text());
        if (!QPlatformMessageDialogHelper::options()->informativeText().isNull())
            m_dialog.setInformativeText(QPlatformMessageDialogHelper::options()->informativeText());
#if QT_CONFIG(textedit)
        if (!QPlatformMessageDialogHelper::options()->detailedText().isNull())
            m_dialog.setDetailedText(QPlatformMessageDialogHelper::options()->detailedText());
#endif
        m_dialog.setStandardButtons(static_cast<QMessageBox::StandardButtons>(static_cast<int>(
            QPlatformMessageDialogHelper::options()->standardButtons())));
        m_dialog.show();
        return m_dialog.isVisible();
    }

    virtual void hide() { m_dialog.hide(); }

    QCloseableMessageBox m_dialog;

public Q_SLOTS:
    void buttonClicked(QAbstractButton* button) {
        emit clicked(static_cast<QPlatformDialogHelper::StandardButton>(m_dialog.standardButton(button)),
            static_cast<QPlatformDialogHelper::ButtonRole>(m_dialog.buttonRole(button)));
    }
};

QT_END_NAMESPACE

#endif // QMESSAGEBOXHELPER_P_H
