// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDATAMODEL_P_H
#define QQMLDATAMODEL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qtqmlmodelsglobal_p.h>
#include <private/qqmllistcompositor_p.h>
#include <private/qqmlobjectmodel_p.h>
#include <private/qqmlincubator_p.h>

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qstringlist.h>

QT_REQUIRE_CONFIG(qml_delegate_model);

QT_BEGIN_NAMESPACE

class QQmlChangeSet;
class QQuickPackage;
class QQmlV4Function;
class QQmlDelegateModelGroup;
class QQmlDelegateModelAttached;
class QQmlDelegateModelPrivate;


class Q_QMLMODELS_PRIVATE_EXPORT QQmlDelegateModel : public QQmlInstanceModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlDelegateModel)

    Q_PROPERTY(QVariant model READ model WRITE setModel)
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(QString filterOnGroup READ filterGroup WRITE setFilterGroup NOTIFY filterGroupChanged RESET resetFilterGroup)
    Q_PROPERTY(QQmlDelegateModelGroup *items READ items CONSTANT) //TODO : worth renaming?
    Q_PROPERTY(QQmlDelegateModelGroup *persistedItems READ persistedItems CONSTANT)
    Q_PROPERTY(QQmlListProperty<QQmlDelegateModelGroup> groups READ groups CONSTANT)
    Q_PROPERTY(QObject *parts READ parts CONSTANT)
    Q_PROPERTY(QVariant rootIndex READ rootIndex WRITE setRootIndex NOTIFY rootIndexChanged)
    Q_CLASSINFO("DefaultProperty", "delegate")
    QML_NAMED_ELEMENT(DelegateModel)
    QML_ADDED_IN_VERSION(2, 1)
    QML_ATTACHED(QQmlDelegateModelAttached)
    Q_INTERFACES(QQmlParserStatus)

public:
    QQmlDelegateModel();
    QQmlDelegateModel(QQmlContext *, QObject *parent=nullptr);
    ~QQmlDelegateModel();

    void classBegin() override;
    void componentComplete() override;

    QVariant model() const;
    void setModel(const QVariant &);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *);

    QVariant rootIndex() const;
    void setRootIndex(const QVariant &root);

    Q_INVOKABLE QVariant modelIndex(int idx) const;
    Q_INVOKABLE QVariant parentModelIndex() const;

    int count() const override;
    bool isValid() const override { return delegate() != nullptr; }
    QObject *object(int index, QQmlIncubator::IncubationMode incubationMode = QQmlIncubator::AsynchronousIfNested) override;
    ReleaseFlags release(QObject *object, ReusableFlag reusableFlag = NotReusable) override;
    void cancel(int index) override;
    QVariant variantValue(int index, const QString &role) override;
    void setWatchedRoles(const QList<QByteArray> &roles) override;
    QQmlIncubator::Status incubationStatus(int index) override;

    void drainReusableItemsPool(int maxPoolTime) override;
    int poolSize() override;

    int indexOf(QObject *object, QObject *objectContext) const override;

    QString filterGroup() const;
    void setFilterGroup(const QString &group);
    void resetFilterGroup();

    QQmlDelegateModelGroup *items();
    QQmlDelegateModelGroup *persistedItems();
    QQmlListProperty<QQmlDelegateModelGroup> groups();
    QObject *parts();

    const QAbstractItemModel *abstractItemModel() const override;

    bool event(QEvent *) override;

    static QQmlDelegateModelAttached *qmlAttachedProperties(QObject *obj);

Q_SIGNALS:
    void filterGroupChanged();
    void defaultGroupsChanged();
    void rootIndexChanged();
    void delegateChanged();

private Q_SLOTS:
    void _q_itemsChanged(int index, int count, const QVector<int> &roles);
    void _q_itemsInserted(int index, int count);
    void _q_itemsRemoved(int index, int count);
    void _q_itemsMoved(int from, int to, int count);
    void _q_modelAboutToBeReset();
    void _q_rowsInserted(const QModelIndex &,int,int);
    void _q_columnsInserted(const QModelIndex &, int, int);
    void _q_columnsRemoved(const QModelIndex &, int, int);
    void _q_columnsMoved(const QModelIndex &, int, int, const QModelIndex &, int);
    void _q_rowsAboutToBeRemoved(const QModelIndex &parent, int begin, int end);
    void _q_rowsRemoved(const QModelIndex &,int,int);
    void _q_rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int);
    void _q_dataChanged(const QModelIndex&,const QModelIndex&,const QVector<int> &);
    void _q_layoutChanged(const QList<QPersistentModelIndex>&, QAbstractItemModel::LayoutChangeHint);

private:
    void handleModelReset();
    bool isDescendantOf(const QPersistentModelIndex &desc, const QList<QPersistentModelIndex> &parents) const;

    Q_DISABLE_COPY(QQmlDelegateModel)
};

class QQmlDelegateModelGroupPrivate;
class Q_QMLMODELS_PRIVATE_EXPORT QQmlDelegateModelGroup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool includeByDefault READ defaultInclude WRITE setDefaultInclude NOTIFY defaultIncludeChanged)
    QML_NAMED_ELEMENT(DelegateModelGroup)
    QML_ADDED_IN_VERSION(2, 1)
public:
    QQmlDelegateModelGroup(QObject *parent = nullptr);
    QQmlDelegateModelGroup(const QString &name, QQmlDelegateModel *model, int compositorType, QObject *parent = nullptr);
    ~QQmlDelegateModelGroup();

    QString name() const;
    void setName(const QString &name);

    int count() const;

    bool defaultInclude() const;
    void setDefaultInclude(bool include);

    Q_INVOKABLE QJSValue get(int index);

public Q_SLOTS:
    void insert(QQmlV4Function *);
    void create(QQmlV4Function *);
    void resolve(QQmlV4Function *);
    void remove(QQmlV4Function *);
    void addGroups(QQmlV4Function *);
    void removeGroups(QQmlV4Function *);
    void setGroups(QQmlV4Function *);
    void move(QQmlV4Function *);

Q_SIGNALS:
    void countChanged();
    void nameChanged();
    void defaultIncludeChanged();
    void changed(const QJSValue &removed, const QJSValue &inserted);
private:
    Q_DECLARE_PRIVATE(QQmlDelegateModelGroup)
};

class QQmlDelegateModelItem;
class QQmlDelegateModelAttachedMetaObject;
class QQmlDelegateModelAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlDelegateModel *model READ model CONSTANT FINAL)
    Q_PROPERTY(QStringList groups READ groups WRITE setGroups NOTIFY groupsChanged FINAL)
    Q_PROPERTY(bool isUnresolved READ isUnresolved NOTIFY unresolvedChanged FINAL)
    Q_PROPERTY(bool inPersistedItems READ inPersistedItems WRITE setInPersistedItems NOTIFY groupsChanged FINAL)
    Q_PROPERTY(bool inItems READ inItems WRITE setInItems NOTIFY groupsChanged FINAL)
    Q_PROPERTY(int persistedItemsIndex READ persistedItemsIndex NOTIFY groupsChanged FINAL)
    Q_PROPERTY(int itemsIndex READ itemsIndex NOTIFY groupsChanged FINAL)

public:
    QQmlDelegateModelAttached(QObject *parent);
    QQmlDelegateModelAttached(QQmlDelegateModelItem *cacheItem, QObject *parent);
    ~QQmlDelegateModelAttached() {}

    void resetCurrentIndex();
    void setCacheItem(QQmlDelegateModelItem *item);

    void setInPersistedItems(bool inPersisted);
    bool inPersistedItems() const;
    int persistedItemsIndex() const;

    void setInItems(bool inItems);
    bool inItems() const;
    int itemsIndex() const;

    QQmlDelegateModel *model() const;

    QStringList groups() const;
    void setGroups(const QStringList &groups);

    bool isUnresolved() const;

    void emitChanges();

    void emitUnresolvedChanged() { Q_EMIT unresolvedChanged(); }

Q_SIGNALS:
    void groupsChanged();
    void unresolvedChanged();

private:
    void setInGroup(QQmlListCompositor::Group group, bool inGroup);

public:
    QQmlDelegateModelItem *m_cacheItem;
    int m_previousGroups;
    int m_currentIndex[QQmlListCompositor::MaximumGroupCount];
    int m_previousIndex[QQmlListCompositor::MaximumGroupCount];

    friend class QQmlDelegateModelAttachedMetaObject;
};

QT_END_NAMESPACE

#endif // QQMLDATAMODEL_P_H
