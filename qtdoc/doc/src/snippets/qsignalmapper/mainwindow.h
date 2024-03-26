// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QMainWindow>
#include <QStatusBar>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow()
    {
        statusBar()->showMessage(tr("Ready"));
    }

public slots:
    void buttonPressed(const QString &text)
    {
        statusBar()->showMessage(tr("Chose %1").arg(text));
    }
};
