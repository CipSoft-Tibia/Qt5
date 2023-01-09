/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSCriptTools module of the Qt Toolkit.
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

#include "qscriptdebuggercodewidget_p.h"
#include "qscriptdebuggercodewidgetinterface_p_p.h"
#include "qscriptdebuggercodeview_p.h"
#include "qscriptdebuggerscriptsmodel_p.h"
#include "qscriptbreakpointsmodel_p.h"
#include "qscripttooltipproviderinterface_p.h"

#include <QtCore/qdebug.h>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qstackedwidget.h>

QT_BEGIN_NAMESPACE

class QScriptDebuggerCodeWidgetPrivate
    : public QScriptDebuggerCodeWidgetInterfacePrivate
{
    Q_DECLARE_PUBLIC(QScriptDebuggerCodeWidget)
public:
    QScriptDebuggerCodeWidgetPrivate();
    ~QScriptDebuggerCodeWidgetPrivate();

    qint64 scriptId(QScriptDebuggerCodeViewInterface *view) const;

    // private slots
    void _q_onBreakpointToggleRequest(int lineNumber, bool on);
    void _q_onBreakpointEnableRequest(int lineNumber, bool enable);
    void _q_onBreakpointsAboutToBeRemoved(const QModelIndex&, int, int);
    void _q_onBreakpointsInserted(const QModelIndex&, int, int);
    void _q_onBreakpointsDataChanged(const QModelIndex &, const QModelIndex &);
    void _q_onScriptsChanged();
    void _q_onToolTipRequest(const QPoint &pos, int lineNumber, const QStringList &path);

    QScriptDebuggerScriptsModel *scriptsModel;
    QStackedWidget *viewStack;
    QHash<qint64, QScriptDebuggerCodeViewInterface*> viewHash;
    QScriptBreakpointsModel *breakpointsModel;
    QScriptToolTipProviderInterface *toolTipProvider;
};

QScriptDebuggerCodeWidgetPrivate::QScriptDebuggerCodeWidgetPrivate()
{
    scriptsModel = 0;
    breakpointsModel = 0;
    toolTipProvider = 0;
}

QScriptDebuggerCodeWidgetPrivate::~QScriptDebuggerCodeWidgetPrivate()
{
}

qint64 QScriptDebuggerCodeWidgetPrivate::scriptId(QScriptDebuggerCodeViewInterface *view) const
{
    if (!view)
        return -1;
    return viewHash.key(view);
}

void QScriptDebuggerCodeWidgetPrivate::_q_onBreakpointToggleRequest(int lineNumber, bool on)
{
    QScriptDebuggerCodeViewInterface *view = qobject_cast<QScriptDebuggerCodeViewInterface*>(q_func()->sender());
    qint64 sid = scriptId(view);
    Q_ASSERT(sid != -1);
    if (on) {
        QScriptBreakpointData data(sid, lineNumber);
        data.setFileName(scriptsModel->scriptData(sid).fileName());
        breakpointsModel->setBreakpoint(data);
    } else {
        int bpid = breakpointsModel->resolveBreakpoint(sid, lineNumber);
        if (bpid == -1)
            bpid = breakpointsModel->resolveBreakpoint(scriptsModel->scriptData(sid).fileName(), lineNumber);
        Q_ASSERT(bpid != -1);
        breakpointsModel->deleteBreakpoint(bpid);
    }
}

void QScriptDebuggerCodeWidgetPrivate::_q_onBreakpointEnableRequest(int lineNumber, bool enable)
{
    QScriptDebuggerCodeViewInterface *view = qobject_cast<QScriptDebuggerCodeViewInterface*>(q_func()->sender());
    qint64 sid = scriptId(view);
    int bpid = breakpointsModel->resolveBreakpoint(sid, lineNumber);
    if (bpid == -1)
        bpid = breakpointsModel->resolveBreakpoint(scriptsModel->scriptData(sid).fileName(), lineNumber);
    Q_ASSERT(bpid != -1);
    QScriptBreakpointData data = breakpointsModel->breakpointData(bpid);
    data.setEnabled(enable);
    breakpointsModel->setBreakpointData(bpid, data);
}

void QScriptDebuggerCodeWidgetPrivate::_q_onBreakpointsAboutToBeRemoved(
    const QModelIndex &, int first, int last)
{
    for (int i = first; i <= last; ++i) {
        QScriptBreakpointData data = breakpointsModel->breakpointDataAt(i);
        qint64 scriptId = data.scriptId();
        if (scriptId == -1) {
            scriptId = scriptsModel->resolveScript(data.fileName());
            if (scriptId == -1)
                continue;
        }
        QScriptDebuggerCodeViewInterface *view = viewHash.value(scriptId);
        if (!view)
            continue;
        view->deleteBreakpoint(data.lineNumber());
    }
}

void QScriptDebuggerCodeWidgetPrivate::_q_onBreakpointsInserted(
    const QModelIndex &, int first, int last)
{
    for (int i = first; i <= last; ++i) {
        QScriptBreakpointData data = breakpointsModel->breakpointDataAt(i);
        qint64 scriptId = data.scriptId();
        if (scriptId == -1) {
            scriptId = scriptsModel->resolveScript(data.fileName());
            if (scriptId == -1)
                continue;
        }
        QScriptDebuggerCodeViewInterface *view = viewHash.value(scriptId);
        if (!view)
            continue;
        view->setBreakpoint(data.lineNumber());
    }
}

void QScriptDebuggerCodeWidgetPrivate::_q_onBreakpointsDataChanged(
    const QModelIndex &tl, const QModelIndex &br)
{
    for (int i = tl.row(); i <= br.row(); ++i) {
        QScriptBreakpointData data = breakpointsModel->breakpointDataAt(i);
        qint64 scriptId = data.scriptId();
        if (scriptId == -1) {
            scriptId = scriptsModel->resolveScript(data.fileName());
            if (scriptId == -1)
                continue;
        }
        QScriptDebuggerCodeViewInterface *view = viewHash.value(scriptId);
        if (!view)
            continue;
        view->setBreakpointEnabled(data.lineNumber(), data.isEnabled());
    }
}

void QScriptDebuggerCodeWidgetPrivate::_q_onScriptsChanged()
{
    // kill editors for scripts that have been removed
    QHash<qint64, QScriptDebuggerCodeViewInterface*>::iterator it;
    for (it = viewHash.begin(); it != viewHash.end(); ) {
        if (!scriptsModel->scriptData(it.key()).isValid()) {
            it = viewHash.erase(it);
        } else
            ++it;
    }
}

void QScriptDebuggerCodeWidgetPrivate::_q_onToolTipRequest(
    const QPoint &pos, int lineNumber, const QStringList &path)
{
    toolTipProvider->showToolTip(pos, /*frameIndex=*/-1, lineNumber, path);
}

QScriptDebuggerCodeWidget::QScriptDebuggerCodeWidget(QWidget *parent)
    : QScriptDebuggerCodeWidgetInterface(*new QScriptDebuggerCodeWidgetPrivate, parent, {})
{
    Q_D(QScriptDebuggerCodeWidget);
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);
    d->viewStack = new QStackedWidget();
    vbox->addWidget(d->viewStack);
}

QScriptDebuggerCodeWidget::~QScriptDebuggerCodeWidget()
{
}

QScriptDebuggerScriptsModel *QScriptDebuggerCodeWidget::scriptsModel() const
{
    Q_D(const QScriptDebuggerCodeWidget);
    return d->scriptsModel;
}

void QScriptDebuggerCodeWidget::setScriptsModel(QScriptDebuggerScriptsModel *model)
{
    Q_D(QScriptDebuggerCodeWidget);
    d->scriptsModel = model;
    QObject::connect(model, SIGNAL(layoutChanged()),
                     this, SLOT(_q_onScriptsChanged()));
}

qint64 QScriptDebuggerCodeWidget::currentScriptId() const
{
    Q_D(const QScriptDebuggerCodeWidget);
    return d->scriptId(currentView());
}

void QScriptDebuggerCodeWidget::setCurrentScript(qint64 scriptId)
{
    Q_D(QScriptDebuggerCodeWidget);
    if (scriptId == -1) {
        // ### show "native script"
        return;
    }
    QScriptDebuggerCodeViewInterface *view = d->viewHash.value(scriptId);
    if (!view) {
        Q_ASSERT(d->scriptsModel != 0);
        QScriptScriptData data = d->scriptsModel->scriptData(scriptId);
        if (!data.isValid())
            return;
        view = new QScriptDebuggerCodeView(); // ### use factory, so user can provide his own view
        view->setBaseLineNumber(data.baseLineNumber());
        view->setText(data.contents());
        view->setExecutableLineNumbers(d->scriptsModel->executableLineNumbers(scriptId));
        QObject::connect(view, SIGNAL(breakpointToggleRequest(int,bool)),
                         this, SLOT(_q_onBreakpointToggleRequest(int,bool)));
        QObject::connect(view, SIGNAL(breakpointEnableRequest(int,bool)),
                         this, SLOT(_q_onBreakpointEnableRequest(int,bool)));
        QObject::connect(view, SIGNAL(toolTipRequest(QPoint,int,QStringList)),
                         this, SLOT(_q_onToolTipRequest(QPoint,int,QStringList)));
        d->viewStack->addWidget(view);
        d->viewHash.insert(scriptId, view);
    }
    d->viewStack->setCurrentWidget(view);
}

void QScriptDebuggerCodeWidget::invalidateExecutionLineNumbers()
{
    Q_D(QScriptDebuggerCodeWidget);
    QHash<qint64, QScriptDebuggerCodeViewInterface*>::const_iterator it;
    for (it = d->viewHash.constBegin(); it != d->viewHash.constEnd(); ++it)
        it.value()->setExecutionLineNumber(-1, /*error=*/false);
}

QScriptBreakpointsModel *QScriptDebuggerCodeWidget::breakpointsModel() const
{
    Q_D(const QScriptDebuggerCodeWidget);
    return d->breakpointsModel;
}

void QScriptDebuggerCodeWidget::setBreakpointsModel(QScriptBreakpointsModel *model)
{
    Q_D(QScriptDebuggerCodeWidget);
    d->breakpointsModel = model;
    QObject::connect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
                     this, SLOT(_q_onBreakpointsAboutToBeRemoved(QModelIndex,int,int)));
    QObject::connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)),
                     this, SLOT(_q_onBreakpointsInserted(QModelIndex,int,int)));
    QObject::connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                     this, SLOT(_q_onBreakpointsDataChanged(QModelIndex,QModelIndex)));
}

void QScriptDebuggerCodeWidget::setToolTipProvider(QScriptToolTipProviderInterface *toolTipProvider)
{
    Q_D(QScriptDebuggerCodeWidget);
    d->toolTipProvider = toolTipProvider;
}

QScriptDebuggerCodeViewInterface *QScriptDebuggerCodeWidget::currentView() const
{
    Q_D(const QScriptDebuggerCodeWidget);
    return qobject_cast<QScriptDebuggerCodeViewInterface*>(d->viewStack->currentWidget());
}

QT_END_NAMESPACE

#include "moc_qscriptdebuggercodewidget_p.cpp"
