// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "tracer.h"

#include "bookmarkmanager.h"
#include "bookmarkmanagerwidget.h"
#include "bookmarkdialog.h"
#include "bookmarkfiltermodel.h"
#include "bookmarkitem.h"
#include "bookmarkmodel.h"
#include "centralwidget.h"
#include "helpenginewrapper.h"

#include <QtWidgets/QMenu>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QMessageBox>
#include <QtCore/QSortFilterProxyModel>
#include <QtWidgets/QToolBar>

QT_BEGIN_NAMESPACE

// -- BookmarkManager::BookmarkWidget

void BookmarkManager::BookmarkWidget::focusInEvent(QFocusEvent *event)
{
    TRACE_OBJ
    if (event->reason() != Qt::MouseFocusReason) {
        ui.lineEdit->selectAll();
        ui.lineEdit->setFocus();

        // force the focus in event on bookmark manager
        emit focusInEventOccurred();
    }
}

// -- BookmarkManager::BookmarkTreeView

BookmarkManager::BookmarkTreeView::BookmarkTreeView(QWidget *parent)
    : QTreeView(parent)
{
    TRACE_OBJ
    setAcceptDrops(true);
    setDragEnabled(true);
    setAutoExpandDelay(1000);
    setUniformRowHeights(true);
    setDropIndicatorShown(true);
    setExpandsOnDoubleClick(true);

    connect(this, &QTreeView::expanded, this, &BookmarkTreeView::setExpandedData);
    connect(this, &QTreeView::collapsed, this, &BookmarkTreeView::setExpandedData);
}

void BookmarkManager::BookmarkTreeView::subclassKeyPressEvent(QKeyEvent *event)
{
    TRACE_OBJ
    QTreeView::keyPressEvent(event);
}

void BookmarkManager::BookmarkTreeView::commitData(QWidget *editor)
{
    QTreeView::commitData(editor);
    emit editingDone();
}

void BookmarkManager::BookmarkTreeView::setExpandedData(const QModelIndex &index)
{
    TRACE_OBJ
    if (BookmarkModel *treeModel = qobject_cast<BookmarkModel*> (model()))
        treeModel->setData(index, isExpanded(index), UserRoleExpanded);
}

// -- BookmarkManager

QMutex BookmarkManager::mutex;
BookmarkManager* BookmarkManager::bookmarkManager = nullptr;

// -- public

BookmarkManager* BookmarkManager::instance()
{
    TRACE_OBJ
    if (!bookmarkManager) {
        QMutexLocker _(&mutex);
        if (!bookmarkManager)
            bookmarkManager = new BookmarkManager();
    }
    return bookmarkManager;
}

void BookmarkManager::destroy()
{
    TRACE_OBJ
    delete bookmarkManager;
    bookmarkManager = nullptr;
}

QWidget* BookmarkManager::bookmarkDockWidget() const
{
    TRACE_OBJ
    if (bookmarkWidget)
        return bookmarkWidget;
    return nullptr;
}

void BookmarkManager::setBookmarksMenu(QMenu* menu)
{
    TRACE_OBJ
    bookmarkMenu = menu;
    refreshBookmarkMenu();
}

void BookmarkManager::setBookmarksToolbar(QToolBar *toolBar)
{
    TRACE_OBJ
    m_toolBar = toolBar;
    refreshBookmarkToolBar();
}

// -- public slots

void BookmarkManager::addBookmark(const QString &title, const QString &url)
{
    TRACE_OBJ
    showBookmarkDialog(title.isEmpty() ? tr("Untitled") : title,
        url.isEmpty() ? QLatin1String("about:blank") : url);

    storeBookmarks();
}

// -- private

BookmarkManager::BookmarkManager()
    : bookmarkModel(new BookmarkModel)
    , bookmarkWidget(new BookmarkWidget)
    , bookmarkTreeView(new BookmarkTreeView)
{
    TRACE_OBJ
    bookmarkWidget->installEventFilter(this);
    connect(bookmarkWidget->ui.add, &QAbstractButton::clicked,
            this, &BookmarkManager::addBookmarkActivated);
    connect(bookmarkWidget->ui.remove, &QAbstractButton::clicked,
            this, &BookmarkManager::removeBookmarkActivated);
    connect(bookmarkWidget->ui.lineEdit, &QLineEdit::textChanged,
            this, &BookmarkManager::textChanged);
    connect(bookmarkWidget, &BookmarkWidget::focusInEventOccurred,
            this, &BookmarkManager::focusInEventOccurred);

    bookmarkTreeView->setModel(bookmarkModel);
    bookmarkTreeView->installEventFilter(this);
    bookmarkTreeView->viewport()->installEventFilter(this);
    bookmarkTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    bookmarkWidget->ui.stackedWidget->addWidget(bookmarkTreeView);

    connect(bookmarkTreeView, &QAbstractItemView::activated,
            this, [this](const QModelIndex &index) { setSourceFromIndex(index, false); });
    connect(bookmarkTreeView, &QWidget::customContextMenuRequested,
            this, &BookmarkManager::customContextMenuRequested);
    connect(bookmarkTreeView, &BookmarkTreeView::editingDone,
            this, &BookmarkManager::storeBookmarks);

    connect(&HelpEngineWrapper::instance(), &HelpEngineWrapper::setupFinished,
            this, &BookmarkManager::setupFinished);

    connect(bookmarkModel, &QAbstractItemModel::rowsRemoved,
            this, &BookmarkManager::refreshBookmarkMenu);
    connect(bookmarkModel, &QAbstractItemModel::rowsInserted,
            this, &BookmarkManager::refreshBookmarkMenu);
    connect(bookmarkModel, &QAbstractItemModel::dataChanged,
            this, &BookmarkManager::refreshBookmarkMenu);

    connect(bookmarkModel, &QAbstractItemModel::rowsRemoved,
            this, &BookmarkManager::refreshBookmarkToolBar);
    connect(bookmarkModel, &QAbstractItemModel::rowsInserted,
            this, &BookmarkManager::refreshBookmarkToolBar);
    connect(bookmarkModel, &QAbstractItemModel::dataChanged,
            this, &BookmarkManager::refreshBookmarkToolBar);

}

BookmarkManager::~BookmarkManager()
{
    TRACE_OBJ
    delete bookmarkManagerWidget;
    storeBookmarks();
    delete bookmarkModel;
}

void BookmarkManager::removeItem(const QModelIndex &index)
{
    TRACE_OBJ
    QModelIndex current = index;
    if (typeAndSearch) { // need to map because of proxy
        current = typeAndSearchModel->mapToSource(current);
        current = bookmarkFilterModel->mapToSource(current);
    } else if (!bookmarkModel->parent(index).isValid()) {
        return;  // check if we should delete the "Bookmarks Menu", bail
    }

    if (bookmarkModel->hasChildren(current)) {
        int value = QMessageBox::question(bookmarkTreeView, tr("Remove"),
            tr("You are going to delete a Folder, this will also<br>"
            "remove it's content. Are you sure to continue?"),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (value == QMessageBox::Cancel)
            return;
    }
    bookmarkModel->removeItem(current);

    storeBookmarks();
}

bool BookmarkManager::eventFilter(QObject *object, QEvent *event)
{
    if (object != bookmarkTreeView && object != bookmarkTreeView->viewport()
        && object != bookmarkWidget)
            return QObject::eventFilter(object, event);

    TRACE_OBJ
    const bool isWidget = object == bookmarkWidget;
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        switch (ke->key()) {
            case Qt::Key_F2:
                renameBookmark(bookmarkTreeView->currentIndex());
                break;

            case Qt::Key_Delete:
                removeItem(bookmarkTreeView->currentIndex());
                return true;

            case Qt::Key_Up:    // needs event filter on widget
            case Qt::Key_Down:
                if (isWidget)
                    bookmarkTreeView->subclassKeyPressEvent(ke);
                break;

            case Qt::Key_Escape:
                emit escapePressed();
                break;

            default: break;
        }
    }

    if (event->type() == QEvent::MouseButtonRelease && !isWidget) {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        switch (me->button()) {
            case Qt::LeftButton:
                if (me->modifiers() & Qt::ControlModifier)
                    setSourceFromIndex(bookmarkTreeView->currentIndex(), true);
                break;

            case Qt::MiddleButton:
                setSourceFromIndex(bookmarkTreeView->currentIndex(), true);
                break;

            default: break;
        }
    }

    return QObject::eventFilter(object, event);
}

void BookmarkManager::buildBookmarksMenu(const QModelIndex &index, QMenu* menu)
{
    TRACE_OBJ
    if (!index.isValid())
        return;

    const QString &text = index.data().toString();
    const QIcon &icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
    if (index.data(UserRoleFolder).toBool()) {
        if (QMenu* subMenu = menu->addMenu(icon, text)) {
            for (int i = 0; i < bookmarkModel->rowCount(index); ++i)
                buildBookmarksMenu(bookmarkModel->index(i, 0, index), subMenu);
        }
    } else {
        QAction *action = menu->addAction(icon, text);
        action->setData(index.data(UserRoleUrl).toString());
        connect(action, &QAction::triggered,
                this, &BookmarkManager::setSourceFromAction);
    }
}

void BookmarkManager::showBookmarkDialog(const QString &name, const QString &url)
{
    TRACE_OBJ
    BookmarkDialog dialog(bookmarkModel, name, url, bookmarkTreeView);
    dialog.exec();
}

// -- private slots

void BookmarkManager::setupFinished()
{
    TRACE_OBJ
    bookmarkModel->setBookmarks(HelpEngineWrapper::instance().bookmarks());
    bookmarkModel->expandFoldersIfNeeeded(bookmarkTreeView);

    refreshBookmarkMenu();
    refreshBookmarkToolBar();

    bookmarkTreeView->hideColumn(1);
    bookmarkTreeView->header()->setVisible(false);
    bookmarkTreeView->header()->setStretchLastSection(true);

    if (!bookmarkFilterModel)
        bookmarkFilterModel = new BookmarkFilterModel(this);
    bookmarkFilterModel->setSourceModel(bookmarkModel);
    bookmarkFilterModel->filterBookmarkFolders();

    if (!typeAndSearchModel)
        typeAndSearchModel = new QSortFilterProxyModel(this);
    typeAndSearchModel->setDynamicSortFilter(true);
    typeAndSearchModel->setSourceModel(bookmarkFilterModel);
}

void BookmarkManager::storeBookmarks()
{
    HelpEngineWrapper::instance().setBookmarks(bookmarkModel->bookmarks());
}

void BookmarkManager::addBookmarkActivated()
{
    TRACE_OBJ
    if (CentralWidget *widget = CentralWidget::instance())
        addBookmark(widget->currentTitle(), widget->currentSource().toString());
}

void BookmarkManager::removeBookmarkActivated()
{
    TRACE_OBJ
    removeItem(bookmarkTreeView->currentIndex());
}

void BookmarkManager::manageBookmarks()
{
    TRACE_OBJ
    if (bookmarkManagerWidget == nullptr) {
        bookmarkManagerWidget = new BookmarkManagerWidget(bookmarkModel);
        connect(bookmarkManagerWidget, &BookmarkManagerWidget::setSource,
                this, &BookmarkManager::setSource);
        connect(bookmarkManagerWidget, &BookmarkManagerWidget::setSourceInNewTab,
                this, &BookmarkManager::setSourceInNewTab);
        connect(bookmarkManagerWidget, &BookmarkManagerWidget::managerWidgetAboutToClose,
                this, &BookmarkManager::managerWidgetAboutToClose);
    }
    bookmarkManagerWidget->show();
    bookmarkManagerWidget->raise();
}

void BookmarkManager::refreshBookmarkMenu()
{
    TRACE_OBJ
    if (!bookmarkMenu)
        return;

    bookmarkMenu->clear();

    bookmarkMenu->addAction(tr("Manage Bookmarks..."), this,
                            &BookmarkManager::manageBookmarks);
    bookmarkMenu->addAction(QIcon::fromTheme("bookmark-new"), tr("Add Bookmark..."),
                            QKeySequence(tr("Ctrl+D")),
                            this, &BookmarkManager::addBookmarkActivated);

    bookmarkMenu->addSeparator();

    QModelIndex root = bookmarkModel->index(0, 0, QModelIndex()).parent();
    buildBookmarksMenu(bookmarkModel->index(0, 0, root), bookmarkMenu);

    bookmarkMenu->addSeparator();

    root = bookmarkModel->index(1, 0, QModelIndex());
    for (int i = 0; i < bookmarkModel->rowCount(root); ++i)
        buildBookmarksMenu(bookmarkModel->index(i, 0, root), bookmarkMenu);
}

void BookmarkManager::refreshBookmarkToolBar()
{
    TRACE_OBJ
    if (!m_toolBar)
        return;

    m_toolBar->clear();
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    const QModelIndex &root = bookmarkModel->index(0, 0, QModelIndex());
    for (int i = 0; i < bookmarkModel->rowCount(root); ++i) {
        const QModelIndex &index = bookmarkModel->index(i, 0, root);
        if (index.data(UserRoleFolder).toBool()) {
            QToolButton *button = new QToolButton(m_toolBar);
            button->setPopupMode(QToolButton::InstantPopup);
            button->setText(index.data().toString());
            QMenu *menu = new QMenu(button);
            for (int j = 0; j < bookmarkModel->rowCount(index); ++j)
                buildBookmarksMenu(bookmarkModel->index(j, 0, index), menu);
            button->setMenu(menu);
            button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            button->setIcon(qvariant_cast<QIcon>(index.data(Qt::DecorationRole)));
            QAction *a = m_toolBar->addWidget(button);
            a->setText(index.data().toString());
        } else {
            QAction *action = m_toolBar->addAction(
                qvariant_cast<QIcon>(index.data(Qt::DecorationRole)),
                index.data().toString(), this, &BookmarkManager::setSourceFromAction);
            action->setData(index.data(UserRoleUrl).toString());
        }
    }
}

void BookmarkManager::renameBookmark(const QModelIndex &index)
{
    // check if we should rename the "Bookmarks Menu", bail
    if (!typeAndSearch && !bookmarkModel->parent(index).isValid())
        return;

    bookmarkModel->setItemsEditable(true);
    bookmarkTreeView->edit(index);
    bookmarkModel->setItemsEditable(false);
}


void BookmarkManager::setSourceFromAction()
{
    TRACE_OBJ
    const QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;

    const QVariant &data = action->data();
    if (data.canConvert<QUrl>())
        emit setSource(data.toUrl());
}

void BookmarkManager::setSourceFromIndex(const QModelIndex &index, bool newTab)
{
    TRACE_OBJ
    QAbstractItemModel *base = bookmarkModel;
    if (typeAndSearch)
        base = typeAndSearchModel;

    if (base->data(index, UserRoleFolder).toBool())
        return;

    const QVariant &data = base->data(index, UserRoleUrl);
    if (data.canConvert<QUrl>()) {
        if (newTab)
            emit setSourceInNewTab(data.toUrl());
        else
            emit setSource(data.toUrl());
    }
}

void BookmarkManager::customContextMenuRequested(const QPoint &point)
{
    TRACE_OBJ
    QModelIndex index = bookmarkTreeView->indexAt(point);
    if (!index.isValid())
        return;

    // check if we should open the menu on "Bookmarks Menu", bail
    if (!typeAndSearch && !bookmarkModel->parent(index).isValid())
        return;

    QAction *remove = nullptr;
    QAction *rename = nullptr;
    QAction *showItem = nullptr;
    QAction *showItemInNewTab = nullptr;

    QMenu menu;
    if (!typeAndSearch && bookmarkModel->data(index, UserRoleFolder).toBool()) {
        remove = menu.addAction(tr("Delete Folder"));
        rename = menu.addAction(tr("Rename Folder"));
    } else {
        showItem = menu.addAction(tr("Show Bookmark"));
        showItemInNewTab = menu.addAction(tr("Show Bookmark in New Tab"));
        menu.addSeparator();
        remove = menu.addAction(tr("Delete Bookmark"));
        rename = menu.addAction(tr("Rename Bookmark"));
    }

    QAction *pickedAction = menu.exec(bookmarkTreeView->mapToGlobal(point));
    if (pickedAction == rename)
        renameBookmark(index);
    else if (pickedAction == remove)
        removeItem(index);
    else if (pickedAction == showItem || pickedAction == showItemInNewTab)
        setSourceFromIndex(index, pickedAction == showItemInNewTab);
}

void BookmarkManager::focusInEventOccurred()
{
    TRACE_OBJ
    const QModelIndex &index = bookmarkTreeView->indexAt(QPoint(2, 2));
    if (index.isValid())
        bookmarkTreeView->setCurrentIndex(index);
}

void BookmarkManager::managerWidgetAboutToClose()
{
    if (bookmarkManagerWidget)
        bookmarkManagerWidget->deleteLater();
    bookmarkManagerWidget = nullptr;

    storeBookmarks();
}

void BookmarkManager::textChanged(const QString &text)
{
    TRACE_OBJ
    if (!bookmarkWidget->ui.lineEdit->text().isEmpty()) {
        if (!typeAndSearch) {
            typeAndSearch = true;
            bookmarkTreeView->setItemsExpandable(false);
            bookmarkTreeView->setRootIsDecorated(false);
            bookmarkTreeView->setModel(typeAndSearchModel);
        }
        typeAndSearchModel->setFilterRegularExpression(text);
    } else {
        typeAndSearch = false;
        bookmarkTreeView->setModel(bookmarkModel);
        bookmarkTreeView->setItemsExpandable(true);
        bookmarkTreeView->setRootIsDecorated(true);
        bookmarkModel->expandFoldersIfNeeeded(bookmarkTreeView);
    }
}

QT_END_NAMESPACE
