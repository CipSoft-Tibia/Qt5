// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLTYPE_P_H
#define QQMLTYPE_P_H

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

#include <functional>

#include <private/qtqmlglobal_p.h>
#include <private/qqmlrefcount_p.h>

#include <QtQml/qqmlprivate.h>
#include <QtQml/qjsvalue.h>

#include <QtCore/qobject.h>
#include <QtCore/qversionnumber.h>

QT_BEGIN_NAMESPACE

class QHashedCStringRef;
class QQmlTypePrivate;
class QHashedString;
class QHashedStringRef;
class QQmlCustomParser;
class QQmlEnginePrivate;
class QQmlPropertyCache;

namespace QV4 {
struct String;
}

class Q_QML_PRIVATE_EXPORT QQmlType
{
public:
    QQmlType();
    QQmlType(const QQmlType &other);
    QQmlType(QQmlType &&other);
    QQmlType &operator =(const QQmlType &other);
    QQmlType &operator =(QQmlType &&other);
    explicit QQmlType(const QQmlTypePrivate *priv);
    ~QQmlType();

    bool operator ==(const QQmlType &other) const {
        return d.data() == other.d.data();
    }

    bool isValid() const { return !d.isNull(); }

    QByteArray typeName() const;
    QString qmlTypeName() const;
    QString elementName() const;

    QHashedString module() const;
    QTypeRevision version() const;

    bool availableInVersion(QTypeRevision version) const;
    bool availableInVersion(const QHashedStringRef &module, QTypeRevision version) const;

    typedef QVariant (*CreateValueTypeFunc)(const QJSValue &);
    CreateValueTypeFunc createValueTypeFunction() const;

    bool canConstructValueType() const;
    bool canPopulateValueType() const;

    QObject *create() const;
    QObject *create(void **, size_t) const;
    QObject *createWithQQmlData() const;

    typedef void (*CreateFunc)(void *, void *);
    CreateFunc createFunction() const;

    QQmlCustomParser *customParser() const;

    bool isCreatable() const;
    typedef QObject *(*ExtensionFunc)(QObject *);
    ExtensionFunc extensionFunction() const;
    const QMetaObject *extensionMetaObject() const;
    bool isExtendedType() const;
    QString noCreationReason() const;

    bool isSingleton() const;
    bool isInterface() const;
    bool isComposite() const;
    bool isCompositeSingleton() const;
    bool isQObjectSingleton() const;
    bool isQJSValueSingleton() const;
    bool isSequentialContainer() const;
    bool isValueType() const;

    QMetaType typeId() const;
    QMetaType qListTypeId() const;
    QMetaSequence listMetaSequence() const;

    const QMetaObject *metaObject() const;

    // Precondition: The type is actually a value type!
    const QMetaObject *metaObjectForValueType() const;

    const QMetaObject *baseMetaObject() const;
    QTypeRevision metaObjectRevision() const;
    bool containsRevisionedAttributes() const;

    QQmlAttachedPropertiesFunc attachedPropertiesFunction(QQmlEnginePrivate *engine) const;
    const QMetaObject *attachedPropertiesType(QQmlEnginePrivate *engine) const;

    int parserStatusCast() const;
    const char *interfaceIId() const;
    int propertyValueSourceCast() const;
    int propertyValueInterceptorCast() const;
    int finalizerCast() const;

    int index() const;

    bool isInlineComponentType() const;

    struct Q_QML_PRIVATE_EXPORT SingletonInstanceInfo final
        : public QQmlRefCounted<SingletonInstanceInfo>
    {
        using Ptr = QQmlRefPointer<SingletonInstanceInfo>;
        using ConstPtr = QQmlRefPointer<const SingletonInstanceInfo>;

        static Ptr create() { return Ptr(new SingletonInstanceInfo, Ptr::Adopt); }

        std::function<QJSValue(QQmlEngine *, QJSEngine *)> scriptCallback = {};
        std::function<QObject *(QQmlEngine *, QJSEngine *)> qobjectCallback = {};
        QByteArray typeName;
        QUrl url; // used by composite singletons

    private:
        Q_DISABLE_COPY_MOVE(SingletonInstanceInfo)
        SingletonInstanceInfo() = default;
    };
    SingletonInstanceInfo::ConstPtr singletonInstanceInfo() const;

    QUrl sourceUrl() const;

    int enumValue(QQmlEnginePrivate *engine, const QHashedStringRef &, bool *ok) const;
    int enumValue(QQmlEnginePrivate *engine, const QHashedCStringRef &, bool *ok) const;
    int enumValue(QQmlEnginePrivate *engine, const QV4::String *, bool *ok) const;

    int scopedEnumIndex(QQmlEnginePrivate *engine, const QV4::String *, bool *ok) const;
    int scopedEnumIndex(QQmlEnginePrivate *engine, const QString &, bool *ok) const;
    int scopedEnumValue(QQmlEnginePrivate *engine, int index, const QV4::String *, bool *ok) const;
    int scopedEnumValue(QQmlEnginePrivate *engine, int index, const QString &, bool *ok) const;
    int scopedEnumValue(QQmlEnginePrivate *engine, const QHashedStringRef &, const QHashedStringRef &, bool *ok) const;

    const QQmlTypePrivate *priv() const { return d.data(); }
    static void refHandle(const QQmlTypePrivate *priv);
    static void derefHandle(const QQmlTypePrivate *priv);
    static int refCount(const QQmlTypePrivate *priv);

    enum RegistrationType {
        CppType = 0,
        SingletonType = 1,
        InterfaceType = 2,
        CompositeType = 3,
        CompositeSingletonType = 4,
        InlineComponentType = 5,
        SequentialContainerType = 6,
        AnyRegistrationType = 255
    };

    void createProxy(QObject *instance) const;

private:
    friend class QQmlTypePrivate;
    friend size_t qHash(const QQmlType &t, size_t seed);
    QQmlRefPointer<const QQmlTypePrivate> d;
};

inline size_t qHash(const QQmlType &t, size_t seed = 0)
{
    return qHash(reinterpret_cast<quintptr>(t.d.data()), seed);
}

QT_END_NAMESPACE

#endif // QQMLTYPE_P_H
