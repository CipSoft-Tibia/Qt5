// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QQMLPROPERTYVALIDATOR_P_H
#define QQMLPROPERTYVALIDATOR_P_H

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

#include <private/qqmlengine_p.h>
#include <private/qqmlimport_p.h>
#include <private/qqmljsdiagnosticmessage_p.h>
#include <private/qqmlpropertycache_p.h>
#include <private/qv4compileddata_p.h>

#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

class QQmlPropertyValidator
{
    Q_DECLARE_TR_FUNCTIONS(QQmlPropertyValidator)
public:
    QQmlPropertyValidator(QQmlEnginePrivate *enginePrivate, const QQmlImports *imports, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit);

    QVector<QQmlError> validate();

    QQmlPropertyCache::ConstPtr rootPropertyCache() const { return propertyCaches.at(0); }
    QUrl documentSourceUrl() const { return compilationUnit->url(); }

private:
    QVector<QQmlError> validateObject(
            int objectIndex, const QV4::CompiledData::Binding *instantiatingBinding,
            bool populatingValueTypeGroupProperty = false) const;
    QQmlError validateLiteralBinding(
            const QQmlPropertyCache::ConstPtr &propertyCache, const QQmlPropertyData *property,
            const QV4::CompiledData::Binding *binding) const;
    QQmlError validateObjectBinding(
            const QQmlPropertyData *property, const QString &propertyName,
            const QV4::CompiledData::Binding *binding) const;

    bool canCoerce(QMetaType to, QQmlPropertyCache::ConstPtr fromMo) const;

    Q_REQUIRED_RESULT QVector<QQmlError> recordError(
            const QV4::CompiledData::Location &location, const QString &description) const;
    Q_REQUIRED_RESULT QVector<QQmlError> recordError(const QQmlError &error) const;
    QString stringAt(int index) const { return compilationUnit->stringAt(index); }
    QV4::ResolvedTypeReference *resolvedType(int id) const
    {
        return compilationUnit->resolvedType(id);
    }

    QQmlEnginePrivate *enginePrivate;
    QQmlRefPointer<QV4::ExecutableCompilationUnit> compilationUnit;
    const QQmlImports *imports;
    const QV4::CompiledData::Unit *qmlUnit;
    const QQmlPropertyCacheVector &propertyCaches;

    QVector<QV4::BindingPropertyData> * const bindingPropertyDataPerObject;
};

QT_END_NAMESPACE

#endif // QQMLPROPERTYVALIDATOR_P_H
