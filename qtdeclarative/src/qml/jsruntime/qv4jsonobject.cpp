// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include <qv4jsonobject_p.h>
#include <qv4objectproto_p.h>
#include <qv4numberobject_p.h>
#include <qv4stringobject_p.h>
#include <qv4booleanobject_p.h>
#include <qv4objectiterator_p.h>
#include <qv4scopedvalue_p.h>
#include <qv4runtime_p.h>
#include <qv4variantobject_p.h>
#include "qv4jscall_p.h"
#include <qv4symbol_p.h>

#include <qstack.h>
#include <qstringlist.h>

#include <wtf/MathExtras.h>

using namespace QV4;

//#define PARSER_DEBUG
#ifdef PARSER_DEBUG
static int indent = 0;
#define BEGIN qDebug() << QByteArray(4*indent++, ' ').constData()
#define END --indent
#define DEBUG qDebug() << QByteArray(4*indent, ' ').constData()
#else
#define BEGIN if (1) ; else qDebug()
#define END do {} while (0)
#define DEBUG if (1) ; else qDebug()
#endif


DEFINE_OBJECT_VTABLE(JsonObject);

static const int nestingLimit = 1024;


JsonParser::JsonParser(ExecutionEngine *engine, const QChar *json, int length)
    : engine(engine), head(json), json(json), nestingLevel(0), lastError(QJsonParseError::NoError)
{
    end = json + length;
}



/*

begin-array     = ws %x5B ws  ; [ left square bracket

begin-object    = ws %x7B ws  ; { left curly bracket

end-array       = ws %x5D ws  ; ] right square bracket

end-object      = ws %x7D ws  ; } right curly bracket

name-separator  = ws %x3A ws  ; : colon

value-separator = ws %x2C ws  ; , comma

Insignificant whitespace is allowed before or after any of the six
structural characters.

ws = *(
          %x20 /              ; Space
          %x09 /              ; Horizontal tab
          %x0A /              ; Line feed or New line
          %x0D                ; Carriage return
      )

*/

enum {
    Space = 0x20,
    Tab = 0x09,
    LineFeed = 0x0a,
    Return = 0x0d,
    BeginArray = 0x5b,
    BeginObject = 0x7b,
    EndArray = 0x5d,
    EndObject = 0x7d,
    NameSeparator = 0x3a,
    ValueSeparator = 0x2c,
    Quote = 0x22
};

bool JsonParser::eatSpace()
{
    while (json < end) {
        const char16_t ch = json->unicode();
        if (ch > Space)
            break;
        if (ch != Space &&
            ch != Tab &&
            ch != LineFeed &&
            ch != Return)
            break;
        ++json;
    }
    return (json < end);
}

QChar JsonParser::nextToken()
{
    if (!eatSpace())
        return u'\0';
    QChar token = *json++;
    switch (token.unicode()) {
    case BeginArray:
    case BeginObject:
    case NameSeparator:
    case ValueSeparator:
    case EndArray:
    case EndObject:
        eatSpace();
    case Quote:
        break;
    default:
        token = u'\0';
        break;
    }
    return token;
}

/*
    JSON-text = object / array
*/
ReturnedValue JsonParser::parse(QJsonParseError *error)
{
#ifdef PARSER_DEBUG
    indent = 0;
    qDebug() << ">>>>> parser begin";
#endif

    eatSpace();

    Scope scope(engine);
    ScopedValue v(scope);
    if (!parseValue(v)) {
#ifdef PARSER_DEBUG
        qDebug() << ">>>>> parser error";
#endif
        if (lastError == QJsonParseError::NoError)
            lastError = QJsonParseError::IllegalValue;
        error->offset = json - head;
        error->error  = lastError;
        return Encode::undefined();
    }

    // some input left...
    if (eatSpace()) {
        lastError = QJsonParseError::IllegalValue;
        error->offset = json - head;
        error->error  = lastError;
        return Encode::undefined();
    }

    END;
    error->offset = 0;
    error->error = QJsonParseError::NoError;
    return v->asReturnedValue();
}

/*
    object = begin-object [ member *( value-separator member ) ]
    end-object
*/

ReturnedValue JsonParser::parseObject()
{
    if (++nestingLevel > nestingLimit) {
        lastError = QJsonParseError::DeepNesting;
        return Encode::undefined();
    }

    BEGIN << "parseObject pos=" << json;
    Scope scope(engine);

    ScopedObject o(scope, engine->newObject());

    QChar token = nextToken();
    while (token.unicode() == Quote) {
        if (!parseMember(o))
            return Encode::undefined();
        token = nextToken();
        if (token.unicode() != ValueSeparator)
            break;
        token = nextToken();
        if (token.unicode() == EndObject) {
            lastError = QJsonParseError::MissingObject;
            return Encode::undefined();
        }
    }

    DEBUG << "end token=" << token;
    if (token.unicode() != EndObject) {
        lastError = QJsonParseError::UnterminatedObject;
        return Encode::undefined();
    }

    END;

    --nestingLevel;
    return o.asReturnedValue();
}

/*
    member = string name-separator value
*/
bool JsonParser::parseMember(Object *o)
{
    BEGIN << "parseMember";
    Scope scope(engine);

    QString key;
    if (!parseString(&key))
        return false;
    QChar token = nextToken();
    if (token.unicode() != NameSeparator) {
        lastError = QJsonParseError::MissingNameSeparator;
        return false;
    }
    ScopedValue val(scope);
    if (!parseValue(val))
        return false;

    ScopedString s(scope, engine->newString(key));
    PropertyKey skey = s->toPropertyKey();
    if (skey.isArrayIndex()) {
        o->put(skey.asArrayIndex(), val);
    } else {
        // avoid trouble with properties named __proto__
        o->insertMember(s, val);
    }

    END;
    return true;
}

/*
    array = begin-array [ value *( value-separator value ) ] end-array
*/
ReturnedValue JsonParser::parseArray()
{
    Scope scope(engine);
    BEGIN << "parseArray";
    ScopedArrayObject array(scope, engine->newArrayObject());

    if (++nestingLevel > nestingLimit) {
        lastError = QJsonParseError::DeepNesting;
        return Encode::undefined();
    }

    if (!eatSpace()) {
        lastError = QJsonParseError::UnterminatedArray;
        return Encode::undefined();
    }
    if (json->unicode() == EndArray) {
        nextToken();
    } else {
        uint index = 0;
        while (1) {
            ScopedValue val(scope);
            if (!parseValue(val))
                return Encode::undefined();
            array->arraySet(index, val);
            QChar token = nextToken();
            if (token.unicode() == EndArray)
                break;
            else if (token.unicode() != ValueSeparator) {
                if (!eatSpace())
                    lastError = QJsonParseError::UnterminatedArray;
                else
                    lastError = QJsonParseError::MissingValueSeparator;
                return Encode::undefined();
            }
            ++index;
        }
    }

    DEBUG << "size =" << array->getLength();
    END;

    --nestingLevel;
    return array.asReturnedValue();
}

/*
value = false / null / true / object / array / number / string

*/

bool JsonParser::parseValue(Value *val)
{
    BEGIN << "parse Value" << *json;

    switch ((json++)->unicode()) {
    case u'n':
        if (end - json < 3) {
            lastError = QJsonParseError::IllegalValue;
            return false;
        }
        if (*json++ == u'u' &&
            *json++ == u'l' &&
            *json++ == u'l') {
            *val = Value::nullValue();
            DEBUG << "value: null";
            END;
            return true;
        }
        lastError = QJsonParseError::IllegalValue;
        return false;
    case u't':
        if (end - json < 3) {
            lastError = QJsonParseError::IllegalValue;
            return false;
        }
        if (*json++ == u'r' &&
            *json++ == u'u' &&
            *json++ == u'e') {
            *val = Value::fromBoolean(true);
            DEBUG << "value: true";
            END;
            return true;
        }
        lastError = QJsonParseError::IllegalValue;
        return false;
    case u'f':
        if (end - json < 4) {
            lastError = QJsonParseError::IllegalValue;
            return false;
        }
        if (*json++ == u'a' &&
            *json++ == u'l' &&
            *json++ == u's' &&
            *json++ == u'e') {
            *val = Value::fromBoolean(false);
            DEBUG << "value: false";
            END;
            return true;
        }
        lastError = QJsonParseError::IllegalValue;
        return false;
    case Quote: {
        QString value;
        if (!parseString(&value))
            return false;
        DEBUG << "value: string";
        END;
        *val = Value::fromHeapObject(engine->newString(value));
        return true;
    }
    case BeginArray: {
        *val = parseArray();
        if (val->isUndefined())
            return false;
        DEBUG << "value: array";
        END;
        return true;
    }
    case BeginObject: {
        *val = parseObject();
        if (val->isUndefined())
            return false;
        DEBUG << "value: object";
        END;
        return true;
    }
    case EndArray:
        lastError = QJsonParseError::MissingObject;
        return false;
    default:
        --json;
        if (!parseNumber(val))
            return false;
        DEBUG << "value: number";
        END;
    }

    return true;
}





/*
        number = [ minus ] int [ frac ] [ exp ]
        decimal-point = %x2E       ; .
        digit1-9 = %x31-39         ; 1-9
        e = %x65 / %x45            ; e E
        exp = e [ minus / plus ] 1*DIGIT
        frac = decimal-point 1*DIGIT
        int = zero / ( digit1-9 *DIGIT )
        minus = %x2D               ; -
        plus = %x2B                ; +
        zero = %x30                ; 0

*/

bool JsonParser::parseNumber(Value *val)
{
    BEGIN << "parseNumber" << *json;

    const QChar *start = json;
    bool isInt = true;

    // minus
    if (json < end && *json == u'-')
        ++json;

    // int = zero / ( digit1-9 *DIGIT )
    if (json < end && *json == u'0') {
        ++json;
    } else {
        while (json < end && *json >= u'0' && *json <= u'9')
            ++json;
    }

    // frac = decimal-point 1*DIGIT
    if (json < end && *json == u'.') {
        isInt = false;
        ++json;
        while (json < end && *json >= u'0' && *json <= u'9')
            ++json;
    }

    // exp = e [ minus / plus ] 1*DIGIT
    if (json < end && (*json == u'e' || *json == u'E')) {
        isInt = false;
        ++json;
        if (json < end && (*json == u'-' || *json == u'+'))
            ++json;
        while (json < end && *json >= u'0' && *json <= u'9')
            ++json;
    }

    QString number(start, json - start);
    DEBUG << "numberstring" << number;

    if (isInt) {
        bool ok;
        int n = number.toInt(&ok);
        if (ok && n < (1<<25) && n > -(1<<25)) {
            *val = Value::fromInt32(n);
            END;
            return true;
        }
    }

    bool ok;
    double d;
    d = number.toDouble(&ok);

    if (!ok) {
        lastError = QJsonParseError::IllegalNumber;
        return false;
    }

    * val = Value::fromDouble(d);

    END;
    return true;
}

/*

        string = quotation-mark *char quotation-mark

        char = unescaped /
               escape (
                   %x22 /          ; "    quotation mark  U+0022
                   %x5C /          ; \    reverse solidus U+005C
                   %x2F /          ; /    solidus         U+002F
                   %x62 /          ; b    backspace       U+0008
                   %x66 /          ; f    form feed       U+000C
                   %x6E /          ; n    line feed       U+000A
                   %x72 /          ; r    carriage return U+000D
                   %x74 /          ; t    tab             U+0009
                   %x75 4HEXDIG )  ; uXXXX                U+XXXX

        escape = %x5C              ; \

        quotation-mark = %x22      ; "

        unescaped = %x20-21 / %x23-5B / %x5D-10FFFF
 */
static inline bool addHexDigit(QChar digit, uint *result)
{
    ushort d = digit.unicode();
    *result <<= 4;
    if (d >= u'0' && d <= u'9')
        *result |= (d - u'0');
    else if (d >= u'a' && d <= u'f')
        *result |= (d - u'a') + 10;
    else if (d >= u'A' && d <= u'F')
        *result |= (d - u'A') + 10;
    else
        return false;
    return true;
}

static inline bool scanEscapeSequence(const QChar *&json, const QChar *end, uint *ch)
{
    ++json;
    if (json >= end)
        return false;

    DEBUG << "scan escape";
    uint escaped = (json++)->unicode();
    switch (escaped) {
    case u'"':
        *ch = '"'; break;
    case u'\\':
        *ch = '\\'; break;
    case u'/':
        *ch = '/'; break;
    case u'b':
        *ch = 0x8; break;
    case u'f':
        *ch = 0xc; break;
    case u'n':
        *ch = 0xa; break;
    case u'r':
        *ch = 0xd; break;
    case u't':
        *ch = 0x9; break;
    case u'u': {
        *ch = 0;
        if (json > end - 4)
            return false;
        for (int i = 0; i < 4; ++i) {
            if (!addHexDigit(*json, ch))
                return false;
            ++json;
        }
        return true;
    }
    default:
        return false;
    }
    return true;
}


bool JsonParser::parseString(QString *string)
{
    BEGIN << "parse string stringPos=" << json;

    while (json < end) {
        if (*json == u'"')
            break;
        else if (*json == u'\\') {
            uint ch = 0;
            if (!scanEscapeSequence(json, end, &ch)) {
                lastError = QJsonParseError::IllegalEscapeSequence;
                return false;
            }
            if (QChar::requiresSurrogates(ch)) {
                *string += QChar(QChar::highSurrogate(ch)) + QChar(QChar::lowSurrogate(ch));
            } else {
                *string += QChar(ch);
            }
        } else {
            if (json->unicode() <= 0x1f) {
                lastError = QJsonParseError::IllegalEscapeSequence;
                return false;
            }
            *string += *json;
            ++json;
        }
    }
    ++json;

    if (json > end) {
        lastError = QJsonParseError::UnterminatedString;
        return false;
    }

    END;
    return true;
}


struct Stringify
{
    ExecutionEngine *v4;
    FunctionObject *replacerFunction;
    QV4::String *propertyList;
    int propertyListSize;
    QString gap;
    QString indent;
    QStack<Object *> stack;

    bool stackContains(Object *o) {
        for (int i = 0; i < stack.size(); ++i)
            if (stack.at(i)->d() == o->d())
                return true;
        return false;
    }

    Stringify(ExecutionEngine *e) : v4(e), replacerFunction(nullptr), propertyList(nullptr), propertyListSize(0) {}

    QString Str(const QString &key, const Value &v);
    QString JA(Object *a);
    QString JO(Object *o);

    QString makeMember(const QString &key, const Value &v);
};

class [[nodiscard]] CallDepthAndCycleChecker
{
    Q_DISABLE_COPY_MOVE(CallDepthAndCycleChecker);

public:
    CallDepthAndCycleChecker(Stringify *stringify, Object *o)
        : m_callDepthRecorder(stringify->v4)
    {
        if (stringify->stackContains(o)) {
            stringify->v4->throwTypeError(
                        QStringLiteral("Cannot convert circular structure to JSON"));
        }

        stringify->v4->checkStackLimits();
    }

    bool foundProblem() const { return m_callDepthRecorder.ee->hasException; }

private:
    ExecutionEngineCallDepthRecorder<1> m_callDepthRecorder;
};

static QString quote(const QString &str)
{
    QString product;
    const int length = str.size();
    product.reserve(length + 2);
    product += u'"';
    for (int i = 0; i < length; ++i) {
        QChar c = str.at(i);
        switch (c.unicode()) {
        case u'"':
            product += QLatin1String("\\\"");
            break;
        case u'\\':
            product += QLatin1String("\\\\");
            break;
        case u'\b':
            product += QLatin1String("\\b");
            break;
        case u'\f':
            product += QLatin1String("\\f");
            break;
        case u'\n':
            product += QLatin1String("\\n");
            break;
        case u'\r':
            product += QLatin1String("\\r");
            break;
        case u'\t':
            product += QLatin1String("\\t");
            break;
        default:
            if (c.unicode() <= 0x1f) {
                product += QLatin1String("\\u00");
                product += (c.unicode() > 0xf ? u'1' : u'0') +
                        QLatin1Char("0123456789abcdef"[c.unicode() & 0xf]);
            } else {
                product += c;
            }
        }
    }
    product += u'"';
    return product;
}

QString Stringify::Str(const QString &key, const Value &v)
{
    Scope scope(v4);

    ScopedValue value(scope, v);
    ScopedObject o(scope, value);
    if (o) {
        ScopedString s(scope, v4->newString(QStringLiteral("toJSON")));
        ScopedFunctionObject toJSON(scope, o->get(s));
        if (!!toJSON) {
            JSCallArguments jsCallData(scope, 1);
            *jsCallData.thisObject = value;
            jsCallData.args[0] = v4->newString(key);
            value = toJSON->call(jsCallData);
            if (v4->hasException)
                return QString();
        }
    }

    if (replacerFunction) {
        JSCallArguments jsCallData(scope, 2);
        jsCallData.args[0] = v4->newString(key);
        jsCallData.args[1] = value;

        if (stack.isEmpty()) {
            ScopedObject holder(scope, v4->newObject());
            holder->put(scope.engine->id_empty(), v);
            *jsCallData.thisObject = holder;
        } else {
            *jsCallData.thisObject = stack.top();
        }

        value = replacerFunction->call(jsCallData);
        if (v4->hasException)
            return QString();
    }

    o = value->asReturnedValue();
    if (o) {
        if (NumberObject *n = o->as<NumberObject>())
            value = Encode(n->value());
        else if (StringObject *so = o->as<StringObject>())
            value = so->d()->string;
        else if (BooleanObject *b = o->as<BooleanObject>())
            value = Encode(b->value());
    }

    if (value->isNull())
        return QStringLiteral("null");
    if (value->isBoolean())
        return value->booleanValue() ? QStringLiteral("true") : QStringLiteral("false");
    if (value->isString())
        return quote(value->stringValue()->toQString());

    if (value->isNumber()) {
        double d = value->toNumber();
        return std::isfinite(d) ? value->toQString() : QStringLiteral("null");
    }

    if (const QV4::VariantObject *v = value->as<QV4::VariantObject>()) {
        return quote(v->d()->data().toString());
    }

    o = value->asReturnedValue();
    if (o) {
        if (!o->as<FunctionObject>()) {
            if (o->isArrayLike()) {
                return JA(o.getPointer());
            } else {
                return JO(o);
            }
        }
    }

    return QString();
}

QString Stringify::makeMember(const QString &key, const Value &v)
{
    QString strP = Str(key, v);
    if (!strP.isEmpty()) {
        QString member = quote(key) + u':';
        if (!gap.isEmpty())
            member += u' ';
        member += strP;
        return member;
    }
    return QString();
}

QString Stringify::JO(Object *o)
{
    CallDepthAndCycleChecker check(this, o);
    if (check.foundProblem())
        return QString();

    Scope scope(v4);

    QString result;
    stack.push(o);
    QString stepback = indent;
    indent += gap;

    QStringList partial;
    if (!propertyListSize) {
        ObjectIterator it(scope, o, ObjectIterator::EnumerableOnly);
        ScopedValue name(scope);

        ScopedValue val(scope);
        while (1) {
            name = it.nextPropertyNameAsString(val);
            if (name->isNull())
                break;
            QString key = name->toQString();
            QString member = makeMember(key, val);
            if (!member.isEmpty())
                partial += member;
        }
    } else {
        ScopedValue v(scope);
        for (int i = 0; i < propertyListSize; ++i) {
            bool exists;
            String *s = propertyList + i;
            if (!s)
                continue;
            v = o->get(s, &exists);
            if (!exists)
                continue;
            QString member = makeMember(s->toQString(), v);
            if (!member.isEmpty())
                partial += member;
        }
    }

    if (partial.isEmpty()) {
        result = QStringLiteral("{}");
    } else if (gap.isEmpty()) {
        result = u'{' + partial.join(u',') + u'}';
    } else {
        QString separator = QLatin1String(",\n") + indent;
        result = QLatin1String("{\n") + indent + partial.join(separator) + u'\n'
                 + stepback + u'}';
    }

    indent = stepback;
    stack.pop();
    return result;
}

QString Stringify::JA(Object *a)
{
    CallDepthAndCycleChecker check(this, a);
    if (check.foundProblem())
        return QString();

    Scope scope(a->engine());

    QString result;
    stack.push(a);
    QString stepback = indent;
    indent += gap;

    QStringList partial;
    uint len = a->getLength();
    ScopedValue v(scope);
    for (uint i = 0; i < len; ++i) {
        bool exists;
        v = a->get(i, &exists);
        if (!exists) {
            partial += QStringLiteral("null");
            continue;
        }
        QString strP = Str(QString::number(i), v);
        if (!strP.isEmpty())
            partial += strP;
        else
            partial += QStringLiteral("null");
    }

    if (partial.isEmpty()) {
        result = QStringLiteral("[]");
    } else if (gap.isEmpty()) {
        result = u'[' + partial.join(u',') + u']';
    } else {
        QString separator = QLatin1String(",\n") + indent;
        result = QLatin1String("[\n") + indent + partial.join(separator) + u'\n' + stepback + u']';
    }

    indent = stepback;
    stack.pop();
    return result;
}


void Heap::JsonObject::init()
{
    Object::init();
    Scope scope(internalClass->engine);
    ScopedObject o(scope, this);

    o->defineDefaultProperty(QStringLiteral("parse"), QV4::JsonObject::method_parse, 2);
    o->defineDefaultProperty(QStringLiteral("stringify"), QV4::JsonObject::method_stringify, 3);
    ScopedString json(scope, scope.engine->newString(QStringLiteral("JSON")));
    o->defineReadonlyConfigurableProperty(scope.engine->symbol_toStringTag(), json);
}


ReturnedValue JsonObject::method_parse(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    QString jtext;
    if (argc > 0)
        jtext = argv[0].toQString();

    DEBUG << "parsing source = " << jtext;
    JsonParser parser(v4, jtext.constData(), jtext.size());
    QJsonParseError error;
    ReturnedValue result = parser.parse(&error);
    if (error.error != QJsonParseError::NoError) {
        DEBUG << "parse error" << error.errorString();
        RETURN_RESULT(v4->throwSyntaxError(QStringLiteral("JSON.parse: Parse error")));
    }

    return result;
}

ReturnedValue JsonObject::method_stringify(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    Scope scope(b);
    Stringify stringify(scope.engine);

    ScopedObject o(scope, argc > 1 ? argv[1] : Value::undefinedValue());
    if (o) {
        stringify.replacerFunction = o->as<FunctionObject>();
        if (o->isArrayObject()) {
            int arrayLen = scope.engine->safeForAllocLength(o->getLength());
            CHECK_EXCEPTION();
            stringify.propertyList = static_cast<QV4::String *>(scope.alloc(arrayLen));
            for (int i = 0; i < arrayLen; ++i) {
                Value *v = stringify.propertyList + i;
                *v = o->get(i);
                if (v->as<NumberObject>() || v->as<StringObject>() || v->isNumber())
                    *v = v->toString(scope.engine);
                if (!v->isString()) {
                    v->setM(nullptr);
                } else {
                    for (int j = 0; j <i; ++j) {
                        if (stringify.propertyList[j].m() == v->m()) {
                            v->setM(nullptr);
                            break;
                        }
                    }
                }
            }
        }
    }

    ScopedValue s(scope, argc > 2 ? argv[2] : Value::undefinedValue());
    if (NumberObject *n = s->as<NumberObject>())
        s = Encode(n->value());
    else if (StringObject *so = s->as<StringObject>())
        s = so->d()->string;

    if (s->isNumber()) {
        stringify.gap = QString(qMin(10, (int)s->toInteger()), u' ');
    } else if (String *str = s->stringValue()) {
        stringify.gap = str->toQString().left(10);
    }


    ScopedValue arg0(scope, argc ? argv[0] : Value::undefinedValue());
    QString result = stringify.Str(QString(), arg0);
    if (result.isEmpty() || scope.hasException())
        RETURN_UNDEFINED();
    return Encode(scope.engine->newString(result));
}



ReturnedValue JsonObject::fromJsonValue(ExecutionEngine *engine, const QJsonValue &value)
{
    if (value.isString())
        return engine->newString(value.toString())->asReturnedValue();
    else if (value.isDouble())
        return Encode(value.toDouble());
    else if (value.isBool())
        return Encode(value.toBool());
    else if (value.isArray())
        return fromJsonArray(engine, value.toArray());
    else if (value.isObject())
        return fromJsonObject(engine, value.toObject());
    else if (value.isNull())
        return Encode::null();
    else
        return Encode::undefined();
}

QJsonValue JsonObject::toJsonValue(const Value &value, V4ObjectSet &visitedObjects)
{
    if (value.isNumber())
        return QJsonValue(value.toNumber());
    else if (value.isBoolean())
        return QJsonValue((bool)value.booleanValue());
    else if (value.isNull())
        return QJsonValue(QJsonValue::Null);
    else if (value.isUndefined())
        return QJsonValue(QJsonValue::Undefined);
    else if (String *s = value.stringValue())
        return QJsonValue(s->toQString());

    Q_ASSERT(value.isObject());
    Scope scope(value.as<Object>()->engine());
    ScopedArrayObject a(scope, value);
    if (a)
        return toJsonArray(a, visitedObjects);
    ScopedObject o(scope, value);
    if (o)
        return toJsonObject(o, visitedObjects);
    return QJsonValue(value.toQString());
}

QV4::ReturnedValue JsonObject::fromJsonObject(ExecutionEngine *engine, const QJsonObject &object)
{
    Scope scope(engine);
    ScopedObject o(scope, engine->newObject());
    ScopedString s(scope);
    ScopedValue v(scope);
    for (QJsonObject::const_iterator it = object.begin(), cend = object.end(); it != cend; ++it) {
        v = fromJsonValue(engine, it.value());
        o->put((s = engine->newString(it.key())), v);
    }
    return o.asReturnedValue();
}

QJsonObject JsonObject::toJsonObject(const Object *o, V4ObjectSet &visitedObjects)
{
    QJsonObject result;
    if (!o || o->as<FunctionObject>())
        return result;

    Scope scope(o->engine());

    if (visitedObjects.contains(ObjectItem(o))) {
        // Avoid recursion.
        // For compatibility with QVariant{List,Map} conversion, we return an
        // empty object (and no error is thrown).
        return result;
    }

    visitedObjects.insert(ObjectItem(o));

    ObjectIterator it(scope, o, ObjectIterator::EnumerableOnly);
    ScopedValue name(scope);
    QV4::ScopedValue val(scope);
    while (1) {
        name = it.nextPropertyNameAsString(val);
        if (name->isNull())
            break;

        QString key = name->toQStringNoThrow();
        if (!val->as<FunctionObject>())
            result.insert(key, toJsonValue(val, visitedObjects));
    }

    visitedObjects.remove(ObjectItem(o));

    return result;
}

QV4::ReturnedValue JsonObject::fromJsonArray(ExecutionEngine *engine, const QJsonArray &array)
{
    Scope scope(engine);
    int size = array.size();
    ScopedArrayObject a(scope, engine->newArrayObject());
    a->arrayReserve(size);
    ScopedValue v(scope);
    for (int i = 0; i < size; i++)
        a->arrayPut(i, (v = fromJsonValue(engine, array.at(i))));
    a->setArrayLengthUnchecked(size);
    return a.asReturnedValue();
}

QJsonArray JsonObject::toJsonArray(const ArrayObject *a, V4ObjectSet &visitedObjects)
{
    QJsonArray result;
    if (!a)
        return result;

    Scope scope(a->engine());

    if (visitedObjects.contains(ObjectItem(a))) {
        // Avoid recursion.
        // For compatibility with QVariant{List,Map} conversion, we return an
        // empty array (and no error is thrown).
        return result;
    }

    visitedObjects.insert(ObjectItem(a));

    ScopedValue v(scope);
    quint32 length = a->getLength();
    for (quint32 i = 0; i < length; ++i) {
        v = a->get(i);
        if (v->as<FunctionObject>())
            v = Encode::null();
        result.append(toJsonValue(v, visitedObjects));
    }

    visitedObjects.remove(ObjectItem(a));

    return result;
}
