/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Designer of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "formbuilder.h"
#include "formbuilderextra_p.h"
#include "ui4_p.h"

#include <QtUiPlugin/customwidget.h>
#include <QtWidgets/QtWidgets>

QT_BEGIN_NAMESPACE

#ifdef QFORMINTERNAL_NAMESPACE
namespace QFormInternal {
#endif

/*!
    \class QFormBuilder

    \brief The QFormBuilder class is used to dynamically construct
    user interfaces from UI files at run-time.

    \inmodule QtDesigner

    The QFormBuilder class provides a mechanism for dynamically
    creating user interfaces at run-time, based on UI files
    created with \QD. For example:

    \snippet lib/tools_designer_src_lib_uilib_formbuilder.cpp 0

    By including the user interface in the example's resources (\c
    myForm.qrc), we ensure that it will be present when the example is
    run:

    \snippet lib/tools_designer_src_lib_uilib_formbuilder.cpp 1

    QFormBuilder extends the QAbstractFormBuilder base class with a
    number of functions that are used to support custom widget
    plugins:

    \list
    \li pluginPaths() returns the list of paths that the form builder
       searches when loading custom widget plugins.
    \li addPluginPath() allows additional paths to be registered with
       the form builder.
    \li setPluginPath() is used to replace the existing list of paths
       with a list obtained from some other source.
    \li clearPluginPaths() removes all paths registered with the form
       builder.
    \li customWidgets() returns a list of interfaces to plugins that
       can be used to create new instances of registered custom widgets.
    \endlist

    The QFormBuilder class is typically used by custom components and
    applications that embed \QD. Standalone applications that need to
    dynamically generate user interfaces at run-time use the
    QUiLoader class, found in the QtUiTools module.

    \sa QAbstractFormBuilder, {Qt UI Tools}
*/

/*!
    \fn QFormBuilder::QFormBuilder()

    Constructs a new form builder.
*/

QFormBuilder::QFormBuilder()
{
}

/*!
    Destroys the form builder.
*/
QFormBuilder::~QFormBuilder()
{
}

/*!
    \internal
*/
QWidget *QFormBuilder::create(DomWidget *ui_widget, QWidget *parentWidget)
{
    if (!d->parentWidgetIsSet())
        d->setParentWidget(parentWidget);
    // Is this a QLayoutWidget with a margin of 0: Not a known page-based
    // container and no method for adding pages registered.
    d->setProcessingLayoutWidget(false);
    if (ui_widget->attributeClass() == QFormBuilderStrings::instance().qWidgetClass && !ui_widget->hasAttributeNative()
            && parentWidget
#if QT_CONFIG(mainwindow)
            && !qobject_cast<QMainWindow *>(parentWidget)
#endif
#if QT_CONFIG(toolbox)
            && !qobject_cast<QToolBox *>(parentWidget)
#endif
#if QT_CONFIG(stackedwidget)
            && !qobject_cast<QStackedWidget *>(parentWidget)
#endif
#if QT_CONFIG(tabwidget)
            && !qobject_cast<QTabWidget *>(parentWidget)
#endif
#if QT_CONFIG(scrollarea)
            && !qobject_cast<QScrollArea *>(parentWidget)
#endif
#if QT_CONFIG(mdiarea)
            && !qobject_cast<QMdiArea *>(parentWidget)
#endif
#if QT_CONFIG(dockwidget)
            && !qobject_cast<QDockWidget *>(parentWidget)
#endif
        ) {
        const QString parentClassName = QLatin1String(parentWidget->metaObject()->className());
        if (!d->isCustomWidgetContainer(parentClassName))
            d->setProcessingLayoutWidget(true);
    }
    return QAbstractFormBuilder::create(ui_widget, parentWidget);
}


/*!
    \internal
*/
QWidget *QFormBuilder::createWidget(const QString &widgetName, QWidget *parentWidget, const QString &name)
{
    if (widgetName.isEmpty()) {
        //: Empty class name passed to widget factory method
        qWarning() << QCoreApplication::translate("QFormBuilder", "An empty class name was passed on to %1 (object name: '%2').").arg(QString::fromUtf8(Q_FUNC_INFO), name);
        return 0;
    }

    QWidget *w = 0;

#if QT_CONFIG(tabwidget)
    if (qobject_cast<QTabWidget*>(parentWidget))
        parentWidget = 0;
#endif
#if QT_CONFIG(stackedwidget)
    if (qobject_cast<QStackedWidget*>(parentWidget))
        parentWidget = 0;
#endif
#if QT_CONFIG(toolbox)
    if (qobject_cast<QToolBox*>(parentWidget))
        parentWidget = 0;
#endif

    // ### special-casing for Line (QFrame) -- fix for 4.2
    do {
        if (widgetName == QFormBuilderStrings::instance().lineClass) {
            w = new QFrame(parentWidget);
            static_cast<QFrame*>(w)->setFrameStyle(QFrame::HLine | QFrame::Sunken);
            break;
        }
        const QByteArray widgetNameBA = widgetName.toUtf8();
        const char *widgetNameC = widgetNameBA.constData();
        if (w) { // symmetry for macro
        }

#define DECLARE_LAYOUT(L, C)
#define DECLARE_COMPAT_WIDGET(W, C)
#define DECLARE_WIDGET(W, C) else if (!qstrcmp(widgetNameC, #W)) { Q_ASSERT(w == 0); w = new W(parentWidget); }
#define DECLARE_WIDGET_1(W, C) else if (!qstrcmp(widgetNameC, #W)) { Q_ASSERT(w == 0); w = new W(0, parentWidget); }

#include "widgets.table"

#undef DECLARE_COMPAT_WIDGET
#undef DECLARE_LAYOUT
#undef DECLARE_WIDGET
#undef DECLARE_WIDGET_1

        if (w)
            break;

        // try with a registered custom widget
        QDesignerCustomWidgetInterface *factory = d->m_customWidgets.value(widgetName);
        if (factory != 0)
            w = factory->createWidget(parentWidget);
    } while(false);

    if (w == 0) { // Attempt to instantiate base class of promoted/custom widgets
        const QString baseClassName = d->customWidgetBaseClass(widgetName);
        if (!baseClassName.isEmpty()) {
            qWarning() << QCoreApplication::translate("QFormBuilder", "QFormBuilder was unable to create a custom widget of the class '%1'; defaulting to base class '%2'.").arg(widgetName, baseClassName);
            return createWidget(baseClassName, parentWidget, name);
        }
    }

    if (w == 0) { // nothing to do
        qWarning() << QCoreApplication::translate("QFormBuilder", "QFormBuilder was unable to create a widget of the class '%1'.").arg(widgetName);
        return 0;
    }

    w->setObjectName(name);

    if (qobject_cast<QDialog *>(w))
        w->setParent(parentWidget);

    return w;
}

/*!
    \internal
*/
QLayout *QFormBuilder::createLayout(const QString &layoutName, QObject *parent, const QString &name)
{
    QLayout *l = 0;

    QWidget *parentWidget = qobject_cast<QWidget*>(parent);
    QLayout *parentLayout = qobject_cast<QLayout*>(parent);

    Q_ASSERT(parentWidget || parentLayout);

#define DECLARE_WIDGET(W, C)
#define DECLARE_COMPAT_WIDGET(W, C)

#define DECLARE_LAYOUT(L, C) \
    if (layoutName == QLatin1String(#L)) { \
        Q_ASSERT(l == 0); \
        l = parentLayout \
            ? new L() \
            : new L(parentWidget); \
    }

#include "widgets.table"

#undef DECLARE_LAYOUT
#undef DECLARE_COMPAT_WIDGET
#undef DECLARE_WIDGET

    if (l) {
        l->setObjectName(name);
    } else {
        qWarning() << QCoreApplication::translate("QFormBuilder", "The layout type `%1' is not supported.").arg(layoutName);
    }

    return l;
}

/*!
    \internal
*/
bool QFormBuilder::addItem(DomLayoutItem *ui_item, QLayoutItem *item, QLayout *layout)
{
    return QAbstractFormBuilder::addItem(ui_item, item, layout);
}

/*!
    \internal
*/
bool QFormBuilder::addItem(DomWidget *ui_widget, QWidget *widget, QWidget *parentWidget)
{
    return QAbstractFormBuilder::addItem(ui_widget, widget, parentWidget);
}

/*!
    \internal
*/
QWidget *QFormBuilder::widgetByName(QWidget *topLevel, const QString &name)
{
    Q_ASSERT(topLevel);
    if (topLevel->objectName() == name)
        return topLevel;

    return topLevel->findChild<QWidget*>(name);
}

static QObject *objectByName(QWidget *topLevel, const QString &name)
{
    Q_ASSERT(topLevel);
    if (topLevel->objectName() == name)
        return topLevel;

    return topLevel->findChild<QObject*>(name);
}

/*!
    \internal
*/
void QFormBuilder::createConnections(DomConnections *ui_connections, QWidget *widget)
{
    Q_ASSERT(widget != 0);

    if (ui_connections == 0)
        return;

    const auto &connections = ui_connections->elementConnection();
    for (const DomConnection *c : connections) {
        QObject *sender = objectByName(widget, c->elementSender());
        QObject *receiver = objectByName(widget, c->elementReceiver());
        if (!sender || !receiver)
            continue;

        QByteArray sig = c->elementSignal().toUtf8();
        sig.prepend("2");
        QByteArray sl = c->elementSlot().toUtf8();
        sl.prepend("1");
        QObject::connect(sender, sig, receiver, sl);
    }
}

/*!
    \internal
*/
QWidget *QFormBuilder::create(DomUI *ui, QWidget *parentWidget)
{
    return QAbstractFormBuilder::create(ui, parentWidget);
}

/*!
    \internal
*/
QLayout *QFormBuilder::create(DomLayout *ui_layout, QLayout *layout, QWidget *parentWidget)
{
    // Is this a temporary layout widget used to represent QLayout hierarchies in Designer?
    // Set its margins to 0.
    bool layoutWidget = d->processingLayoutWidget();
    QLayout *l = QAbstractFormBuilder::create(ui_layout, layout, parentWidget);
    if (layoutWidget) {
        const QFormBuilderStrings &strings = QFormBuilderStrings::instance();
        int left, top, right, bottom;
        left = top = right = bottom = 0;
        const DomPropertyHash properties = propertyMap(ui_layout->elementProperty());

        if (DomProperty *prop = properties.value(strings.leftMarginProperty))
            left = prop->elementNumber();

        if (DomProperty *prop = properties.value(strings.topMarginProperty))
            top = prop->elementNumber();

        if (DomProperty *prop = properties.value(strings.rightMarginProperty))
            right = prop->elementNumber();

        if (DomProperty *prop = properties.value(strings.bottomMarginProperty))
            bottom = prop->elementNumber();

        l->setContentsMargins(left, top, right, bottom);
        d->setProcessingLayoutWidget(false);
    }
    return l;
}

/*!
    \internal
*/
QLayoutItem *QFormBuilder::create(DomLayoutItem *ui_layoutItem, QLayout *layout, QWidget *parentWidget)
{
    return QAbstractFormBuilder::create(ui_layoutItem, layout, parentWidget);
}

/*!
    \internal
*/
QAction *QFormBuilder::create(DomAction *ui_action, QObject *parent)
{
    return QAbstractFormBuilder::create(ui_action, parent);
}

/*!
    \internal
*/
QActionGroup *QFormBuilder::create(DomActionGroup *ui_action_group, QObject *parent)
{
    return QAbstractFormBuilder::create(ui_action_group, parent);
}

/*!
    Returns the list of paths the form builder searches for plugins.

    \sa addPluginPath()
*/
QStringList QFormBuilder::pluginPaths() const
{
    return d->m_pluginPaths;
}

/*!
    Clears the list of paths that the form builder uses to search for
    custom widget plugins.

    \sa pluginPaths()
*/
void QFormBuilder::clearPluginPaths()
{
    d->m_pluginPaths.clear();
    updateCustomWidgets();
}

/*!
    Adds a new plugin path specified by \a pluginPath to the list of
    paths that will be searched by the form builder when loading a
    custom widget plugin.

    \sa setPluginPath(), clearPluginPaths()
*/
void QFormBuilder::addPluginPath(const QString &pluginPath)
{
    d->m_pluginPaths.append(pluginPath);
    updateCustomWidgets();
}

/*!
    Sets the list of plugin paths to the list specified by \a pluginPaths.

    \sa addPluginPath()
*/
void QFormBuilder::setPluginPath(const QStringList &pluginPaths)
{
    d->m_pluginPaths = pluginPaths;
    updateCustomWidgets();
}

static void insertPlugins(QObject *o, QMap<QString, QDesignerCustomWidgetInterface*> *customWidgets)
{
    // step 1) try with a normal plugin
    if (QDesignerCustomWidgetInterface *iface = qobject_cast<QDesignerCustomWidgetInterface *>(o)) {
        customWidgets->insert(iface->name(), iface);
        return;
    }
    // step 2) try with a collection of plugins
    if (QDesignerCustomWidgetCollectionInterface *c = qobject_cast<QDesignerCustomWidgetCollectionInterface *>(o)) {
        const auto &collectionCustomWidgets = c->customWidgets();
        for (QDesignerCustomWidgetInterface *iface : collectionCustomWidgets)
            customWidgets->insert(iface->name(), iface);
    }
}

/*!
    \internal
*/
void QFormBuilder::updateCustomWidgets()
{
    d->m_customWidgets.clear();

#if QT_CONFIG(library)
    for (const QString &path : qAsConst(d->m_pluginPaths)) {
        const QDir dir(path);
        const QStringList candidates = dir.entryList(QDir::Files);

        for (const QString &plugin : candidates) {
            if (!QLibrary::isLibrary(plugin))
                continue;

            QString loaderPath = path;
            loaderPath += QLatin1Char('/');
            loaderPath += plugin;

            QPluginLoader loader(loaderPath);
            if (loader.load())
                insertPlugins(loader.instance(), &d->m_customWidgets);
        }
    }
#endif // QT_CONFIG(library)

    // Check statically linked plugins
    const QObjectList staticPlugins = QPluginLoader::staticInstances();
    for (QObject *o : staticPlugins)
        insertPlugins(o, &d->m_customWidgets);
}

/*!
    \fn QList<QDesignerCustomWidgetInterface*> QFormBuilder::customWidgets() const

    Returns a list of the available plugins.
*/
QList<QDesignerCustomWidgetInterface*> QFormBuilder::customWidgets() const
{
    return d->m_customWidgets.values();
}

/*!
    \internal
*/

void QFormBuilder::applyProperties(QObject *o, const QList<DomProperty*> &properties)
{

    if (properties.empty())
        return;

    const QFormBuilderStrings &strings = QFormBuilderStrings::instance();

    for (DomProperty *p : properties) {
        const QVariant v = toVariant(o->metaObject(), p);
        if (!v.isValid()) // QTBUG-33130, do not fall for QVariant(QString()).isNull() == true.
            continue;

        const QString attributeName = p->attributeName();
        const bool isWidget = o->isWidgetType();
        if (isWidget && o->parent() == d->parentWidget() && attributeName == strings.geometryProperty) {
            // apply only the size part of a geometry for the root widget
            static_cast<QWidget*>(o)->resize(qvariant_cast<QRect>(v).size());
        } else if (d->applyPropertyInternally(o, attributeName, v)) {
        } else if (isWidget && !qstrcmp("QFrame", o->metaObject()->className ()) && attributeName == strings.orientationProperty) {
            // ### special-casing for Line (QFrame) -- try to fix me
            o->setProperty("frameShape", v); // v is of QFrame::Shape enum
        } else {
            o->setProperty(attributeName.toUtf8(), v);
        }
    }
}

#ifdef QFORMINTERNAL_NAMESPACE
} // namespace QFormInternal
#endif

QT_END_NAMESPACE
