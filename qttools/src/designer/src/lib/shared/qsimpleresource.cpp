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

#include "qsimpleresource_p.h"
#include "widgetfactory_p.h"
#include "widgetdatabase_p.h"

#include <QtDesigner/private/properties_p.h>
#include <QtDesigner/private/ui4_p.h>

#include <QtDesigner/abstractformeditor.h>
#include <QtDesigner/abstractlanguage.h>
#include <QtDesigner/qextensionmanager.h>
#include <QtDesigner/extrainfo.h>

#include <QtUiPlugin/customwidget.h>

#include <QtGui/qicon.h>
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qaction.h>
#include <QtCore/qdebug.h>
#include <QtCore/qcoreapplication.h>


QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

bool QSimpleResource::m_warningsEnabled = true;

QSimpleResource::QSimpleResource(QDesignerFormEditorInterface *core) :
    QAbstractFormBuilder(),
    m_core(core)
{
    QString workingDirectory = QDir::homePath();
    workingDirectory +=  QDir::separator();
    workingDirectory +=  QStringLiteral(".designer");
    setWorkingDirectory(QDir(workingDirectory));
}

QSimpleResource::~QSimpleResource() = default;

QBrush QSimpleResource::setupBrush(DomBrush *brush)
{
    return QAbstractFormBuilder::setupBrush(brush);
}

DomBrush *QSimpleResource::saveBrush(const QBrush &brush)
{
    return QAbstractFormBuilder::saveBrush(brush);
}

void QSimpleResource::addExtensionDataToDOM(QAbstractFormBuilder * /* afb */,
                                            QDesignerFormEditorInterface *core,
                                            DomWidget *ui_widget, QWidget *widget)
{
    QExtensionManager *emgr = core->extensionManager();
    if (QDesignerExtraInfoExtension *extra = qt_extension<QDesignerExtraInfoExtension*>(emgr, widget)) {
        extra->saveWidgetExtraInfo(ui_widget);
    }
}

void QSimpleResource::applyExtensionDataFromDOM(QAbstractFormBuilder * /* afb */,
                                                QDesignerFormEditorInterface *core,
                                                DomWidget *ui_widget, QWidget *widget)
{
    QExtensionManager *emgr = core->extensionManager();
    if (QDesignerExtraInfoExtension *extra = qt_extension<QDesignerExtraInfoExtension*>(emgr, widget)) {
        extra->loadWidgetExtraInfo(ui_widget);
    }
}

QString QSimpleResource::customWidgetScript(QDesignerFormEditorInterface *core, QObject *object)
{
    return customWidgetScript(core, qdesigner_internal::WidgetFactory::classNameOf(core, object));
}

bool QSimpleResource::hasCustomWidgetScript(QDesignerFormEditorInterface *, QObject *)
{
    return false;
}

QString QSimpleResource::customWidgetScript(QDesignerFormEditorInterface *, const QString &)
{
    return QString();
}

// Custom widgets handling helpers

// Add unique fake slots and signals to lists
bool QSimpleResource::addFakeMethods(const DomSlots *domSlots, QStringList &fakeSlots, QStringList &fakeSignals)
{
    if (!domSlots)
        return false;

    bool rc = false;
    const QStringList &elementSlots = domSlots->elementSlot();
    for (const QString &fakeSlot : elementSlots)
        if (fakeSlots.indexOf(fakeSlot) == -1) {
            fakeSlots += fakeSlot;
            rc = true;
        }

    const QStringList &elementSignals = domSlots->elementSignal();
    for (const QString &fakeSignal : elementSignals)
        if (fakeSignals.indexOf(fakeSignal) == -1) {
            fakeSignals += fakeSignal;
            rc = true;
        }
    return rc;
}

void QSimpleResource::addFakeMethodsToWidgetDataBase(const DomCustomWidget *domCustomWidget, WidgetDataBaseItem *item)
{
    const DomSlots *domSlots = domCustomWidget->elementSlots();
    if (!domSlots)
        return;

    // Merge in new slots, signals
    QStringList fakeSlots = item->fakeSlots();
    QStringList fakeSignals = item->fakeSignals();
    if (addFakeMethods(domSlots, fakeSlots, fakeSignals)) {
        item->setFakeSlots(fakeSlots);
        item->setFakeSignals(fakeSignals);
    }
}

// Perform one iteration of adding the custom widgets to the database,
// looking up the base class and inheriting its data.
// Remove the succeeded custom widgets from the list.
// Classes whose base class could not be found are left in the list.

void QSimpleResource::addCustomWidgetsToWidgetDatabase(const QDesignerFormEditorInterface *core,
                                                       QVector<DomCustomWidget *> &custom_widget_list)
{
    QDesignerWidgetDataBaseInterface *db = core->widgetDataBase();
    for (int i=0; i < custom_widget_list.size(); ) {
        bool classInserted = false;
        DomCustomWidget *custom_widget = custom_widget_list[i];
        const QString customClassName = custom_widget->elementClass();
        const QString base_class = custom_widget->elementExtends();
        QString includeFile;
        IncludeType includeType = IncludeLocal;
        if (const DomHeader *header = custom_widget->elementHeader()) {
            includeFile = header->text();
            if (header->hasAttributeLocation() && header->attributeLocation() == QStringLiteral("global"))
                includeType = IncludeGlobal;
        }
        const bool domIsContainer = custom_widget->elementContainer();
        // Append a new item
        if (base_class.isEmpty()) {
            WidgetDataBaseItem *item = new WidgetDataBaseItem(customClassName);
            item->setPromoted(false);
            item->setGroup(QCoreApplication::translate("Designer", "Custom Widgets"));
            item->setIncludeFile(buildIncludeFile(includeFile, includeType));
            item->setContainer(domIsContainer);
            item->setCustom(true);
            addFakeMethodsToWidgetDataBase(custom_widget, item);
            db->append(item);
            custom_widget_list.removeAt(i);
            classInserted = true;
        } else {
            // Create a new entry cloned from base class. Note that this will ignore existing
            // classes, eg, plugin custom widgets.
            QDesignerWidgetDataBaseItemInterface *item =
                appendDerived(db, customClassName, QCoreApplication::translate("Designer", "Promoted Widgets"),
                              base_class,
                              buildIncludeFile(includeFile, includeType),
                              true,true);
            // Ok, base class found.
            if (item) {
                // Hack to accommodate for old UI-files in which "container" is not set properly:
                // Apply "container" from DOM only if true (else, eg classes from QFrame might not accept
                // dropping child widgets on them as container=false). This also allows for
                // QWidget-derived stacked pages.
                if (domIsContainer)
                    item->setContainer(domIsContainer);

                addFakeMethodsToWidgetDataBase(custom_widget, static_cast<WidgetDataBaseItem*>(item));
                custom_widget_list.removeAt(i);
                classInserted = true;
            }
        }
        // Skip failed item.
        if (!classInserted)
            i++;
    }

}

void QSimpleResource::handleDomCustomWidgets(const QDesignerFormEditorInterface *core,
                                             const DomCustomWidgets *dom_custom_widgets)
{
    if (dom_custom_widgets == 0)
        return;
    auto custom_widget_list = dom_custom_widgets->elementCustomWidget();
    // Attempt to insert each item derived from its base class.
    // This should at most require two iterations in the event that the classes are out of order
    // (derived first, max depth: promoted custom plugin = 2)
    for (int iteration = 0;  iteration < 2;  iteration++) {
        addCustomWidgetsToWidgetDatabase(core, custom_widget_list);
        if (custom_widget_list.empty())
            return;
    }
    // Oops, there are classes left whose base class could not be found.
    // Default them to QWidget with warnings.
    const QString fallBackBaseClass = QStringLiteral("QWidget");
    for (DomCustomWidget *custom_widget : qAsConst(custom_widget_list)) {
        const QString customClassName = custom_widget->elementClass();
        const QString base_class = custom_widget->elementExtends();
        qDebug() << "** WARNING The base class " << base_class << " of the custom widget class " << customClassName
            << " could not be found. Defaulting to " << fallBackBaseClass << '.';
        custom_widget->setElementExtends(fallBackBaseClass);
    }
    // One more pass.
    addCustomWidgetsToWidgetDatabase(core, custom_widget_list);
}

// ------------ FormBuilderClipboard

FormBuilderClipboard::FormBuilderClipboard(QWidget *w)
{
    m_widgets += w;
}

bool FormBuilderClipboard::empty() const
{
    return m_widgets.empty() && m_actions.empty();
}
}

QT_END_NAMESPACE
