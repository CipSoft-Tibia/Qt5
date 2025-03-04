// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlpropertycachecreator_p.h"

#include <private/qqmlengine_p.h>

#if QT_CONFIG(regularexpression)
#include <QtCore/qregularexpression.h>
#endif

QT_BEGIN_NAMESPACE

QAtomicInt QQmlPropertyCacheCreatorBase::classIndexCounter(0);


QMetaType QQmlPropertyCacheCreatorBase::metaTypeForPropertyType(QV4::CompiledData::CommonType type)
{
    switch (type) {
    case QV4::CompiledData::CommonType::Void:     return QMetaType();
    case QV4::CompiledData::CommonType::Var:      return QMetaType::fromType<QVariant>();
    case QV4::CompiledData::CommonType::Int:      return QMetaType::fromType<int>();
    case QV4::CompiledData::CommonType::Bool:     return QMetaType::fromType<bool>();
    case QV4::CompiledData::CommonType::Real:     return QMetaType::fromType<qreal>();
    case QV4::CompiledData::CommonType::String:   return QMetaType::fromType<QString>();
    case QV4::CompiledData::CommonType::Url:      return QMetaType::fromType<QUrl>();
    case QV4::CompiledData::CommonType::Time:     return QMetaType::fromType<QTime>();
    case QV4::CompiledData::CommonType::Date:     return QMetaType::fromType<QDate>();
    case QV4::CompiledData::CommonType::DateTime: return QMetaType::fromType<QDateTime>();
#if QT_CONFIG(regularexpression)
    case QV4::CompiledData::CommonType::RegExp:   return QMetaType::fromType<QRegularExpression>();
#else
    case QV4::CompiledData::CommonType::RegExp:   return QMetaType();
#endif
    case QV4::CompiledData::CommonType::Rect:     return QMetaType::fromType<QRectF>();
    case QV4::CompiledData::CommonType::Point:    return QMetaType::fromType<QPointF>();
    case QV4::CompiledData::CommonType::Size:     return QMetaType::fromType<QSizeF>();
    case QV4::CompiledData::CommonType::Invalid:  break;
    };
    return QMetaType {};
}

QMetaType QQmlPropertyCacheCreatorBase::listTypeForPropertyType(QV4::CompiledData::CommonType type)
{
    switch (type) {
    case QV4::CompiledData::CommonType::Void:     return QMetaType();
    case QV4::CompiledData::CommonType::Var:      return QMetaType::fromType<QList<QVariant>>();
    case QV4::CompiledData::CommonType::Int:      return QMetaType::fromType<QList<int>>();
    case QV4::CompiledData::CommonType::Bool:     return QMetaType::fromType<QList<bool>>();
    case QV4::CompiledData::CommonType::Real:     return QMetaType::fromType<QList<qreal>>();
    case QV4::CompiledData::CommonType::String:   return QMetaType::fromType<QList<QString>>();
    case QV4::CompiledData::CommonType::Url:      return QMetaType::fromType<QList<QUrl>>();
    case QV4::CompiledData::CommonType::Time:     return QMetaType::fromType<QList<QTime>>();
    case QV4::CompiledData::CommonType::Date:     return QMetaType::fromType<QList<QDate>>();
    case QV4::CompiledData::CommonType::DateTime: return QMetaType::fromType<QList<QDateTime>>();
#if QT_CONFIG(regularexpression)
    case QV4::CompiledData::CommonType::RegExp:   return QMetaType::fromType<QList<QRegularExpression>>();
#else
    case QV4::CompiledData::CommonType::RegExp:   return QMetaType();
#endif
    case QV4::CompiledData::CommonType::Rect:     return QMetaType::fromType<QList<QRectF>>();
    case QV4::CompiledData::CommonType::Point:    return QMetaType::fromType<QList<QPointF>>();
    case QV4::CompiledData::CommonType::Size:     return QMetaType::fromType<QList<QSizeF>>();
    case QV4::CompiledData::CommonType::Invalid:  break;
    };
    return QMetaType {};
}

template<typename BaseNameHandler, typename FailHandler>
auto processUrlForClassName(
    const QUrl &url, BaseNameHandler &&baseNameHandler, FailHandler &&failHandler)
{
    const QString path = url.path();

    // Not a reusable type if we don't have an absolute Url
    const qsizetype lastSlash = path.lastIndexOf(QLatin1Char('/'));
    if (lastSlash <= -1)
        return failHandler();

    // ### this might not be correct for .ui.qml files
    const QStringView baseName = QStringView{path}.mid(lastSlash + 1, path.size() - lastSlash - 5);

    // Not a reusable type if it doesn't start with a upper case letter.
    return (!baseName.isEmpty() && baseName.at(0).isUpper())
        ? baseNameHandler(baseName)
        : failHandler();
}

bool QQmlPropertyCacheCreatorBase::canCreateClassNameTypeByUrl(const QUrl &url)
{
    return processUrlForClassName(url, [](QStringView) {
        return true;
    }, []() {
        return false;
    });
}

QByteArray QQmlPropertyCacheCreatorBase::createClassNameTypeByUrl(const QUrl &url)
{
    return processUrlForClassName(url, [](QStringView nameBase) {
        return nameBase.toUtf8() + QByteArray("_QMLTYPE_");
    }, []() {
        return QByteArray("ANON_QML_TYPE_");
    }) + QByteArray::number(classIndexCounter.fetchAndAddRelaxed(1));
}

QByteArray QQmlPropertyCacheCreatorBase::createClassNameForInlineComponent(
    const QUrl &baseUrl, const QString &name)
{
    QByteArray baseName = processUrlForClassName(baseUrl, [](QStringView nameBase) {
        return QByteArray(nameBase.toUtf8() + "_QMLTYPE_");
    }, []() {
        return QByteArray("ANON_QML_IC_");
    });
    return baseName + name.toUtf8() + '_'
            + QByteArray::number(classIndexCounter.fetchAndAddRelaxed(1));
}

QQmlBindingInstantiationContext::QQmlBindingInstantiationContext(
        int referencingObjectIndex,
        const QV4::CompiledData::Binding *instantiatingBinding,
        const QString &instantiatingPropertyName,
        const QQmlPropertyCache::ConstPtr &referencingObjectPropertyCache)
    : referencingObjectIndex(referencingObjectIndex)
    , instantiatingBinding(instantiatingBinding)
    , instantiatingPropertyName(instantiatingPropertyName)
    , referencingObjectPropertyCache(referencingObjectPropertyCache)
{
}

bool QQmlBindingInstantiationContext::resolveInstantiatingProperty()
{
    if (!instantiatingBinding
           || instantiatingBinding->type() != QV4::CompiledData::Binding::Type_GroupProperty) {
        return true;
    }

    if (!referencingObjectPropertyCache)
        return false;

    Q_ASSERT(referencingObjectIndex >= 0);
    Q_ASSERT(instantiatingBinding->propertyNameIndex != 0);

    bool notInRevision = false;
    instantiatingProperty = QQmlPropertyResolver(referencingObjectPropertyCache)
                                    .property(instantiatingPropertyName, &notInRevision,
                                              QQmlPropertyResolver::IgnoreRevision);
    return instantiatingProperty != nullptr;
}

QQmlPropertyCache::ConstPtr QQmlBindingInstantiationContext::instantiatingPropertyCache() const
{
    if (instantiatingProperty) {
        if (instantiatingProperty->isQObject()) {
            // rawPropertyCacheForType assumes a given unspecified version means "any version".
            // There is another overload that takes no version, which we shall not use here.
            auto result = QQmlMetaType::rawPropertyCacheForType(instantiatingProperty->propType(),
                                                         instantiatingProperty->typeVersion());
            if (result)
                return result;
            /* We might end up here if there's a grouped property, and the type hasn't been registered.
               Still try to get a property cache, as long as the type of the property is well-behaved
               (i.e., not dynamic)*/
            if (auto metaObject = instantiatingProperty->propType().metaObject(); metaObject) {
                // we'll warn about dynamic meta-object later in the property validator
                if (!(QMetaObjectPrivate::get(metaObject)->flags & DynamicMetaObject))
                    return QQmlMetaType::propertyCache(metaObject);
            }
            // fall through intentional
        } else if (const QMetaObject *vtmo = QQmlMetaType::metaObjectForValueType(instantiatingProperty->propType())) {
            return QQmlMetaType::propertyCache(vtmo, instantiatingProperty->typeVersion());
        }
    }
    return QQmlPropertyCache::ConstPtr();
}

void QQmlPendingGroupPropertyBindings::resolveMissingPropertyCaches(
        QQmlPropertyCacheVector *propertyCaches) const
{
    for (QQmlBindingInstantiationContext pendingBinding: *this) {
        const int groupPropertyObjectIndex = pendingBinding.instantiatingBinding->value.objectIndex;

        if (propertyCaches->at(groupPropertyObjectIndex))
            continue;

        Q_ASSERT(!pendingBinding.instantiatingPropertyName.isEmpty());

        if (!pendingBinding.referencingObjectPropertyCache) {
            pendingBinding.referencingObjectPropertyCache
                    = propertyCaches->at(pendingBinding.referencingObjectIndex);
        }

        if (!pendingBinding.resolveInstantiatingProperty())
            continue;
        auto cache = pendingBinding.instantiatingPropertyCache();
        propertyCaches->set(groupPropertyObjectIndex, cache);
    }
}

QT_END_NAMESPACE
