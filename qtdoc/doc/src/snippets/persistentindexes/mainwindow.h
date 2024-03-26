// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QModelIndex>

class QAbstractItemModel;
class QAction;
class QItemSelectionModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);

private slots:
    void insertItem();
    void removeItem();
    void updateMenus(const QModelIndex &currentIndex);

private:
    QAbstractItemModel *model;
    QAction *insertAction;
    QAction *removeAction;
    QItemSelectionModel *selectionModel;
};

#endif
