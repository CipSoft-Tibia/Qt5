// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "objectinspector.h"
#include "objectinspectormodel_p.h"
#include "formwindow.h"

// sdk
#include <QtDesigner/abstractformeditor.h>
#include <QtDesigner/taskmenu.h>
#include <QtDesigner/qextensionmanager.h>
#include <QtDesigner/abstractformwindowcursor.h>
#include <QtDesigner/abstractformwindowmanager.h>
#include <QtDesigner/container.h>
#include <QtDesigner/abstractmetadatabase.h>
#include <QtDesigner/abstractpropertyeditor.h>

// shared
#include <qdesigner_utils_p.h>
#include <formwindowbase_p.h>
#include <qdesigner_dnditem_p.h>
#include <textpropertyeditor_p.h>
#include <qdesigner_command_p.h>
#include <grid_p.h>

// Qt
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qheaderview.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qscrollbar.h>
#include <QtGui/qpainter.h>
#include <QtWidgets/qboxlayout.h>
#include <QtCore/qitemselectionmodel.h>
#include <QtWidgets/qmenu.h>
#include <QtWidgets/qtreeview.h>
#include <QtWidgets/qstyleditemdelegate.h>
#include <QtGui/qevent.h>

#include <QtCore/qdebug.h>
#include <QtCore/qlist.h>
#include <QtCore/qsortfilterproxymodel.h>

QT_BEGIN_NAMESPACE

namespace {
    // Selections: Basically, ObjectInspector has to ensure a consistent
    // selection, that is, either form-managed widgets (represented
    // by the cursor interface selection), or unmanaged widgets/objects,
    // for example actions, container pages, menu bars, tool bars
    // and the like. The selection state of the latter is managed only in the object inspector.
    // As soon as a managed widget is selected, unmanaged objects
    // have to be unselected
    // Normally, an empty selection is not allowed, the main container
    // should be selected in this case (applyCursorSelection()).
    // An exception is when clearSelection is called directly for example
    // by the action editor that puts an unassociated action into the property
    // editor. A hack exists to avoid the update in this case.

    enum SelectionType {
        NoSelection,
        // A QObject that has a meta database entry
        QObjectSelection,
        // Unmanaged widget, menu bar or the like
        UnmanagedWidgetSelection,
        // A widget managed by the form window cursor
        ManagedWidgetSelection };
}

static inline SelectionType selectionType(const QDesignerFormWindowInterface *fw, QObject *o)
{
    if (!o->isWidgetType())
        return fw->core()->metaDataBase()->item(o) ?  QObjectSelection : NoSelection;
    return fw->isManaged(qobject_cast<QWidget *>(o)) ? ManagedWidgetSelection :  UnmanagedWidgetSelection;
}

// Return an offset for dropping (when dropping widgets on the object
// inspector, we fake a position on the form based on the widget dropped on).
// Position the dropped widget with form grid offset to avoid overlapping unless we
// drop on a layout. Position doesn't matter in the layout case
// and this enables us to drop on a squeezed layout widget of size zero

static inline QPoint dropPointOffset(const qdesigner_internal::FormWindowBase *fw, const QWidget *dropTarget)
{
    if (!dropTarget || dropTarget->layout())
        return QPoint(0, 0);
    return QPoint(fw->designerGrid().deltaX(), fw->designerGrid().deltaY());
}

namespace qdesigner_internal {
// Delegate with object name validator for the object name column
class ObjectInspectorDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

QWidget *ObjectInspectorDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & option, const QModelIndex &index) const
{
    if (index.column() != ObjectInspectorModel::ObjectNameColumn)
        return QStyledItemDelegate::createEditor(parent, option, index);
    // Object name editor
    const bool isMainContainer = !index.parent().isValid();
    return new TextPropertyEditor(parent, TextPropertyEditor::EmbeddingTreeView,
                                  isMainContainer ? ValidationObjectNameScope : ValidationObjectName);
}

// ------------ ObjectInspectorTreeView:
// - Makes the Space key start editing
// - Suppresses a range selection by dragging or Shift-up/down, which does not really work due
//   to the need to maintain a consistent selection.

class ObjectInspectorTreeView : public QTreeView {
public:
    using QTreeView::QTreeView;

protected:
    void mouseMoveEvent (QMouseEvent * event) override;
    void keyPressEvent(QKeyEvent *event) override;

};

void ObjectInspectorTreeView::mouseMoveEvent(QMouseEvent *event)
{
    event->ignore(); // suppress a range selection by dragging
}

void ObjectInspectorTreeView::keyPressEvent(QKeyEvent *event)
{
    bool handled = false;
    switch (event->key()) {
    case Qt::Key_Up:
    case Qt::Key_Down: // suppress shift-up/down range selection
        if (event->modifiers() & Qt::ShiftModifier) {
            event->ignore();
            handled = true;
        }
        break;
    case Qt::Key_Space: { // Space pressed: Start editing
        const QModelIndex index = currentIndex();
        if (index.isValid() && index.column() == 0 && !model()->hasChildren(index) && model()->flags(index) & Qt::ItemIsEditable) {
            event->accept();
            handled = true;
            edit(index);
        }
    }
        break;
    default:
        break;
    }
    if (!handled)
        QTreeView::keyPressEvent(event);
}

// ------------ ObjectInspectorPrivate

class ObjectInspector::ObjectInspectorPrivate {
    Q_DISABLE_COPY_MOVE(ObjectInspectorPrivate)
public:
    ObjectInspectorPrivate(QDesignerFormEditorInterface *core);
    ~ObjectInspectorPrivate();

    QLineEdit *filterLineEdit() const { return m_filterLineEdit; }
    QTreeView *treeView() const { return m_treeView; }
    QDesignerFormEditorInterface *core() const { return m_core; }
    const QPointer<FormWindowBase> &formWindow() const { return m_formWindow; }

    void clear();
    void setFormWindow(QDesignerFormWindowInterface *fwi);

    QWidget *managedWidgetAt(const QPoint &global_mouse_pos);

    void restoreDropHighlighting();
    void handleDragEnterMoveEvent(const QWidget *objectInspectorWidget, QDragMoveEvent * event, bool isDragEnter);
    void dropEvent (QDropEvent * event);

    void clearSelection();
    bool selectObject(QObject *o);
    void slotSelectionChanged(const QItemSelection & selected, const QItemSelection &deselected);
    void getSelection(Selection &s) const;

    QModelIndexList indexesOf(QObject *o) const;
    QObject *objectAt(const QModelIndex &index) const;
    QObjectList indexesToObjects(const QModelIndexList &indexes) const;

    void slotHeaderDoubleClicked(int column)       {  m_treeView->resizeColumnToContents(column); }
    void slotPopupContextMenu(QWidget *parent, const QPoint &pos);

private:
    void setFormWindowBlocked(QDesignerFormWindowInterface *fwi);
    void applyCursorSelection();
    void synchronizeSelection(const QItemSelection & selected, const QItemSelection &deselected);
    bool checkManagedWidgetSelection(const QModelIndexList &selection);
    void showContainersCurrentPage(QWidget *widget);

    enum  SelectionFlags { AddToSelection = 1, MakeCurrent = 2};
    void selectIndexRange(const QModelIndexList &indexes, unsigned flags);

    QDesignerFormEditorInterface *m_core;
    QLineEdit *m_filterLineEdit;
    QTreeView *m_treeView;
    ObjectInspectorModel *m_model;
    QSortFilterProxyModel *m_filterModel;
    QPointer<FormWindowBase> m_formWindow;
    QPointer<QWidget> m_formFakeDropTarget;
    bool m_withinClearSelection;
};

ObjectInspector::ObjectInspectorPrivate::ObjectInspectorPrivate(QDesignerFormEditorInterface *core) :
    m_core(core),
    m_filterLineEdit(new QLineEdit),
    m_treeView(new ObjectInspectorTreeView),
    m_model(new ObjectInspectorModel(m_treeView)),
    m_filterModel(new QSortFilterProxyModel(m_treeView)),
    m_withinClearSelection(false)
{
    m_filterModel->setRecursiveFilteringEnabled(true);
    m_filterLineEdit->setPlaceholderText(ObjectInspector::tr("Filter"));
    m_filterLineEdit->setClearButtonEnabled(true);
    connect(m_filterLineEdit, &QLineEdit::textChanged,
            m_filterModel, &QSortFilterProxyModel::setFilterFixedString);
    // Filtering text collapses nodes, expand on clear.
    connect(m_filterLineEdit, &QLineEdit::textChanged,
            m_core, [this] (const QString &text) {
                if (text.isEmpty())
                    this->m_treeView->expandAll();
            });
    m_filterModel->setSourceModel(m_model);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_treeView->setModel(m_filterModel);
    m_treeView->setSortingEnabled(true);
    m_treeView->sortByColumn(0, Qt::AscendingOrder);
    m_treeView->setItemDelegate(new ObjectInspectorDelegate);
    m_treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_treeView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setTextElideMode (Qt::ElideMiddle);

    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
}

ObjectInspector::ObjectInspectorPrivate::~ObjectInspectorPrivate()
{
    delete m_treeView->itemDelegate();
}

void ObjectInspector::ObjectInspectorPrivate::clearSelection()
{
    m_withinClearSelection = true;
    m_treeView->clearSelection();
    m_withinClearSelection = false;
}

QWidget *ObjectInspector::ObjectInspectorPrivate::managedWidgetAt(const QPoint &global_mouse_pos)
{
    if (!m_formWindow)
        return nullptr;

    const  QPoint pos = m_treeView->viewport()->mapFromGlobal(global_mouse_pos);
    QObject *o = objectAt(m_treeView->indexAt(pos));

    if (!o || !o->isWidgetType())
        return nullptr;

    QWidget *rc = qobject_cast<QWidget *>(o);
    if (!m_formWindow->isManaged(rc))
        return nullptr;
    return rc;
}

void ObjectInspector::ObjectInspectorPrivate::showContainersCurrentPage(QWidget *widget)
{
    if (!widget)
        return;

    FormWindow *fw = FormWindow::findFormWindow(widget);
    if (!fw)
        return;

    QWidget *w = widget->parentWidget();
    bool macroStarted = false;
    // Find a multipage container (tab widgets, etc.) in the hierarchy and set the right page.
    while (w != nullptr) {
        if (fw->isManaged(w) && !qobject_cast<QMainWindow *>(w)) { // Rule out unmanaged internal scroll areas, for example, on QToolBoxes.
            if (QDesignerContainerExtension *c = qt_extension<QDesignerContainerExtension*>(m_core->extensionManager(), w)) {
                const int count = c->count();
                if (count > 1 && !c->widget(c->currentIndex())->isAncestorOf(widget)) {
                    for (int i = 0; i < count; i++)
                        if (c->widget(i)->isAncestorOf(widget)) {
                            if (!macroStarted) {
                                macroStarted = true;
                                fw->beginCommand(tr("Change Current Page"));
                            }
                            ChangeCurrentPageCommand *cmd = new ChangeCurrentPageCommand(fw);
                            cmd->init(w, i);
                            fw->commandHistory()->push(cmd);
                            break;
                        }
                }
            }
        }
        w = w->parentWidget();
    }
    if (macroStarted)
        fw->endCommand();
}

void ObjectInspector::ObjectInspectorPrivate::restoreDropHighlighting()
{
    if (m_formFakeDropTarget) {
        if (m_formWindow) {
            m_formWindow->highlightWidget(m_formFakeDropTarget, QPoint(5, 5), FormWindow::Restore);
        }
        m_formFakeDropTarget = nullptr;
    }
}

void ObjectInspector::ObjectInspectorPrivate::handleDragEnterMoveEvent(const QWidget *objectInspectorWidget, QDragMoveEvent * event, bool isDragEnter)
{
    if (!m_formWindow) {
        event->ignore();
        return;
    }

    const QDesignerMimeData *mimeData =  qobject_cast<const QDesignerMimeData *>(event->mimeData());
    if (!mimeData) {
        event->ignore();
        return;
    }

    QWidget *dropTarget = nullptr;
    QPoint fakeDropTargetOffset = QPoint(0, 0);
    if (QWidget *managedWidget = managedWidgetAt(objectInspectorWidget->mapToGlobal(event->position().toPoint()))) {
        fakeDropTargetOffset = dropPointOffset(m_formWindow, managedWidget);
        // pretend we drag over the managed widget on the form
        const QPoint fakeFormPos = m_formWindow->mapFromGlobal(managedWidget->mapToGlobal(fakeDropTargetOffset));
        const FormWindowBase::WidgetUnderMouseMode wum = mimeData->items().size() == 1 ? FormWindowBase::FindSingleSelectionDropTarget : FormWindowBase::FindMultiSelectionDropTarget;
        dropTarget = m_formWindow->widgetUnderMouse(fakeFormPos, wum);
    }

    if (m_formFakeDropTarget && dropTarget != m_formFakeDropTarget)
        m_formWindow->highlightWidget(m_formFakeDropTarget, fakeDropTargetOffset, FormWindow::Restore);

    m_formFakeDropTarget =  dropTarget;
    if (m_formFakeDropTarget)
        m_formWindow->highlightWidget(m_formFakeDropTarget, fakeDropTargetOffset, FormWindow::Highlight);

    // Do not refuse drag enter even if the area is not droppable
    if (isDragEnter || m_formFakeDropTarget)
        mimeData->acceptEvent(event);
    else
        event->ignore();
}
void  ObjectInspector::ObjectInspectorPrivate::dropEvent (QDropEvent * event)
{
    if (!m_formWindow || !m_formFakeDropTarget) {
        event->ignore();
        return;
    }

    const QDesignerMimeData *mimeData =  qobject_cast<const QDesignerMimeData *>(event->mimeData());
    if (!mimeData) {
        event->ignore();
        return;
    }
    const QPoint fakeGlobalDropFormPos = m_formFakeDropTarget->mapToGlobal(dropPointOffset(m_formWindow , m_formFakeDropTarget));
    mimeData->moveDecoration(fakeGlobalDropFormPos + mimeData->hotSpot());
    if (!m_formWindow->dropWidgets(mimeData->items(), m_formFakeDropTarget, fakeGlobalDropFormPos)) {
        event->ignore();
        return;
    }
    mimeData->acceptEvent(event);
}

QModelIndexList ObjectInspector::ObjectInspectorPrivate::indexesOf(QObject *o) const
{
    QModelIndexList result;
    const auto srcIndexes = m_model->indexesOf(o);
    if (!srcIndexes.isEmpty()) {
        result.reserve(srcIndexes.size());
        for (const auto &srcIndex : srcIndexes)
            result.append(m_filterModel->mapFromSource(srcIndex));
    }
    return result;
}

QObject *ObjectInspector::ObjectInspectorPrivate::objectAt(const QModelIndex &index) const
{
    return m_model->objectAt(m_filterModel->mapToSource(index));
}

bool ObjectInspector::ObjectInspectorPrivate::selectObject(QObject *o)
{
    if (!m_core->metaDataBase()->item(o))
        return false;

    using ModelIndexSet = QSet<QModelIndex>;

    const QModelIndexList objectIndexes = indexesOf(o);
    if (objectIndexes.isEmpty())
        return false;

    QItemSelectionModel *selectionModel = m_treeView->selectionModel();
    const auto currentSelectedItemList = selectionModel->selectedRows(0);
    const ModelIndexSet currentSelectedItems(currentSelectedItemList.cbegin(), currentSelectedItemList.cend());

    // Change in selection?
    if (!currentSelectedItems.isEmpty()
        && currentSelectedItems == ModelIndexSet(objectIndexes.cbegin(), objectIndexes.cend())) {
        return true;
    }

    // do select and update
    selectIndexRange(objectIndexes, MakeCurrent);
    return true;
}

void ObjectInspector::ObjectInspectorPrivate::selectIndexRange(const QModelIndexList &indexes, unsigned flags)
{
    if (indexes.isEmpty())
        return;

    QItemSelectionModel::SelectionFlags selectFlags = QItemSelectionModel::Select|QItemSelectionModel::Rows;
    if (!(flags & AddToSelection))
        selectFlags |= QItemSelectionModel::Clear;
    if (flags & MakeCurrent)
        selectFlags |= QItemSelectionModel::Current;

    QItemSelectionModel *selectionModel = m_treeView->selectionModel();
    for (const auto &mi : indexes) {
        if (mi.column() == 0) {
            selectionModel->select(mi, selectFlags);
            selectFlags &= ~(QItemSelectionModel::Clear|QItemSelectionModel::Current);
        }
    }
    if (flags & MakeCurrent)
        m_treeView->scrollTo(indexes.constFirst(), QAbstractItemView::EnsureVisible);
}

void ObjectInspector::ObjectInspectorPrivate::clear()
{
    m_formFakeDropTarget = nullptr;
    m_formWindow = nullptr;
}

// Form window cursor is in state 'main container only'
static inline bool mainContainerIsCurrent(const QDesignerFormWindowInterface *fw)
{
    const QDesignerFormWindowCursorInterface *cursor = fw->cursor();
    if (cursor->selectedWidgetCount() > 1)
        return false;
    const QWidget *current = cursor->current();
    return current == fw || current == fw->mainContainer();
}

void ObjectInspector::ObjectInspectorPrivate::setFormWindow(QDesignerFormWindowInterface *fwi)
{
    const bool blocked = m_treeView->selectionModel()->blockSignals(true);
    {
        UpdateBlocker ub(m_treeView);
        setFormWindowBlocked(fwi);
    }

    m_treeView->update();
    m_treeView->selectionModel()->blockSignals(blocked);
}

void ObjectInspector::ObjectInspectorPrivate::setFormWindowBlocked(QDesignerFormWindowInterface *fwi)
{
    FormWindowBase *fw = qobject_cast<FormWindowBase *>(fwi);
    const bool formWindowChanged = m_formWindow != fw;

    m_formWindow = fw;

    const int oldWidth = m_treeView->columnWidth(0);
    const int xoffset = m_treeView->horizontalScrollBar()->value();
    const int yoffset = m_treeView->verticalScrollBar()->value();

    if (formWindowChanged)
        m_formFakeDropTarget = nullptr;

    switch (m_model->update(m_formWindow)) {
    case ObjectInspectorModel::NoForm:
        clear();
        return;
    case ObjectInspectorModel::Rebuilt: // Complete rebuild: Just apply cursor selection
        applyCursorSelection();
        m_treeView->expandAll();
        if (formWindowChanged) {
            m_treeView->resizeColumnToContents(0);
        } else {
            m_treeView->setColumnWidth(0, oldWidth);
            m_treeView->horizontalScrollBar()->setValue(xoffset);
            m_treeView->verticalScrollBar()->setValue(yoffset);
        }
        break;
    case ObjectInspectorModel::Updated: {
        // Same structure (property changed or click on the form)
        // We maintain a selection of unmanaged objects
        // only if the cursor is in state "mainContainer() == current".
        // and we have a non-managed selection.
        // Else we take over the cursor selection.
        bool applySelection = !mainContainerIsCurrent(m_formWindow);
        if (!applySelection) {
            const QModelIndexList currentIndexes = m_treeView->selectionModel()->selectedRows(0);
            if (currentIndexes.isEmpty()) {
                applySelection = true;
            } else {
                applySelection = selectionType(m_formWindow, objectAt(currentIndexes.constFirst())) == ManagedWidgetSelection;
            }
        }
        if (applySelection)
            applyCursorSelection();
    }
        break;
    }
}

// Apply selection of form window cursor to object inspector, set current
void ObjectInspector::ObjectInspectorPrivate::applyCursorSelection()
{
    const QDesignerFormWindowCursorInterface *cursor = m_formWindow->cursor();
    const int count = cursor->selectedWidgetCount();
    if (!count)
        return;

    // Set the current widget first which also clears the selection
    QWidget *currentWidget = cursor->current();
    if (currentWidget)
        selectIndexRange(indexesOf(currentWidget), MakeCurrent);
    else
        m_treeView->selectionModel()->clearSelection();

    for (int i = 0;i < count; i++) {
        QWidget *widget = cursor->selectedWidget(i);
        if (widget != currentWidget)
            selectIndexRange(indexesOf(widget), AddToSelection);
    }
}

// Synchronize managed widget in the form (select in cursor). Block updates
static int selectInCursor(FormWindowBase *fw, const QObjectList &objects, bool value)
{
    int rc = 0;
    const bool blocked = fw->blockSelectionChanged(true);
    for (auto *o : objects) {
        if (selectionType(fw, o) == ManagedWidgetSelection) {
            fw->selectWidget(static_cast<QWidget *>(o), value);
            rc++;
        }
    }
    fw->blockSelectionChanged(blocked);
    return rc;
}

void ObjectInspector::ObjectInspectorPrivate::slotSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    if (m_formWindow) {
        synchronizeSelection(selected, deselected);
        QMetaObject::invokeMethod(m_core->formWindowManager(), "slotUpdateActions");
    }
}

// Convert indexes to object vectors taking into account that
// some index lists are multicolumn ranges
QObjectList ObjectInspector::ObjectInspectorPrivate::indexesToObjects(const QModelIndexList &indexes) const
{
    QObjectList rc;
    if (indexes.isEmpty())
        return rc;
    rc.reserve(indexes.size());
    for (const auto &mi : indexes) {
        if (mi.column() == 0)
            rc.append(objectAt(mi));
    }
    return rc;
}

// Check if any managed widgets are selected. If so, iterate over
// selection and deselect all unmanaged objects
bool ObjectInspector::ObjectInspectorPrivate::checkManagedWidgetSelection(const QModelIndexList &rowSelection)
{
    bool isManagedWidgetSelection = false;
    QItemSelectionModel *selectionModel = m_treeView->selectionModel();
    for (const auto &mi : rowSelection) {
        QObject *object = objectAt(mi);
        if (selectionType(m_formWindow, object) == ManagedWidgetSelection) {
            isManagedWidgetSelection = true;
            break;
        }
    }

    if (!isManagedWidgetSelection)
        return false;
    // Need to unselect unmanaged ones
    const bool blocked = selectionModel->blockSignals(true);
    for (const auto &mi : rowSelection) {
        QObject *object = objectAt(mi);
        if (selectionType(m_formWindow, object) != ManagedWidgetSelection)
            selectionModel->select(mi, QItemSelectionModel::Deselect|QItemSelectionModel::Rows);
    }
    selectionModel->blockSignals(blocked);
    return true;
}

void ObjectInspector::ObjectInspectorPrivate::synchronizeSelection(const QItemSelection & selectedSelection, const QItemSelection &deselectedSelection)
{
    // Synchronize form window cursor.
    const QObjectList deselected = indexesToObjects(deselectedSelection.indexes());
    const QObjectList newlySelected = indexesToObjects(selectedSelection.indexes());

    const QModelIndexList currentSelectedIndexes = m_treeView->selectionModel()->selectedRows(0);

    int deselectedManagedWidgetCount = 0;
    if (!deselected.isEmpty())
        deselectedManagedWidgetCount = selectInCursor(m_formWindow, deselected, false);

    if (newlySelected.isEmpty()) { // Nothing selected
        if (currentSelectedIndexes.isEmpty()) // Do not allow a null-selection, reset to main container
            m_formWindow->clearSelection(!m_withinClearSelection);
        return;
    }

    const int selectManagedWidgetCount = selectInCursor(m_formWindow, newlySelected, true);
    // Check consistency: Make sure either  managed widgets or  unmanaged objects are selected.
    // No newly-selected managed widgets: Unless there are ones in the (old) current selection,
    // select the unmanaged object
    if (selectManagedWidgetCount == 0) {
        if (checkManagedWidgetSelection(currentSelectedIndexes)) {
            // Managed selection exists, refuse and update if necessary
            if (deselectedManagedWidgetCount != 0 || selectManagedWidgetCount != 0)
                m_formWindow->emitSelectionChanged();
            return;
        }
        // And now for the unmanaged selection
        m_formWindow->clearSelection(false);
        QObject *unmanagedObject = newlySelected.constFirst();
        m_core->propertyEditor()->setObject(unmanagedObject);
        m_core->propertyEditor()->setEnabled(true);
        // open container page if it is a single widget
        if (newlySelected.size() == 1 && unmanagedObject->isWidgetType())
            showContainersCurrentPage(static_cast<QWidget*>(unmanagedObject));
        return;
    }
    // Open container page if it is a single widget
    if (newlySelected.size() == 1) {
        QObject *object = newlySelected.constFirst();
        if (object->isWidgetType())
            showContainersCurrentPage(static_cast<QWidget*>(object));
    }

    // A managed widget was newly selected. Make sure  there are no unmanaged objects
    // in the whole unless just single selection
    if (currentSelectedIndexes.size() > selectManagedWidgetCount)
        checkManagedWidgetSelection(currentSelectedIndexes);
    // Update form
    if (deselectedManagedWidgetCount != 0 || selectManagedWidgetCount != 0)
        m_formWindow->emitSelectionChanged();
}


void ObjectInspector::ObjectInspectorPrivate::getSelection(Selection &s) const
{
    s.clear();

    if (!m_formWindow)
        return;

    const QModelIndexList currentSelectedIndexes = m_treeView->selectionModel()->selectedRows(0);
    if (currentSelectedIndexes.isEmpty())
        return;

    // sort objects
    for (const QModelIndex &index : currentSelectedIndexes) {
        if (QObject *object = objectAt(index)) {
            switch (selectionType(m_formWindow, object)) {
            case NoSelection:
                break;
            case QObjectSelection:
                // It is actually possible to select an action twice if it is in a menu bar
                // and in a tool bar.
                if (!s.objects.contains(object))
                    s.objects.push_back(object);
                break;
            case UnmanagedWidgetSelection:
                s.unmanaged.push_back(qobject_cast<QWidget *>(object));
                break;
            case ManagedWidgetSelection:
                s.managed.push_back(qobject_cast<QWidget *>(object));
                break;
            }
        }
    }
}

// Utility to create a task menu
static inline QMenu *createTaskMenu(QObject *object, QDesignerFormWindowInterface *fw)
{
    // 1) Objects
    if (!object->isWidgetType())
        return FormWindowBase::createExtensionTaskMenu(fw, object, false);
    // 2) Unmanaged widgets
    QWidget *w = static_cast<QWidget *>(object);
    if (!fw->isManaged(w))
        return FormWindowBase::createExtensionTaskMenu(fw, w, false);
    // 3) Mananaged widgets
    if (qdesigner_internal::FormWindowBase *fwb = qobject_cast<qdesigner_internal::FormWindowBase*>(fw))
        return fwb->initializePopupMenu(w);
    return nullptr;
}

void ObjectInspector::ObjectInspectorPrivate::slotPopupContextMenu(QWidget * /*parent*/, const QPoint &pos)
{
    if (m_formWindow == nullptr || m_formWindow->currentTool() != 0)
        return;

    if (QObject *object = objectAt(m_treeView->indexAt(pos))) {
        if (QMenu *menu = createTaskMenu(object, m_formWindow)) {
            menu->exec(m_treeView->viewport()->mapToGlobal(pos));
            delete menu;
        }
    }
}

// ------------ ObjectInspector
ObjectInspector::ObjectInspector(QDesignerFormEditorInterface *core, QWidget *parent) :
    QDesignerObjectInspector(parent),
    m_impl(new ObjectInspectorPrivate(core))
{
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(QMargins());

    vbox->addWidget(m_impl->filterLineEdit());
    QTreeView *treeView = m_impl->treeView();
    vbox->addWidget(treeView);

    connect(treeView, &QWidget::customContextMenuRequested,
            this, &ObjectInspector::slotPopupContextMenu);

    connect(treeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &ObjectInspector::slotSelectionChanged);

    connect(treeView->header(), &QHeaderView::sectionDoubleClicked,
            this, &ObjectInspector::slotHeaderDoubleClicked);
    setAcceptDrops(true);
}

ObjectInspector::~ObjectInspector()
{
    delete m_impl;
}

QDesignerFormEditorInterface *ObjectInspector::core() const
{
    return m_impl->core();
}

void ObjectInspector::slotPopupContextMenu(const QPoint &pos)
{
    m_impl->slotPopupContextMenu(this, pos);
}

void ObjectInspector::setFormWindow(QDesignerFormWindowInterface *fwi)
{
    m_impl->setFormWindow(fwi);
}

void ObjectInspector::slotSelectionChanged(const QItemSelection & selected, const QItemSelection &deselected)
{
    m_impl->slotSelectionChanged(selected, deselected);
}

void ObjectInspector::getSelection(Selection &s) const
{
    m_impl->getSelection(s);
}

bool ObjectInspector::selectObject(QObject *o)
{
   return m_impl->selectObject(o);
}

void ObjectInspector::clearSelection()
{
    m_impl->clearSelection();
}

void ObjectInspector::slotHeaderDoubleClicked(int column)
{
    m_impl->slotHeaderDoubleClicked(column);
}

void ObjectInspector::mainContainerChanged()
{
    // Invalidate references to objects kept in items
    if (sender() == m_impl->formWindow())
        setFormWindow(nullptr);
}

void  ObjectInspector::dragEnterEvent (QDragEnterEvent * event)
{
    m_impl->handleDragEnterMoveEvent(this, event, true);
}

void  ObjectInspector::dragMoveEvent(QDragMoveEvent * event)
{
    m_impl->handleDragEnterMoveEvent(this, event, false);
}

void  ObjectInspector::dragLeaveEvent(QDragLeaveEvent * /* event*/)
{
    m_impl->restoreDropHighlighting();
}

void  ObjectInspector::dropEvent (QDropEvent * event)
{
    m_impl->dropEvent(event);

QT_END_NAMESPACE
}
}
