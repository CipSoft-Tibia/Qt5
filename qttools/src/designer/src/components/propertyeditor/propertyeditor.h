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

#ifndef PROPERTYEDITOR_H
#define PROPERTYEDITOR_H

#include "propertyeditor_global.h"
#include <qdesigner_propertyeditor_p.h>

#include <QtCore/qpointer.h>
#include <QtCore/qmap.h>
#include <QtCore/qvector.h>
#include <QtCore/qset.h>

QT_BEGIN_NAMESPACE

class DomProperty;
class QDesignerMetaDataBaseItemInterface;
class QDesignerPropertySheetExtension;
class QLineEdit;

class QtAbstractPropertyBrowser;
class QtButtonPropertyBrowser;
class QtTreePropertyBrowser;
class QtProperty;
class QtVariantProperty;
class QtBrowserItem;
class QStackedWidget;

namespace qdesigner_internal {

class StringProperty;
class DesignerPropertyManager;
class DesignerEditorFactory;
class ElidingLabel;

class QT_PROPERTYEDITOR_EXPORT PropertyEditor: public QDesignerPropertyEditor
{
    Q_OBJECT
public:
    explicit PropertyEditor(QDesignerFormEditorInterface *core, QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~PropertyEditor() override;

    QDesignerFormEditorInterface *core() const override;

    bool isReadOnly() const override;
    void setReadOnly(bool readOnly) override;
    void setPropertyValue(const QString &name, const QVariant &value, bool changed = true) override;
    void updatePropertySheet() override;

    void setObject(QObject *object) override;

    void reloadResourceProperties() override;

    QObject *object() const override
    { return m_object; }

    QString currentPropertyName() const override;

protected:

    bool event(QEvent *event) override;

private slots:
    void slotResetProperty(QtProperty *property);
    void slotValueChanged(QtProperty *property, const QVariant &value, bool enableSubPropertyHandling);
    void slotViewTriggered(QAction *action);
    void slotAddDynamicProperty(QAction *action);
    void slotRemoveDynamicProperty();
    void slotSorting(bool sort);
    void slotColoring(bool color);
    void slotCurrentItemChanged(QtBrowserItem*);
    void setFilter(const QString &pattern);

private:
    void updateBrowserValue(QtVariantProperty *property, const QVariant &value);
    void updateToolBarLabel();
    int toBrowserType(const QVariant &value, const QString &propertyName) const;
    QString removeScope(const QString &value) const;
    QDesignerMetaDataBaseItemInterface *metaDataBaseItem() const;
    void setupStringProperty(QtVariantProperty *property, bool isMainContainer);
    void setupPaletteProperty(QtVariantProperty *property);
    QString realClassName(QObject *object) const;
    void storeExpansionState();
    void applyExpansionState();
    void storePropertiesExpansionState(const QList<QtBrowserItem *> &items);
    void applyPropertiesExpansionState(const QList<QtBrowserItem *> &items);
    void applyFilter();
    int applyPropertiesFilter(const QList<QtBrowserItem *> &items);
    void setExpanded(QtBrowserItem *item, bool expanded);
    bool isExpanded(QtBrowserItem *item) const;
    void setItemVisible(QtBrowserItem *item, bool visible);
    bool isItemVisible(QtBrowserItem *item) const;
    void collapseAll();
    void clearView();
    void fillView();
    bool isLayoutGroup(QtProperty *group) const;
    void updateColors();
    void updateForegroundBrightness();
    QColor propertyColor(QtProperty *property) const;
    void updateActionsState();
    QtBrowserItem *nonFakePropertyBrowserItem(QtBrowserItem *item) const;
    void saveSettings() const;
    void editProperty(const QString &name);
    bool isDynamicProperty(const QtBrowserItem* item) const;

    struct Strings {
        Strings();
        QSet<QString> m_alignmentProperties;
        const QString m_fontProperty;
        const QString m_qLayoutWidget;
        const QString m_designerPrefix;
        const QString m_layout;
        const QString m_validationModeAttribute;
        const QString m_fontAttribute;
        const QString m_superPaletteAttribute;
        const QString m_enumNamesAttribute;
        const QString m_resettableAttribute;
        const QString m_flagsAttribute;
    };

    const Strings m_strings;
    QDesignerFormEditorInterface *m_core;
    QDesignerPropertySheetExtension *m_propertySheet;
    QtAbstractPropertyBrowser *m_currentBrowser;
    QtButtonPropertyBrowser *m_buttonBrowser;
    QtTreePropertyBrowser *m_treeBrowser;
    DesignerPropertyManager *m_propertyManager;
    DesignerEditorFactory *m_treeFactory;
    DesignerEditorFactory *m_groupFactory;
    QPointer<QObject> m_object;
    QMap<QString, QtVariantProperty*> m_nameToProperty;
    QMap<QtProperty*, QString> m_propertyToGroup;
    QMap<QString, QtVariantProperty*> m_nameToGroup;
    QList<QtProperty *> m_groups;
    QtProperty *m_dynamicGroup;
    QString m_recentlyAddedDynamicProperty;
    bool m_updatingBrowser;

    QStackedWidget *m_stackedWidget;
    QLineEdit *m_filterWidget;
    int m_buttonIndex;
    int m_treeIndex;
    QAction *m_addDynamicAction;
    QAction *m_removeDynamicAction;
    QAction *m_sortingAction;
    QAction *m_coloringAction;
    QAction *m_treeAction;
    QAction *m_buttonAction;
    ElidingLabel *m_classLabel;

    bool m_sorting;
    bool m_coloring;

    QMap<QString, bool> m_expansionState;

    QString m_filterPattern;
    QVector<QPair<QColor, QColor> > m_colors;
    QPair<QColor, QColor> m_dynamicColor;
    QPair<QColor, QColor> m_layoutColor;

    bool m_brightness;
};

}  // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // PROPERTYEDITOR_H
