// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "bookmarkmanagerwidget.h"
#include "bookmarkitem.h"
#include "bookmarkmodel.h"
#include "tracer.h"
#include "xbelsupport.h"

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include <QtGui/QCloseEvent>
#include <QtGui/QKeySequence>
#include <QtGui/QShortcut>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QUrl>

QT_BEGIN_NAMESPACE

BookmarkManagerWidget::BookmarkManagerWidget(BookmarkModel *sourceModel,
        QWidget *parent)
    : QWidget(parent)
    , bookmarkModel(sourceModel)
{
    TRACE_OBJ
    ui.setupUi(this);

    ui.treeView->setModel(bookmarkModel);

    ui.treeView->expandAll();
    ui.treeView->installEventFilter(this);
    ui.treeView->viewport()->installEventFilter(this);
    ui.treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui.treeView, &QWidget::customContextMenuRequested,
            this, &BookmarkManagerWidget::customContextMenuRequested);

    connect(ui.remove, &QAbstractButton::clicked,
            this, [this]() { removeItem(); });
    connect(ui.lineEdit, &QLineEdit::textChanged,
            this, &BookmarkManagerWidget::textChanged);
    QShortcut *shortcut = new QShortcut(QKeySequence::Find, ui.lineEdit);
    connect(shortcut, &QShortcut::activated,
            ui.lineEdit, QOverload<>::of(&QWidget::setFocus));

    importExportMenu.addAction(tr("Import..."), this,
                               &BookmarkManagerWidget::importBookmarks);
    importExportMenu.addAction(tr("Export..."), this,
                               &BookmarkManagerWidget::exportBookmarks);
    ui.importExport->setMenu(&importExportMenu);

    shortcut = new QShortcut(QKeySequence::FindNext, this);
    connect(shortcut, &QShortcut::activated,
            this, &BookmarkManagerWidget::findNext);
    shortcut = new QShortcut(QKeySequence::FindPrevious, this);
    connect(shortcut, &QShortcut::activated,
            this, &BookmarkManagerWidget::findPrevious);

    connect(bookmarkModel, &QAbstractItemModel::rowsRemoved,
            this, &BookmarkManagerWidget::refeshBookmarkCache);
    connect(bookmarkModel, &QAbstractItemModel::rowsInserted,
            this, &BookmarkManagerWidget::refeshBookmarkCache);
    connect(bookmarkModel, &QAbstractItemModel::dataChanged,
            this, &BookmarkManagerWidget::refeshBookmarkCache);

    ui.treeView->setCurrentIndex(ui.treeView->indexAt(QPoint(2, 2)));
}

BookmarkManagerWidget::~BookmarkManagerWidget()
{
    TRACE_OBJ
}

void BookmarkManagerWidget::closeEvent(QCloseEvent *event)
{
    TRACE_OBJ
    event->accept();
    emit managerWidgetAboutToClose();
}

void BookmarkManagerWidget::renameItem(const QModelIndex &index)
{
    TRACE_OBJ
    // check if we should rename the "Bookmarks Menu", bail
    if (!bookmarkModel->parent(index).isValid())
        return;

    bookmarkModel->setItemsEditable(true);
    ui.treeView->edit(index);
    bookmarkModel->setItemsEditable(false);
}

static int nextIndex(int current, int count, bool forward)
{
    TRACE_OBJ
    if (current >= 0)
        return (forward ? (current + 1) : ((current - 1) + count)) % count;
    return 0;
}

void BookmarkManagerWidget::selectNextIndex(bool direction) const
{
    QModelIndex current = ui.treeView->currentIndex();
    if (current.isValid() && !cache.isEmpty()) {
        current = cache.at(nextIndex(cache.indexOf(current), cache.size(),
            direction));
    }
    ui.treeView->setCurrentIndex(current);
}

bool BookmarkManagerWidget::eventFilter(QObject *object, QEvent *event)
{
    TRACE_OBJ
    if (object != ui.treeView && object != ui.treeView->viewport())
        return QWidget::eventFilter(object, event);

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        switch (ke->key()) {
            case Qt::Key_F2:
                renameItem(ui.treeView->currentIndex());
                break;

            case Qt::Key_Delete:
                removeItem(ui.treeView->currentIndex());
                break;

            default: break;
        }
    }

    if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        switch (me->button()) {
            case Qt::LeftButton:
                if (me->modifiers() & Qt::ControlModifier)
                    setSourceFromIndex(ui.treeView->currentIndex(), true);
                break;

            case Qt::MiddleButton:
                setSourceFromIndex(ui.treeView->currentIndex(), true);
                break;

            default: break;
        }
    }
    return QObject::eventFilter(object, event);
}

void BookmarkManagerWidget::findNext()
{
    TRACE_OBJ
    selectNextIndex(true);
}

void BookmarkManagerWidget::findPrevious()
{
    TRACE_OBJ
    selectNextIndex(false);
}

void BookmarkManagerWidget::importBookmarks()
{
    TRACE_OBJ
    const QString &fileName = QFileDialog::getOpenFileName(nullptr, tr("Open File"),
        QDir::currentPath(), tr("Files (*.xbel)"));

    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
        XbelReader reader(bookmarkModel);
        reader.readFromFile(&file);
    }
}

void BookmarkManagerWidget::exportBookmarks()
{
    TRACE_OBJ
    QString fileName = QFileDialog::getSaveFileName(nullptr, tr("Save File"),
        QLatin1String("untitled.xbel"), tr("Files (*.xbel)"));

    const QLatin1String suffix(".xbel");
    if (!fileName.endsWith(suffix))
        fileName.append(suffix);

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        XbelWriter writer(bookmarkModel);
        writer.writeToFile(&file);
    } else {
        QMessageBox::information(this, tr("Qt Assistant"),
            tr("Unable to save bookmarks."), QMessageBox::Ok);
    }
}

void BookmarkManagerWidget::refeshBookmarkCache()
{
    TRACE_OBJ
    cache.clear();

    const QString &text = ui.lineEdit->text();
    if (!text.isEmpty())
        cache = bookmarkModel->indexListFor(text);
}

void BookmarkManagerWidget::textChanged(const QString &/*text*/)
{
    TRACE_OBJ
    refeshBookmarkCache();
    if (!cache.isEmpty())
        ui.treeView->setCurrentIndex(cache.at(0));
}

void BookmarkManagerWidget::removeItem(const QModelIndex &index)
{
    TRACE_OBJ
    QModelIndex current = index.isValid() ? index : ui.treeView->currentIndex();
    if (!current.parent().isValid() && current.row() < 2)
        return;  // check if we should delete the "Bookmarks Menu", bail

    if (bookmarkModel->hasChildren(current)) {
        int value = QMessageBox::question(this, tr("Remove"), tr("You are going"
            "to delete a Folder, this will also<br> remove it's content. Are "
            "you sure to continue?"),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (value == QMessageBox::Cancel)
            return;
    }
    bookmarkModel->removeItem(current);
}

void BookmarkManagerWidget::customContextMenuRequested(const QPoint &point)
{
    TRACE_OBJ
    const QModelIndex &index = ui.treeView->indexAt(point);
    if (!index.isValid())
        return;

    // check if we should open the menu on "Bookmarks Menu", bail
    if (!bookmarkModel->parent(index).isValid())
        return;

    QAction *remove = nullptr;
    QAction *rename = nullptr;
    QAction *showItem = nullptr;
    QAction *showItemInNewTab = nullptr;

    QMenu menu;
    if (bookmarkModel->data(index, UserRoleFolder).toBool()) {
        remove = menu.addAction(tr("Delete Folder"));
        rename = menu.addAction(tr("Rename Folder"));
    } else {
        showItem = menu.addAction(tr("Show Bookmark"));
        showItemInNewTab = menu.addAction(tr("Show Bookmark in New Tab"));
        menu.addSeparator();
        remove = menu.addAction(tr("Delete Bookmark"));
        rename = menu.addAction(tr("Rename Bookmark"));
    }

    QAction *pickedAction = menu.exec(ui.treeView->mapToGlobal(point));
    if (pickedAction == rename)
        renameItem(index);
    else if (pickedAction == remove)
        removeItem(index);
    else if (pickedAction == showItem || pickedAction == showItemInNewTab)
        setSourceFromIndex(index, pickedAction == showItemInNewTab);
}

void
BookmarkManagerWidget::setSourceFromIndex(const QModelIndex &index, bool newTab)
{
    TRACE_OBJ
    if (bookmarkModel->data(index, UserRoleFolder).toBool())
        return;

    const QVariant &data = bookmarkModel->data(index, UserRoleUrl);
    if (data.canConvert<QUrl>()) {
        if (newTab)
            emit setSourceInNewTab(data.toUrl());
        else
            emit setSource(data.toUrl());
    }
}

QT_END_NAMESPACE
