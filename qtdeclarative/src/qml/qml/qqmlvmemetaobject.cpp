/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 BasysKom GmbH.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qqmlvmemetaobject_p.h"


#include "qqml.h"
#include <private/qqmlrefcount_p.h>
#include "qqmlexpression.h"
#include "qqmlexpression_p.h"
#include "qqmlcontext_p.h"
#include "qqmlbinding_p.h"
#include "qqmlpropertyvalueinterceptor_p.h"
#include <qqmlinfo.h>

#include <private/qqmlglobal_p.h>

#include <private/qv4object_p.h>
#include <private/qv4variantobject_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4scopedvalue_p.h>
#include <private/qv4jscall_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qqmlpropertycachecreator_p.h>
#include <private/qqmlpropertycachemethodarguments_p.h>

#include <climits> // for CHAR_BIT

QT_BEGIN_NAMESPACE

class ResolvedList
{
    Q_DISABLE_COPY_MOVE(ResolvedList)

public:
    ResolvedList(QQmlListProperty<QObject> *prop)
    {
        // see QQmlVMEMetaObject::metaCall for how this was constructed
        auto encodedIndex = quintptr(prop->data);
        constexpr quintptr usableBits = sizeof(quintptr)  * CHAR_BIT;
        quintptr inheritanceDepth = encodedIndex >> (usableBits / 2);
        m_id = encodedIndex & ((quintptr(1) << (usableBits / 2)) - 1);

        // walk up to the correct meta object if necessary
        auto mo = prop->object->metaObject();
        while (inheritanceDepth--)
            mo = mo->superClass();
        m_metaObject = static_cast<QQmlVMEMetaObject *>(const_cast<QMetaObject *>(mo));
        Q_ASSERT(m_metaObject);
        Q_ASSERT( ::strstr(m_metaObject->property(m_metaObject->propOffset() + m_id).typeName(), "QQmlListProperty") );
        Q_ASSERT(m_metaObject->object == prop->object);

        // readPropertyAsList() with checks transformed into Q_ASSERT
        // and without allocation.
        if (m_metaObject->propertyAndMethodStorage.isUndefined() &&
                m_metaObject->propertyAndMethodStorage.valueRef()) {
            return;
        }

        if (auto *md = static_cast<QV4::MemberData *>(
                    m_metaObject->propertyAndMethodStorage.asManaged())) {
            const auto *v = (md->data() + m_id)->as<QV4::VariantObject>();
            Q_ASSERT(v);
            Q_ASSERT(v->d());
            QVariant &data = v->d()->data();
            Q_ASSERT(data.userType() == qMetaTypeId<QVector<QQmlGuard<QObject>>>());
            m_list = static_cast<QVector<QQmlGuard<QObject>> *>(data.data());
            Q_ASSERT(m_list);
        }
    }

    ~ResolvedList() = default;

    QQmlVMEMetaObject *metaObject() const { return m_metaObject; }
    QVector<QQmlGuard<QObject>> *list() const { return m_list; }
    quintptr id() const { return m_id; }

    void activateSignal() const
    {
        m_metaObject->activate(m_metaObject->object, int(m_id + m_metaObject->methodOffset()),
                               nullptr);
    }

private:
    QQmlVMEMetaObject *m_metaObject = nullptr;
    QVector<QQmlGuard<QObject>> *m_list = nullptr;
    quintptr m_id = 0;
};

static void list_append(QQmlListProperty<QObject> *prop, QObject *o)
{
    const ResolvedList resolved(prop);
    resolved.list()->append(o);
    resolved.activateSignal();
}

static int list_count(QQmlListProperty<QObject> *prop)
{
    return ResolvedList(prop).list()->count();
}

static QObject *list_at(QQmlListProperty<QObject> *prop, int index)
{
    return ResolvedList(prop).list()->at(index);
}

static void list_clear(QQmlListProperty<QObject> *prop)
{
    const ResolvedList resolved(prop);
    resolved.list()->clear();
    resolved.activateSignal();
}

static void list_replace(QQmlListProperty<QObject> *prop, int index, QObject *o)
{
    const ResolvedList resolved(prop);
    resolved.list()->replace(index, o);
    resolved.activateSignal();
}

static void list_removeLast(QQmlListProperty<QObject> *prop)
{
    const ResolvedList resolved(prop);
    resolved.list()->removeLast();
    resolved.activateSignal();
}

QQmlVMEVariantQObjectPtr::QQmlVMEVariantQObjectPtr()
    : QQmlGuard<QObject>(nullptr), m_target(nullptr), m_index(-1)
{
}

QQmlVMEVariantQObjectPtr::~QQmlVMEVariantQObjectPtr()
{
}

void QQmlVMEVariantQObjectPtr::objectDestroyed(QObject *)
{
    if (!m_target || QQmlData::wasDeleted(m_target->object))
        return;

    if (m_index >= 0) {
        QV4::ExecutionEngine *v4 = m_target->propertyAndMethodStorage.engine();
        if (v4) {
            QV4::Scope scope(v4);
            QV4::Scoped<QV4::MemberData> sp(scope, m_target->propertyAndMethodStorage.value());
            if (sp) {
                QV4::PropertyIndex index{ sp->d(), sp->d()->values.values + m_index };
                index.set(v4, QV4::Value::nullValue());
            }
        }

        m_target->activate(m_target->object, m_target->methodOffset() + m_index, nullptr);
    }
}

void QQmlVMEVariantQObjectPtr::setGuardedValue(QObject *obj, QQmlVMEMetaObject *target, int index)
{
    m_target = target;
    m_index = index;
    setObject(obj);
}

class QQmlVMEMetaObjectEndpoint : public QQmlNotifierEndpoint
{
public:
    QQmlVMEMetaObjectEndpoint();
    void tryConnect();

    QFlagPointer<QQmlVMEMetaObject> metaObject;
};

QQmlVMEMetaObjectEndpoint::QQmlVMEMetaObjectEndpoint()
    : QQmlNotifierEndpoint(QQmlNotifierEndpoint::QQmlVMEMetaObjectEndpoint)
{
}

void QQmlVMEMetaObjectEndpoint_callback(QQmlNotifierEndpoint *e, void **)
{
    QQmlVMEMetaObjectEndpoint *vmee = static_cast<QQmlVMEMetaObjectEndpoint*>(e);
    vmee->tryConnect();
}

void QQmlVMEMetaObjectEndpoint::tryConnect()
{
    Q_ASSERT(metaObject->compiledObject);
    int aliasId = this - metaObject->aliasEndpoints;

    if (metaObject.flag()) {
        // This is actually notify
        int sigIdx = metaObject->methodOffset() + aliasId + metaObject->compiledObject->nProperties;
        metaObject->activate(metaObject->object, sigIdx, nullptr);
    } else {
        const QV4::CompiledData::Alias *aliasData = &metaObject->compiledObject->aliasTable()[aliasId];
        if (!aliasData->isObjectAlias()) {
            QQmlContextData *ctxt = metaObject->ctxt;
            QObject *target = ctxt->idValues[aliasData->targetObjectId].data();
            if (!target)
                return;

            QQmlData *targetDData = QQmlData::get(target, /*create*/false);
            if (!targetDData)
                return;
            QQmlPropertyIndex encodedIndex = QQmlPropertyIndex::fromEncoded(aliasData->encodedMetaPropertyIndex);
            int coreIndex = encodedIndex.coreIndex();
            int valueTypeIndex = encodedIndex.valueTypeIndex();
            const QQmlPropertyData *pd = targetDData->propertyCache->property(coreIndex);
            if (pd && valueTypeIndex != -1 && !QQmlValueTypeFactory::valueType(pd->propType())) {
                // deep alias
                QQmlEnginePrivate *enginePriv = QQmlEnginePrivate::get(metaObject->compilationUnit->engine->qmlEngine());
                auto const *newPropertyCache = enginePriv->propertyCacheForType(pd->propType());
                void *argv[1] = { &target };
                QMetaObject::metacall(target, QMetaObject::ReadProperty, coreIndex, argv);
                Q_ASSERT(newPropertyCache);
                pd = newPropertyCache->property(valueTypeIndex);
            }
            if (!pd)
                return;

            if (pd->notifyIndex() != -1)
                connect(target, pd->notifyIndex(), ctxt->engine);
        }

        metaObject.setFlag();
    }
}


QQmlInterceptorMetaObject::QQmlInterceptorMetaObject(QObject *obj, const QQmlRefPointer<QQmlPropertyCache> &cache)
    : object(obj),
      cache(cache),
      interceptors(nullptr),
      hasAssignedMetaObjectData(false)
{
    QObjectPrivate *op = QObjectPrivate::get(obj);

    if (op->metaObject) {
        parent = op->metaObject;
        // Use the extra flag in QBiPointer to know if we can safely cast parent.asT1() to QQmlVMEMetaObject*
        parent.setFlagValue(QQmlData::get(obj)->hasVMEMetaObject);
    } else {
        parent = obj->metaObject();
    }

    op->metaObject = this;
    QQmlData::get(obj)->hasInterceptorMetaObject = true;
}

QQmlInterceptorMetaObject::~QQmlInterceptorMetaObject()
{

}

void QQmlInterceptorMetaObject::registerInterceptor(QQmlPropertyIndex index, QQmlPropertyValueInterceptor *interceptor)
{
    interceptor->m_propertyIndex = index;
    interceptor->m_next = interceptors;
    interceptors = interceptor;
}

int QQmlInterceptorMetaObject::metaCall(QObject *o, QMetaObject::Call c, int id, void **a)
{
    Q_ASSERT(o == object);
    Q_UNUSED(o);

    if (intercept(c, id, a))
        return -1;
    return object->qt_metacall(c, id, a);
}

bool QQmlInterceptorMetaObject::intercept(QMetaObject::Call c, int id, void **a)
{
    if (c == QMetaObject::WriteProperty && interceptors &&
       !(*reinterpret_cast<int*>(a[3]) & QQmlPropertyData::BypassInterceptor)) {

        for (QQmlPropertyValueInterceptor *vi = interceptors; vi; vi = vi->m_next) {
            if (vi->m_propertyIndex.coreIndex() != id)
                continue;

            const int valueIndex = vi->m_propertyIndex.valueTypeIndex();
            const QQmlData *data = QQmlData::get(object);
            const int type = data->propertyCache->property(id)->propType();

            if (type != QMetaType::UnknownType) {
                if (valueIndex != -1) {
                    QQmlGadgetPtrWrapper *valueType = QQmlGadgetPtrWrapper::instance(
                                data->context->engine, type);
                    Q_ASSERT(valueType);

                    //
                    // Consider the following case:
                    //  color c = { 0.1, 0.2, 0.3 }
                    //  interceptor exists on c.r
                    //  write { 0.2, 0.4, 0.6 }
                    //
                    // The interceptor may choose not to update the r component at this
                    // point (for example, a behavior that creates an animation). But we
                    // need to ensure that the g and b components are updated correctly.
                    //
                    // So we need to perform a full write where the value type is:
                    //    r = old value, g = new value, b = new value
                    //
                    // And then call the interceptor which may or may not write the
                    // new value to the r component.
                    //
                    // This will ensure that the other components don't contain stale data
                    // and any relevant signals are emitted.
                    //
                    // To achieve this:
                    //   (1) Store the new value type as a whole (needed due to
                    //       aliasing between a[0] and static storage in value type).
                    //   (2) Read the entire existing value type from object -> valueType temp.
                    //   (3) Read the previous value of the component being changed
                    //       from the valueType temp.
                    //   (4) Write the entire new value type into the temp.
                    //   (5) Overwrite the component being changed with the old value.
                    //   (6) Perform a full write to the value type (which may emit signals etc).
                    //   (7) Issue the interceptor call with the new component value.
                    //

                    QMetaProperty valueProp = valueType->property(valueIndex);
                    QVariant newValue(type, a[0]);

                    valueType->read(object, id);
                    QVariant prevComponentValue = valueProp.read(valueType);

                    valueType->setValue(newValue);
                    QVariant newComponentValue = valueProp.read(valueType);

                    // Don't apply the interceptor if the intercepted value has not changed
                    bool updated = false;
                    if (newComponentValue != prevComponentValue) {
                        valueProp.write(valueType, prevComponentValue);
                        valueType->write(object, id, QQmlPropertyData::DontRemoveBinding | QQmlPropertyData::BypassInterceptor);

                        vi->write(newComponentValue);
                        updated = true;
                    }

                    if (updated)
                        return true;
                } else {
                    vi->write(QVariant(type, a[0]));
                    return true;
                }
            }
        }
    }
    return false;
}


QAbstractDynamicMetaObject *QQmlInterceptorMetaObject::toDynamicMetaObject(QObject *o)
{
    if (!hasAssignedMetaObjectData) {
        *static_cast<QMetaObject *>(this) = *cache->createMetaObject();

        if (parent.isT1())
            this->d.superdata = parent.asT1()->toDynamicMetaObject(o);
        else
            this->d.superdata = parent.asT2();

        hasAssignedMetaObjectData = true;
    }

    return this;
}

QQmlVMEMetaObject::QQmlVMEMetaObject(QV4::ExecutionEngine *engine,
                                     QObject *obj,
                                     const QQmlRefPointer<QQmlPropertyCache> &cache, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &qmlCompilationUnit, int qmlObjectId)
    : QQmlInterceptorMetaObject(obj, cache),
      engine(engine),
      ctxt(QQmlData::get(obj, true)->outerContext),
      aliasEndpoints(nullptr), compilationUnit(qmlCompilationUnit), compiledObject(nullptr)
{
    Q_ASSERT(engine);
    QQmlData::get(obj)->hasVMEMetaObject = true;

    if (compilationUnit && qmlObjectId >= 0) {
        compiledObject = compilationUnit->objectAt(qmlObjectId);

        if (compiledObject->nProperties || compiledObject->nFunctions) {
            uint size = compiledObject->nProperties + compiledObject->nFunctions;
            if (size) {
                QV4::Heap::MemberData *data = QV4::MemberData::allocate(engine, size);
                propertyAndMethodStorage.set(engine, data);
                std::fill(data->values.values, data->values.values + data->values.size, QV4::Encode::undefined());
            }

            // Need JS wrapper to ensure properties/methods are marked.
            ensureQObjectWrapper();
        }
    }
}

QQmlVMEMetaObject::~QQmlVMEMetaObject()
{
    if (parent.isT1()) parent.asT1()->objectDestroyed(object);
    delete [] aliasEndpoints;

    qDeleteAll(varObjectGuards);
}

QV4::MemberData *QQmlVMEMetaObject::propertyAndMethodStorageAsMemberData() const
{
    if (propertyAndMethodStorage.isUndefined()) {
        if (propertyAndMethodStorage.valueRef())
            // in some situations, the QObject wrapper (and associated data,
            // such as the varProperties array) will have been cleaned up, but the
            // QObject ptr will not yet have been deleted (eg, waiting on deleteLater).
            // In this situation, return 0.
            return nullptr;
    }

    return static_cast<QV4::MemberData*>(propertyAndMethodStorage.asManaged());
}

void QQmlVMEMetaObject::writeProperty(int id, int v)
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (md)
        md->set(engine, id, QV4::Value::fromInt32(v));
}

void QQmlVMEMetaObject::writeProperty(int id, bool v)
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (md)
        md->set(engine, id, QV4::Value::fromBoolean(v));
}

void QQmlVMEMetaObject::writeProperty(int id, double v)
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (md)
        md->set(engine, id, QV4::Value::fromDouble(v));
}

void QQmlVMEMetaObject::writeProperty(int id, const QString& v)
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (md) {
        QV4::Scope scope(engine);
        QV4::Scoped<QV4::MemberData>(scope, md)->set(engine, id, engine->newString(v));
    }
}

void QQmlVMEMetaObject::writeProperty(int id, QObject* v)
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (md) {
        QV4::Scope scope(engine);
        QV4::Scoped<QV4::MemberData>(scope, md)->set(engine, id, QV4::Value::fromReturnedValue(
                                                         QV4::QObjectWrapper::wrap(engine, v)));
    }

    QQmlVMEVariantQObjectPtr *guard = getQObjectGuardForProperty(id);
    if (v && !guard) {
        guard = new QQmlVMEVariantQObjectPtr();
        varObjectGuards.append(guard);
    }
    if (guard)
        guard->setGuardedValue(v, this, id);
}

int QQmlVMEMetaObject::readPropertyAsInt(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return 0;

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    if (!sv->isInt32())
        return 0;
    return sv->integerValue();
}

bool QQmlVMEMetaObject::readPropertyAsBool(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return false;

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    if (!sv->isBoolean())
        return false;
    return sv->booleanValue();
}

double QQmlVMEMetaObject::readPropertyAsDouble(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return 0.0;

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    if (!sv->isDouble())
        return 0.0;
    return sv->doubleValue();
}

QString QQmlVMEMetaObject::readPropertyAsString(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return QString();

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    if (QV4::String *s = sv->stringValue())
        return s->toQString();
    return QString();
}

QUrl QQmlVMEMetaObject::readPropertyAsUrl(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return QUrl();

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    const QV4::VariantObject *v = sv->as<QV4::VariantObject>();
    if (!v || v->d()->data().userType() != QMetaType::QUrl)
        return QUrl();
    return v->d()->data().value<QUrl>();
}

QDate QQmlVMEMetaObject::readPropertyAsDate(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return QDate();

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    const QV4::VariantObject *v = sv->as<QV4::VariantObject>();
    if (!v || v->d()->data().userType() != QMetaType::QDate)
        return QDate();
    return v->d()->data().value<QDate>();
}

QDateTime QQmlVMEMetaObject::readPropertyAsDateTime(int id)
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return QDateTime();

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    const QV4::VariantObject *v = sv->as<QV4::VariantObject>();
    if (!v || v->d()->data().userType() != QMetaType::QDateTime)
        return QDateTime();
    return v->d()->data().value<QDateTime>();
}

QSizeF QQmlVMEMetaObject::readPropertyAsSizeF(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return QSizeF();

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    const QV4::VariantObject *v = sv->as<QV4::VariantObject>();
    if (!v || v->d()->data().userType() != QMetaType::QSizeF)
        return QSizeF();
    return v->d()->data().value<QSizeF>();
}

QPointF QQmlVMEMetaObject::readPropertyAsPointF(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return QPointF();

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    const QV4::VariantObject *v = sv->as<QV4::VariantObject>();
    if (!v || v->d()->data().userType() != QMetaType::QPointF)
        return QPointF();
    return v->d()->data().value<QPointF>();
}

QObject* QQmlVMEMetaObject::readPropertyAsQObject(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return nullptr;

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    const QV4::QObjectWrapper *wrapper = sv->as<QV4::QObjectWrapper>();
    if (!wrapper)
        return nullptr;
    return wrapper->object();
}

QVector<QQmlGuard<QObject>> *QQmlVMEMetaObject::readPropertyAsList(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return nullptr;

    QV4::Scope scope(engine);
    QV4::Scoped<QV4::VariantObject> v(scope, *(md->data() + id));
    if (!v || (int)v->d()->data().userType() != qMetaTypeId<QVector<QQmlGuard<QObject>> >()) {
        QVariant variant(QVariant::fromValue(QVector<QQmlGuard<QObject>>()));
        v = engine->newVariantObject(variant);
        md->set(engine, id, v);
    }
    return static_cast<QVector<QQmlGuard<QObject>> *>(v->d()->data().data());
}

QRectF QQmlVMEMetaObject::readPropertyAsRectF(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return QRectF();

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    const QV4::VariantObject *v = sv->as<QV4::VariantObject>();
    if (!v || v->d()->data().userType() != QMetaType::QRectF)
        return QRectF();
    return v->d()->data().value<QRectF>();
}

#if defined(Q_OS_WINRT) && defined(_M_ARM)
#pragma optimize("", off)
#endif
int QQmlVMEMetaObject::metaCall(QObject *o, QMetaObject::Call c, int _id, void **a)
{
    Q_ASSERT(o == object);
    Q_UNUSED(o);

    int id = _id;

    if (intercept(c, _id, a))
        return -1;

    const int propertyCount = compiledObject ? int(compiledObject->nProperties) : 0;
    const int aliasCount = compiledObject ? int(compiledObject->nAliases) : 0;
    const int signalCount = compiledObject ? int(compiledObject->nSignals) : 0;
    const int methodCount = compiledObject ? int(compiledObject->nFunctions) : 0;

    if (c == QMetaObject::ReadProperty || c == QMetaObject::WriteProperty || c == QMetaObject::ResetProperty) {
        if (id >= propOffset()) {
            id -= propOffset();

            if (id < propertyCount) {
                const QV4::CompiledData::Property &property = compiledObject->propertyTable()[id];
                const QV4::CompiledData::BuiltinType t = property.builtinType();

                // the context can be null if accessing var properties from cpp after re-parenting an item.
                QQmlEnginePrivate *ep = (ctxt == nullptr || ctxt->engine == nullptr) ? nullptr : QQmlEnginePrivate::get(ctxt->engine);

                const int fallbackMetaType = QQmlPropertyCacheCreatorBase::metaTypeForPropertyType(t);

                if (c == QMetaObject::ReadProperty) {
                    switch (t) {
                    case QV4::CompiledData::BuiltinType::Int:
                        *reinterpret_cast<int *>(a[0]) = readPropertyAsInt(id);
                        break;
                    case QV4::CompiledData::BuiltinType::Bool:
                        *reinterpret_cast<bool *>(a[0]) = readPropertyAsBool(id);
                        break;
                    case QV4::CompiledData::BuiltinType::Real:
                        *reinterpret_cast<double *>(a[0]) = readPropertyAsDouble(id);
                        break;
                    case QV4::CompiledData::BuiltinType::String:
                        *reinterpret_cast<QString *>(a[0]) = readPropertyAsString(id);
                        break;
                    case QV4::CompiledData::BuiltinType::Url:
                        *reinterpret_cast<QUrl *>(a[0]) = readPropertyAsUrl(id);
                        break;
                    case QV4::CompiledData::BuiltinType::Date:
                        *reinterpret_cast<QDate *>(a[0]) = readPropertyAsDate(id);
                        break;
                    case QV4::CompiledData::BuiltinType::DateTime:
                        *reinterpret_cast<QDateTime *>(a[0]) = readPropertyAsDateTime(id);
                        break;
                    case QV4::CompiledData::BuiltinType::Rect:
                        *reinterpret_cast<QRectF *>(a[0]) = readPropertyAsRectF(id);
                        break;
                    case QV4::CompiledData::BuiltinType::Size:
                        *reinterpret_cast<QSizeF *>(a[0]) = readPropertyAsSizeF(id);
                        break;
                    case QV4::CompiledData::BuiltinType::Point:
                        *reinterpret_cast<QPointF *>(a[0]) = readPropertyAsPointF(id);
                        break;
                    case QV4::CompiledData::BuiltinType::Variant:
                        *reinterpret_cast<QVariant *>(a[0]) = readPropertyAsVariant(id);
                        break;
                    case QV4::CompiledData::BuiltinType::Font:
                    case QV4::CompiledData::BuiltinType::Time:
                    case QV4::CompiledData::BuiltinType::Color:
                    case QV4::CompiledData::BuiltinType::Vector2D:
                    case QV4::CompiledData::BuiltinType::Vector3D:
                    case QV4::CompiledData::BuiltinType::Vector4D:
                    case QV4::CompiledData::BuiltinType::Matrix4x4:
                    case QV4::CompiledData::BuiltinType::Quaternion:
                        Q_ASSERT(fallbackMetaType != QMetaType::UnknownType);
                        if (QV4::MemberData *md = propertyAndMethodStorageAsMemberData()) {
                            QVariant propertyAsVariant;
                            if (const QV4::VariantObject *v = (md->data() + id)->as<QV4::VariantObject>())
                                propertyAsVariant = v->d()->data();
                            QQml_valueTypeProvider()->readValueType(propertyAsVariant, a[0], fallbackMetaType);
                        }
                        break;
                    case QV4::CompiledData::BuiltinType::Var:
                        if (ep) {
                            *reinterpret_cast<QVariant *>(a[0]) = readPropertyAsVariant(id);
                        } else {
                            // if the context was disposed, we just return an invalid variant from read.
                            *reinterpret_cast<QVariant *>(a[0]) = QVariant();
                        }
                        break;
                    case QV4::CompiledData::BuiltinType::InvalidBuiltin:
                        if (property.isList) {
                            // when reading from the list, we need to find the correct MetaObject,
                            // namely this. However, obejct->metaObject might point to any MetaObject
                            // down the inheritance hierarchy, so we need to store how far we have
                            // to go down
                            // To do this, we encode the hierarchy depth together with the id of the
                            // property in a single quintptr, with the first half storing the depth
                            // and the second half storing the property id
                            auto mo = object->metaObject();
                            quintptr inheritanceDepth = 0u;
                            while (mo && mo != this) {
                                mo = mo->superClass();
                                ++inheritanceDepth;
                            }
                            constexpr quintptr usableBits = sizeof(quintptr)  * CHAR_BIT;
                            if (Q_UNLIKELY(inheritanceDepth >= (quintptr(1) << quintptr(usableBits / 2u) ) )) {
                                qmlWarning(object) << "Too many objects in inheritance hierarchy for list property";
                                return -1;
                            }
                            if (Q_UNLIKELY(quintptr(id) >= (quintptr(1) << quintptr(usableBits / 2) ) )) {
                                qmlWarning(object) << "Too many properties in object for list property";
                                return -1;
                            }
                            quintptr encodedIndex = (inheritanceDepth << (usableBits/2)) + id;


                            readPropertyAsList(id); // Initializes if necessary
                            *static_cast<QQmlListProperty<QObject> *>(a[0])
                                    = QQmlListProperty<QObject>(
                                        object, reinterpret_cast<void *>(quintptr(encodedIndex)),
                                        list_append, list_count, list_at,
                                        list_clear, list_replace, list_removeLast);
                        } else {
                            *reinterpret_cast<QObject **>(a[0]) = readPropertyAsQObject(id);
                        }
                    }

                } else if (c == QMetaObject::WriteProperty) {
                    bool needActivate = false;
                    switch (t) {
                    case QV4::CompiledData::BuiltinType::Int:
                        needActivate = *reinterpret_cast<int *>(a[0]) != readPropertyAsInt(id);
                        writeProperty(id, *reinterpret_cast<int *>(a[0]));
                        break;
                    case QV4::CompiledData::BuiltinType::Bool:
                        needActivate = *reinterpret_cast<bool *>(a[0]) != readPropertyAsBool(id);
                        writeProperty(id, *reinterpret_cast<bool *>(a[0]));
                        break;
                    case QV4::CompiledData::BuiltinType::Real:
                        needActivate = *reinterpret_cast<double *>(a[0]) != readPropertyAsDouble(id);
                        writeProperty(id, *reinterpret_cast<double *>(a[0]));
                        break;
                    case QV4::CompiledData::BuiltinType::String:
                        needActivate = *reinterpret_cast<QString *>(a[0]) != readPropertyAsString(id);
                        writeProperty(id, *reinterpret_cast<QString *>(a[0]));
                        break;
                    case QV4::CompiledData::BuiltinType::Url:
                        needActivate = *reinterpret_cast<QUrl *>(a[0]) != readPropertyAsUrl(id);
                        writeProperty(id, *reinterpret_cast<QUrl *>(a[0]));
                        break;
                    case QV4::CompiledData::BuiltinType::Date:
                        needActivate = *reinterpret_cast<QDate *>(a[0]) != readPropertyAsDate(id);
                        writeProperty(id, *reinterpret_cast<QDate *>(a[0]));
                        break;
                    case QV4::CompiledData::BuiltinType::DateTime:
                        needActivate = *reinterpret_cast<QDateTime *>(a[0]) != readPropertyAsDateTime(id);
                        writeProperty(id, *reinterpret_cast<QDateTime *>(a[0]));
                        break;
                    case QV4::CompiledData::BuiltinType::Rect:
                        needActivate = *reinterpret_cast<QRectF *>(a[0]) != readPropertyAsRectF(id);
                        writeProperty(id, *reinterpret_cast<QRectF *>(a[0]));
                        break;
                    case QV4::CompiledData::BuiltinType::Size:
                        needActivate = *reinterpret_cast<QSizeF *>(a[0]) != readPropertyAsSizeF(id);
                        writeProperty(id, *reinterpret_cast<QSizeF *>(a[0]));
                        break;
                    case QV4::CompiledData::BuiltinType::Point:
                        needActivate = *reinterpret_cast<QPointF *>(a[0]) != readPropertyAsPointF(id);
                        writeProperty(id, *reinterpret_cast<QPointF *>(a[0]));
                        break;
                    case QV4::CompiledData::BuiltinType::Variant:
                        writeProperty(id, *reinterpret_cast<QVariant *>(a[0]));
                        break;
                    case QV4::CompiledData::BuiltinType::Font:
                    case QV4::CompiledData::BuiltinType::Time:
                    case QV4::CompiledData::BuiltinType::Color:
                    case QV4::CompiledData::BuiltinType::Vector2D:
                    case QV4::CompiledData::BuiltinType::Vector3D:
                    case QV4::CompiledData::BuiltinType::Vector4D:
                    case QV4::CompiledData::BuiltinType::Matrix4x4:
                    case QV4::CompiledData::BuiltinType::Quaternion:
                        Q_ASSERT(fallbackMetaType != QMetaType::UnknownType);
                        if (QV4::MemberData *md = propertyAndMethodStorageAsMemberData()) {
                            const QV4::VariantObject *v = (md->data() + id)->as<QV4::VariantObject>();
                            if (!v) {
                                md->set(engine, id, engine->newVariantObject(QVariant()));
                                v = (md->data() + id)->as<QV4::VariantObject>();
                                QQml_valueTypeProvider()->initValueType(fallbackMetaType, v->d()->data());
                            }
                            needActivate = !QQml_valueTypeProvider()->equalValueType(fallbackMetaType, a[0], v->d()->data());
                            QQml_valueTypeProvider()->writeValueType(fallbackMetaType, a[0], v->d()->data());
                        }
                        break;
                    case QV4::CompiledData::BuiltinType::Var:
                        if (ep)
                            writeProperty(id, *reinterpret_cast<QVariant *>(a[0]));
                        break;
                    case QV4::CompiledData::BuiltinType::InvalidBuiltin:
                        if (property.isList) {
                            // Writing such a property is not supported. Content is added through the list property
                            // methods.
                        } else {
                            needActivate = *reinterpret_cast<QObject **>(a[0]) != readPropertyAsQObject(id);
                            writeProperty(id, *reinterpret_cast<QObject **>(a[0]));
                        }

                    }

                    if (needActivate)
                        activate(object, methodOffset() + id, nullptr);
                }

                return -1;
            }

            id -= propertyCount;

            if (id < aliasCount) {
                const QV4::CompiledData::Alias *aliasData = &compiledObject->aliasTable()[id];

                if ((aliasData->flags & QV4::CompiledData::Alias::AliasPointsToPointerObject) && c == QMetaObject::ReadProperty)
                        *reinterpret_cast<void **>(a[0]) = nullptr;

                if (!ctxt) return -1;

                while (aliasData->aliasToLocalAlias)
                    aliasData = &compiledObject->aliasTable()[aliasData->localAliasIndex];

                QQmlContext *context = ctxt->asQQmlContext();
                QQmlContextPrivate *ctxtPriv = QQmlContextPrivate::get(context);

                QObject *target = ctxtPriv->data->idValues[aliasData->targetObjectId].data();
                if (!target)
                    return -1;

                connectAlias(id);

                if (aliasData->isObjectAlias()) {
                    *reinterpret_cast<QObject **>(a[0]) = target;
                    return -1;
                }

                QQmlData *targetDData = QQmlData::get(target, /*create*/false);
                if (!targetDData)
                    return -1;

                QQmlPropertyIndex encodedIndex = QQmlPropertyIndex::fromEncoded(aliasData->encodedMetaPropertyIndex);
                int coreIndex = encodedIndex.coreIndex();
                const int valueTypePropertyIndex = encodedIndex.valueTypeIndex();

                // Remove binding (if any) on write
                if(c == QMetaObject::WriteProperty) {
                    int flags = *reinterpret_cast<int*>(a[3]);
                    if (flags & QQmlPropertyData::RemoveBindingOnAliasWrite) {
                        QQmlData *targetData = QQmlData::get(target);
                        if (targetData && targetData->hasBindingBit(coreIndex))
                            QQmlPropertyPrivate::removeBinding(target, encodedIndex);
                    }
                }

                if (valueTypePropertyIndex != -1) {
                    if (!targetDData->propertyCache)
                        return -1;
                    const QQmlPropertyData *pd = targetDData->propertyCache->property(coreIndex);
                    // Value type property or deep alias
                    QQmlGadgetPtrWrapper *valueType = QQmlGadgetPtrWrapper::instance(
                                ctxt->engine, pd->propType());
                    if (valueType) {
                        valueType->read(target, coreIndex);
                        int rv = QMetaObject::metacall(valueType, c, valueTypePropertyIndex, a);

                        if (c == QMetaObject::WriteProperty)
                            valueType->write(target, coreIndex, {});

                        return rv;
                    } else {
                        // deep alias
                        void *argv[1] = { &target };
                        QMetaObject::metacall(target, QMetaObject::ReadProperty, coreIndex, argv);
                        return QMetaObject::metacall(target, c, valueTypePropertyIndex, a);
                    }

                } else {
                    return QMetaObject::metacall(target, c, coreIndex, a);
                }

            }
            return -1;

        }

    } else if(c == QMetaObject::InvokeMetaMethod) {

        if (id >= methodOffset()) {

            id -= methodOffset();
            int plainSignals = signalCount + propertyCount + aliasCount;
            if (id < plainSignals) {
                activate(object, _id, a);
                return -1;
            }

            id -= plainSignals;

            if (id < methodCount) {
                QQmlEngine *engine = ctxt->engine;
                if (!engine)
                    return -1; // We can't run the method

                QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);
                QV4::ExecutionEngine *v4 = engine->handle();
                ep->referenceScarceResources(); // "hold" scarce resources in memory during evaluation.
                QV4::Scope scope(v4);


                QV4::ScopedFunctionObject function(scope, method(id));
                if (!function) {
                    // The function was not compiled.  There are some exceptional cases which the
                    // expression rewriter does not rewrite properly (e.g., \r-terminated lines
                    // are not rewritten correctly but this bug is deemed out-of-scope to fix for
                    // performance reasons; see QTBUG-24064) and thus compilation will have failed.
                    QQmlError e;
                    e.setDescription(QLatin1String("Exception occurred during compilation of "
                                                         "function: ")
                                     + QString::fromUtf8(QMetaObject::method(_id)
                                                         .methodSignature()));
                    ep->warning(e);
                    return -1; // The dynamic method with that id is not available.
                }

                auto methodData = cache->method(_id);
                auto arguments = methodData->hasArguments() ? methodData->arguments() : nullptr;

                const unsigned int parameterCount = (arguments && arguments->names) ? arguments->names->count() : 0;
                Q_ASSERT(parameterCount == function->formalParameterCount());

                QV4::JSCallData jsCallData(scope, parameterCount);
                *jsCallData->thisObject = v4->global();

                for (uint ii = 0; ii < parameterCount; ++ii) {
                    jsCallData->args[ii] = scope.engine->metaTypeToJS(arguments->arguments[ii + 1], a[ii + 1]);
                }

                const int returnType = methodData->propType();
                QV4::ScopedValue result(scope, function->call(jsCallData));
                if (scope.hasException()) {
                    QQmlError error = scope.engine->catchExceptionAsQmlError();
                    if (error.isValid())
                        ep->warning(error);
                    if (a[0]) {
                        QMetaType::destruct(returnType, a[0]);
                        QMetaType::construct(returnType, a[0], nullptr);
                    }
                } else {
                    if (a[0]) {
                        // When the return type is QVariant, JS objects are to be returned as QJSValue wrapped in
                        // QVariant.
                        if (returnType == QMetaType::QVariant)
                            *(QVariant *)a[0] = scope.engine->toVariant(result, 0);
                        else
                            scope.engine->metaTypeFromJS(result, returnType, a[0]);
                    }
                }

                ep->dereferenceScarceResources(); // "release" scarce resources if top-level expression evaluation is complete.
                return -1;
            }
            return -1;
        }
    }

    if (parent.isT1())
        return parent.asT1()->metaCall(object, c, _id, a);
    else
        return object->qt_metacall(c, _id, a);
}
#if defined(Q_OS_WINRT) && defined(_M_ARM)
#pragma optimize("", on)
#endif

QV4::ReturnedValue QQmlVMEMetaObject::method(int index) const
{
    if (!ctxt || !ctxt->isValid() || !compiledObject) {
        qWarning("QQmlVMEMetaObject: Internal error - attempted to evaluate a function in an invalid context");
        return QV4::Encode::undefined();
    }

    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return QV4::Encode::undefined();

    return (md->data() + index + compiledObject->nProperties)->asReturnedValue();
}

QV4::ReturnedValue QQmlVMEMetaObject::readVarProperty(int id) const
{
    Q_ASSERT(compiledObject && compiledObject->propertyTable()[id].builtinType() == QV4::CompiledData::BuiltinType::Var);

    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (md)
        return (md->data() + id)->asReturnedValue();
    return QV4::Value::undefinedValue().asReturnedValue();
}

QVariant QQmlVMEMetaObject::readPropertyAsVariant(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (md) {
        const QV4::QObjectWrapper *wrapper = (md->data() + id)->as<QV4::QObjectWrapper>();
        if (wrapper)
            return QVariant::fromValue(wrapper->object());
        const QV4::VariantObject *v = (md->data() + id)->as<QV4::VariantObject>();
        if (v)
            return v->d()->data();
        return engine->toVariant(*(md->data() + id), -1);
    }
    return QVariant();
}

void QQmlVMEMetaObject::writeVarProperty(int id, const QV4::Value &value)
{
    Q_ASSERT(compiledObject && compiledObject->propertyTable()[id].builtinType() == QV4::CompiledData::BuiltinType::Var);

    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return;

    // Importantly, if the current value is a scarce resource, we need to ensure that it
    // gets automatically released by the engine if no other references to it exist.
    const QV4::VariantObject *oldVariant = (md->data() + id)->as<QV4::VariantObject>();
    if (oldVariant)
        oldVariant->removeVmePropertyReference();

    QObject *valueObject = nullptr;
    QQmlVMEVariantQObjectPtr *guard = getQObjectGuardForProperty(id);

    // And, if the new value is a scarce resource, we need to ensure that it does not get
    // automatically released by the engine until no other references to it exist.
    if (QV4::VariantObject *v = const_cast<QV4::VariantObject*>(value.as<QV4::VariantObject>())) {
        v->addVmePropertyReference();
    } else if (QV4::QObjectWrapper *wrapper = const_cast<QV4::QObjectWrapper*>(value.as<QV4::QObjectWrapper>())) {
        // We need to track this QObject to signal its deletion
        valueObject = wrapper->object();

        // Do we already have a QObject guard for this property?
        if (valueObject && !guard) {
            guard = new QQmlVMEVariantQObjectPtr();
            varObjectGuards.append(guard);
        }
    }

    if (guard)
        guard->setGuardedValue(valueObject, this, id);

    // Write the value and emit change signal as appropriate.
    md->set(engine, id, value);
    activate(object, methodOffset() + id, nullptr);
}

void QQmlVMEMetaObject::writeProperty(int id, const QVariant &value)
{
    if (compiledObject && compiledObject->propertyTable()[id].builtinType() == QV4::CompiledData::BuiltinType::Var) {
        QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
        if (!md)
            return;

        // Importantly, if the current value is a scarce resource, we need to ensure that it
        // gets automatically released by the engine if no other references to it exist.
        const QV4::VariantObject *oldv = (md->data() + id)->as<QV4::VariantObject>();
        if (oldv)
            oldv->removeVmePropertyReference();

        // And, if the new value is a scarce resource, we need to ensure that it does not get
        // automatically released by the engine until no other references to it exist.
        QV4::Scope scope(engine);
        QV4::ScopedValue newv(scope, engine->fromVariant(value));
        QV4::Scoped<QV4::VariantObject> v(scope, newv);
        if (!!v)
            v->addVmePropertyReference();

        // Write the value and emit change signal as appropriate.
        QVariant currentValue = readPropertyAsVariant(id);
        md->set(engine, id, newv);
        if ((currentValue.userType() != value.userType() || currentValue != value))
            activate(object, methodOffset() + id, nullptr);
    } else {
        bool needActivate = false;
        if (value.userType() == QMetaType::QObjectStar) {
            QObject *o = *(QObject *const *)value.data();
            needActivate = readPropertyAsQObject(id) != o;  // TODO: still correct?
            writeProperty(id, o);
        } else {
            QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
            if (md) {
                const QV4::VariantObject *v = (md->data() + id)->as<QV4::VariantObject>();
                needActivate = (!v ||
                                 v->d()->data().userType() != value.userType() ||
                                 v->d()->data() != value);
                if (v)
                    v->removeVmePropertyReference();
                md->set(engine, id, engine->newVariantObject(value));
                v = static_cast<const QV4::VariantObject *>(md->data() + id);
                v->addVmePropertyReference();
            }
        }

        if (needActivate)
            activate(object, methodOffset() + id, nullptr);
    }
}

QV4::ReturnedValue QQmlVMEMetaObject::vmeMethod(int index) const
{
    if (index < methodOffset()) {
        Q_ASSERT(parentVMEMetaObject());
        return parentVMEMetaObject()->vmeMethod(index);
    }
    if (!compiledObject)
        return QV4::Value::undefinedValue().asReturnedValue();
    const int plainSignals = compiledObject->nSignals + compiledObject->nProperties + compiledObject->nAliases;
    Q_ASSERT(index >= (methodOffset() + plainSignals) && index < (methodOffset() + plainSignals + int(compiledObject->nFunctions)));
    return method(index - methodOffset() - plainSignals);
}

// Used by debugger
void QQmlVMEMetaObject::setVmeMethod(int index, const QV4::Value &function)
{
    if (index < methodOffset()) {
        Q_ASSERT(parentVMEMetaObject());
        return parentVMEMetaObject()->setVmeMethod(index, function);
    }
    if (!compiledObject)
        return;
    const int plainSignals = compiledObject->nSignals + compiledObject->nProperties + compiledObject->nAliases;
    Q_ASSERT(index >= (methodOffset() + plainSignals) && index < (methodOffset() + plainSignals + int(compiledObject->nFunctions)));

    int methodIndex = index - methodOffset() - plainSignals;
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return;
    md->set(engine, methodIndex + compiledObject->nProperties, function);
}

QV4::ReturnedValue QQmlVMEMetaObject::vmeProperty(int index) const
{
    if (index < propOffset()) {
        Q_ASSERT(parentVMEMetaObject());
        return parentVMEMetaObject()->vmeProperty(index);
    }
    return readVarProperty(index - propOffset());
}

void QQmlVMEMetaObject::setVMEProperty(int index, const QV4::Value &v)
{
    if (index < propOffset()) {
        Q_ASSERT(parentVMEMetaObject());
        parentVMEMetaObject()->setVMEProperty(index, v);
        return;
    }
    return writeVarProperty(index - propOffset(), v);
}

void QQmlVMEMetaObject::ensureQObjectWrapper()
{
    Q_ASSERT(cache);
    QV4::QObjectWrapper::wrap(engine, object);
}

void QQmlVMEMetaObject::mark(QV4::MarkStack *markStack)
{
    if (engine != markStack->engine())
        return;

    propertyAndMethodStorage.markOnce(markStack);

    if (QQmlVMEMetaObject *parent = parentVMEMetaObject())
        parent->mark(markStack);
}

bool QQmlVMEMetaObject::aliasTarget(int index, QObject **target, int *coreIndex, int *valueTypeIndex) const
{
    Q_ASSERT(compiledObject && (index >= propOffset() + int(compiledObject->nProperties)));

    *target = nullptr;
    *coreIndex = -1;
    *valueTypeIndex = -1;

    if (!ctxt)
        return false;

    const int aliasId = index - propOffset() - compiledObject->nProperties;
    const QV4::CompiledData::Alias *aliasData = &compiledObject->aliasTable()[aliasId];
    while (aliasData->aliasToLocalAlias)
        aliasData = &compiledObject->aliasTable()[aliasData->localAliasIndex];
    *target = ctxt->idValues[aliasData->targetObjectId].data();
    if (!*target)
        return false;

    if (!aliasData->isObjectAlias()) {
        QQmlPropertyIndex encodedIndex = QQmlPropertyIndex::fromEncoded(aliasData->encodedMetaPropertyIndex);
        *coreIndex = encodedIndex.coreIndex();
        *valueTypeIndex = encodedIndex.valueTypeIndex();
    }
    return true;
}

void QQmlVMEMetaObject::connectAlias(int aliasId)
{
    Q_ASSERT(compiledObject);
    if (!aliasEndpoints)
        aliasEndpoints = new QQmlVMEMetaObjectEndpoint[compiledObject->nAliases];

    const QV4::CompiledData::Alias *aliasData = &compiledObject->aliasTable()[aliasId];

    QQmlVMEMetaObjectEndpoint *endpoint = aliasEndpoints + aliasId;
    if (endpoint->metaObject.data()) {
        // already connected
        Q_ASSERT(endpoint->metaObject.data() == this);
        return;
    }

    endpoint->metaObject = this;
    endpoint->connect(&ctxt->idValues[aliasData->targetObjectId].bindings);
    endpoint->tryConnect();
}

void QQmlVMEMetaObject::connectAliasSignal(int index, bool indexInSignalRange)
{
    Q_ASSERT(compiledObject);
    int aliasId = (index - (indexInSignalRange ? signalOffset() : methodOffset())) - compiledObject->nProperties;
    if (aliasId < 0 || aliasId >= int(compiledObject->nAliases))
        return;

    connectAlias(aliasId);
}

/*! \internal
    \a index is in the method index range (QMetaMethod::methodIndex()).
*/
void QQmlVMEMetaObject::activate(QObject *object, int index, void **args)
{
    QMetaObject::activate(object, signalOffset(), index - methodOffset(), args);
}

QQmlVMEMetaObject *QQmlVMEMetaObject::getForProperty(QObject *o, int coreIndex)
{
    QQmlVMEMetaObject *vme = QQmlVMEMetaObject::get(o);
    while (vme && vme->propOffset() > coreIndex)
        vme = vme->parentVMEMetaObject();

    Q_ASSERT(vme);
    return vme;
}

QQmlVMEMetaObject *QQmlVMEMetaObject::getForMethod(QObject *o, int coreIndex)
{
    QQmlVMEMetaObject *vme = QQmlVMEMetaObject::get(o);
    while (vme && vme->methodOffset() > coreIndex)
        vme = vme->parentVMEMetaObject();

    Q_ASSERT(vme);
    return vme;
}

/*! \internal
    \a coreIndex is in the signal index range (see QObjectPrivate::signalIndex()).
    This is different from QMetaMethod::methodIndex().
*/
QQmlVMEMetaObject *QQmlVMEMetaObject::getForSignal(QObject *o, int coreIndex)
{
    QQmlVMEMetaObject *vme = QQmlVMEMetaObject::get(o);
    while (vme && vme->signalOffset() > coreIndex)
        vme = vme->parentVMEMetaObject();

    Q_ASSERT(vme);
    return vme;
}

QQmlVMEVariantQObjectPtr *QQmlVMEMetaObject::getQObjectGuardForProperty(int index) const
{
    QList<QQmlVMEVariantQObjectPtr *>::ConstIterator it = varObjectGuards.constBegin(), end = varObjectGuards.constEnd();
    for ( ; it != end; ++it) {
        if ((*it)->m_index == index) {
            return *it;
        }
    }

    return nullptr;
}

QT_END_NAMESPACE
