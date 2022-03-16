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

#include "qdesigner.h"
#include "qdesigner_toolwindow.h"
#include "qdesigner_settings.h"
#include "qdesigner_workbench.h"

#include <QtDesigner/abstractpropertyeditor.h>
#include <QtDesigner/abstractformeditor.h>
#include <QtDesigner/abstractactioneditor.h>
#include <QtDesigner/abstractobjectinspector.h>
#include <QtDesigner/abstractwidgetbox.h>
#include <QtDesigner/QDesignerComponents>

#include <QtCore/qdebug.h>
#include <QtWidgets/qaction.h>
#include <QtGui/qevent.h>

enum { debugToolWindow = 0 };

QT_BEGIN_NAMESPACE

// ---------------- QDesignerToolWindowFontSettings
bool ToolWindowFontSettings::equals(const ToolWindowFontSettings &rhs) const
{
    return m_useFont == rhs.m_useFont &&
           m_writingSystem == rhs.m_writingSystem &&
           m_font == rhs.m_font;
}

// ---------------- QDesignerToolWindow
QDesignerToolWindow::QDesignerToolWindow(QDesignerWorkbench *workbench,
                                         QWidget *w,
                                         const QString &objectName,
                                         const QString &title,
                                         const QString &actionObjectName,
                                         Qt::DockWidgetArea dockAreaHint,
                                         QWidget *parent,
                                         Qt::WindowFlags flags) :
    MainWindowBase(parent, flags),
    m_dockAreaHint(dockAreaHint),
    m_workbench(workbench),
    m_action(new QAction(this))
{
    setObjectName(objectName);
    setCentralWidget(w);

    setWindowTitle(title);

    m_action->setObjectName(actionObjectName);
    m_action->setShortcutContext(Qt::ApplicationShortcut);
    m_action->setText(title);
    m_action->setCheckable(true);
    connect(m_action, &QAction::triggered, this, &QDesignerToolWindow::showMe);
}

void QDesignerToolWindow::showMe(bool v)
{
    // Access the QMdiSubWindow in MDI mode.
    if (QWidget *target = m_workbench->mode() == DockedMode ? parentWidget() : this) {
        if (v)
            target->setWindowState(target->windowState() & ~Qt::WindowMinimized);
        target->setVisible(v);
    }
}

void QDesignerToolWindow::showEvent(QShowEvent *e)
{
    Q_UNUSED(e);

    bool blocked = m_action->blockSignals(true);
    m_action->setChecked(true);
    m_action->blockSignals(blocked);
}

void QDesignerToolWindow::hideEvent(QHideEvent *e)
{
    Q_UNUSED(e);

    bool blocked = m_action->blockSignals(true);
    m_action->setChecked(false);
    m_action->blockSignals(blocked);
}

QAction *QDesignerToolWindow::action() const
{
    return m_action;
}

void QDesignerToolWindow::changeEvent(QEvent *e)
{
    switch (e->type()) {
        case QEvent::WindowTitleChange:
            m_action->setText(windowTitle());
            break;
        case QEvent::WindowIconChange:
            m_action->setIcon(windowIcon());
            break;
        default:
            break;
    }
    QMainWindow::changeEvent(e);
}

QDesignerWorkbench *QDesignerToolWindow::workbench() const
{
    return m_workbench;
}

QRect QDesignerToolWindow::geometryHint() const
{
    return QRect();
}

QRect QDesignerToolWindow::availableToolWindowGeometry() const
{
    return m_workbench->availableGeometry();
}

//  ---------------------- PropertyEditorToolWindow

static inline QWidget *createPropertyEditor(QDesignerFormEditorInterface *core, QWidget *parent = 0)
{
    QDesignerPropertyEditorInterface *widget = QDesignerComponents::createPropertyEditor(core, parent);
    core->setPropertyEditor(widget);
    return widget;
}

class PropertyEditorToolWindow : public QDesignerToolWindow
{
public:
    explicit PropertyEditorToolWindow(QDesignerWorkbench *workbench);

    QRect geometryHint() const override;

protected:
    void showEvent(QShowEvent *event) override;
};

PropertyEditorToolWindow::PropertyEditorToolWindow(QDesignerWorkbench *workbench) :
    QDesignerToolWindow(workbench,
                        createPropertyEditor(workbench->core()),
                        QStringLiteral("qt_designer_propertyeditor"),
                        QDesignerToolWindow::tr("Property Editor"),
                        QStringLiteral("__qt_property_editor_action"),
                        Qt::RightDockWidgetArea)
{
    action()->setShortcut(Qt::CTRL + Qt::Key_I);

}

QRect PropertyEditorToolWindow::geometryHint() const
{
    const QRect g = availableToolWindowGeometry();
    const int margin = workbench()->marginHint();
    const int spacing = 40;
    const QSize sz(g.width() * 1/4, g.height() * 4/6);

    const QRect rc = QRect((g.right() + 1 - sz.width() - margin),
                           (g.top() + margin + g.height() * 1/6) + spacing,
                           sz.width(), sz.height());
    if (debugToolWindow)
        qDebug() << Q_FUNC_INFO << rc;
    return rc;
}

void PropertyEditorToolWindow::showEvent(QShowEvent *event)
{
    if (QDesignerPropertyEditorInterface *e = workbench()->core()->propertyEditor()) {
        // workaround to update the propertyeditor when it is not visible!
        e->setObject(e->object()); // ### remove me
    }

    QDesignerToolWindow::showEvent(event);
}

//  ---------------------- ActionEditorToolWindow

static inline QWidget *createActionEditor(QDesignerFormEditorInterface *core, QWidget *parent = 0)
{
    QDesignerActionEditorInterface *widget = QDesignerComponents::createActionEditor(core, parent);
    core->setActionEditor(widget);
    return widget;
}

class ActionEditorToolWindow: public QDesignerToolWindow
{
public:
    explicit ActionEditorToolWindow(QDesignerWorkbench *workbench);

    QRect geometryHint() const override;
};

ActionEditorToolWindow::ActionEditorToolWindow(QDesignerWorkbench *workbench) :
    QDesignerToolWindow(workbench,
                        createActionEditor(workbench->core()),
                        QStringLiteral("qt_designer_actioneditor"),
                        QDesignerToolWindow::tr("Action Editor"),
                        QStringLiteral("__qt_action_editor_tool_action"),
                        Qt::RightDockWidgetArea)
{
}

QRect ActionEditorToolWindow::geometryHint() const
{
    const QRect g = availableToolWindowGeometry();
    const int margin = workbench()->marginHint();

    const QSize sz(g.width() * 1/4, g.height() * 1/6);

    const QRect rc = QRect((g.right() + 1 - sz.width() - margin),
                            g.top() + margin,
                            sz.width(), sz.height());
    if (debugToolWindow)
        qDebug() << Q_FUNC_INFO << rc;
    return rc;
}

//  ---------------------- ObjectInspectorToolWindow

static inline QWidget *createObjectInspector(QDesignerFormEditorInterface *core, QWidget *parent = 0)
{
    QDesignerObjectInspectorInterface *widget = QDesignerComponents::createObjectInspector(core, parent);
    core->setObjectInspector(widget);
    return widget;
}

class ObjectInspectorToolWindow: public QDesignerToolWindow
{
public:
    explicit ObjectInspectorToolWindow(QDesignerWorkbench *workbench);

    QRect geometryHint() const override;
};

ObjectInspectorToolWindow::ObjectInspectorToolWindow(QDesignerWorkbench *workbench) :
    QDesignerToolWindow(workbench,
                        createObjectInspector(workbench->core()),
                        QStringLiteral("qt_designer_objectinspector"),
                        QDesignerToolWindow::tr("Object Inspector"),
                        QStringLiteral("__qt_object_inspector_tool_action"),
                        Qt::RightDockWidgetArea)
{
}

QRect ObjectInspectorToolWindow::geometryHint() const
{
    const QRect g = availableToolWindowGeometry();
    const int margin = workbench()->marginHint();

    const QSize sz(g.width() * 1/4, g.height() * 1/6);

    const QRect rc = QRect((g.right() + 1 - sz.width() - margin),
                            g.top() + margin,
                           sz.width(), sz.height());
    if (debugToolWindow)
        qDebug() << Q_FUNC_INFO << rc;
    return rc;
}

//  ---------------------- ResourceEditorToolWindow

class ResourceEditorToolWindow: public QDesignerToolWindow
{
public:
    explicit ResourceEditorToolWindow(QDesignerWorkbench *workbench);

    QRect geometryHint() const override;
};

ResourceEditorToolWindow::ResourceEditorToolWindow(QDesignerWorkbench *workbench)  :
    QDesignerToolWindow(workbench,
                        QDesignerComponents::createResourceEditor(workbench->core(), 0),
                        QStringLiteral("qt_designer_resourceeditor"),
                        QDesignerToolWindow::tr("Resource Browser"),
                        QStringLiteral("__qt_resource_editor_tool_action"),
                        Qt::RightDockWidgetArea)
{
}

QRect ResourceEditorToolWindow::geometryHint() const
{
    const QRect g = availableToolWindowGeometry();
    const int margin = workbench()->marginHint();

    const QSize sz(g.width() * 1/3, g.height() * 1/6);
    QRect r(QPoint(0, 0), sz);
    r.moveCenter(g.center());
    r.moveBottom(g.bottom() - margin);
    if (debugToolWindow)
        qDebug() << Q_FUNC_INFO << r;
    return r;
}

//  ---------------------- SignalSlotEditorToolWindow

class SignalSlotEditorToolWindow: public QDesignerToolWindow
{
public:
    explicit SignalSlotEditorToolWindow(QDesignerWorkbench *workbench);

    QRect geometryHint() const override;
};

SignalSlotEditorToolWindow::SignalSlotEditorToolWindow(QDesignerWorkbench *workbench) :
    QDesignerToolWindow(workbench,
                        QDesignerComponents::createSignalSlotEditor(workbench->core(), 0),
                        QStringLiteral("qt_designer_signalsloteditor"),
                        QDesignerToolWindow::tr("Signal/Slot Editor"),
                        QStringLiteral("__qt_signal_slot_editor_tool_action"),
                        Qt::RightDockWidgetArea)
{
}

QRect SignalSlotEditorToolWindow::geometryHint() const
{
    const QRect g = availableToolWindowGeometry();
    const int margin = workbench()->marginHint();

    const QSize sz(g.width() * 1/3, g.height() * 1/6);
    QRect r(QPoint(0, 0), sz);
    r.moveCenter(g.center());
    r.moveTop(margin + g.top());
    if (debugToolWindow)
        qDebug() << Q_FUNC_INFO << r;
    return r;
}

//  ---------------------- WidgetBoxToolWindow

static inline QWidget *createWidgetBox(QDesignerFormEditorInterface *core, QWidget *parent = 0)
{
    QDesignerWidgetBoxInterface *widget = QDesignerComponents::createWidgetBox(core, parent);
    core->setWidgetBox(widget);
    return widget;
}

class WidgetBoxToolWindow: public QDesignerToolWindow
{
public:
    explicit WidgetBoxToolWindow(QDesignerWorkbench *workbench);

    QRect geometryHint() const override;
};

WidgetBoxToolWindow::WidgetBoxToolWindow(QDesignerWorkbench *workbench) :
    QDesignerToolWindow(workbench,
                        createWidgetBox(workbench->core()),
                        QStringLiteral("qt_designer_widgetbox"),
                        QDesignerToolWindow::tr("Widget Box"),
                        QStringLiteral("__qt_widget_box_tool_action"),
                        Qt::LeftDockWidgetArea)
{
}

QRect WidgetBoxToolWindow::geometryHint() const
{
    const QRect g = availableToolWindowGeometry();
    const int margin = workbench()->marginHint();
    const  QRect rc = QRect(g.left() + margin,
                            g.top() + margin,
                            g.width() * 1/4, g.height() * 5/6);
    if (debugToolWindow)
        qDebug() << Q_FUNC_INFO << rc;
    return rc;
}

// -- Factory
QDesignerToolWindow *QDesignerToolWindow::createStandardToolWindow(StandardToolWindow which,
                                                                   QDesignerWorkbench *workbench)
{
    switch (which) {
    case ActionEditor:
        return new ActionEditorToolWindow(workbench);
    case ResourceEditor:
        return new ResourceEditorToolWindow(workbench);
    case SignalSlotEditor:
        return new SignalSlotEditorToolWindow(workbench);
    case PropertyEditor:
        return new PropertyEditorToolWindow(workbench);
    case ObjectInspector:
        return new ObjectInspectorToolWindow(workbench);
    case WidgetBox:
        return new WidgetBoxToolWindow(workbench);
    default:
        break;
    }
    return 0;
}


QT_END_NAMESPACE
