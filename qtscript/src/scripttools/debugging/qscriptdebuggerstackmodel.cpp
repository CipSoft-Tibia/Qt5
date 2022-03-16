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

#include "qscriptdebuggerstackmodel_p.h"

#include "private/qabstractitemmodel_p.h"

#include <QtScript/qscriptcontextinfo.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

class QScriptDebuggerStackModelPrivate
    : public QAbstractItemModelPrivate
{
    Q_DECLARE_PUBLIC(QScriptDebuggerStackModel)
public:
    QScriptDebuggerStackModelPrivate();
    ~QScriptDebuggerStackModelPrivate();

    QList<QScriptContextInfo> contextInfos;
};

QScriptDebuggerStackModelPrivate::QScriptDebuggerStackModelPrivate()
{
}

QScriptDebuggerStackModelPrivate::~QScriptDebuggerStackModelPrivate()
{
}

QScriptDebuggerStackModel::QScriptDebuggerStackModel(QObject *parent)
    : QAbstractTableModel(*new QScriptDebuggerStackModelPrivate, parent)
{
}

QScriptDebuggerStackModel::~QScriptDebuggerStackModel()
{
}

QList<QScriptContextInfo> QScriptDebuggerStackModel::contextInfos() const
{
    Q_D(const QScriptDebuggerStackModel);
    return d->contextInfos;
}

void QScriptDebuggerStackModel::setContextInfos(const QList<QScriptContextInfo> &infos)
{
    Q_D(QScriptDebuggerStackModel);
    layoutAboutToBeChanged();
    d->contextInfos = infos;
    layoutChanged();
}

/*!
  \reimp
*/
int QScriptDebuggerStackModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 3;
    return 0;
}

/*!
  \reimp
*/
int QScriptDebuggerStackModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const QScriptDebuggerStackModel);
    if (!parent.isValid())
        return d->contextInfos.count();
    return 0;
}

/*!
  \reimp
*/
QVariant QScriptDebuggerStackModel::data(const QModelIndex &index, int role) const
{
    Q_D(const QScriptDebuggerStackModel);
    if (!index.isValid())
        return QVariant();
    if (index.row() >= d->contextInfos.count())
        return QVariant();
    const QScriptContextInfo &info = d->contextInfos.at(index.row());
    if (role == Qt::DisplayRole) {
        if (index.column() == 0) {
            return index.row();
        } else if (index.column() == 1) {
            QString name = info.functionName();
            if (name.isEmpty())
                name = QString::fromLatin1("<anonymous>");
            return name;
        } else if (index.column() == 2) {
            QString fn = QFileInfo(info.fileName()).fileName();
            if (fn.isEmpty()) {
                if (info.functionType() == QScriptContextInfo::ScriptFunction)
                    fn = QString::fromLatin1("<anonymous script, id=%0>").arg(info.scriptId());
                else
                    fn = QString::fromLatin1("<native>");

            }
            return QString::fromLatin1("%0:%1").arg(fn).arg(info.lineNumber());
        }
    } else if (role == Qt::ToolTipRole) {
        if (QFileInfo(info.fileName()).fileName() != info.fileName())
            return info.fileName();
    }
    return QVariant();
}

/*!
  \reimp
*/
QVariant QScriptDebuggerStackModel::headerData(int section, Qt::Orientation orient, int role) const
{
    if (orient != Qt::Horizontal)
        return QVariant();
    if (role == Qt::DisplayRole) {
        if (section == 0)
            return QCoreApplication::translate("QScriptDebuggerStackModel", "Level");
        else if (section == 1)
            return QCoreApplication::translate("QScriptDebuggerStackModel", "Name");
        else if (section == 2)
            return QCoreApplication::translate("QScriptDebuggerStackModel", "Location");
    }
    return QVariant();
}

QT_END_NAMESPACE
