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

#include "qv4serialize_p.h"

#include <private/qv4value_p.h>
#include <private/qv4dateobject_p.h>
#include <private/qv4regexpobject_p.h>
#if QT_CONFIG(qml_sequence_object)
#include <private/qv4sequenceobject_p.h>
#endif
#include <private/qv4objectproto_p.h>
#include <private/qv4qobjectwrapper_p.h>

QT_BEGIN_NAMESPACE

using namespace QV4;

// We allow the following JavaScript types to be passed between the main and
// the secondary thread:
//    + undefined
//    + null
//    + Boolean
//    + String
//    + Function
//    + Array
//    + "Simple" Objects
//    + Number
//    + Date
//    + RegExp
// <quint8 type><quint24 size><data>

enum Type {
    WorkerUndefined,
    WorkerNull,
    WorkerTrue,
    WorkerFalse,
    WorkerString,
    WorkerFunction,
    WorkerArray,
    WorkerObject,
    WorkerInt32,
    WorkerUint32,
    WorkerNumber,
    WorkerDate,
    WorkerRegexp,
    WorkerListModel,
    WorkerUrl,
#if QT_CONFIG(qml_sequence_object)
    WorkerSequence
#endif
};

static inline quint32 valueheader(Type type, quint32 size = 0)
{
    return quint8(type) << 24 | (size & 0xFFFFFF);
}

static inline Type headertype(quint32 header)
{
    return (Type)(header >> 24);
}

static inline quint32 headersize(quint32 header)
{
    return header & 0xFFFFFF;
}

static inline void push(QByteArray &data, quint32 value)
{
    data.append((const char *)&value, sizeof(quint32));
}

static inline void push(QByteArray &data, double value)
{
    data.append((const char *)&value, sizeof(double));
}

static inline void push(QByteArray &data, void *ptr)
{
    data.append((const char *)&ptr, sizeof(void *));
}

static inline void reserve(QByteArray &data, int extra)
{
    data.reserve(data.size() + extra);
}

static inline quint32 popUint32(const char *&data)
{
    quint32 rv = *((const quint32 *)data);
    data += sizeof(quint32);
    return rv;
}

static inline double popDouble(const char *&data)
{
    double rv = *((const double *)data);
    data += sizeof(double);
    return rv;
}

static inline void *popPtr(const char *&data)
{
    void *rv = *((void *const *)data);
    data += sizeof(void *);
    return rv;
}

#define ALIGN(size) (((size) + 3) & ~3)
static inline void serializeString(QByteArray &data, const QString &str, Type type)
{
    int length = str.length();
    if (length > 0xFFFFFF) {
        push(data, valueheader(WorkerUndefined));
        return;
    }
    int utf16size = ALIGN(length * sizeof(quint16));

    reserve(data, utf16size + sizeof(quint32));
    push(data, valueheader(type, length));

    int offset = data.size();
    data.resize(data.size() + utf16size);
    char *buffer = data.data() + offset;

    memcpy(buffer, str.constData(), length*sizeof(QChar));
}

// XXX TODO: Check that worker script is exception safe in the case of
// serialization/deserialization failures

void Serialize::serialize(QByteArray &data, const QV4::Value &v, ExecutionEngine *engine)
{
    QV4::Scope scope(engine);

    if (v.isEmpty()) {
        Q_ASSERT(!"Serialize: got empty value");
    } else if (v.isUndefined()) {
        push(data, valueheader(WorkerUndefined));
    } else if (v.isNull()) {
        push(data, valueheader(WorkerNull));
    } else if (v.isBoolean()) {
        push(data, valueheader(v.booleanValue() == true ? WorkerTrue : WorkerFalse));
    } else if (v.isString()) {
        serializeString(data, v.toQString(), WorkerString);
    } else if (v.as<FunctionObject>()) {
        // XXX TODO: Implement passing function objects between the main and
        // worker scripts
        push(data, valueheader(WorkerUndefined));
    } else if (const QV4::ArrayObject *array = v.as<ArrayObject>()) {
        uint length = array->getLength();
        if (length > 0xFFFFFF) {
            push(data, valueheader(WorkerUndefined));
            return;
        }
        reserve(data, sizeof(quint32) + length * sizeof(quint32));
        push(data, valueheader(WorkerArray, length));
        ScopedValue val(scope);
        for (uint ii = 0; ii < length; ++ii)
            serialize(data, (val = array->get(ii)), engine);
    } else if (v.isInteger()) {
        reserve(data, 2 * sizeof(quint32));
        push(data, valueheader(WorkerInt32));
        push(data, (quint32)v.integerValue());
//    } else if (v.IsUint32()) {
//        reserve(data, 2 * sizeof(quint32));
//        push(data, valueheader(WorkerUint32));
//        push(data, v.Uint32Value());
    } else if (v.isNumber()) {
        reserve(data, sizeof(quint32) + sizeof(double));
        push(data, valueheader(WorkerNumber));
        push(data, v.asDouble());
    } else if (const QV4::DateObject *d = v.as<DateObject>()) {
        reserve(data, sizeof(quint32) + sizeof(double));
        push(data, valueheader(WorkerDate));
        push(data, d->date());
    } else if (const RegExpObject *re = v.as<RegExpObject>()) {
        quint32 flags = re->flags();
        QString pattern = re->source();
        int length = pattern.length() + 1;
        if (length > 0xFFFFFF) {
            push(data, valueheader(WorkerUndefined));
            return;
        }
        int utf16size = ALIGN(length * sizeof(quint16));

        reserve(data, sizeof(quint32) + utf16size);
        push(data, valueheader(WorkerRegexp, flags));
        push(data, (quint32)length);

        int offset = data.size();
        data.resize(data.size() + utf16size);
        char *buffer = data.data() + offset;

        memcpy(buffer, pattern.constData(), length*sizeof(QChar));
    } else if (const QObjectWrapper *qobjectWrapper = v.as<QV4::QObjectWrapper>()) {
        // XXX TODO: Generalize passing objects between the main thread and worker scripts so
        // that others can trivially plug in their elements.
        if (QObject *lm = qobjectWrapper->object()) {
            if (QObject *agent = qvariant_cast<QObject *>(lm->property("agent"))) {
                if (QMetaObject::invokeMethod(agent, "addref")) {
                    push(data, valueheader(WorkerListModel));
                    push(data, (void *)agent);
                    return;
                }
            }
        }
        // No other QObject's are allowed to be sent
        push(data, valueheader(WorkerUndefined));
    } else if (const Object *o = v.as<Object>()) {
#if QT_CONFIG(qml_sequence_object)
        if (o->isListType()) {
            // valid sequence.  we generate a length (sequence length + 1 for the sequence type)
            uint seqLength = ScopedValue(scope, o->get(engine->id_length()))->toUInt32();
            uint length = seqLength + 1;
            if (length > 0xFFFFFF) {
                push(data, valueheader(WorkerUndefined));
                return;
            }
            reserve(data, sizeof(quint32) + length * sizeof(quint32));
            push(data, valueheader(WorkerSequence, length));
            serialize(data, QV4::Value::fromInt32(QV4::SequencePrototype::metaTypeForSequence(o)), engine); // sequence type
            ScopedValue val(scope);
            for (uint ii = 0; ii < seqLength; ++ii)
                serialize(data, (val = o->get(ii)), engine); // sequence elements

            return;
        }
#endif
        const QVariant variant = engine->toVariant(v, QMetaType::QUrl, false);
        if (variant.userType() == QMetaType::QUrl) {
            serializeString(data, variant.value<QUrl>().toString(), WorkerUrl);
            return;
        }

        // regular object
        QV4::ScopedValue val(scope, v);
        QV4::ScopedArrayObject properties(scope, QV4::ObjectPrototype::getOwnPropertyNames(engine, val));
        quint32 length = properties->getLength();
        if (length > 0xFFFFFF) {
            push(data, valueheader(WorkerUndefined));
            return;
        }
        push(data, valueheader(WorkerObject, length));

        QV4::ScopedValue s(scope);
        for (quint32 ii = 0; ii < length; ++ii) {
            s = properties->get(ii);
            serialize(data, s, engine);

            QV4::String *str = s->as<String>();
            val = o->get(str);
            if (scope.hasException())
                scope.engine->catchException();

            serialize(data, val, engine);
        }
        return;
    } else {
        push(data, valueheader(WorkerUndefined));
    }
}

struct VariantRef
{
    VariantRef() : obj(nullptr) {}
    VariantRef(const VariantRef &r) : obj(r.obj) { addref(); }
    VariantRef(QObject *a) : obj(a) { addref(); }
    ~VariantRef() { release(); }

    VariantRef &operator=(const VariantRef &o) {
        o.addref();
        release();
        obj = o.obj;
        return *this;
    }

    void addref() const
    {
        if (obj)
            QMetaObject::invokeMethod(obj, "addref");
    }

    void release() const
    {
        if (obj)
            QMetaObject::invokeMethod(obj, "release");

    }

    QObject *obj;
};

QT_END_NAMESPACE
Q_DECLARE_METATYPE(VariantRef)
Q_DECLARE_METATYPE(QV4::ExecutionEngine *)
QT_BEGIN_NAMESPACE

ReturnedValue Serialize::deserialize(const char *&data, ExecutionEngine *engine)
{
    quint32 header = popUint32(data);
    Type type = headertype(header);

    Scope scope(engine);

    switch (type) {
    case WorkerUndefined:
        return QV4::Encode::undefined();
    case WorkerNull:
        return QV4::Encode::null();
    case WorkerTrue:
        return QV4::Encode(true);
    case WorkerFalse:
        return QV4::Encode(false);
    case WorkerString:
    case WorkerUrl:
    {
        quint32 size = headersize(header);
        QString qstr((const QChar *)data, size);
        data += ALIGN(size * sizeof(quint16));
        return  (type == WorkerUrl)
                ? engine->fromVariant(QVariant::fromValue(QUrl(qstr)))
                : Encode(engine->newString(qstr));
    }
    case WorkerFunction:
        Q_ASSERT(!"Unreachable");
        break;
    case WorkerArray:
    {
        quint32 size = headersize(header);
        ScopedArrayObject a(scope, engine->newArrayObject());
        ScopedValue v(scope);
        for (quint32 ii = 0; ii < size; ++ii) {
            v = deserialize(data, engine);
            a->put(ii, v);
        }
        return a.asReturnedValue();
    }
    case WorkerObject:
    {
        quint32 size = headersize(header);
        ScopedObject o(scope, engine->newObject());
        ScopedValue name(scope);
        ScopedString n(scope);
        ScopedValue value(scope);
        for (quint32 ii = 0; ii < size; ++ii) {
            name = deserialize(data, engine);
            value = deserialize(data, engine);
            n = name->asReturnedValue();
            o->put(n, value);
        }
        return o.asReturnedValue();
    }
    case WorkerInt32:
        return QV4::Encode((qint32)popUint32(data));
    case WorkerUint32:
        return QV4::Encode(popUint32(data));
    case WorkerNumber:
        return QV4::Encode(popDouble(data));
    case WorkerDate:
        return QV4::Encode(engine->newDateObject(QV4::Value::fromDouble(popDouble(data))));
    case WorkerRegexp:
    {
        quint32 flags = headersize(header);
        quint32 length = popUint32(data);
        QString pattern = QString((const QChar *)data, length - 1);
        data += ALIGN(length * sizeof(quint16));
        return Encode(engine->newRegExpObject(pattern, flags));
    }
    case WorkerListModel:
    {
        QObject *agent = reinterpret_cast<QObject *>(popPtr(data));
        QV4::ScopedValue rv(scope, QV4::QObjectWrapper::wrap(engine, agent));
        // ### Find a better solution then the ugly property
        VariantRef ref(agent);
        QVariant var = QVariant::fromValue(ref);
        QV4::ScopedValue v(scope, scope.engine->fromVariant(var));
        QV4::ScopedString s(scope, engine->newString(QStringLiteral("__qml:hidden:ref")));
        rv->as<Object>()->defineReadonlyProperty(s, v);

        QMetaObject::invokeMethod(agent, "release");
        agent->setProperty("engine", QVariant::fromValue(engine));
        return rv->asReturnedValue();
    }
#if QT_CONFIG(qml_sequence_object)
    case WorkerSequence:
    {
        ScopedValue value(scope);
        bool succeeded = false;
        quint32 length = headersize(header);
        quint32 seqLength = length - 1;
        value = deserialize(data, engine);
        int sequenceType = value->integerValue();
        ScopedArrayObject array(scope, engine->newArrayObject());
        array->arrayReserve(seqLength);
        for (quint32 ii = 0; ii < seqLength; ++ii) {
            value = deserialize(data, engine);
            array->arrayPut(ii, value);
        }
        array->setArrayLengthUnchecked(seqLength);
        QVariant seqVariant = QV4::SequencePrototype::toVariant(array, sequenceType, &succeeded);
        return QV4::SequencePrototype::fromVariant(engine, seqVariant, &succeeded);
    }
#endif
    }
    Q_ASSERT(!"Unreachable");
    return QV4::Encode::undefined();
}

QByteArray Serialize::serialize(const QV4::Value &value, ExecutionEngine *engine)
{
    QByteArray rv;
    serialize(rv, value, engine);
    return rv;
}

ReturnedValue Serialize::deserialize(const QByteArray &data, ExecutionEngine *engine)
{
    const char *stream = data.constData();
    return deserialize(stream, engine);
}

QT_END_NAMESPACE
