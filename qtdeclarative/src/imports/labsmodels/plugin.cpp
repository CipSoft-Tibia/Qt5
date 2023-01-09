/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>

#include <private/qqmlmodelsmodule_p.h>

#if QT_CONFIG(qml_table_model)
#include "qqmltablemodel_p.h"
#include "qqmltablemodelcolumn_p.h"
#endif
#if QT_CONFIG(qml_delegate_model)
#include "qqmldelegatecomponent_p.h"
#endif

extern void qml_register_types_Qt_labs_qmlmodels();
GHS_KEEP_REFERENCE(qml_register_types_Qt_labs_qmlmodels);

QT_BEGIN_NAMESPACE

/*!
    \qmlmodule Qt.labs.qmlmodels 1.0
    \title Qt QML Models experimental QML Types
    \ingroup qmlmodules
    \brief Provides QML experimental types for data models.
    \since 5.12

    This QML module contains experimental QML types related to data models.

    To use the types in this module, import the module with the following line:

    \code
    import Qt.labs.qmlmodels 1.0
    \endcode
*/

//![class decl]
class QtQmlLabsModelsPlugin : public QQmlEngineExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)
public:
    QtQmlLabsModelsPlugin(QObject *parent = nullptr) : QQmlEngineExtensionPlugin(parent)
    {
        volatile auto registration = &qml_register_types_Qt_labs_qmlmodels;
        Q_UNUSED(registration);
    }
};
//![class decl]

QT_END_NAMESPACE

#include "plugin.moc"
