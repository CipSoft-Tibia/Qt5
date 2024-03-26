// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QFILTERNAMEDIALOG_H
#define QFILTERNAMEDIALOG_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience
// of the help generator tools. This header file may change from version
// to version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/QDialog>
#include "ui_qfilternamedialog.h"

QT_BEGIN_NAMESPACE

class QFilterNameDialog : public QDialog
{
    Q_OBJECT

public:
    QFilterNameDialog(QWidget *parent = nullptr);

    void setFilterName(const QString &filter);
    QString filterName() const;

private slots:
    void updateOkButton();

private:
    Ui::FilterNameDialogClass m_ui;
};

QT_END_NAMESPACE

#endif // QFILTERNAMEDIALOG_H
