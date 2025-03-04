// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qv4internalclass_p.h>
#include <qv4string_p.h>
#include <qv4engine_p.h>
#include <qv4identifierhash_p.h>
#include "qv4object_p.h"
#include "qv4value_p.h"
#include "qv4mm_p.h"
#include <private/qprimefornumbits_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

PropertyHashData::PropertyHashData(int numBits)
    : refCount(1)
    , size(0)
    , numBits(numBits)
{
    alloc = qPrimeForNumBits(numBits);
    entries = (PropertyHash::Entry *)malloc(alloc*sizeof(PropertyHash::Entry));
    memset(entries, 0, alloc*sizeof(PropertyHash::Entry));
}

void PropertyHash::addEntry(const PropertyHash::Entry &entry, int classSize)
{
    // fill up to max 50%
    bool grow = (d->alloc <= d->size*2);

    if (classSize < d->size || grow)
        detach(grow, classSize);

    uint idx = entry.identifier.id() % d->alloc;
    while (d->entries[idx].identifier.isValid()) {
        ++idx;
        idx %= d->alloc;
    }
    d->entries[idx] = entry;
    ++d->size;
}

void PropertyHash::detach(bool grow, int classSize)
{
    if (d->refCount == 1 && !grow)
        return;

    PropertyHashData *dd = new PropertyHashData(grow ? d->numBits + 1 : d->numBits);
    for (int i = 0; i < d->alloc; ++i) {
        const Entry &e = d->entries[i];
        if (!e.identifier.isValid() || e.index >= static_cast<unsigned>(classSize))
            continue;
        uint idx = e.identifier.id() % dd->alloc;
        while (dd->entries[idx].identifier.isValid()) {
            ++idx;
            idx %= dd->alloc;
        }
        dd->entries[idx] = e;
    }
    dd->size = classSize;
    if (!--d->refCount)
        delete d;
    d = dd;
}


SharedInternalClassDataPrivate<PropertyKey>::SharedInternalClassDataPrivate(const SharedInternalClassDataPrivate<PropertyKey> &other)
    : refcount(1),
      engine(other.engine),
      data(nullptr)
{
    if (other.alloc()) {
        const uint s = other.size();
        data = MemberData::allocate(engine, other.alloc(), other.data);
        setSize(s);
    }
}

SharedInternalClassDataPrivate<PropertyKey>::SharedInternalClassDataPrivate(const SharedInternalClassDataPrivate<PropertyKey> &other,
                                                                            uint pos, PropertyKey value)
    : refcount(1),
      engine(other.engine)
{
    data = MemberData::allocate(engine, other.alloc(), nullptr);
    memcpy(data, other.data, sizeof(Heap::MemberData) - sizeof(Value) + pos*sizeof(Value));
    data->values.size = pos + 1;
    data->values.set(engine, pos, Value::fromReturnedValue(value.id()));
}

void SharedInternalClassDataPrivate<PropertyKey>::grow()
{
    const uint a = alloc() * 2;
    const uint s = size();
    data = MemberData::allocate(engine, a, data);
    setSize(s);
    Q_ASSERT(alloc() >= a);
}

uint SharedInternalClassDataPrivate<PropertyKey>::alloc() const
{
    return data ? data->values.alloc : 0;
}

uint SharedInternalClassDataPrivate<PropertyKey>::size() const
{
    return data ? data->values.size : 0;
}

void SharedInternalClassDataPrivate<PropertyKey>::setSize(uint s)
{
    Q_ASSERT(data && s <= alloc());
    data->values.size = s;
}

PropertyKey SharedInternalClassDataPrivate<PropertyKey>::at(uint i) const
{
    Q_ASSERT(data && i < size());
    return PropertyKey::fromId(data->values.values[i].rawValue());
}

void SharedInternalClassDataPrivate<PropertyKey>::set(uint i, PropertyKey t)
{
    Q_ASSERT(data && i < size());
    data->values.values[i].rawValueRef() = t.id();
}

void SharedInternalClassDataPrivate<PropertyKey>::mark(MarkStack *s)
{
    if (data)
        data->mark(s);
}

SharedInternalClassDataPrivate<PropertyAttributes>::SharedInternalClassDataPrivate(
        const SharedInternalClassDataPrivate<PropertyAttributes> &other, uint pos,
        PropertyAttributes value)
    : refcount(1),
      m_alloc(qMin(other.m_alloc, pos + 8)),
      m_size(pos + 1),
      m_engine(other.m_engine)
{
    Q_ASSERT(m_size <= m_alloc);
    Q_ASSERT(m_alloc > 0);

    m_engine->memoryManager->changeUnmanagedHeapSizeUsage(m_alloc * sizeof(PropertyAttributes));
    const PropertyAttributes *source = other.m_alloc > NumAttributesInPointer
            ? other.m_data
            : other.m_inlineData;
    PropertyAttributes *target;
    if (m_alloc > NumAttributesInPointer)
        m_data = target = new PropertyAttributes[m_alloc];
    else
        target = m_inlineData;

    memcpy(target, source, (m_size - 1) * sizeof(PropertyAttributes));
    target[pos] = value;
}

SharedInternalClassDataPrivate<PropertyAttributes>::SharedInternalClassDataPrivate(
        const SharedInternalClassDataPrivate<PropertyAttributes> &other)
    : refcount(1),
      m_alloc(other.m_alloc),
      m_size(other.m_size),
      m_engine(other.m_engine)
{
    m_engine->memoryManager->changeUnmanagedHeapSizeUsage(m_alloc * sizeof(PropertyAttributes));
    if (m_alloc > NumAttributesInPointer) {
        m_data = new PropertyAttributes[m_alloc];
        memcpy(m_data, other.m_data, m_size*sizeof(PropertyAttributes));
    } else if (m_alloc > 0) {
        memcpy(m_inlineData, other.m_inlineData, m_alloc * sizeof(PropertyAttributes));
    } else {
        m_data = nullptr;
    }
}

SharedInternalClassDataPrivate<PropertyAttributes>::~SharedInternalClassDataPrivate()
{
    m_engine->memoryManager->changeUnmanagedHeapSizeUsage(
            -qptrdiff(m_alloc * sizeof(PropertyAttributes)));
    if (m_alloc > NumAttributesInPointer)
        delete [] m_data;
}

void SharedInternalClassDataPrivate<PropertyAttributes>::grow() {
    uint alloc;
    if (!m_alloc) {
        alloc = NumAttributesInPointer;
        m_engine->memoryManager->changeUnmanagedHeapSizeUsage(alloc * sizeof(PropertyAttributes));
    } else {
        // yes, signed. We don't want to deal with stuff > 2G
        const uint currentSize = m_alloc * sizeof(PropertyAttributes);
        if (currentSize < uint(std::numeric_limits<int>::max() / 2))
            alloc = m_alloc * 2;
        else
            alloc = std::numeric_limits<int>::max() / sizeof(PropertyAttributes);

        m_engine->memoryManager->changeUnmanagedHeapSizeUsage(
                (alloc - m_alloc) * sizeof(PropertyAttributes));
    }

    if (alloc > NumAttributesInPointer) {
        auto *n = new PropertyAttributes[alloc];
        if (m_alloc > NumAttributesInPointer) {
            memcpy(n, m_data, m_alloc * sizeof(PropertyAttributes));
            delete [] m_data;
        } else if (m_alloc > 0) {
            memcpy(n, m_inlineData, m_alloc * sizeof(PropertyAttributes));
        }
        m_data = n;
    }
    m_alloc = alloc;
}

namespace Heap {

void InternalClass::init(ExecutionEngine *engine)
{
//    InternalClass is automatically zeroed during allocation:
//    prototype = nullptr;
//    parent = nullptr;
//    size = 0;
//    numRedundantTransitions = 0;
//    flags = 0;

    Base::init();
    new (&propertyTable) PropertyHash();
    new (&nameMap) SharedInternalClassData<PropertyKey>(engine);
    new (&propertyData) SharedInternalClassData<PropertyAttributes>(engine);
    new (&transitions) QVarLengthArray<Transition, 1>();

    this->engine = engine;
    vtable = QV4::InternalClass::staticVTable();
    protoId = engine->newProtoId();

    // Also internal classes need an internal class pointer. Simply make it point to itself
    internalClass.set(engine, this);
}


void InternalClass::init(Heap::InternalClass *other)
{
    Base::init();
    new (&propertyTable) PropertyHash(other->propertyTable);
    new (&nameMap) SharedInternalClassData<PropertyKey>(other->nameMap);
    new (&propertyData) SharedInternalClassData<PropertyAttributes>(other->propertyData);
    new (&transitions) QVarLengthArray<Transition, 1>();

    engine = other->engine;
    vtable = other->vtable;
    prototype = other->prototype;
    parent = other;
    size = other->size;
    numRedundantTransitions = other->numRedundantTransitions;
    flags = other->flags;
    protoId = engine->newProtoId();

    internalClass.set(engine, other->internalClass);
}

void InternalClass::destroy()
{
    for (const auto &t : transitions) {
        if (t.lookup) {
#ifndef QT_NO_DEBUG
            Q_ASSERT(t.lookup->parent == this);
#endif
            t.lookup->parent = nullptr;
        }
    }

    if (parent && parent->engine && parent->isMarked())
        parent->removeChildEntry(this);

    propertyTable.~PropertyHash();
    nameMap.~SharedInternalClassData<PropertyKey>();
    propertyData.~SharedInternalClassData<PropertyAttributes>();
    transitions.~QVarLengthArray<Transition, 1>();
    engine = nullptr;
    Base::destroy();
}

ReturnedValue InternalClass::keyAt(uint index) const
{
    PropertyKey key = nameMap.at(index);
    if (!key.isValid())
        return Encode::undefined();
    if (key.isArrayIndex())
        return Encode(key.asArrayIndex());
    Q_ASSERT(key.isStringOrSymbol());
    return key.asStringOrSymbol()->asReturnedValue();
}

void InternalClass::changeMember(QV4::Object *object, PropertyKey id, PropertyAttributes data, InternalClassEntry *entry)
{
    Q_ASSERT(id.isStringOrSymbol());

    Heap::InternalClass *oldClass = object->internalClass();
    Heap::InternalClass *newClass = oldClass->changeMember(id, data, entry);
    object->setInternalClass(newClass);
}

InternalClassTransition &InternalClass::lookupOrInsertTransition(const InternalClassTransition &t)
{
    QVarLengthArray<Transition, 1>::iterator it = std::lower_bound(transitions.begin(), transitions.end(), t);
    if (it != transitions.end() && *it == t) {
        return *it;
    } else {
        it = transitions.insert(it, t);
        return *it;
    }
}

static void addDummyEntry(InternalClass *newClass, PropertyHash::Entry e)
{
    // add a dummy entry, since we need two entries for accessors
    newClass->propertyTable.addEntry(e, newClass->size);
    newClass->nameMap.add(newClass->size, PropertyKey::invalid());
    newClass->propertyData.add(newClass->size, PropertyAttributes());
    ++newClass->size;
}

static PropertyAttributes attributesFromFlags(int flags)
{
    PropertyAttributes attributes;
    attributes.m_all = uchar(flags);
    return attributes;
}

static Heap::InternalClass *cleanInternalClass(Heap::InternalClass *orig)
{
    if (++orig->numRedundantTransitions < Heap::InternalClass::MaxRedundantTransitions)
        return orig;

    // We will generally add quite a few transitions here. We have 255 redundant ones.
    // We can expect at least as many significant ones in addition.
    QVarLengthArray<InternalClassTransition, 1> transitions;

    Scope scope(orig->engine);
    Scoped<QV4::InternalClass> child(scope, orig);

    {
        quint8 remainingRedundantTransitions = orig->numRedundantTransitions;
        QSet<PropertyKey> properties;
        int structureChanges = 0;

        Scoped<QV4::InternalClass> parent(scope, orig->parent);
        while (parent && remainingRedundantTransitions > 0) {
            Q_ASSERT(child->d() != scope.engine->classes[ExecutionEngine::Class_Empty]);
            const auto it = std::find_if(
                        parent->d()->transitions.begin(), parent->d()->transitions.end(),
                        [&child](const InternalClassTransition &t) {
                return child->d() == t.lookup;
            });
            Q_ASSERT(it != parent->d()->transitions.end());

            if (it->flags & InternalClassTransition::StructureChange) {
                // A structural change. Each kind of structural change has to be recorded only once.
                if ((structureChanges & it->flags) != it->flags) {
                    transitions.push_back(*it);
                    structureChanges |= it->flags;
                } else {
                    --remainingRedundantTransitions;
                }
            } else if (!properties.contains(it->id)) {
                // We only need the final state of the property.
                properties.insert(it->id);

                // Property removal creates _two_ redundant transitions.
                // We don't have to replay either, but numRedundantTransitions only records one.
                if (it->flags != 0)
                    transitions.push_back(*it);
            } else {
                --remainingRedundantTransitions;
            }

            child = parent->d();
            parent = child->d()->parent;
            Q_ASSERT(child->d() != parent->d());
        }
    }

    for (auto it = transitions.rbegin(); it != transitions.rend(); ++it) {
        switch (it->flags) {
        case InternalClassTransition::NotExtensible:
            child = child->d()->nonExtensible();
            continue;
        case InternalClassTransition::VTableChange:
            child = child->d()->changeVTable(it->vtable);
            continue;
        case InternalClassTransition::PrototypeChange:
            child = child->d()->changePrototype(it->prototype);
            continue;
        case InternalClassTransition::ProtoClass:
            child = child->d()->asProtoClass();
            continue;
        case InternalClassTransition::Sealed:
            child = child->d()->sealed();
            continue;
        case InternalClassTransition::Frozen:
            child = child->d()->frozen();
            continue;
        case InternalClassTransition::Locked:
            child = child->d()->locked();
            continue;
        default:
            Q_ASSERT(it->flags != 0);
            Q_ASSERT(it->flags < InternalClassTransition::StructureChange);
            child = child->addMember(it->id, attributesFromFlags(it->flags));
            continue;
        }
    }

    return child->d();
}

Heap::InternalClass *InternalClass::changeMember(
        PropertyKey identifier, PropertyAttributes data, InternalClassEntry *entry)
{
    if (!data.isEmpty())
        data.resolve();
    PropertyHash::Entry *e = findEntry(identifier);
    Q_ASSERT(e && e->index != UINT_MAX);
    uint idx = e->index;
    Q_ASSERT(idx != UINT_MAX);

    if (entry) {
        entry->index = idx;
        entry->setterIndex = e->setterIndex;
        entry->attributes = data;
    }

    if (data == propertyData.at(idx))
        return this;

    Transition temp = { { identifier }, nullptr, int(data.all()) };
    Transition &t = lookupOrInsertTransition(temp);
    if (t.lookup)
        return t.lookup;

    // create a new class and add it to the tree
    Heap::InternalClass *newClass = engine->newClass(this);
    if (data.isAccessor() && e->setterIndex == UINT_MAX) {
        Q_ASSERT(!propertyData.at(idx).isAccessor());

        // add a dummy entry for the accessor
        if (entry)
            entry->setterIndex = newClass->size;
        e->setterIndex = newClass->size;
        addDummyEntry(newClass, *e);
    }

    newClass->propertyData.set(idx, data);

    t.lookup = newClass;
    Q_ASSERT(t.lookup);

    return cleanInternalClass(newClass);
}

Heap::InternalClass *InternalClass::changePrototypeImpl(Heap::Object *proto)
{
    Scope scope(engine);
    ScopedValue protectThis(scope, this);
    if (proto)
        proto->setUsedAsProto();
    Q_ASSERT(prototype != proto);
    Q_ASSERT(!proto || proto->internalClass->isUsedAsProto());

    Transition temp = { { PropertyKey::invalid() }, nullptr, Transition::PrototypeChange };
    temp.prototype = proto;

    Transition &t = lookupOrInsertTransition(temp);
    if (t.lookup)
        return t.lookup;

    // create a new class and add it to the tree
    Heap::InternalClass *newClass = engine->newClass(this);
    newClass->prototype = proto;

    t.lookup = newClass;
    return prototype ? cleanInternalClass(newClass) : newClass;
}

Heap::InternalClass *InternalClass::changeVTableImpl(const VTable *vt)
{
    Q_ASSERT(vtable != vt);

    Transition temp = { { PropertyKey::invalid() }, nullptr, Transition::VTableChange };
    temp.vtable = vt;

    Transition &t = lookupOrInsertTransition(temp);
    if (t.lookup)
        return t.lookup;

    // create a new class and add it to the tree
    Heap::InternalClass *newClass = engine->newClass(this);
    newClass->vtable = vt;

    t.lookup = newClass;
    Q_ASSERT(t.lookup);
    Q_ASSERT(newClass->vtable);
    return vtable == QV4::InternalClass::staticVTable()
            ? newClass
            : cleanInternalClass(newClass);
}

Heap::InternalClass *InternalClass::nonExtensible()
{
    if (!isExtensible())
        return this;

    Transition temp = { { PropertyKey::invalid() }, nullptr, Transition::NotExtensible};
    Transition &t = lookupOrInsertTransition(temp);
    if (t.lookup)
        return t.lookup;

    Heap::InternalClass *newClass = engine->newClass(this);
    newClass->flags |= NotExtensible;

    t.lookup = newClass;
    Q_ASSERT(t.lookup);
    return newClass;
}

InternalClass *InternalClass::locked()
{
    if (isLocked())
        return this;

    Transition temp = { { PropertyKey::invalid() }, nullptr, Transition::Locked};
    Transition &t = lookupOrInsertTransition(temp);
    if (t.lookup)
        return t.lookup;

    Heap::InternalClass *newClass = engine->newClass(this);
    newClass->flags |= Locked;

    t.lookup = newClass;
    Q_ASSERT(t.lookup);
    return newClass;
}

void InternalClass::addMember(QV4::Object *object, PropertyKey id, PropertyAttributes data, InternalClassEntry *entry)
{
    Q_ASSERT(id.isStringOrSymbol());
    if (!data.isEmpty())
        data.resolve();
    PropertyHash::Entry *e = object->internalClass()->findEntry(id);
    if (e) {
        changeMember(object, id, data, entry);
        return;
    }

    Heap::InternalClass *newClass = object->internalClass()->addMemberImpl(id, data, entry);
    object->setInternalClass(newClass);
}

Heap::InternalClass *InternalClass::addMember(PropertyKey identifier, PropertyAttributes data, InternalClassEntry *entry)
{
    Q_ASSERT(identifier.isStringOrSymbol());
    if (!data.isEmpty())
        data.resolve();

    PropertyHash::Entry *e = findEntry(identifier);
    if (e)
        return changeMember(identifier, data, entry);

    return addMemberImpl(identifier, data, entry);
}

Heap::InternalClass *InternalClass::addMemberImpl(PropertyKey identifier, PropertyAttributes data, InternalClassEntry *entry)
{
    Transition temp = { { identifier }, nullptr, int(data.all()) };
    Transition &t = lookupOrInsertTransition(temp);

    if (entry) {
        entry->index = size;
        entry->setterIndex = data.isAccessor() ? size + 1 : UINT_MAX;
        entry->attributes = data;
    }

    if (t.lookup)
        return t.lookup;

    // create a new class and add it to the tree
    Scope scope(engine);
    Scoped<QV4::InternalClass> ic(scope, engine->newClass(this));
    InternalClass *newClass = ic->d();
    PropertyHash::Entry e = { identifier, newClass->size, data.isAccessor() ? newClass->size + 1 : UINT_MAX };
    newClass->propertyTable.addEntry(e, newClass->size);

    newClass->nameMap.add(newClass->size, identifier);
    newClass->propertyData.add(newClass->size, data);
    ++newClass->size;
    if (data.isAccessor())
        addDummyEntry(newClass, e);

    t.lookup = newClass;
    Q_ASSERT(t.lookup);
    return newClass;
}

void InternalClass::removeChildEntry(InternalClass *child)
{
    Q_ASSERT(engine);
    for (auto &t : transitions) {
        if (t.lookup == child) {
            t.lookup = nullptr;
            return;
        }
    }
    Q_UNREACHABLE();

}

void InternalClass::removeMember(QV4::Object *object, PropertyKey identifier)
{
#ifndef QT_NO_DEBUG
    Heap::InternalClass *oldClass = object->internalClass();
    Q_ASSERT(oldClass->findEntry(identifier) != nullptr);
#endif

    changeMember(object, identifier, Attr_Invalid);

#ifndef QT_NO_DEBUG
    // We didn't remove the data slot, just made it inaccessible.
    // ... unless we've rebuilt the whole class. Then all the deleted properties are gone.
    Q_ASSERT(object->internalClass()->numRedundantTransitions == 0
             || object->internalClass()->size == oldClass->size);
#endif
}

Heap::InternalClass *InternalClass::sealed()
{
    if (isSealed())
        return this;

    Transition temp = { { PropertyKey::invalid() }, nullptr, InternalClassTransition::Sealed };
    Transition &t = lookupOrInsertTransition(temp);

    if (t.lookup) {
        Q_ASSERT(t.lookup && t.lookup->isSealed());
        return t.lookup;
    }

    Scope scope(engine);
    Scoped<QV4::InternalClass> ic(scope, engine->newClass(this));
    Heap::InternalClass *s = ic->d();

    if (!isFrozen()) { // freezing also makes all properties non-configurable
        for (uint i = 0; i < size; ++i) {
            PropertyAttributes attrs = propertyData.at(i);
            if (attrs.isEmpty())
                continue;
            attrs.setConfigurable(false);
            s->propertyData.set(i, attrs);
        }
    }
    s->flags |= Sealed;

    t.lookup = s;
    return s;
}

Heap::InternalClass *InternalClass::frozen()
{
    if (isFrozen())
        return this;

    Transition temp = { { PropertyKey::invalid() }, nullptr, InternalClassTransition::Frozen };
    Transition &t = lookupOrInsertTransition(temp);

    if (t.lookup) {
        Q_ASSERT(t.lookup && t.lookup->isFrozen());
        return t.lookup;
    }

    Scope scope(engine);
    Scoped<QV4::InternalClass> ic(scope, engine->newClass(this));
    Heap::InternalClass *f = ic->d();

    for (uint i = 0; i < size; ++i) {
        PropertyAttributes attrs = propertyData.at(i);
        if (attrs.isEmpty())
            continue;
        if (attrs.isData())
            attrs.setWritable(false);
        attrs.setConfigurable(false);
        f->propertyData.set(i, attrs);
    }
    f->flags |= Frozen;

    t.lookup = f;
    return f;
}

InternalClass *InternalClass::canned()
{
    // scope the intermediate result to prevent it from getting garbage collected
    Scope scope(engine);
    Scoped<QV4::InternalClass> ic(scope, sealed());
    return ic->d()->nonExtensible();
}

InternalClass *InternalClass::cryopreserved()
{
    // scope the intermediate result to prevent it from getting garbage collected
    Scope scope(engine);
    Scoped<QV4::InternalClass> ic(scope, frozen());
    return ic->d()->canned();
}

bool InternalClass::isImplicitlyFrozen() const
{
    if (isFrozen())
        return true;

    for (uint i = 0; i < size; ++i) {
        const PropertyAttributes attrs = propertyData.at(i);
        if (attrs.isEmpty())
            continue;
        if ((attrs.isData() && attrs.isWritable()) || attrs.isConfigurable())
            return false;
    }

    return true;
}

Heap::InternalClass *InternalClass::asProtoClass()
{
    if (isUsedAsProto())
        return this;

    Transition temp = { { PropertyKey::invalid() }, nullptr, Transition::ProtoClass };
    Transition &t = lookupOrInsertTransition(temp);
    if (t.lookup)
        return t.lookup;

    Heap::InternalClass *newClass = engine->newClass(this);
    newClass->flags |= UsedAsProto;

    t.lookup = newClass;
    Q_ASSERT(t.lookup);
    return newClass;
}

static void updateProtoUsage(Heap::Object *o, Heap::InternalClass *ic)
{
    if (ic->prototype == o)
        ic->protoId = ic->engine->newProtoId();
    for (auto &t : ic->transitions) {
        if (t.lookup)
            updateProtoUsage(o, t.lookup);
    }
}


void InternalClass::updateProtoUsage(Heap::Object *o)
{
    Q_ASSERT(isUsedAsProto());
    Heap::InternalClass *ic = engine->internalClasses(EngineBase::Class_Empty);
    Q_ASSERT(!ic->prototype);

    Heap::updateProtoUsage(o, ic);
}

void InternalClass::markObjects(Heap::Base *b, MarkStack *stack)
{
    Heap::InternalClass *ic = static_cast<Heap::InternalClass *>(b);
    if (ic->prototype)
        ic->prototype->mark(stack);

    if (ic->parent)
        ic->parent->mark(stack);

    ic->nameMap.mark(stack);
}

}

}

QT_END_NAMESPACE
