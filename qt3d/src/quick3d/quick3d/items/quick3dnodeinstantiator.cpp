// Copyright (C) 2013 Research In Motion.
// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "quick3dnodeinstantiator_p.h"

#include <QtQml/QQmlContext>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlError>
#include <QtQml/QQmlInfo>
#include <QQmlIncubator>

#include <Qt3DCore/private/qnode_p.h>
#include <private/qqmlchangeset_p.h>
#if QT_CONFIG(qml_delegate_model)
#include <private/qqmldelegatemodel_p.h>
#endif
#include <private/qqmlobjectmodel_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DCore {
namespace Quick {

class Quick3DNodeInstantiatorPrivate : public QNodePrivate
{
    Q_DECLARE_PUBLIC(Quick3DNodeInstantiator)

public:
    Quick3DNodeInstantiatorPrivate();
    ~Quick3DNodeInstantiatorPrivate();

    void clear();
    void regenerate();
#if QT_CONFIG(qml_delegate_model)
    void makeModel();
#endif
    void _q_createdItem(int, QObject *);
    void _q_modelUpdated(const QQmlChangeSet &, bool);
    QObject *modelObject(int index, bool async);

    bool m_componentComplete:1;
    bool m_effectiveReset:1;
    bool m_active:1;
    bool m_async:1;
#if QT_CONFIG(qml_delegate_model)
    bool m_ownModel:1;
#endif
    int m_requestedIndex;
    QVariant m_model;
    QQmlInstanceModel *m_instanceModel;
    QQmlComponent *m_delegate;
    QList<QPointer<QObject>> m_objects;
};

/*!
    \internal
*/
Quick3DNodeInstantiatorPrivate::Quick3DNodeInstantiatorPrivate()
    : QNodePrivate()
    , m_componentComplete(true)
    , m_effectiveReset(false)
    , m_active(true)
    , m_async(false)
#if QT_CONFIG(qml_delegate_model)
    , m_ownModel(false)
#endif
    , m_requestedIndex(-1)
    , m_model(QVariant(1))
    , m_instanceModel(0)
    , m_delegate(0)
{
}

Quick3DNodeInstantiatorPrivate::~Quick3DNodeInstantiatorPrivate()
{
#if QT_CONFIG(qml_delegate_model)
    if (m_ownModel)
        delete m_instanceModel;
#endif
}

void Quick3DNodeInstantiatorPrivate::clear()
{
    Q_Q(Quick3DNodeInstantiator);
    if (!m_instanceModel)
        return;
    if (!m_objects.size())
        return;

    for (int i = 0; i < m_objects.size(); i++) {
        emit q->objectRemoved(i, m_objects[i]);
        m_instanceModel->release(m_objects[i]);
    }
    m_objects.clear();
    emit q->objectChanged();
}

QObject *Quick3DNodeInstantiatorPrivate::modelObject(int index, bool async)
{
    m_requestedIndex = index;
    QObject *o = m_instanceModel->object(index, async ? QQmlIncubator::Asynchronous : QQmlIncubator::AsynchronousIfNested);
    m_requestedIndex = -1;
    return o;
}

void Quick3DNodeInstantiatorPrivate::regenerate()
{
    Q_Q(Quick3DNodeInstantiator);
    if (!m_componentComplete)
        return;

    int prevCount = q->count();

    clear();

    if (!m_active || !m_instanceModel || !m_instanceModel->count() || !m_instanceModel->isValid()) {
        if (prevCount)
            emit q->countChanged();
        return;
    }

    for (int i = 0; i < m_instanceModel->count(); i++) {
        QObject *object = modelObject(i, m_async);
        // If the item was already created we won't get a createdItem
        if (object)
            _q_createdItem(i, object);
    }
    if (q->count() != prevCount)
        emit q->countChanged();
}

void Quick3DNodeInstantiatorPrivate::_q_createdItem(int idx, QObject *item)
{
    Q_Q(Quick3DNodeInstantiator);
    if (m_objects.contains(item)) //Case when it was created synchronously in regenerate
        return;
    if (m_requestedIndex != idx) // Asynchronous creation, reference the object                                                                                             |
        (void)m_instanceModel->object(idx);
    static_cast<QNode *>(item)->setParent(q->parentNode());
    if (m_objects.size() < idx + 1) {
        int modelCount = m_instanceModel->count();
        if (m_objects.capacity() < modelCount)
            m_objects.reserve(modelCount);
        m_objects.resize(idx + 1);
    }
    if (QObject *o = m_objects.at(idx))
        m_instanceModel->release(o);
    m_objects.replace(idx, item);

    if (m_objects.size() == 1)
        emit q->objectChanged();
    emit q->objectAdded(idx, item);
}

void Quick3DNodeInstantiatorPrivate::_q_modelUpdated(const QQmlChangeSet &changeSet, bool reset)
{
    Q_Q(Quick3DNodeInstantiator);

    if (!m_componentComplete || m_effectiveReset)
        return;

    if (reset) {
        regenerate();
        if (changeSet.difference() != 0)
            emit q->countChanged();
        return;
    }

    int difference = 0;
    QHash<int, QList<QPointer<QObject>>> moved;
    const auto removes = changeSet.removes();
    for (const QQmlChangeSet::Change &remove : removes) {
        int index = qMin(remove.index, m_objects.size());
        int count = qMin(remove.index + remove.count, m_objects.size()) - index;
        if (remove.isMove()) {
            moved.insert(remove.moveId, m_objects.mid(index, count));
            m_objects.erase(
                    m_objects.begin() + index,
                    m_objects.begin() + index + count);
        } else {
            while (count--) {
                QObject *obj = m_objects.at(index);
                m_objects.removeAt(index);
                emit q->objectRemoved(index, obj);
                if (obj)
                    m_instanceModel->release(obj);
            }
        }

        difference -= remove.count;
    }

    const auto inserts = changeSet.inserts();
    for (const QQmlChangeSet::Change &insert : inserts) {
        int index = qMin(insert.index, m_objects.size());
        if (insert.isMove()) {
            QList<QPointer<QObject>> movedObjects = moved.value(insert.moveId);
            m_objects = m_objects.mid(0, index) + movedObjects + m_objects.mid(index);
        } else for (int i = 0; i < insert.count; ++i) {
            if (insert.index <= m_objects.size())
                m_objects.insert(insert.index, insert.count, nullptr);
            for (int i = 0; i < insert.count; ++i) {
                int modelIndex = index + i;
                QObject *obj = modelObject(modelIndex, m_async);
                if (obj)
                    _q_createdItem(modelIndex, obj);
            }
        }
        difference += insert.count;
    }

    if (difference != 0)
        emit q->countChanged();
}

#if QT_CONFIG(qml_delegate_model)
void Quick3DNodeInstantiatorPrivate::makeModel()
{
    Q_Q(Quick3DNodeInstantiator);
    QQmlDelegateModel* delegateModel = new QQmlDelegateModel(qmlContext(q));
    m_instanceModel = delegateModel;
    m_ownModel = true;
    delegateModel->setDelegate(m_delegate);
    delegateModel->classBegin(); //Pretend it was made in QML
    if (m_componentComplete)
        delegateModel->componentComplete();
}
#endif

/*!
    \qmltype NodeInstantiator
    \inqmlmodule Qt3D.Core
    \brief Dynamically creates nodes.
    \since 5.5

    A NodeInstantiator can be used to control the dynamic creation of nodes,
    or to dynamically create multiple objects from a template.

    The NodeInstantiator element will manage the objects it creates. Those
    objects are parented to the Instantiator and can also be deleted by the
    NodeInstantiator if the NodeInstantiator's properties change. Nodes can
    also be destroyed dynamically through other means, and the NodeInstantiator
    will not recreate them unless the properties of the NodeInstantiator
    change.

*/
Quick3DNodeInstantiator::Quick3DNodeInstantiator(QNode *parent)
    : QNode(*new Quick3DNodeInstantiatorPrivate, parent)
{
    connect(this, &QNode::parentChanged, this, &Quick3DNodeInstantiator::onParentChanged);
}

/*!
    \qmlsignal Qt3D.Core::NodeInstantiator::objectAdded(int index, QtObject object)

    This signal is emitted when a node is added to the NodeInstantiator. The \a index
    parameter holds the index which the node has been given, and the \a object
    parameter holds the \l Node that has been added.

    The corresponding handler is \c onNodeAdded.
*/

/*!
    \qmlsignal Qt3D.Core::NodeInstantiator::objectRemoved(int index, QtObject object)

    This signal is emitted when an object is removed from the Instantiator. The \a index
    parameter holds the index which the object had been given, and the \a object
    parameter holds the \l [QML] {QtQml::}{QtObject} that has been removed.

    Do not keep a reference to \a object if it was created by this Instantiator, as
    in these cases it will be deleted shortly after the signal is handled.

    The corresponding handler is \c onObjectRemoved.
*/
/*!
    \qmlproperty bool Qt3D.Core::NodeInstantiator::active

    When active is \c true, and the delegate component is ready, the Instantiator will
    create objects according to the model. When active is \c false, no objects
    will be created and any previously created objects will be destroyed.

    Default is \c true.
*/
bool Quick3DNodeInstantiator::isActive() const
{
    Q_D(const Quick3DNodeInstantiator);
    return d->m_active;
}

void Quick3DNodeInstantiator::setActive(bool newVal)
{
    Q_D(Quick3DNodeInstantiator);
    if (newVal == d->m_active)
        return;
    d->m_active = newVal;
    emit activeChanged();
    d->regenerate();
}

/*!
    \qmlproperty bool Qt3D.Core::NodeInstantiator::asynchronous

    When asynchronous is true the Instantiator will attempt to create objects
    asynchronously. This means that objects may not be available immediately,
    even if active is set to true.

    You can use the objectAdded signal to respond to items being created.

    Default is \c false.
*/
bool Quick3DNodeInstantiator::isAsync() const
{
    Q_D(const Quick3DNodeInstantiator);
    return d->m_async;
}

void Quick3DNodeInstantiator::setAsync(bool newVal)
{
    Q_D(Quick3DNodeInstantiator);
    if (newVal == d->m_async)
        return;
    d->m_async = newVal;
    emit asynchronousChanged();
}


/*!
    \qmlproperty int Qt3D.Core::NodeInstantiator::count
    \readonly

    The number of objects the Instantiator is currently managing.
*/

int Quick3DNodeInstantiator::count() const
{
    Q_D(const Quick3DNodeInstantiator);
    return d->m_objects.size();
}

/*!
    \qmlproperty QtQml::Component Qt3D.Core::NodeInstantiator::delegate
    \qmldefault

    The component used to create all objects.

    Note that an extra variable, index, will be available inside instances of the
    delegate. This variable refers to the index of the instance inside the Instantiator,
    and can be used to obtain the object through the itemAt method of the Instantiator.

    If this property is changed, all instances using the old delegate will be destroyed
    and new instances will be created using the new delegate.
*/
QQmlComponent *Quick3DNodeInstantiator::delegate()
{
    Q_D(Quick3DNodeInstantiator);
    return d->m_delegate;
}

void Quick3DNodeInstantiator::setDelegate(QQmlComponent *c)
{
    Q_D(Quick3DNodeInstantiator);
    if (c == d->m_delegate)
        return;

    d->m_delegate = c;
    emit delegateChanged();

#if QT_CONFIG(qml_delegate_model)
    if (!d->m_ownModel)
        return;

    if (QQmlDelegateModel *dModel = qobject_cast<QQmlDelegateModel*>(d->m_instanceModel))
        dModel->setDelegate(c);
#endif
    if (d->m_componentComplete)
        d->regenerate();
}

/*!
    \qmlproperty variant Qt3D.Core::NodeInstantiator::model

    This property can be set to any of the supported \l {qml-data-models}{data models}:

    \list
    \li A number that indicates the number of delegates to be created by the repeater
    \li A model (for example, a ListModel item or a QAbstractItemModel subclass)
    \li A string list
    \li An object list
    \endlist

    The type of model affects the properties that are exposed to the \l delegate.

    Default value is 1, which creates a single delegate instance.

    \sa {qml-data-models}{Data Models}
*/

QVariant Quick3DNodeInstantiator::model() const
{
    Q_D(const Quick3DNodeInstantiator);
    return d->m_model;
}

void Quick3DNodeInstantiator::setModel(const QVariant &v)
{
    Q_D(Quick3DNodeInstantiator);
    if (d->m_model == v)
        return;

    d->m_model = v;
    //Don't actually set model until componentComplete in case it wants to create its delegates immediately
    if (!d->m_componentComplete)
        return;

    QQmlInstanceModel *prevModel = d->m_instanceModel;
    QObject *object = qvariant_cast<QObject*>(v);
    QQmlInstanceModel *vim = 0;
    if (object && (vim = qobject_cast<QQmlInstanceModel *>(object))) {
#if QT_CONFIG(qml_delegate_model)
        if (d->m_ownModel) {
            delete d->m_instanceModel;
            prevModel = 0;
            d->m_ownModel = false;
        }
#endif
        d->m_instanceModel = vim;
    }
#if QT_CONFIG(qml_delegate_model)
    else if (v != QVariant(0)) {
        if (!d->m_ownModel)
            d->makeModel();

        if (QQmlDelegateModel *dataModel = qobject_cast<QQmlDelegateModel *>(d->m_instanceModel)) {
            d->m_effectiveReset = true;
            dataModel->setModel(v);
            d->m_effectiveReset = false;
        }
    }
#endif

    if (d->m_instanceModel != prevModel) {
        if (prevModel) {
            disconnect(prevModel, SIGNAL(modelUpdated(QQmlChangeSet,bool)),
                    this, SLOT(_q_modelUpdated(QQmlChangeSet,bool)));
            disconnect(prevModel, SIGNAL(createdItem(int,QObject*)), this, SLOT(_q_createdItem(int,QObject*)));
            //disconnect(prevModel, SIGNAL(initItem(int,QObject*)), this, SLOT(initItem(int,QObject*)));
        }

        connect(d->m_instanceModel, SIGNAL(modelUpdated(QQmlChangeSet,bool)),
                this, SLOT(_q_modelUpdated(QQmlChangeSet,bool)));
        connect(d->m_instanceModel, SIGNAL(createdItem(int,QObject*)), this, SLOT(_q_createdItem(int,QObject*)));
        //connect(d->m_instanceModel, SIGNAL(initItem(int,QObject*)), this, SLOT(initItem(int,QObject*)));
    }

    d->regenerate();
    emit modelChanged();
}

/*!
    \qmlproperty QtQml::QtObject Qt3D.Core::NodeInstantiator::object
    \readonly

    This is a reference to the first created object, intended as a convenience
    for the case where only one object has been created.
*/
QObject *Quick3DNodeInstantiator::object() const
{
    Q_D(const Quick3DNodeInstantiator);
    if (d->m_objects.size())
        return d->m_objects[0];
    return 0;
}

/*!
    \qmlmethod QtQml::QtObject Qt3D.Core::NodeInstantiator::objectAt(int index)

    Returns a reference to the object with the given \a index.
*/
QObject *Quick3DNodeInstantiator::objectAt(int index) const
{
    Q_D(const Quick3DNodeInstantiator);
    if (index >= 0 && index < d->m_objects.size())
        return d->m_objects[index];
    return 0;
}

/*!
 \internal
*/
void Quick3DNodeInstantiator::classBegin()
{
    Q_D(Quick3DNodeInstantiator);
    d->m_componentComplete = false;
}

/*!
 \internal
*/
void Quick3DNodeInstantiator::componentComplete()
{
    Q_D(Quick3DNodeInstantiator);
    d->m_componentComplete = true;
#if QT_CONFIG(qml_delegate_model)
    if (d->m_ownModel) {
        static_cast<QQmlDelegateModel *>(d->m_instanceModel)->componentComplete();
        d->regenerate();
    } else
#endif
    {
        QVariant realModel = d->m_model;
        d->m_model = QVariant(0);
        setModel(realModel); //If realModel == d->m_model this won't do anything, but that's fine since the model's 0
        //setModel calls regenerate
    }
}

/*!
 \internal
*/
void Quick3DNodeInstantiator::onParentChanged(QObject *parent)
{
    Q_D(const Quick3DNodeInstantiator);
    auto parentNode = static_cast<QNode *>(parent);
    for (auto obj : d->m_objects)
        static_cast<QNode *>(obj.data())->setParent(parentNode);
}

// TODO: Avoid cloning here
//void Quick3DNodeInstantiator::copy(const QNode *ref)
//{
//    QNode::copy(ref);
//    const Quick3DNodeInstantiator *instantiator = static_cast<const Quick3DNodeInstantiator*>(ref);
//    // We only need to clone the children as the instantiator itself has no
//    // corresponding backend node type.
//    for (int i = 0; i < instantiator->d_func()->m_objects.size(); ++i) {
//        QNode *n = qobject_cast<QNode *>(instantiator->d_func()->m_objects.at(i));
//        if (!n)
//            continue;
//        QNode *clonedNode = QNode::clone(n);
//        clonedNode->setParent(this);
//        d_func()->m_objects.append(clonedNode);
//    }
//}

} // namespace Quick
} // namespace Qt3DCore

QT_END_NAMESPACE

#include "moc_quick3dnodeinstantiator_p.cpp"
