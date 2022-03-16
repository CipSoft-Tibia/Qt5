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

#include "qdesigner_formwindowmanager_p.h"
#include "plugindialog_p.h"

#include <QtDesigner/abstractformeditor.h>

QT_BEGIN_NAMESPACE

using namespace qdesigner_internal;

/*!
    \class qdesigner_internal::QDesignerFormWindowManager
    \inmodule QtDesigner

    Extends QDesignerFormWindowManagerInterface with methods to control
    the preview and printing of forms. It provides a facade that simplifies
    the complexity of the more general PreviewConfiguration & PreviewManager
    interfaces.

    \since 4.5
  */


QDesignerFormWindowManager::QDesignerFormWindowManager(QObject *parent)
    : QDesignerFormWindowManagerInterface(parent), m_unused(0)
{
}

QDesignerFormWindowManager::~QDesignerFormWindowManager()
{
}

/*!
    \fn PreviewManager *qdesigner_internal::QDesignerFormWindowManager::previewManager() const

    Accesses the previewmanager implementation.

    \since 4.5
    \internal
  */

void QDesignerFormWindowManager::showPluginDialog()
{
    PluginDialog dlg(core(), core()->topLevel());
    dlg.exec();
}

QT_END_NAMESPACE
