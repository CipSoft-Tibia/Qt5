/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qqmlpropertycache_p.h"

#include <private/qqmlengine_p.h>
#include <private/qqmlbinding_p.h>
#include <private/qqmlvmemetaobject_p.h>

#include <private/qmetaobject_p.h>
#include <private/qmetaobjectbuilder_p.h>
#include <private/qqmlpropertycachemethodarguments_p.h>

#include <private/qv4value_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/QCryptographicHash>

#include <ctype.h> // for toupper
#include <limits.h>
#include <algorithm>

#ifdef Q_CC_MSVC
// nonstandard extension used : zero-sized array in struct/union.
#  pragma warning( disable : 4200 )
#endif

QT_BEGIN_NAMESPACE

#define Q_INT16_MAX 32767

// Flags that do *NOT* depend on the property's QMetaProperty::userType() and thus are quick
// to load
static QQmlPropertyData::Flags fastFlagsForProperty(const QMetaProperty &p)
{
    QQmlPropertyData::Flags flags;

    flags.setIsConstant(p.isConstant());
    flags.setIsWritable(p.isWritable());
    flags.setIsResettable(p.isResettable());
    flags.setIsFinal(p.isFinal());
    flags.setIsRequired(p.isRequired());

    if (p.isEnumType())
        flags.type = QQmlPropertyData::Flags::EnumType;

    return flags;
}

// Flags that do depend on the property's QMetaProperty::userType() and thus are slow to
// load
static void flagsForPropertyType(int propType, QQmlPropertyData::Flags &flags)
{
    Q_ASSERT(propType != -1);

    if (propType == QMetaType::QObjectStar) {
        flags.type = QQmlPropertyData::Flags::QObjectDerivedType;
    } else if (propType == QMetaType::QVariant) {
        flags.type = QQmlPropertyData::Flags::QVariantType;
    } else if (propType < static_cast<int>(QMetaType::User)) {
        // nothing to do
    } else if (propType == qMetaTypeId<QQmlBinding *>()) {
        flags.type = QQmlPropertyData::Flags::QmlBindingType;
    } else if (propType == qMetaTypeId<QJSValue>()) {
        flags.type = QQmlPropertyData::Flags::QJSValueType;
    } else {
        QQmlMetaType::TypeCategory cat = QQmlMetaType::typeCategory(propType);

        if (cat == QQmlMetaType::Object || QMetaType::typeFlags(propType) & QMetaType::PointerToQObject)
            flags.type = QQmlPropertyData::Flags::QObjectDerivedType;
        else if (cat == QQmlMetaType::List)
            flags.type = QQmlPropertyData::Flags::QListType;
    }
}

static int metaObjectSignalCount(const QMetaObject *metaObject)
{
    int signalCount = 0;
    for (const QMetaObject *obj = metaObject; obj; obj = obj->superClass())
        signalCount += QMetaObjectPrivate::get(obj)->signalCount;
    return signalCount;
}

QQmlPropertyData::Flags
QQmlPropertyData::flagsForProperty(const QMetaProperty &p)
{
    auto flags = fastFlagsForProperty(p);
    flagsForPropertyType(p.userType(), flags);
    return flags;
}

static void populate(QQmlPropertyData *data, const QMetaProperty &p)
{
    Q_ASSERT(p.revision() <= Q_INT16_MAX);
    data->setCoreIndex(p.propertyIndex());
    data->setNotifyIndex(QMetaObjectPrivate::signalIndex(p.notifySignal()));
    data->setFlags(fastFlagsForProperty(p));
    data->setRevision(p.revision());
}

void QQmlPropertyData::lazyLoad(const QMetaProperty &p)
{
    populate(this, p);
    int type = static_cast<int>(p.userType());

    if (type >= QMetaType::User || type == 0)
        return; // Resolve later

    if (type == QMetaType::QObjectStar)
        m_flags.type = Flags::QObjectDerivedType;
    else if (type == QMetaType::QVariant)
        m_flags.type = Flags::QVariantType;

    // This is OK because lazyLoad is done before exposing the property data.
    setPropType(type);
}

void QQmlPropertyData::load(const QMetaProperty &p)
{
    populate(this, p);
    setPropType(p.userType());
    flagsForPropertyType(propType(), m_flags);
}

static void populate(QQmlPropertyData *data, const QMetaMethod &m)
{
    data->setCoreIndex(m.methodIndex());

    QQmlPropertyData::Flags flags = data->flags();
    flags.type = QQmlPropertyData::Flags::FunctionType;
    if (m.methodType() == QMetaMethod::Signal)
        flags.setIsSignal(true);
    else if (m.methodType() == QMetaMethod::Constructor)
        flags.setIsConstructor(true);

    const int paramCount = m.parameterCount();
    if (paramCount) {
        flags.setHasArguments(true);
        if ((paramCount == 1) && (m.parameterTypes().constFirst() == "QQmlV4Function*"))
            flags.setIsV4Function(true);
    }

    if (m.attributes() & QMetaMethod::Cloned)
        flags.setIsCloned(true);

    data->setFlags(flags);

    Q_ASSERT(m.revision() <= Q_INT16_MAX);
    data->setRevision(m.revision());
}

void QQmlPropertyData::load(const QMetaMethod &m)
{
    populate(this, m);
    setPropType(m.methodType() == QMetaMethod::Constructor
                    ? QMetaType::QObjectStar
                    : m.returnType());
}

void QQmlPropertyData::lazyLoad(const QMetaMethod &m)
{
    const char *returnType = m.typeName();
    if (!returnType || *returnType != 'v' || qstrcmp(returnType + 1, "oid") != 0)
        populate(this, m);
    else
        load(m); // If it's void, resolve it right away
}

/*!
Creates a new empty QQmlPropertyCache.
*/
QQmlPropertyCache::QQmlPropertyCache()
    : _parent(nullptr), propertyIndexCacheStart(0), methodIndexCacheStart(0),
      signalHandlerIndexCacheStart(0), _hasPropertyOverrides(false), _ownMetaObject(false),
      _metaObject(nullptr), argumentsCache(nullptr), _jsFactoryMethodIndex(-1)
{
}

/*!
Creates a new QQmlPropertyCache of \a metaObject.
*/
QQmlPropertyCache::QQmlPropertyCache(const QMetaObject *metaObject, int metaObjectRevision)
    : QQmlPropertyCache()
{
    Q_ASSERT(metaObject);

    update(metaObject);

    if (metaObjectRevision > 0) {
        // Set the revision of the meta object that this cache describes to be
        // 'metaObjectRevision'. This is useful when constructing a property cache
        // from a type that was created directly in C++, and not through QML. For such
        // types, the revision for each recorded QMetaObject would normally be zero, which
        // would exclude any revisioned properties.
        for (int metaObjectOffset = 0; metaObjectOffset < allowedRevisionCache.size(); ++metaObjectOffset)
            allowedRevisionCache[metaObjectOffset] = metaObjectRevision;
    }
}

QQmlPropertyCache::~QQmlPropertyCache()
{
    QQmlPropertyCacheMethodArguments *args = argumentsCache;
    while (args) {
        QQmlPropertyCacheMethodArguments *next = args->next;
        delete args->signalParameterStringForJS;
        delete args->names;
        free(args);
        args = next;
    }

    // We must clear this prior to releasing the parent incase it is a
    // linked hash
    stringCache.clear();
    if (_parent) _parent->release();

    if (_ownMetaObject) free(const_cast<QMetaObject *>(_metaObject));
    _metaObject = nullptr;
    _parent = nullptr;
}

QQmlPropertyCache *QQmlPropertyCache::copy(int reserve)
{
    QQmlPropertyCache *cache = new QQmlPropertyCache();
    cache->_parent = this;
    cache->_parent->addref();
    cache->propertyIndexCacheStart = propertyIndexCache.count() + propertyIndexCacheStart;
    cache->methodIndexCacheStart = methodIndexCache.count() + methodIndexCacheStart;
    cache->signalHandlerIndexCacheStart = signalHandlerIndexCache.count() + signalHandlerIndexCacheStart;
    cache->stringCache.linkAndReserve(stringCache, reserve);
    cache->allowedRevisionCache = allowedRevisionCache;
    cache->_metaObject = _metaObject;
    cache->_defaultPropertyName = _defaultPropertyName;

    return cache;
}

QQmlPropertyCache *QQmlPropertyCache::copy()
{
    return copy(0);
}

QQmlPropertyCache *QQmlPropertyCache::copyAndReserve(int propertyCount, int methodCount,
                                                     int signalCount, int enumCount)
{
    QQmlPropertyCache *rv = copy(propertyCount + methodCount + signalCount);
    rv->propertyIndexCache.reserve(propertyCount);
    rv->methodIndexCache.reserve(methodCount);
    rv->signalHandlerIndexCache.reserve(signalCount);
    rv->enumCache.reserve(enumCount);
    rv->_metaObject = nullptr;

    return rv;
}

/*! \internal

    \a notifyIndex MUST be in the signal index range (see QObjectPrivate::signalIndex()).
    This is different from QMetaMethod::methodIndex().
*/
void QQmlPropertyCache::appendProperty(const QString &name, QQmlPropertyData::Flags flags,
                                       int coreIndex, int propType, int minorVersion, int notifyIndex)
{
    QQmlPropertyData data;
    data.setPropType(propType);
    data.setCoreIndex(coreIndex);
    data.setNotifyIndex(notifyIndex);
    data.setFlags(flags);
    data.setTypeMinorVersion(minorVersion);

    QQmlPropertyData *old = findNamedProperty(name);
    if (old)
        data.markAsOverrideOf(old);

    int index = propertyIndexCache.count();
    propertyIndexCache.append(data);

    setNamedProperty(name, index + propertyOffset(), propertyIndexCache.data() + index, (old != nullptr));
}

void QQmlPropertyCache::appendSignal(const QString &name, QQmlPropertyData::Flags flags,
                                     int coreIndex, const int *types,
                                     const QList<QByteArray> &names)
{
    QQmlPropertyData data;
    data.setPropType(QMetaType::UnknownType);
    data.setCoreIndex(coreIndex);
    data.setFlags(flags);

    QQmlPropertyData handler = data;
    handler.m_flags.setIsSignalHandler(true);

    if (types) {
        int argumentCount = *types;
        QQmlPropertyCacheMethodArguments *args = createArgumentsObject(argumentCount, names);
        ::memcpy(args->arguments, types, (argumentCount + 1) * sizeof(int));
        data.setArguments(args);
    }

    QQmlPropertyData *old = findNamedProperty(name);
    if (old)
        data.markAsOverrideOf(old);

    int methodIndex = methodIndexCache.count();
    methodIndexCache.append(data);

    int signalHandlerIndex = signalHandlerIndexCache.count();
    signalHandlerIndexCache.append(handler);

    QString handlerName = QLatin1String("on") + name;
    handlerName[2] = handlerName.at(2).toUpper();

    setNamedProperty(name, methodIndex + methodOffset(), methodIndexCache.data() + methodIndex, (old != nullptr));
    setNamedProperty(handlerName, signalHandlerIndex + signalOffset(), signalHandlerIndexCache.data() + signalHandlerIndex, (old != nullptr));
}

void QQmlPropertyCache::appendMethod(const QString &name, QQmlPropertyData::Flags flags,
                                     int coreIndex, int returnType, const QList<QByteArray> &names,
                                     const QVector<int> &parameterTypes)
{
    int argumentCount = names.count();

    QQmlPropertyData data;
    data.setPropType(returnType);
    data.setCoreIndex(coreIndex);

    QQmlPropertyCacheMethodArguments *args = createArgumentsObject(argumentCount, names);
    for (int ii = 0; ii < argumentCount; ++ii)
        args->arguments[ii + 1] = parameterTypes.at(ii);
    data.setArguments(args);

    data.setFlags(flags);

    QQmlPropertyData *old = findNamedProperty(name);
    if (old)
        data.markAsOverrideOf(old);

    int methodIndex = methodIndexCache.count();
    methodIndexCache.append(data);

    setNamedProperty(name, methodIndex + methodOffset(), methodIndexCache.data() + methodIndex, (old != nullptr));
}

void QQmlPropertyCache::appendEnum(const QString &name, const QVector<QQmlEnumValue> &values)
{
    QQmlEnumData data;
    data.name = name;
    data.values = values;
    enumCache.append(data);
}

// Returns this property cache's metaObject, creating it if necessary.
const QMetaObject *QQmlPropertyCache::createMetaObject()
{
    if (!_metaObject) {
        _ownMetaObject = true;

        QMetaObjectBuilder builder;
        toMetaObjectBuilder(builder);
        builder.setSuperClass(_parent->createMetaObject());
        _metaObject = builder.toMetaObject();
    }

    return _metaObject;
}

QQmlPropertyData *QQmlPropertyCache::maybeUnresolvedProperty(int index) const
{
    if (index < 0 || index >= (propertyIndexCacheStart + propertyIndexCache.count()))
        return nullptr;

    QQmlPropertyData *rv = nullptr;
    if (index < propertyIndexCacheStart)
        return _parent->maybeUnresolvedProperty(index);
    else
        rv = const_cast<QQmlPropertyData *>(&propertyIndexCache.at(index - propertyIndexCacheStart));
    return rv;
}

QQmlPropertyData *QQmlPropertyCache::defaultProperty() const
{
    return property(defaultPropertyName(), nullptr, nullptr);
}

void QQmlPropertyCache::setParent(QQmlPropertyCache *newParent)
{
    if (_parent == newParent)
        return;
    if (_parent)
        _parent->release();
    _parent = newParent;
    _parent->addref();
}

QQmlPropertyCache *
QQmlPropertyCache::copyAndAppend(const QMetaObject *metaObject,
                                 QQmlPropertyData::Flags propertyFlags,
                                 QQmlPropertyData::Flags methodFlags,
                                 QQmlPropertyData::Flags signalFlags)
{
    return copyAndAppend(metaObject, -1, propertyFlags, methodFlags, signalFlags);
}

QQmlPropertyCache *
QQmlPropertyCache::copyAndAppend(const QMetaObject *metaObject,
                                 int typeMinorVersion,
                                 QQmlPropertyData::Flags propertyFlags,
                                 QQmlPropertyData::Flags methodFlags,
                                 QQmlPropertyData::Flags signalFlags)
{
    Q_ASSERT(QMetaObjectPrivate::get(metaObject)->revision >= 4);

    // Reserve enough space in the name hash for all the methods (including signals), all the
    // signal handlers and all the properties.  This assumes no name clashes, but this is the
    // common case.
    QQmlPropertyCache *rv = copy(QMetaObjectPrivate::get(metaObject)->methodCount +
                                         QMetaObjectPrivate::get(metaObject)->signalCount +
                                         QMetaObjectPrivate::get(metaObject)->propertyCount);

    rv->append(metaObject, typeMinorVersion, propertyFlags, methodFlags, signalFlags);

    return rv;
}

void QQmlPropertyCache::append(const QMetaObject *metaObject,
                               int typeMinorVersion,
                               QQmlPropertyData::Flags propertyFlags,
                               QQmlPropertyData::Flags methodFlags,
                               QQmlPropertyData::Flags signalFlags)
{
    _metaObject = metaObject;

    bool dynamicMetaObject = isDynamicMetaObject(metaObject);

    allowedRevisionCache.append(0);

    int methodCount = metaObject->methodCount();
    Q_ASSERT(QMetaObjectPrivate::get(metaObject)->revision >= 4);
    int signalCount = metaObjectSignalCount(metaObject);
    int classInfoCount = QMetaObjectPrivate::get(metaObject)->classInfoCount;

    if (classInfoCount) {
        int classInfoOffset = metaObject->classInfoOffset();
        for (int ii = 0; ii < classInfoCount; ++ii) {
            int idx = ii + classInfoOffset;
            QMetaClassInfo mci = metaObject->classInfo(idx);
            const char *name = mci.name();
            if (0 == qstrcmp(name, "DefaultProperty")) {
                _defaultPropertyName = QString::fromUtf8(mci.value());
            } else if (0 == qstrcmp(name, "qt_QmlJSWrapperFactoryMethod")) {
                const char * const factoryMethod = mci.value();
                _jsFactoryMethodIndex = metaObject->indexOfSlot(factoryMethod);
                if (_jsFactoryMethodIndex != -1)
                    _jsFactoryMethodIndex -= metaObject->methodOffset();
            }
        }
    }

    //Used to block access to QObject::destroyed() and QObject::deleteLater() from QML
    static const int destroyedIdx1 = QObject::staticMetaObject.indexOfSignal("destroyed(QObject*)");
    static const int destroyedIdx2 = QObject::staticMetaObject.indexOfSignal("destroyed()");
    static const int deleteLaterIdx = QObject::staticMetaObject.indexOfSlot("deleteLater()");
    // These indices don't apply to gadgets, so don't block them.
    // It is enough to check for QObject::staticMetaObject here because the loop below excludes
    // methods of parent classes: It starts at metaObject->methodOffset()
    const bool preventDestruction = (metaObject == &QObject::staticMetaObject);

    int methodOffset = metaObject->methodOffset();
    int signalOffset = signalCount - QMetaObjectPrivate::get(metaObject)->signalCount;

    // update() should have reserved enough space in the vector that this doesn't cause a realloc
    // and invalidate the stringCache.
    methodIndexCache.resize(methodCount - methodIndexCacheStart);
    signalHandlerIndexCache.resize(signalCount - signalHandlerIndexCacheStart);
    int signalHandlerIndex = signalOffset;
    for (int ii = methodOffset; ii < methodCount; ++ii) {
        if (preventDestruction && (ii == destroyedIdx1 || ii == destroyedIdx2 || ii == deleteLaterIdx))
            continue;
        QMetaMethod m = metaObject->method(ii);
        if (m.access() == QMetaMethod::Private)
            continue;

        // Extract method name
        // It's safe to keep the raw name pointer
        Q_ASSERT(QMetaObjectPrivate::get(metaObject)->revision >= 7);
        const char *rawName = m.name().constData();
        const char *cptr = rawName;
        char utf8 = 0;
        while (*cptr) {
            utf8 |= *cptr & 0x80;
            ++cptr;
        }

        QQmlPropertyData *data = &methodIndexCache[ii - methodIndexCacheStart];
        QQmlPropertyData *sigdata = nullptr;

        if (m.methodType() == QMetaMethod::Signal)
            data->setFlags(signalFlags);
        else
            data->setFlags(methodFlags);

        data->lazyLoad(m);
        data->m_flags.setIsDirect(!dynamicMetaObject);

        Q_ASSERT((allowedRevisionCache.count() - 1) < Q_INT16_MAX);
        data->setMetaObjectOffset(allowedRevisionCache.count() - 1);

        if (data->isSignal()) {
            sigdata = &signalHandlerIndexCache[signalHandlerIndex - signalHandlerIndexCacheStart];
            *sigdata = *data;
            sigdata->m_flags.setIsSignalHandler(true);
        }

        QQmlPropertyData *old = nullptr;

        if (utf8) {
            QHashedString methodName(QString::fromUtf8(rawName, cptr - rawName));
            if (StringCache::mapped_type *it = stringCache.value(methodName))
                old = it->second;
            setNamedProperty(methodName, ii, data, (old != nullptr));

            if (data->isSignal()) {
                QHashedString on(QLatin1String("on") % methodName.at(0).toUpper() % methodName.midRef(1));
                setNamedProperty(on, ii, sigdata, (old != nullptr));
                ++signalHandlerIndex;
            }
        } else {
            QHashedCStringRef methodName(rawName, cptr - rawName);
            if (StringCache::mapped_type *it = stringCache.value(methodName))
                old = it->second;
            setNamedProperty(methodName, ii, data, (old != nullptr));

            if (data->isSignal()) {
                int length = methodName.length();

                QVarLengthArray<char, 128> str(length+3);
                str[0] = 'o';
                str[1] = 'n';
                str[2] = toupper(rawName[0]);
                if (length > 1)
                    memcpy(&str[3], &rawName[1], length - 1);
                str[length + 2] = '\0';

                QHashedString on(QString::fromLatin1(str.data()));
                setNamedProperty(on, ii, data, (old != nullptr));
                ++signalHandlerIndex;
            }
        }

        if (old) {
            // We only overload methods in the same class, exactly like C++
            if (old->isFunction() && old->coreIndex() >= methodOffset)
                data->m_flags.setIsOverload(true);

            data->markAsOverrideOf(old);
        }
    }

    int propCount = metaObject->propertyCount();
    int propOffset = metaObject->propertyOffset();

    // update() should have reserved enough space in the vector that this doesn't cause a realloc
    // and invalidate the stringCache.
    propertyIndexCache.resize(propCount - propertyIndexCacheStart);
    for (int ii = propOffset; ii < propCount; ++ii) {
        QMetaProperty p = metaObject->property(ii);
        if (!p.isScriptable())
            continue;

        const char *str = p.name();
        char utf8 = 0;
        const char *cptr = str;
        while (*cptr != 0) {
            utf8 |= *cptr & 0x80;
            ++cptr;
        }

        QQmlPropertyData *data = &propertyIndexCache[ii - propertyIndexCacheStart];

        data->setFlags(propertyFlags);
        data->lazyLoad(p);
        data->setTypeMinorVersion(typeMinorVersion);

        data->m_flags.setIsDirect(!dynamicMetaObject);

        Q_ASSERT((allowedRevisionCache.count() - 1) < Q_INT16_MAX);
        data->setMetaObjectOffset(allowedRevisionCache.count() - 1);

        QQmlPropertyData *old = nullptr;

        if (utf8) {
            QHashedString propName(QString::fromUtf8(str, cptr - str));
            if (StringCache::mapped_type *it = stringCache.value(propName))
                old = it->second;
            setNamedProperty(propName, ii, data, (old != nullptr));
        } else {
            QHashedCStringRef propName(str, cptr - str);
            if (StringCache::mapped_type *it = stringCache.value(propName))
                old = it->second;
            setNamedProperty(propName, ii, data, (old != nullptr));
        }

        bool isGadget = true;
        for (const QMetaObject *it = metaObject; it != nullptr; it = it->superClass()) {
            if (it == &QObject::staticMetaObject)
                isGadget = false;
        }

        if (isGadget) // always dispatch over a 'normal' meta-call so the QQmlValueType can intercept
            data->m_flags.setIsDirect(false);
        else
            data->trySetStaticMetaCallFunction(metaObject->d.static_metacall, ii - propOffset);
        if (old)
            data->markAsOverrideOf(old);
    }
}

int QQmlPropertyCache::findPropType(const QQmlPropertyData *data) const
{
    int type = QMetaType::UnknownType;
    const QMetaObject *mo = firstCppMetaObject();
    if (data->isFunction()) {
        auto metaMethod = mo->method(data->coreIndex());
        const char *retTy = metaMethod.typeName();
        if (!retTy)
            retTy = "\0";
        type = QMetaType::type(retTy);
    } else {
        auto metaProperty = mo->property(data->coreIndex());
        type = QMetaType::type(metaProperty.typeName());
    }

    if (!data->isFunction()) {
        if (type == QMetaType::UnknownType) {
            QQmlPropertyCache *p = _parent;
            while (p && (!mo || _ownMetaObject)) {
                mo = p->_metaObject;
                p = p->_parent;
            }

            int propOffset = mo->propertyOffset();
            if (mo && data->coreIndex() < propOffset + mo->propertyCount()) {
                while (data->coreIndex() < propOffset) {
                    mo = mo->superClass();
                    propOffset = mo->propertyOffset();
                }

                int registerResult = -1;
                void *argv[] = { &registerResult };
                mo->static_metacall(QMetaObject::RegisterPropertyMetaType,
                                    data->coreIndex() - propOffset, argv);
                type = registerResult == -1 ? QMetaType::UnknownType : registerResult;
            }
        }
    }

    return type;
}

void QQmlPropertyCache::resolve(QQmlPropertyData *data) const
{
    const int type = findPropType(data);

    // Setting the flags unsynchronized is somewhat dirty but unlikely to cause trouble
    // in practice. We have to do this before setting the property type because otherwise
    // a consumer of the flags might see outdated flags even after the property type has
    // become valid. The flags should only depend on the property type and the property
    // type should be the same across different invocations. So, setting this concurrently
    // should be a noop.
    if (!data->isFunction())
        flagsForPropertyType(type, data->m_flags);

    // This is the one place where we can update the property type after exposing the data.
    if (!data->m_propTypeAndRelativePropIndex.testAndSetOrdered(
                0, type > 0 ? quint32(type) : quint32(QQmlPropertyData::PropTypeUnknown))) {
        return; // Someone else is resolving it already
    }
}

void QQmlPropertyCache::updateRecur(const QMetaObject *metaObject)
{
    if (!metaObject)
        return;

    updateRecur(metaObject->superClass());

    append(metaObject, -1);
}

void QQmlPropertyCache::update(const QMetaObject *metaObject)
{
    Q_ASSERT(metaObject);
    stringCache.clear();

    // Preallocate enough space in the index caches for all the properties/methods/signals that
    // are not cached in a parent cache so that the caches never need to be reallocated as this
    // would invalidate pointers stored in the stringCache.
    int pc = metaObject->propertyCount();
    int mc = metaObject->methodCount();
    int sc = metaObjectSignalCount(metaObject);
    propertyIndexCache.reserve(pc - propertyIndexCacheStart);
    methodIndexCache.reserve(mc - methodIndexCacheStart);
    signalHandlerIndexCache.reserve(sc - signalHandlerIndexCacheStart);

    // Reserve enough space in the stringCache for all properties/methods/signals including those
    // cached in a parent cache.
    stringCache.reserve(pc + mc + sc);

    updateRecur(metaObject);
}

/*! \internal
    invalidates and updates the PropertyCache if the QMetaObject has changed.
    This function is used in the tooling to update dynamic properties.
*/
void QQmlPropertyCache::invalidate(const QMetaObject *metaObject)
{
    propertyIndexCache.clear();
    methodIndexCache.clear();
    signalHandlerIndexCache.clear();

    _hasPropertyOverrides = false;
    argumentsCache = nullptr;

    int pc = metaObject->propertyCount();
    int mc = metaObject->methodCount();
    int sc = metaObjectSignalCount(metaObject);
    int reserve = pc + mc + sc;

    if (parent()) {
        propertyIndexCacheStart = parent()->propertyIndexCache.count() + parent()->propertyIndexCacheStart;
        methodIndexCacheStart = parent()->methodIndexCache.count() + parent()->methodIndexCacheStart;
        signalHandlerIndexCacheStart = parent()->signalHandlerIndexCache.count() + parent()->signalHandlerIndexCacheStart;
        stringCache.linkAndReserve(parent()->stringCache, reserve);
        append(metaObject, -1);
    } else {
        propertyIndexCacheStart = 0;
        methodIndexCacheStart = 0;
        signalHandlerIndexCacheStart = 0;
        update(metaObject);
    }
}

QQmlPropertyData *QQmlPropertyCache::findProperty(StringCache::ConstIterator it, QObject *object, QQmlContextData *context) const
{
    QQmlData *data = (object ? QQmlData::get(object) : nullptr);
    const QQmlVMEMetaObject *vmemo = nullptr;
    if (data && data->hasVMEMetaObject) {
        QObjectPrivate *op = QObjectPrivate::get(object);
        vmemo = static_cast<const QQmlVMEMetaObject *>(op->metaObject);
    }
    return findProperty(it, vmemo, context);
}

namespace {

inline bool contextHasNoExtensions(QQmlContextData *context)
{
    // This context has no extension if its parent is the engine's rootContext,
    // which has children but no imports
    return (!context->parent || !context->parent->imports);
}

inline int maximumIndexForProperty(QQmlPropertyData *prop, const int methodCount, const int signalCount, const int propertyCount)
{
    return prop->isFunction() ? methodCount
                              : prop->isSignalHandler() ? signalCount
                                                        : propertyCount;
}

}

QQmlPropertyData *QQmlPropertyCache::findProperty(StringCache::ConstIterator it, const QQmlVMEMetaObject *vmemo, QQmlContextData *context) const
{
    StringCache::ConstIterator end = stringCache.end();

    if (it != end) {
        QQmlPropertyData *result = it.value().second;

        // If there exists a typed property (not a function or signal handler), of the
        // right name available to the specified context, we need to return that
        // property rather than any subsequent override

        if (vmemo && context && !contextHasNoExtensions(context)) {
            // Find the meta-object that corresponds to the supplied context
            do {
                if (vmemo->ctxt == context)
                    break;

                vmemo = vmemo->parentVMEMetaObject();
            } while (vmemo);
        }

        if (vmemo) {
            const int methodCount = vmemo->cache->methodCount();
            const int signalCount = vmemo->cache->signalCount();
            const int propertyCount = vmemo->cache->propertyCount();

            // Ensure that the property we resolve to is accessible from this meta-object
            do {
                const StringCache::mapped_type &property(it.value());

                if (property.first < maximumIndexForProperty(property.second, methodCount, signalCount, propertyCount)) {
                    // This property is available in the specified context
                    if (property.second->isFunction() || property.second->isSignalHandler()) {
                        // Prefer the earlier resolution
                    } else {
                        // Prefer the typed property to any previous property found
                        result = property.second;
                    }
                    break;
                }

                // See if there is a better candidate
                it = stringCache.findNext(it);
            } while (it != end);
        }

        return ensureResolved(result);
    }

    return nullptr;
}

QString QQmlPropertyData::name(QObject *object) const
{
    if (!object)
        return QString();

    return name(object->metaObject());
}

QString QQmlPropertyData::name(const QMetaObject *metaObject) const
{
    if (!metaObject || coreIndex() == -1)
        return QString();

    if (isFunction()) {
        QMetaMethod m = metaObject->method(coreIndex());

        return QString::fromUtf8(m.name().constData());
    } else {
        QMetaProperty p = metaObject->property(coreIndex());
        return QString::fromUtf8(p.name());
    }
}

void QQmlPropertyData::markAsOverrideOf(QQmlPropertyData *predecessor)
{
    setOverrideIndexIsProperty(!predecessor->isFunction());
    setOverrideIndex(predecessor->coreIndex());

    predecessor->m_flags.setIsOverridden(true);
}

QQmlPropertyCacheMethodArguments *QQmlPropertyCache::createArgumentsObject(int argc, const QList<QByteArray> &names)
{
    typedef QQmlPropertyCacheMethodArguments A;
    A *args = static_cast<A *>(malloc(sizeof(A) + (argc) * sizeof(int)));
    args->arguments[0] = argc;
    args->signalParameterStringForJS = nullptr;
    args->names = argc ? new QList<QByteArray>(names) : nullptr;
    do {
        args->next = argumentsCache;
    } while (!argumentsCache.testAndSetRelease(args->next, args));
    return args;
}

QString QQmlPropertyCache::signalParameterStringForJS(QV4::ExecutionEngine *engine, const QList<QByteArray> &parameterNameList, QString *errorString)
{
    bool unnamedParameter = false;
    const QSet<QString> &illegalNames = engine->illegalNames();
    QString parameters;

    for (int i = 0; i < parameterNameList.count(); ++i) {
        if (i > 0)
            parameters += QLatin1Char(',');
        const QByteArray &param = parameterNameList.at(i);
        if (param.isEmpty())
            unnamedParameter = true;
        else if (unnamedParameter) {
            if (errorString)
                *errorString = QCoreApplication::translate("QQmlRewrite", "Signal uses unnamed parameter followed by named parameter.");
            return QString();
        } else if (illegalNames.contains(QString::fromUtf8(param))) {
            if (errorString)
                *errorString = QCoreApplication::translate("QQmlRewrite", "Signal parameter \"%1\" hides global variable.").arg(QString::fromUtf8(param));
            return QString();
        }
        parameters += QString::fromUtf8(param);
    }

    return parameters;
}

int QQmlPropertyCache::originalClone(int index)
{
    while (signal(index)->isCloned())
        --index;
    return index;
}

int QQmlPropertyCache::originalClone(QObject *object, int index)
{
    QQmlData *data = QQmlData::get(object, false);
    if (data && data->propertyCache) {
        QQmlPropertyCache *cache = data->propertyCache;
        QQmlPropertyData *sig = cache->signal(index);
        while (sig && sig->isCloned()) {
            --index;
            sig = cache->signal(index);
        }
    } else {
        while (QMetaObjectPrivate::signal(object->metaObject(), index).attributes() & QMetaMethod::Cloned)
            --index;
    }
    return index;
}

template<typename T>
static QQmlPropertyData qQmlPropertyCacheCreate(const QMetaObject *metaObject, const T& propertyName)
{
    Q_ASSERT(metaObject);

    QQmlPropertyData rv;

    /* It's important to check the method list before checking for properties;
     * otherwise, if the meta object is dynamic, a property will be created even
     * if not found and it might obscure a method having the same name. */

    //Used to block access to QObject::destroyed() and QObject::deleteLater() from QML
    static const int destroyedIdx1 = QObject::staticMetaObject.indexOfSignal("destroyed(QObject*)");
    static const int destroyedIdx2 = QObject::staticMetaObject.indexOfSignal("destroyed()");
    static const int deleteLaterIdx = QObject::staticMetaObject.indexOfSlot("deleteLater()");
    // These indices don't apply to gadgets, so don't block them.
    const bool preventDestruction = metaObject->superClass() || metaObject == &QObject::staticMetaObject;

    int methodCount = metaObject->methodCount();
    for (int ii = methodCount - 1; ii >= 0; --ii) {
        if (preventDestruction && (ii == destroyedIdx1 || ii == destroyedIdx2 || ii == deleteLaterIdx))
            continue;
        QMetaMethod m = metaObject->method(ii);
        if (m.access() == QMetaMethod::Private)
            continue;

        if (m.name() == propertyName) {
            rv.load(m);
            return rv;
        }
    }

    {
        const QMetaObject *cmo = metaObject;
        while (cmo) {
            int idx = cmo->indexOfProperty(propertyName);
            if (idx != -1) {
                QMetaProperty p = cmo->property(idx);
                if (p.isScriptable()) {
                    rv.load(p);
                    return rv;
                } else {
                    bool changed = false;
                    while (cmo && cmo->propertyOffset() >= idx) {
                        cmo = cmo->superClass();
                        changed = true;
                    }
                    /* If the "cmo" variable didn't change, set it to 0 to
                     * avoid running into an infinite loop */
                    if (!changed) cmo = nullptr;
                }
            } else {
                cmo = nullptr;
            }
        }
    }
    return rv;
}

static inline const char *qQmlPropertyCacheToString(QLatin1String string)
{
    return string.data();
}

static inline QByteArray qQmlPropertyCacheToString(const QStringRef &string)
{
    return string.toUtf8();
}

static inline QByteArray qQmlPropertyCacheToString(const QV4::String *string)
{
    return string->toQString().toUtf8();
}

template<typename T>
QQmlPropertyData *
qQmlPropertyCacheProperty(QJSEngine *engine, QObject *obj, T name,
                          QQmlContextData *context, QQmlPropertyData &local)
{
    QQmlPropertyCache *cache = nullptr;

    QQmlData *ddata = QQmlData::get(obj, false);

    if (ddata && ddata->propertyCache) {
        cache = ddata->propertyCache;
    } else if (engine) {
        QJSEnginePrivate *ep = QJSEnginePrivate::get(engine);
        cache = ep->cache(obj);
        if (cache) {
            ddata = QQmlData::get(obj, true);
            cache->addref();
            ddata->propertyCache = cache;
        }
    }

    QQmlPropertyData *rv = nullptr;

    if (cache) {
        rv = cache->property(name, obj, context);
    } else {
        local = qQmlPropertyCacheCreate(obj->metaObject(), qQmlPropertyCacheToString(name));
        if (local.isValid())
            rv = &local;
    }

    return rv;
}

QQmlPropertyData *
QQmlPropertyCache::property(QJSEngine *engine, QObject *obj, const QV4::String *name,
                            QQmlContextData *context, QQmlPropertyData &local)
{
    return qQmlPropertyCacheProperty<const QV4::String *>(engine, obj, name, context, local);
}

QQmlPropertyData *
QQmlPropertyCache::property(QJSEngine *engine, QObject *obj, const QStringRef &name,
                            QQmlContextData *context, QQmlPropertyData &local)
{
    return qQmlPropertyCacheProperty<const QStringRef &>(engine, obj, name, context, local);
}

QQmlPropertyData *
QQmlPropertyCache::property(QJSEngine *engine, QObject *obj, const QLatin1String &name,
                            QQmlContextData *context, QQmlPropertyData &local)
{
    return qQmlPropertyCacheProperty<const QLatin1String &>(engine, obj, name, context, local);
}

// these two functions are copied from qmetaobject.cpp
static inline const QMetaObjectPrivate *priv(const uint* data)
{ return reinterpret_cast<const QMetaObjectPrivate*>(data); }

static inline const QByteArray stringData(const QMetaObject *mo, int index)
{
    Q_ASSERT(priv(mo->d.data)->revision >= 7);
    const QByteArrayDataPtr data = { const_cast<QByteArrayData*>(&mo->d.stringdata[index]) };
    Q_ASSERT(data.ptr->ref.isStatic());
    Q_ASSERT(data.ptr->alloc == 0);
    Q_ASSERT(data.ptr->capacityReserved == 0);
    Q_ASSERT(data.ptr->size >= 0);
    return data;
}

bool QQmlPropertyCache::isDynamicMetaObject(const QMetaObject *mo)
{
    return priv(mo->d.data)->revision >= 3 && priv(mo->d.data)->flags & DynamicMetaObject;
}

const char *QQmlPropertyCache::className() const
{
    if (!_ownMetaObject && _metaObject)
        return _metaObject->className();
    else
        return _dynamicClassName.constData();
}

void QQmlPropertyCache::toMetaObjectBuilder(QMetaObjectBuilder &builder)
{
    struct Sort { static bool lt(const QPair<QString, QQmlPropertyData *> &lhs,
                                 const QPair<QString, QQmlPropertyData *> &rhs) {
        return lhs.second->coreIndex() < rhs.second->coreIndex();
    } };

    struct Insert { static void in(QQmlPropertyCache *This,
                                   QList<QPair<QString, QQmlPropertyData *> > &properties,
                                   QList<QPair<QString, QQmlPropertyData *> > &methods,
                                   StringCache::ConstIterator iter, QQmlPropertyData *data) {
        if (data->isSignalHandler())
            return;

        if (data->isFunction()) {
            if (data->coreIndex() < This->methodIndexCacheStart)
                return;

            QPair<QString, QQmlPropertyData *> entry = qMakePair((QString)iter.key(), data);
            // Overrides can cause the entry to already exist
            if (!methods.contains(entry)) methods.append(entry);

            data = This->overrideData(data);
            if (data && !data->isFunction()) Insert::in(This, properties, methods, iter, data);
        } else {
            if (data->coreIndex() < This->propertyIndexCacheStart)
                return;

            QPair<QString, QQmlPropertyData *> entry = qMakePair((QString)iter.key(), data);
            // Overrides can cause the entry to already exist
            if (!properties.contains(entry)) properties.append(entry);

            data = This->overrideData(data);
            if (data) Insert::in(This, properties, methods, iter, data);
        }

    } };

    builder.setClassName(_dynamicClassName);

    QList<QPair<QString, QQmlPropertyData *> > properties;
    QList<QPair<QString, QQmlPropertyData *> > methods;

    for (StringCache::ConstIterator iter = stringCache.begin(), cend = stringCache.end(); iter != cend; ++iter)
        Insert::in(this, properties, methods, iter, iter.value().second);

    Q_ASSERT(properties.count() == propertyIndexCache.count());
    Q_ASSERT(methods.count() == methodIndexCache.count());

    std::sort(properties.begin(), properties.end(), Sort::lt);
    std::sort(methods.begin(), methods.end(), Sort::lt);

    for (int ii = 0; ii < properties.count(); ++ii) {
        QQmlPropertyData *data = properties.at(ii).second;

        int notifierId = -1;
        if (data->notifyIndex() != -1)
            notifierId = data->notifyIndex() - signalHandlerIndexCacheStart;

        QMetaPropertyBuilder property = builder.addProperty(properties.at(ii).first.toUtf8(),
                                                            QMetaType::typeName(data->propType()),
                                                            notifierId);

        property.setReadable(true);
        property.setWritable(data->isWritable());
        property.setResettable(data->isResettable());
    }

    for (int ii = 0; ii < methods.count(); ++ii) {
        QQmlPropertyData *data = methods.at(ii).second;

        QByteArray returnType;
        if (data->propType() != 0)
            returnType = QMetaType::typeName(data->propType());

        QByteArray signature;
        // '+=' reserves extra capacity. Follow-up appending will be probably free.
        signature += methods.at(ii).first.toUtf8() + '(';

        QQmlPropertyCacheMethodArguments *arguments = nullptr;
        if (data->hasArguments()) {
            arguments = (QQmlPropertyCacheMethodArguments *)data->arguments();
            for (int ii = 0; ii < arguments->arguments[0]; ++ii) {
                if (ii != 0) signature.append(',');
                signature.append(QMetaType::typeName(arguments->arguments[1 + ii]));
            }
        }

        signature.append(')');

        QMetaMethodBuilder method;
        if (data->isSignal()) {
            method = builder.addSignal(signature);
        } else {
            method = builder.addSlot(signature);
        }
        method.setAccess(QMetaMethod::Public);

        if (arguments && arguments->names)
            method.setParameterNames(*arguments->names);

        if (!returnType.isEmpty())
            method.setReturnType(returnType);
    }

    for (int ii = 0; ii < enumCache.count(); ++ii) {
        const QQmlEnumData &enumData = enumCache.at(ii);
        QMetaEnumBuilder enumeration = builder.addEnumerator(enumData.name.toUtf8());
        enumeration.setIsScoped(true);
        for (int jj = 0; jj < enumData.values.count(); ++jj) {
            const QQmlEnumValue &value = enumData.values.at(jj);
            enumeration.addKey(value.namedValue.toUtf8(), value.value);
        }
    }

    if (!_defaultPropertyName.isEmpty()) {
        QQmlPropertyData *dp = property(_defaultPropertyName, nullptr, nullptr);
        if (dp && dp->coreIndex() >= propertyIndexCacheStart) {
            Q_ASSERT(!dp->isFunction());
            builder.addClassInfo("DefaultProperty", _defaultPropertyName.toUtf8());
        }
    }
}

namespace {
template <typename StringVisitor, typename TypeInfoVisitor>
int visitMethods(const QMetaObject &mo, int methodOffset, int methodCount,
                 StringVisitor visitString, TypeInfoVisitor visitTypeInfo)
{
    const int intsPerMethod = 5;

    int fieldsForParameterData = 0;

    bool hasRevisionedMethods = false;

    for (int i = 0; i < methodCount; ++i) {
        const int handle = methodOffset + i * intsPerMethod;

        const uint flags = mo.d.data[handle + 4];
        if (flags & MethodRevisioned)
            hasRevisionedMethods = true;

        visitString(mo.d.data[handle + 0]); // name
        visitString(mo.d.data[handle + 3]); // tag

        const int argc = mo.d.data[handle + 1];
        const int paramIndex = mo.d.data[handle + 2];

        fieldsForParameterData += argc * 2; // type and name
        fieldsForParameterData += 1; // + return type

        // return type + args
        for (int i = 0; i < 1 + argc; ++i) {
            // type name (maybe)
            visitTypeInfo(mo.d.data[paramIndex + i]);

            // parameter name
            if (i > 0)
                visitString(mo.d.data[paramIndex + argc + i]);
        }
    }

    int fieldsForRevisions = 0;
    if (hasRevisionedMethods)
        fieldsForRevisions = methodCount;

    return methodCount * intsPerMethod + fieldsForRevisions + fieldsForParameterData;
}

template <typename StringVisitor, typename TypeInfoVisitor>
int visitProperties(const QMetaObject &mo, StringVisitor visitString, TypeInfoVisitor visitTypeInfo)
{
    const QMetaObjectPrivate *const priv = reinterpret_cast<const QMetaObjectPrivate*>(mo.d.data);
    const int intsPerProperty = 3;

    bool hasRevisionedProperties = false;
    bool hasNotifySignals = false;

    for (int i = 0; i < priv->propertyCount; ++i) {
        const int handle = priv->propertyData + i * intsPerProperty;

        const auto flags = mo.d.data[handle + 2];
        if (flags & Revisioned) {
            hasRevisionedProperties = true;
        }
        if (flags & Notify)
            hasNotifySignals = true;

        visitString(mo.d.data[handle]); // name
        visitTypeInfo(mo.d.data[handle + 1]);
    }

    int fieldsForPropertyRevisions = 0;
    if (hasRevisionedProperties)
        fieldsForPropertyRevisions = priv->propertyCount;

    int fieldsForNotifySignals = 0;
    if (hasNotifySignals)
        fieldsForNotifySignals = priv->propertyCount;

    return priv->propertyCount * intsPerProperty + fieldsForPropertyRevisions
            + fieldsForNotifySignals;
}

template <typename StringVisitor>
int visitClassInfo(const QMetaObject &mo, StringVisitor visitString)
{
    const QMetaObjectPrivate *const priv = reinterpret_cast<const QMetaObjectPrivate*>(mo.d.data);
    const int intsPerClassInfo = 2;

    for (int i = 0; i < priv->classInfoCount; ++i) {
        const int handle = priv->classInfoData + i * intsPerClassInfo;

        visitString(mo.d.data[handle]); // key
        visitString(mo.d.data[handle + 1]); // value
    }

    return priv->classInfoCount * intsPerClassInfo;
}

template <typename StringVisitor>
int visitEnumerations(const QMetaObject &mo, StringVisitor visitString)
{
    const QMetaObjectPrivate *const priv = reinterpret_cast<const QMetaObjectPrivate*>(mo.d.data);
    const int intsPerEnumerator = priv->revision >= 8 ? 5 : 4;

    int fieldCount = priv->enumeratorCount * intsPerEnumerator;

    for (int i = 0; i < priv->enumeratorCount; ++i) {
        const uint *enumeratorData = mo.d.data + priv->enumeratorData + i * intsPerEnumerator;

        const uint keyCount = enumeratorData[intsPerEnumerator == 5 ? 3 : 2];
        fieldCount += keyCount * 2;

        visitString(enumeratorData[0]); // name
        if (intsPerEnumerator == 5)
            visitString(enumeratorData[1]); // enum name

        const uint keyOffset = enumeratorData[intsPerEnumerator == 5 ? 4 : 3];

        for (uint j = 0; j < keyCount; ++j) {
            visitString(mo.d.data[keyOffset + 2 * j]);
        }
    }

    return fieldCount;
}

template <typename StringVisitor>
int countMetaObjectFields(const QMetaObject &mo, StringVisitor stringVisitor)
{
    const QMetaObjectPrivate *const priv = reinterpret_cast<const QMetaObjectPrivate*>(mo.d.data);

    const auto typeInfoVisitor = [&stringVisitor](uint typeInfo) {
        if (typeInfo & IsUnresolvedType)
            stringVisitor(typeInfo & TypeNameIndexMask);
    };

    int fieldCount = MetaObjectPrivateFieldCount;

    fieldCount += visitMethods(mo, priv->methodData, priv->methodCount, stringVisitor,
                               typeInfoVisitor);
    fieldCount += visitMethods(mo, priv->constructorData, priv->constructorCount, stringVisitor,
                               typeInfoVisitor);

    fieldCount += visitProperties(mo, stringVisitor, typeInfoVisitor);
    fieldCount += visitClassInfo(mo, stringVisitor);
    fieldCount += visitEnumerations(mo, stringVisitor);

    return fieldCount;
}

} // anonymous namespace

bool QQmlPropertyCache::determineMetaObjectSizes(const QMetaObject &mo, int *fieldCount,
                                                 int *stringCount)
{
    const QMetaObjectPrivate *priv = reinterpret_cast<const QMetaObjectPrivate*>(mo.d.data);
    if (priv->revision < 7 || priv->revision > 8) {
        return false;
    }

    uint highestStringIndex = 0;
    const auto stringIndexVisitor = [&highestStringIndex](uint index) {
        highestStringIndex = qMax(highestStringIndex, index);
    };

    *fieldCount = countMetaObjectFields(mo, stringIndexVisitor);
    *stringCount = highestStringIndex + 1;

    return true;
}

bool QQmlPropertyCache::addToHash(QCryptographicHash &hash, const QMetaObject &mo)
{
    int fieldCount = 0;
    int stringCount = 0;
    if (!determineMetaObjectSizes(mo, &fieldCount, &stringCount)) {
        return false;
    }

    hash.addData(reinterpret_cast<const char *>(mo.d.data), fieldCount * sizeof(uint));
    for (int i = 0; i < stringCount; ++i) {
        hash.addData(stringData(&mo, i));
    }

    return true;
}

QByteArray QQmlPropertyCache::checksum(bool *ok)
{
    if (!_checksum.isEmpty()) {
        *ok = true;
        return _checksum;
    }

    // Generate a checksum on the meta-object data only on C++ types.
    if (!_metaObject || _ownMetaObject) {
        *ok = false;
        return _checksum;
    }

    QCryptographicHash hash(QCryptographicHash::Md5);

    if (_parent) {
        hash.addData(_parent->checksum(ok));
        if (!*ok)
            return QByteArray();
    }

    if (!addToHash(hash, *createMetaObject())) {
        *ok = false;
        return QByteArray();
    }

    _checksum = hash.result();
    *ok = !_checksum.isEmpty();
    return _checksum;
}

/*! \internal
    \a index MUST be in the signal index range (see QObjectPrivate::signalIndex()).
    This is different from QMetaMethod::methodIndex().
*/
QList<QByteArray> QQmlPropertyCache::signalParameterNames(int index) const
{
    QQmlPropertyData *signalData = signal(index);
    if (signalData && signalData->hasArguments()) {
        QQmlPropertyCacheMethodArguments *args = (QQmlPropertyCacheMethodArguments *)signalData->arguments();
        if (args && args->names)
            return *args->names;
        const QMetaMethod &method = QMetaObjectPrivate::signal(firstCppMetaObject(), index);
        return method.parameterNames();
    }
    return QList<QByteArray>();
}

QT_END_NAMESPACE
