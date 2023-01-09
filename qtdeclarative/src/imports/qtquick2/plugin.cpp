/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtQml/private/qqmlengine_p.h>
#include <QtQmlModels/private/qqmlmodelsmodule_p.h>
#if QT_CONFIG(qml_worker_script)
#include <QtQmlWorkerScript/private/qqmlworkerscriptmodule_p.h>
#endif
#endif // QT_VERSION < QT_VERSION_CHECK(6, 0, 0)

#include <private/qtquick2_p.h>

QT_BEGIN_NAMESPACE

//![class decl]
class QtQuick2Plugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)
public:
    QtQuick2Plugin(QObject *parent = nullptr) : QQmlExtensionPlugin(parent)
    {
        volatile auto registration = &qml_register_types_QtQuick;
        Q_UNUSED(registration);
    }

    void registerTypes(const char *uri) override
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("QtQuick"));
        Q_UNUSED(uri);
        moduleDefined = true;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QQmlEnginePrivate::registerQuickTypes();
        QQmlModelsModule::registerQuickTypes();
#if QT_CONFIG(qml_worker_script)
        QQmlWorkerScriptModule::registerQuickTypes();
#endif
#endif
        QQmlQtQuick2Module::defineModule();
    }

    ~QtQuick2Plugin() override
    {
        if (moduleDefined)
            QQmlQtQuick2Module::undefineModule();
    }

    bool moduleDefined = false;
};
//![class decl]

QT_END_NAMESPACE

#include "plugin.moc"
