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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef OBJECTINSPECTORMODEL_H
#define OBJECTINSPECTORMODEL_H

#include <layoutinfo_p.h>

#include <QtGui/qstandarditemmodel.h>
#include <QtGui/qicon.h>
#include <QtCore/qstring.h>
#include <QtCore/qlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QDesignerFormWindowInterface;

namespace qdesigner_internal {

    // Data structure containing the fixed item type icons
    struct ObjectInspectorIcons {
        QIcon layoutIcons[LayoutInfo::UnknownLayout + 1];
    };

    struct ModelRecursionContext;

    // Data structure representing one item of the object inspector.
    class ObjectData {
    public:
        enum Type {
            Object,
            Action,
            SeparatorAction,
            ChildWidget,         // A child widget
            LayoutableContainer, // A container that can be laid out
            LayoutWidget,        // A QLayoutWidget
            ExtensionContainer   // QTabWidget and the like, container extension
        };

        typedef QList<QStandardItem *> StandardItemList;

        explicit ObjectData(QObject *parent, QObject *object, const ModelRecursionContext &ctx);
        ObjectData();

        inline Type     type()       const { return m_type; }
        inline QObject *object()     const { return m_object; }
        inline QObject *parent()     const { return m_parent; }
        inline QString  objectName() const { return m_objectName; }

        bool equals(const ObjectData & me) const;

        enum ChangedMask { ClassNameChanged = 1, ObjectNameChanged = 2,
                           ClassIconChanged = 4, TypeChanged = 8,
                           LayoutTypeChanged = 16};

        unsigned compare(const ObjectData & me) const;

        // Initially set up a row
        void setItems(const StandardItemList &row, const ObjectInspectorIcons &icons) const;
        // Update row data according to change mask
        void setItemsDisplayData(const StandardItemList &row, const ObjectInspectorIcons &icons, unsigned mask) const;

    private:
        void initObject(const ModelRecursionContext &ctx);
        void initWidget(QWidget *w, const ModelRecursionContext &ctx);

        QObject *m_parent;
        QObject *m_object;
        Type m_type;
        QString m_className;
        QString m_objectName;
        QIcon m_classIcon;
        LayoutInfo::Type m_managedLayoutType;
    };

    inline bool operator==(const ObjectData &e1, const ObjectData &e2) { return e1.equals(e2); }
    inline bool operator!=(const ObjectData &e1, const ObjectData &e2) { return !e1.equals(e2); }

    typedef QList<ObjectData> ObjectModel;

    // QStandardItemModel for ObjectInspector. Uses ObjectData/ObjectModel
    // internally for its updates.
    class ObjectInspectorModel : public QStandardItemModel {
    public:
        typedef QList<QStandardItem *> StandardItemList;
        enum { ObjectNameColumn, ClassNameColumn, NumColumns };

        explicit ObjectInspectorModel(QObject *parent);

        enum UpdateResult { NoForm, Rebuilt, Updated };
        UpdateResult update(QDesignerFormWindowInterface *fw);

        const QModelIndexList indexesOf(QObject *o) const { return m_objectIndexMultiMap.values(o); }
        QObject *objectAt(const QModelIndex &index) const;

        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    private:
        typedef QMultiMap<QObject *,QModelIndex> ObjectIndexMultiMap;

        void rebuild(const ObjectModel &newModel);
        void updateItemContents(ObjectModel &oldModel, const ObjectModel &newModel);
        void clearItems();
        StandardItemList rowAt(QModelIndex index) const;

        ObjectInspectorIcons m_icons;
        ObjectIndexMultiMap m_objectIndexMultiMap;
        ObjectModel m_model;
        QPointer<QDesignerFormWindowInterface> m_formWindow;
    };
}  // namespace qdesigner_internal

#endif // OBJECTINSPECTORMODEL_H

QT_END_NAMESPACE
