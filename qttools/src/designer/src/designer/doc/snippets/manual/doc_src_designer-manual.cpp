// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [1]
#include <QtUiTools>
//! [1]


//! [2]
void on_<object name>_<signal name>(<signal parameters>);
//! [2]


//! [7]
class MyExtension: public QObject,
                   public QdesignerContainerExtension
{
    Q_OBJECT
    Q_INTERFACE(QDesignerContainerExtension)

    ...
}
//! [7]


//! [8]
QObject *ANewExtensionFactory::createExtension(QObject *object,
        const QString &iid, QObject *parent) const
{
    if (iid != Q_TYPEID(QDesignerContainerExtension))
        return 0;

    if (MyCustomWidget *widget = qobject_cast<MyCustomWidget*>
            (object))
        return new MyContainerExtension(widget, parent);

    return 0;
}
//! [8]


//! [9]
QObject *AGeneralExtensionFactory::createExtension(QObject *object,
        const QString &iid, QObject *parent) const
{
    MyCustomWidget *widget = qobject_cast<MyCustomWidget*>(object);

    if (widget && (iid == Q_TYPEID(QDesignerTaskMenuExtension))) {
         return new MyTaskMenuExtension(widget, parent);

    } else if (widget && (iid == Q_TYPEID(QDesignerContainerExtension))) {
        return new MyContainerExtension(widget, parent);

    } else {
        return 0;
    }
}
//! [9]


//! [10]
void MyPlugin::initialize(QDesignerFormEditorInterface *formEditor)
{
    if (initialized)
        return;

    QExtensionManager *manager = formEditor->extensionManager();
    Q_ASSERT(manager != 0);

    manager->registerExtensions(new MyExtensionFactory(manager),
                                Q_TYPEID(QDesignerTaskMenuExtension));

    initialized = true;
}
//! [10]
