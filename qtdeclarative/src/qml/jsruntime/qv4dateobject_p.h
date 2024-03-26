// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QV4DATEOBJECT_P_H
#define QV4DATEOBJECT_P_H

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

#include "qv4object_p.h"
#include "qv4functionobject_p.h"
#include "qv4referenceobject_p.h"
#include <QtCore/private/qnumeric_p.h>
#include <QtCore/qdatetime.h>

QT_BEGIN_NAMESPACE

class QDateTime;

namespace QV4 {

struct Date
{
    static constexpr quint64 MaxDateVal = 8.64e15;

    void init() { storage = InvalidDateVal; }
    void init(double value);
    void init(const QDateTime &dateTime);
    void init(QDate date);
    void init(QTime time, ExecutionEngine *engine);

    Date &operator=(double value)
    {
        storage = (storage & (HasQDate | HasQTime)) | encode(value);
        return *this;
    }

    operator double() const
    {
        const quint64 raw = (storage & ~(HasQDate | HasQTime));
        if (raw == 0)
            return qt_qnan();

        if (raw > MaxDateVal)
            return double(raw - MaxDateVal - Extra);

        return double(raw) - double(MaxDateVal) - double(Extra);
    }

    QDate toQDate() const;
    QTime toQTime() const;
    QDateTime toQDateTime() const;
    QVariant toVariant() const;

    template<typename Function>
    bool withStoragePointer(Function function)
    {
        switch (storage & (HasQDate | HasQTime)) {
        case HasQDate: {
            QDate date = toQDate();
            return function(&date);
        }
        case HasQTime: {
            QTime time = toQTime();
            return function(&time);
        }
        case (HasQTime | HasQDate): {
            QDateTime dateTime = toQDateTime();
            return function(&dateTime);
        }
        default:
            return false;
        }
    }

private:
    static constexpr quint64 InvalidDateVal = 0;
    static constexpr quint64 Extra = 1;
    static constexpr quint64 HasQDate = 1ull << 63;
    static constexpr quint64 HasQTime = 1ull << 62;

    // Make all our dates fit into quint64, leaving space for the flags
    static_assert(((MaxDateVal * 2 + Extra) & (HasQDate | HasQTime)) == 0ull);

    static quint64 encode(double value);
    static quint64 encode(const QDateTime &dateTime);

    quint64 storage;
};

namespace Heap {

#define DateObjectMembers(class, Member)
DECLARE_HEAP_OBJECT(DateObject, ReferenceObject) {
    DECLARE_MARKOBJECTS(DateObject);

    void doSetLocation()
    {
        if (CppStackFrame *frame = internalClass->engine->currentStackFrame)
            setLocation(frame->v4Function, frame->statementNumber());
    }

    void init()
    {
        ReferenceObject::init(nullptr, -1, {});
        m_date.init();
    }

    void init(double dateTime)
    {
        ReferenceObject::init(nullptr, -1, {});
        m_date.init(dateTime);
    }

    void init(const QDateTime &dateTime)
    {
        ReferenceObject::init(nullptr, -1, {});
        m_date.init(dateTime);
    }

    void init(const QDateTime &dateTime, Heap::Object *parent, int property, Flags flags)
    {
        ReferenceObject::init(parent, property, flags | EnforcesLocation);
        doSetLocation();
        m_date.init(dateTime);
    };

    void init(QDate date, Heap::Object *parent, int property, Flags flags)
    {
        ReferenceObject::init(parent, property, flags | EnforcesLocation);
        doSetLocation();
        m_date.init(date);
    };

    void init(QTime time, Heap::Object *parent, int property, Flags flags)
    {
        ReferenceObject::init(parent, property, flags | EnforcesLocation);
        doSetLocation();
        m_date.init(time, internalClass->engine);
    };

    void setDate(double newDate)
    {
        m_date = newDate;
        if (isAttachedToProperty())
            writeBack();
    }

    double date() const
    {
        return m_date;
    }

    QVariant toVariant() const { return m_date.toVariant(); }
    QDateTime toQDateTime() const { return m_date.toQDateTime(); }

private:
    bool writeBack()
    {
        if (!object() || !canWriteBack())
            return false;

        QV4::Scope scope(internalClass->engine);
        QV4::ScopedObject o(scope, object());

        int flags = 0;
        int status = -1;
        if (isVariant()) {
            QVariant variant = toVariant();
            void *a[] = { &variant, nullptr, &status, &flags };
            return o->metacall(QMetaObject::WriteProperty, property(), a);
        }

        return m_date.withStoragePointer([&](void *storagePointer) {
            void *a[] = { storagePointer, nullptr, &status, &flags };
            return o->metacall(QMetaObject::WriteProperty, property(), a);
        });
    }

    Date m_date;
};


struct DateCtor : FunctionObject {
    void init(QV4::ExecutionContext *scope);
};

}

struct DateObject: ReferenceObject {
    V4_OBJECT2(DateObject, ReferenceObject)
    Q_MANAGED_TYPE(DateObject)
    V4_PROTOTYPE(datePrototype)

    void setDate(double date) { d()->setDate(date); }
    double date() const { return d()->date(); }

    Q_QML_PRIVATE_EXPORT QDateTime toQDateTime() const;
    QString toString() const;

    static QString dateTimeToString(const QDateTime &dateTime, ExecutionEngine *engine);
    static QDate dateTimeToDate(const QDateTime &dateTime);
    static QDateTime stringToDateTime(const QString &string, ExecutionEngine *engine);
};

template<>
inline const DateObject *Value::as() const {
    return isManaged() && m()->internalClass->vtable->type == Managed::Type_DateObject ? static_cast<const DateObject *>(this) : nullptr;
}

struct DateCtor: FunctionObject
{
    V4_OBJECT2(DateCtor, FunctionObject)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int);
};

struct DatePrototype: Object
{
    V4_PROTOTYPE(objectPrototype)

    void init(ExecutionEngine *engine, Object *ctor);

    static double getThisDate(ExecutionEngine *v4, const Value *thisObject);

    static ReturnedValue method_parse(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_UTC(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_now(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);

    static ReturnedValue method_toString(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toDateString(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toTimeString(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toLocaleString(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toLocaleDateString(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toLocaleTimeString(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_valueOf(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_getTime(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_getYear(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_getFullYear(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_getUTCFullYear(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_getMonth(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_getUTCMonth(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_getDate(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_getUTCDate(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_getDay(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_getUTCDay(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_getHours(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_getUTCHours(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_getMinutes(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_getUTCMinutes(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_getSeconds(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_getUTCSeconds(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_getMilliseconds(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_getUTCMilliseconds(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_getTimezoneOffset(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_setTime(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_setMilliseconds(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_setUTCMilliseconds(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_setSeconds(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_setUTCSeconds(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_setMinutes(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_setUTCMinutes(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_setHours(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_setUTCHours(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_setDate(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_setUTCDate(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_setMonth(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_setUTCMonth(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_setYear(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_setFullYear(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_setUTCFullYear(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toUTCString(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toISOString(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toJSON(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_symbolToPrimitive(const FunctionObject *f, const Value *thisObject, const Value *, int);

    static void timezoneUpdated(ExecutionEngine *e);
};

}

QT_END_NAMESPACE

#endif // QV4ECMAOBJECTS_P_H
