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

#include "qscriptdebuggerscriptswidget_p.h"
#include "qscriptdebuggerscriptswidgetinterface_p_p.h"
#include "qscriptdebuggerscriptsmodel_p.h"

#include <QtCore/qdebug.h>
#include <QtWidgets/qheaderview.h>
#include <QtWidgets/qtreeview.h>
#include <QtWidgets/qboxlayout.h>

QT_BEGIN_NAMESPACE

class QScriptDebuggerScriptsWidgetPrivate
    : public QScriptDebuggerScriptsWidgetInterfacePrivate
{
    Q_DECLARE_PUBLIC(QScriptDebuggerScriptsWidget)
public:
    QScriptDebuggerScriptsWidgetPrivate();
    ~QScriptDebuggerScriptsWidgetPrivate();

    // private slots
    void _q_onCurrentChanged(const QModelIndex &index);

    QTreeView *view;
    qint64 currentScriptId;
};

QScriptDebuggerScriptsWidgetPrivate::QScriptDebuggerScriptsWidgetPrivate()
{
    currentScriptId = -1;
}

QScriptDebuggerScriptsWidgetPrivate::~QScriptDebuggerScriptsWidgetPrivate()
{
}

void QScriptDebuggerScriptsWidgetPrivate::_q_onCurrentChanged(const QModelIndex &index)
{
    Q_Q(QScriptDebuggerScriptsWidget);
    if (!index.isValid())
        return;
    qint64 sid = q->scriptsModel()->scriptIdFromIndex(index);
    if (sid != -1) {
        if (currentScriptId != sid) {
            currentScriptId = sid;
            emit q->currentScriptChanged(sid);
        }
    } else {
        qint64 sid = q->scriptsModel()->scriptIdFromIndex(index.parent());
        Q_ASSERT(sid != -1);
        currentScriptId = sid;
        emit q->currentScriptChanged(sid);
        QPair<QString, int> info = q->scriptsModel()->scriptFunctionInfoFromIndex(index);
        emit q->scriptLocationSelected(info.second);
    }
}

QScriptDebuggerScriptsWidget::QScriptDebuggerScriptsWidget(QWidget *parent)
    : QScriptDebuggerScriptsWidgetInterface(*new QScriptDebuggerScriptsWidgetPrivate, parent, {})
{
    Q_D(QScriptDebuggerScriptsWidget);
    d->view = new QTreeView();
    d->view->setEditTriggers(QAbstractItemView::NoEditTriggers);
//    d->view->setAlternatingRowColors(true);
//    d->view->setRootIsDecorated(false);
    d->view->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->view->header()->hide();
//    d->view->header()->setDefaultAlignment(Qt::AlignLeft);
//    d->view->header()->setResizeMode(QHeaderView::ResizeToContents);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->addWidget(d->view);
}

QScriptDebuggerScriptsWidget::~QScriptDebuggerScriptsWidget()
{
}

/*!
  \reimp
*/
QScriptDebuggerScriptsModel *QScriptDebuggerScriptsWidget::scriptsModel() const
{
    Q_D(const QScriptDebuggerScriptsWidget);
    return qobject_cast<QScriptDebuggerScriptsModel*>(d->view->model());
}

/*!
  \reimp
*/
void QScriptDebuggerScriptsWidget::setScriptsModel(QScriptDebuggerScriptsModel *model)
{
    Q_D(QScriptDebuggerScriptsWidget);
    d->view->setModel(model);
    QObject::connect(d->view->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                     this, SLOT(_q_onCurrentChanged(QModelIndex)));
}

qint64 QScriptDebuggerScriptsWidget::currentScriptId() const
{
    Q_D(const QScriptDebuggerScriptsWidget);
    return scriptsModel()->scriptIdFromIndex(d->view->currentIndex());
}

void QScriptDebuggerScriptsWidget::setCurrentScript(qint64 id)
{
    Q_D(QScriptDebuggerScriptsWidget);
    d->view->setCurrentIndex(scriptsModel()->indexFromScriptId(id));
}

QT_END_NAMESPACE

#include "moc_qscriptdebuggerscriptswidget_p.cpp"
