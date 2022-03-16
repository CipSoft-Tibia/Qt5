/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Designer of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qtresourceview_p.h"
#include "qtresourcemodel_p.h"
#include "qtresourceeditordialog_p.h"
#include "iconloader_p.h"

#include <QtDesigner/abstractformeditor.h>
#include <QtDesigner/abstractsettings.h>

#include <QtWidgets/qtoolbar.h>
#include <QtWidgets/qaction.h>
#include <QtWidgets/qsplitter.h>
#include <QtWidgets/qtreewidget.h>
#include <QtWidgets/qlistwidget.h>
#include <QtWidgets/qheaderview.h>
#include <QtWidgets/qboxlayout.h>
#include <QtGui/qpainter.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qqueue.h>
#include <QtGui/qpainter.h>
#include <QtWidgets/qdialogbuttonbox.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qmessagebox.h>
#include <QtWidgets/qapplication.h>
#if QT_CONFIG(clipboard)
#include <QtGui/qclipboard.h>
#endif
#include <QtWidgets/qmenu.h>
#include <QtWidgets/qlineedit.h>
#include <QtGui/qdrag.h>
#include <QtCore/qmimedata.h>
#include <QtXml/qdom.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

static const char *elementResourceData = "resource";
static const char *typeAttribute = "type";
static const char *typeImage = "image";
static const char *typeStyleSheet = "stylesheet";
static const char *typeOther = "other";
static const char *fileAttribute = "file";
static const char *SplitterPosition = "SplitterPosition";
static const char *Geometry = "Geometry";
static const char *ResourceViewDialogC = "ResourceDialog";

// ---------------- ResourceListWidget: A list widget that has drag enabled
class ResourceListWidget : public QListWidget {
public:
    ResourceListWidget(QWidget *parent = 0);

protected:
    void startDrag(Qt::DropActions supportedActions) override;
};

ResourceListWidget::ResourceListWidget(QWidget *parent) :
    QListWidget(parent)
{
    setDragEnabled(true);
}

void ResourceListWidget::startDrag(Qt::DropActions supportedActions)
{
    if (supportedActions == Qt::MoveAction)
        return;

    QListWidgetItem *item = currentItem();
    if (!item)
        return;

    const QString filePath = item->data(Qt::UserRole).toString();
    const QIcon icon = item->icon();

    QMimeData *mimeData = new QMimeData;
    const QtResourceView::ResourceType type = icon.isNull() ? QtResourceView::ResourceOther : QtResourceView::ResourceImage;
    mimeData->setText(QtResourceView::encodeMimeData(type , filePath));

    QDrag *drag = new QDrag(this);
    if (!icon.isNull()) {
        const QSize size = icon.actualSize(iconSize());
        drag->setPixmap(icon.pixmap(size));
        drag->setHotSpot(QPoint(size.width() / 2, size.height() / 2));
    }

    drag->setMimeData(mimeData);
    drag->exec(Qt::CopyAction);
}

/* TODO

   1) load the icons in separate thread...Hmm..if Qt is configured with threads....
*/

// ---------------------------- QtResourceViewPrivate
class QtResourceViewPrivate
{
    QtResourceView *q_ptr;
    Q_DECLARE_PUBLIC(QtResourceView)
public:
    QtResourceViewPrivate(QDesignerFormEditorInterface *core);

    void slotResourceSetActivated(QtResourceSet *resourceSet);
    void slotCurrentPathChanged(QTreeWidgetItem *);
    void slotCurrentResourceChanged(QListWidgetItem *);
    void slotResourceActivated(QListWidgetItem *);
    void slotEditResources();
    void slotReloadResources();
#if QT_CONFIG(clipboard)
    void slotCopyResourcePath();
#endif
    void slotListWidgetContextMenuRequested(const QPoint &pos);
    void slotFilterChanged(const QString &pattern);
    void createPaths();
    QTreeWidgetItem *createPath(const QString &path, QTreeWidgetItem *parent);
    void createResources(const QString &path);
    void storeExpansionState();
    void applyExpansionState();
    void restoreSettings();
    void saveSettings();
    void updateActions();
    void filterOutResources();

    QPixmap makeThumbnail(const QPixmap &pix) const;

    QDesignerFormEditorInterface *m_core;
    QtResourceModel *m_resourceModel;
    QToolBar *m_toolBar;
    QWidget *m_filterWidget;
    QTreeWidget *m_treeWidget;
    QListWidget *m_listWidget;
    QSplitter *m_splitter;
    QMap<QString, QStringList>       m_pathToContents; // full path to contents file names (full path to its resource filenames)
    QMap<QString, QString>           m_pathToParentPath; // full path to full parent path
    QMap<QString, QStringList>       m_pathToSubPaths; // full path to full sub paths
    QMap<QString, QTreeWidgetItem *> m_pathToItem;
    QMap<QTreeWidgetItem *, QString> m_itemToPath;
    QMap<QString, QListWidgetItem *> m_resourceToItem;
    QMap<QListWidgetItem *, QString> m_itemToResource;
    QAction *m_editResourcesAction;
    QAction *m_reloadResourcesAction;
    QAction *m_copyResourcePathAction;

    QMap<QString, bool> m_expansionState;

    bool m_ignoreGuiSignals;
    QString m_settingsKey;
    bool m_resourceEditingEnabled;
    QString m_filterPattern;
};

QtResourceViewPrivate::QtResourceViewPrivate(QDesignerFormEditorInterface *core) :
    q_ptr(0),
    m_core(core),
    m_resourceModel(0),
    m_toolBar(new QToolBar),
    m_treeWidget(new QTreeWidget),
    m_listWidget(new ResourceListWidget),
    m_splitter(0),
    m_editResourcesAction(0),
    m_reloadResourcesAction(0),
    m_copyResourcePathAction(0),
    m_ignoreGuiSignals(false),
    m_resourceEditingEnabled(true)
{
    m_toolBar->setIconSize(QSize(22, 22));
}

void QtResourceViewPrivate::restoreSettings()
{
    if (m_settingsKey.isEmpty())
        return;

    QDesignerSettingsInterface *settings = m_core->settingsManager();
    settings->beginGroup(m_settingsKey);

    m_splitter->restoreState(settings->value(QLatin1String(SplitterPosition)).toByteArray());
    settings->endGroup();
}

void QtResourceViewPrivate::saveSettings()
{
    if (m_settingsKey.isEmpty())
        return;

    QDesignerSettingsInterface *settings = m_core->settingsManager();
    settings->beginGroup(m_settingsKey);

    settings->setValue(QLatin1String(SplitterPosition), m_splitter->saveState());
    settings->endGroup();
}

void QtResourceViewPrivate::slotEditResources()
{
    const QString selectedResource
            = QtResourceEditorDialog::editResources(m_core, m_resourceModel,
                                                    m_core->dialogGui(), q_ptr);
    if (!selectedResource.isEmpty())
        q_ptr->selectResource(selectedResource);
}

void QtResourceViewPrivate::slotReloadResources()
{
    if (m_resourceModel) {
        int errorCount;
        QString errorMessages;
        m_resourceModel->reload(&errorCount, &errorMessages);
        if (errorCount)
            QtResourceEditorDialog::displayResourceFailures(errorMessages, m_core->dialogGui(), q_ptr);
    }
}

#if QT_CONFIG(clipboard)
void QtResourceViewPrivate::slotCopyResourcePath()
{
    const QString path = q_ptr->selectedResource();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(path);
}
#endif

void QtResourceViewPrivate::slotListWidgetContextMenuRequested(const QPoint &pos)
{
    QMenu menu(q_ptr);
    menu.addAction(m_copyResourcePathAction);
    menu.exec(m_listWidget->mapToGlobal(pos));
}

void QtResourceViewPrivate::slotFilterChanged(const QString &pattern)
{
    m_filterPattern = pattern;
    filterOutResources();
}

void QtResourceViewPrivate::storeExpansionState()
{
    for (auto it = m_pathToItem.cbegin(), end = m_pathToItem.cend(); it != end; ++it)
        m_expansionState.insert(it.key(), it.value()->isExpanded());
}

void QtResourceViewPrivate::applyExpansionState()
{
    for (auto it = m_pathToItem.cbegin(), end = m_pathToItem.cend(); it != end; ++it)
        it.value()->setExpanded(m_expansionState.value(it.key(), true));
}

QPixmap QtResourceViewPrivate::makeThumbnail(const QPixmap &pix) const
{
    int w = qMax(48, pix.width());
    int h = qMax(48, pix.height());
    QRect imgRect(0, 0, w, h);
    QImage img(w, h, QImage::Format_ARGB32_Premultiplied);
    img.fill(0);
    if (!pix.isNull()) {
        QRect r(0, 0, pix.width(), pix.height());
        r.moveCenter(imgRect.center());
        QPainter p(&img);
        p.drawPixmap(r.topLeft(), pix);
    }
    return QPixmap::fromImage(img);
}

void QtResourceViewPrivate::updateActions()
{
    bool resourceActive = false;
    if (m_resourceModel)
        resourceActive = m_resourceModel->currentResourceSet();

    m_editResourcesAction->setVisible(m_resourceEditingEnabled);
    m_editResourcesAction->setEnabled(resourceActive);
    m_reloadResourcesAction->setEnabled(resourceActive);
    m_filterWidget->setEnabled(resourceActive);
}

void QtResourceViewPrivate::slotResourceSetActivated(QtResourceSet *resourceSet)
{
    Q_UNUSED(resourceSet)

    updateActions();

    storeExpansionState();
    const QString currentPath = m_itemToPath.value(m_treeWidget->currentItem());
    const QString currentResource = m_itemToResource.value(m_listWidget->currentItem());
    m_treeWidget->clear();
    m_pathToContents.clear();
    m_pathToParentPath.clear();
    m_pathToSubPaths.clear();
    m_pathToItem.clear();
    m_itemToPath.clear();
    m_listWidget->clear();
    m_resourceToItem.clear();
    m_itemToResource.clear();

    createPaths();
    applyExpansionState();

    if (!currentResource.isEmpty())
        q_ptr->selectResource(currentResource);
    else if (!currentPath.isEmpty())
        q_ptr->selectResource(currentPath);
    filterOutResources();
}

void QtResourceViewPrivate::slotCurrentPathChanged(QTreeWidgetItem *item)
{
    if (m_ignoreGuiSignals)
        return;

    m_listWidget->clear();
    m_resourceToItem.clear();
    m_itemToResource.clear();

    if (!item)
        return;

    const QString currentPath = m_itemToPath.value(item);
    createResources(currentPath);
}

void QtResourceViewPrivate::slotCurrentResourceChanged(QListWidgetItem *item)
{
    m_copyResourcePathAction->setEnabled(item);
    if (m_ignoreGuiSignals)
        return;

    emit q_ptr->resourceSelected(m_itemToResource.value(item));
}

void QtResourceViewPrivate::slotResourceActivated(QListWidgetItem *item)
{
    if (m_ignoreGuiSignals)
        return;

    emit q_ptr->resourceActivated(m_itemToResource.value(item));
}

void QtResourceViewPrivate::createPaths()
{
    if (!m_resourceModel)
        return;

    // Resource root up until 4.6 was ':', changed to ":/" as of 4.7
    const QString root(QStringLiteral(":/"));

    QMap<QString, QString> contents = m_resourceModel->contents();
    for (auto it = contents.cbegin(), end = contents.cend(); it != end; ++it) {
        const QFileInfo fi(it.key());
        QString dirPath = fi.absolutePath();
        m_pathToContents[dirPath].append(fi.fileName());
        while (!m_pathToParentPath.contains(dirPath) && dirPath != root) { // create all parent paths
            const QFileInfo fd(dirPath);
            const QString parentDirPath = fd.absolutePath();
            m_pathToParentPath[dirPath] = parentDirPath;
            m_pathToSubPaths[parentDirPath].append(dirPath);
            dirPath = parentDirPath;
        }
    }

    QQueue<QPair<QString, QTreeWidgetItem *> > pathToParentItemQueue;
    pathToParentItemQueue.enqueue(qMakePair(root, static_cast<QTreeWidgetItem *>(0)));
    while (!pathToParentItemQueue.isEmpty()) {
        QPair<QString, QTreeWidgetItem *> pathToParentItem = pathToParentItemQueue.dequeue();
        const QString path = pathToParentItem.first;
        QTreeWidgetItem *item = createPath(path, pathToParentItem.second);
        const QStringList subPaths = m_pathToSubPaths.value(path);
        for (const QString &subPath : subPaths)
            pathToParentItemQueue.enqueue(qMakePair(subPath, item));
    }
}

void QtResourceViewPrivate::filterOutResources()
{
    QMap<QString, bool> pathToMatchingContents; // true means the path has any matching contents
    QMap<QString, bool> pathToVisible; // true means the path has to be shown

    // 1) we go from root path recursively.
    // 2) we check every path if it contains at least one matching resource - if empty we add it
    //                 to pathToMatchingContents and pathToVisible with false, if non empty
    //                 we add it with true and change every parent path in pathToVisible to true.
    // 3) we hide these items which has pathToVisible value false.

    const bool matchAll = m_filterPattern.isEmpty();
    const QString root(QStringLiteral(":/"));

    QQueue<QString> pathQueue;
    pathQueue.enqueue(root);
    while (!pathQueue.isEmpty()) {
        const QString path = pathQueue.dequeue();

        bool hasContents = matchAll;
        if (!matchAll) { // the case filter is not empty - we check if the path contains anything
            // the path contains at least one resource which matches the filter
            const QStringList fileNames = m_pathToContents.value(path);
            hasContents =
                std::any_of(fileNames.cbegin(), fileNames.cend(),
                            [this] (const QString &f) { return f.contains(this->m_filterPattern, Qt::CaseInsensitive); });
        }

        pathToMatchingContents[path] = hasContents;
        pathToVisible[path] = hasContents;

        if (hasContents) { // if the path is going to be shown we need to show all its parent paths
            QString parentPath = m_pathToParentPath.value(path);
            while (!parentPath.isEmpty()) {
                QString p = parentPath;
                if (pathToVisible.value(p)) // parent path is already shown, we break the loop
                    break;
                pathToVisible[p] = true;
                parentPath = m_pathToParentPath.value(p);
            }
        }

        const QStringList subPaths = m_pathToSubPaths.value(path); // we do the same for children paths
        for (const QString &subPath : subPaths)
            pathQueue.enqueue(subPath);
    }

    // we setup here new path and resource to be activated
    const QString currentPath = m_itemToPath.value(m_treeWidget->currentItem());
    QString newCurrentPath = currentPath;
    QString currentResource = m_itemToResource.value(m_listWidget->currentItem());
    if (!matchAll) {
        bool searchForNewPathWithContents = true;

        if (!currentPath.isEmpty()) { // if the currentPath is empty we will search for a new path too
            QMap<QString, bool>::ConstIterator it = pathToMatchingContents.constFind(currentPath);
            if (it != pathToMatchingContents.constEnd() && it.value()) // the current item has contents, we don't need to search for another path
                searchForNewPathWithContents = false;
        }

        if (searchForNewPathWithContents) {
            // we find the first path with the matching contents
            QMap<QString, bool>::ConstIterator itContents = pathToMatchingContents.constBegin();
            while (itContents != pathToMatchingContents.constEnd()) {
                if (itContents.value()) {
                    newCurrentPath = itContents.key(); // the new path will be activated
                    break;
                }

                itContents++;
            }
        }

        QFileInfo fi(currentResource);
        if (!fi.fileName().contains(m_filterPattern, Qt::CaseInsensitive)) { // the case when the current resource is filtered out
            const QStringList fileNames = m_pathToContents.value(newCurrentPath);
            // we try to select the first matching resource from the newCurrentPath
            for (const QString &fileName : fileNames) {
                if (fileName.contains(m_filterPattern, Qt::CaseInsensitive)) {
                    QDir dirPath(newCurrentPath);
                    currentResource = dirPath.absoluteFilePath(fileName); // the new resource inside newCurrentPath will be activated
                    break;
                }
            }
        }
    }

    QTreeWidgetItem *newCurrentItem = m_pathToItem.value(newCurrentPath);
    if (currentPath != newCurrentPath)
        m_treeWidget->setCurrentItem(newCurrentItem);
    else
        slotCurrentPathChanged(newCurrentItem); // trigger filtering on the current path

    QListWidgetItem *currentResourceItem = m_resourceToItem.value(currentResource);
    if (currentResourceItem) {
        m_listWidget->setCurrentItem(currentResourceItem);
        m_listWidget->scrollToItem(currentResourceItem);
    }

    // hide all paths filtered out
    for (auto it = pathToVisible.cbegin(), end = pathToVisible.cend(); it != end; ++it) {
        if (QTreeWidgetItem *item = m_pathToItem.value(it.key()))
            item->setHidden(!it.value());
    }
}

QTreeWidgetItem *QtResourceViewPrivate::createPath(const QString &path, QTreeWidgetItem *parent)
{
    QTreeWidgetItem *item = 0;
    if (parent)
        item = new QTreeWidgetItem(parent);
    else
        item = new QTreeWidgetItem(m_treeWidget);
    m_pathToItem[path] = item;
    m_itemToPath[item] = path;
    QString substPath;
    if (parent) {
        QFileInfo di(path);
        substPath = di.fileName();
    } else {
        substPath = QStringLiteral("<resource root>");
    }
    item->setText(0, substPath);
    item->setToolTip(0, path);
    return item;
}

void QtResourceViewPrivate::createResources(const QString &path)
{
    const bool matchAll = m_filterPattern.isEmpty();

    QDir dir(path);
    const QStringList fileNames = m_pathToContents.value(path);
    for (const QString &fileName : fileNames) {
        const bool showProperty = matchAll || fileName.contains(m_filterPattern, Qt::CaseInsensitive);
        if (showProperty) {
            QString filePath = dir.absoluteFilePath(fileName);
            QFileInfo fi(filePath);
            if (fi.isFile()) {
                QListWidgetItem *item = new QListWidgetItem(fi.fileName(), m_listWidget);
                const QPixmap pix = QPixmap(filePath);
                if (pix.isNull()) {
                    item->setToolTip(filePath);
                } else {
                    item->setIcon(QIcon(makeThumbnail(pix)));
                    const QSize size = pix.size();
                    item->setToolTip(QtResourceView::tr("Size: %1 x %2\n%3").arg(size.width()).arg(size.height()).arg(filePath));
                }
                item->setFlags(item->flags() | Qt::ItemIsDragEnabled);
                item->setData(Qt::UserRole, filePath);
                m_itemToResource[item] = filePath;
                m_resourceToItem[filePath] = item;
            }
        }
    }
}

// -------------- QtResourceView

QtResourceView::QtResourceView(QDesignerFormEditorInterface *core, QWidget *parent) :
    QWidget(parent),
    d_ptr(new QtResourceViewPrivate(core))
{
    d_ptr->q_ptr = this;

    QIcon editIcon = QIcon::fromTheme(QStringLiteral("document-properties"), qdesigner_internal::createIconSet(QStringLiteral("edit.png")));
    d_ptr->m_editResourcesAction = new QAction(editIcon, tr("Edit Resources..."), this);
    d_ptr->m_toolBar->addAction(d_ptr->m_editResourcesAction);
    connect(d_ptr->m_editResourcesAction, SIGNAL(triggered()), this, SLOT(slotEditResources()));
    d_ptr->m_editResourcesAction->setEnabled(false);

    QIcon refreshIcon = QIcon::fromTheme(QStringLiteral("view-refresh"), qdesigner_internal::createIconSet(QStringLiteral("reload.png")));
    d_ptr->m_reloadResourcesAction = new QAction(refreshIcon, tr("Reload"), this);

    d_ptr->m_toolBar->addAction(d_ptr->m_reloadResourcesAction);
    connect(d_ptr->m_reloadResourcesAction, SIGNAL(triggered()), this, SLOT(slotReloadResources()));
    d_ptr->m_reloadResourcesAction->setEnabled(false);

#if QT_CONFIG(clipboard)
    QIcon copyIcon = QIcon::fromTheme(QStringLiteral("edit-copy"), qdesigner_internal::createIconSet(QStringLiteral("editcopy.png")));
    d_ptr->m_copyResourcePathAction = new QAction(copyIcon, tr("Copy Path"), this);
    connect(d_ptr->m_copyResourcePathAction, SIGNAL(triggered()), this, SLOT(slotCopyResourcePath()));
    d_ptr->m_copyResourcePathAction->setEnabled(false);
#endif

    d_ptr->m_filterWidget = new QWidget(d_ptr->m_toolBar);
    QHBoxLayout *filterLayout = new QHBoxLayout(d_ptr->m_filterWidget);
    filterLayout->setContentsMargins(0, 0, 0, 0);
    QLineEdit *filterLineEdit = new QLineEdit(d_ptr->m_filterWidget);
    connect(filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(slotFilterChanged(QString)));
    filterLineEdit->setPlaceholderText(tr("Filter"));
    filterLineEdit->setClearButtonEnabled(true);
    filterLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Ignored));
    filterLayout->addWidget(filterLineEdit);
    d_ptr->m_toolBar->addWidget(d_ptr->m_filterWidget);

    d_ptr->m_splitter = new QSplitter;
    d_ptr->m_splitter->setChildrenCollapsible(false);
    d_ptr->m_splitter->addWidget(d_ptr->m_treeWidget);
    d_ptr->m_splitter->addWidget(d_ptr->m_listWidget);

    QLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(d_ptr->m_toolBar);
    layout->addWidget(d_ptr->m_splitter);

    d_ptr->m_treeWidget->setColumnCount(1);
    d_ptr->m_treeWidget->header()->hide();
    d_ptr->m_treeWidget->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));

    d_ptr->m_listWidget->setViewMode(QListView::IconMode);
    d_ptr->m_listWidget->setResizeMode(QListView::Adjust);
    d_ptr->m_listWidget->setIconSize(QSize(48, 48));
    d_ptr->m_listWidget->setGridSize(QSize(64, 64));

    connect(d_ptr->m_treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
                    this, SLOT(slotCurrentPathChanged(QTreeWidgetItem*)));
    connect(d_ptr->m_listWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
                    this, SLOT(slotCurrentResourceChanged(QListWidgetItem*)));
    connect(d_ptr->m_listWidget, SIGNAL(itemActivated(QListWidgetItem*)),
                    this, SLOT(slotResourceActivated(QListWidgetItem*)));
    d_ptr->m_listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(d_ptr->m_listWidget, SIGNAL(customContextMenuRequested(QPoint)),
                this, SLOT(slotListWidgetContextMenuRequested(QPoint)));
}

QtResourceView::~QtResourceView()
{
    if (!d_ptr->m_settingsKey.isEmpty())
        d_ptr->saveSettings();
}

bool QtResourceView::event(QEvent *event)
{
    if (event->type() == QEvent::Show) {
        d_ptr->m_listWidget->scrollToItem(d_ptr->m_listWidget->currentItem());
        d_ptr->m_treeWidget->scrollToItem(d_ptr->m_treeWidget->currentItem());
    }
    return QWidget::event(event);
}

QtResourceModel *QtResourceView::model() const
{
    return d_ptr->m_resourceModel;
}

QString QtResourceView::selectedResource() const
{
    QListWidgetItem *item = d_ptr->m_listWidget->currentItem();
    return d_ptr->m_itemToResource.value(item);
}

void QtResourceView::selectResource(const QString &resource)
{
    if (resource.isEmpty())
        return;
    QFileInfo fi(resource);
    QDir dir = fi.absoluteDir();
    if (fi.isDir())
        dir = QDir(resource);
    QString dirPath = dir.absolutePath();
    const auto cend = d_ptr->m_pathToItem.constEnd();
    auto it = cend;
    while ((it = d_ptr->m_pathToItem.constFind(dirPath)) == cend) {
        if (!dir.cdUp())
            break;
        dirPath = dir.absolutePath();
    }
    if (it != cend) {
        QTreeWidgetItem *treeItem = it.value();
        d_ptr->m_treeWidget->setCurrentItem(treeItem);
        d_ptr->m_treeWidget->scrollToItem(treeItem);
        // expand all up to current one is done by qt
        // list widget is already propagated (currrent changed was sent by qt)
        QListWidgetItem *item = d_ptr->m_resourceToItem.value(resource);
        if (item) {
            d_ptr->m_listWidget->setCurrentItem(item);
            d_ptr->m_listWidget->scrollToItem(item);
        }
    }
}

QString QtResourceView::settingsKey() const
{
    return d_ptr->m_settingsKey;
}

void QtResourceView::setSettingsKey(const QString &key)
{
    if (d_ptr->m_settingsKey == key)
        return;

    d_ptr->m_settingsKey = key;

    if (key.isEmpty())
        return;

    d_ptr->restoreSettings();
}

void QtResourceView::setResourceModel(QtResourceModel *model)
{
    if (d_ptr->m_resourceModel) {
        disconnect(d_ptr->m_resourceModel, SIGNAL(resourceSetActivated(QtResourceSet*,bool)),
                    this, SLOT(slotResourceSetActivated(QtResourceSet*)));
    }

    // clear here
    d_ptr->m_treeWidget->clear();
    d_ptr->m_listWidget->clear();

    d_ptr->m_resourceModel = model;

    if (!d_ptr->m_resourceModel)
        return;

    connect(d_ptr->m_resourceModel, SIGNAL(resourceSetActivated(QtResourceSet*,bool)),
            this, SLOT(slotResourceSetActivated(QtResourceSet*)));

    // fill new here
    d_ptr->slotResourceSetActivated(d_ptr->m_resourceModel->currentResourceSet());
}

bool QtResourceView::isResourceEditingEnabled() const
{
    return d_ptr->m_resourceEditingEnabled;
}

void QtResourceView::setResourceEditingEnabled(bool enable)
{
    d_ptr->m_resourceEditingEnabled = enable;
    d_ptr->updateActions();
}

void QtResourceView::setDragEnabled(bool dragEnabled)
{
    d_ptr->m_listWidget->setDragEnabled(dragEnabled);
}

bool QtResourceView::dragEnabled() const
{
    return d_ptr->m_listWidget->dragEnabled();
}

QString QtResourceView::encodeMimeData(ResourceType resourceType, const QString &path)
{
    QDomDocument doc;
    QDomElement elem = doc.createElement(QLatin1String(elementResourceData));
    switch (resourceType) {
    case ResourceImage:
        elem.setAttribute(QLatin1String(typeAttribute), QLatin1String(typeImage));
        break;
    case ResourceStyleSheet:
        elem.setAttribute(QLatin1String(typeAttribute), QLatin1String(typeStyleSheet));
        break;
    case ResourceOther:
        elem.setAttribute(QLatin1String(typeAttribute), QLatin1String(typeOther));
        break;
    }
    elem.setAttribute(QLatin1String(fileAttribute), path);
    doc.appendChild(elem);
    return doc.toString();
}

bool QtResourceView::decodeMimeData(const QMimeData *md, ResourceType *t, QString *file)
{
    return md->hasText() ? decodeMimeData(md->text(), t, file) : false;
}

bool QtResourceView::decodeMimeData(const QString &text, ResourceType *t, QString *file)
{

    const QString docElementName = QLatin1String(elementResourceData);
    static const QString docElementString = QLatin1Char('<') + docElementName;

    if (text.isEmpty() || text.indexOf(docElementString) == -1)
        return false;

    QDomDocument doc;
    if (!doc.setContent(text))
        return false;

    const QDomElement domElement = doc.documentElement();
    if (domElement.tagName() != docElementName)
        return false;

    if (t) {
        const QString typeAttr = QLatin1String(typeAttribute);
        if (domElement.hasAttribute (typeAttr)) {
            const QString typeValue = domElement.attribute(typeAttr, QLatin1String(typeOther));
            if (typeValue == QLatin1String(typeImage)) {
                *t = ResourceImage;
            } else {
                *t = typeValue == QLatin1String(typeStyleSheet) ? ResourceStyleSheet : ResourceOther;
            }
        }
    }
    if (file) {
        const QString fileAttr = QLatin1String(fileAttribute);
        if (domElement.hasAttribute(fileAttr)) {
            *file = domElement.attribute(fileAttr, QString());
        } else {
            file->clear();
        }
    }
    return true;
}

// ---------------------------- QtResourceViewDialogPrivate

class QtResourceViewDialogPrivate
{
    QtResourceViewDialog *q_ptr;
    Q_DECLARE_PUBLIC(QtResourceViewDialog)
public:
    QtResourceViewDialogPrivate(QDesignerFormEditorInterface *core);

    void slotResourceSelected(const QString &resource) { setOkButtonEnabled(!resource.isEmpty()); }
    void setOkButtonEnabled(bool v)                    { m_box->button(QDialogButtonBox::Ok)->setEnabled(v); }

    QDesignerFormEditorInterface *m_core;
    QtResourceView *m_view;
    QDialogButtonBox *m_box;
};

QtResourceViewDialogPrivate::QtResourceViewDialogPrivate(QDesignerFormEditorInterface *core) :
    q_ptr(0),
    m_core(core),
    m_view(new QtResourceView(core)),
    m_box(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel))
{
    m_view->setSettingsKey(QLatin1String(ResourceViewDialogC));
}

// ------------ QtResourceViewDialog
QtResourceViewDialog::QtResourceViewDialog(QDesignerFormEditorInterface *core, QWidget *parent) :
    QDialog(parent),
    d_ptr(new QtResourceViewDialogPrivate(core))
{
    setWindowTitle(tr("Select Resource"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    d_ptr->q_ptr = this;
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(d_ptr->m_view);
    layout->addWidget(d_ptr->m_box);
    connect(d_ptr->m_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(d_ptr->m_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(d_ptr->m_view, &QtResourceView::resourceActivated, this, &QDialog::accept);
    connect(d_ptr->m_view, SIGNAL(resourceSelected(QString)), this, SLOT(slotResourceSelected(QString)));
    d_ptr->setOkButtonEnabled(false);
    d_ptr->m_view->setResourceModel(core->resourceModel());

    QDesignerSettingsInterface *settings = core->settingsManager();
    settings->beginGroup(QLatin1String(ResourceViewDialogC));

    const QVariant geometry = settings->value(QLatin1String(Geometry));
    if (geometry.type() == QVariant::ByteArray) // Used to be a QRect up until 5.4.0, QTBUG-43374.
        restoreGeometry(geometry.toByteArray());

    settings->endGroup();
}

QtResourceViewDialog::~QtResourceViewDialog()
{
    QDesignerSettingsInterface *settings = d_ptr->m_core->settingsManager();
    settings->beginGroup(QLatin1String(ResourceViewDialogC));

    settings->setValue(QLatin1String(Geometry), saveGeometry());

    settings->endGroup();
}

QString QtResourceViewDialog::selectedResource() const
{
    return d_ptr->m_view->selectedResource();
}

void QtResourceViewDialog::selectResource(const QString &path)
{
    d_ptr->m_view->selectResource(path);
}

bool QtResourceViewDialog::isResourceEditingEnabled() const
{
    return d_ptr->m_view->isResourceEditingEnabled();
}

void QtResourceViewDialog::setResourceEditingEnabled(bool enable)
{
    d_ptr->m_view->setResourceEditingEnabled(enable);
}

QT_END_NAMESPACE

#include "moc_qtresourceview_p.cpp"
