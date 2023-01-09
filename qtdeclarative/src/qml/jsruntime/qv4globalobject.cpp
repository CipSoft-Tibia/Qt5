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

#include "qv4globalobject_p.h"
#include <private/qv4mm_p.h>
#include "qv4value_p.h"
#include "qv4context_p.h"
#include "qv4function_p.h"
#include "qv4debugging_p.h"
#include "qv4profiling_p.h"
#include "qv4script_p.h"
#include "qv4scopedvalue_p.h"
#include "qv4string_p.h"
#include "qv4jscall_p.h"

#include <private/qv4codegen_p.h>
#include <private/qv4alloca_p.h>
#include "private/qlocale_tools_p.h"
#include "private/qtools_p.h"

#include <QtCore/QDebug>
#include <QtCore/QString>
#include <iostream>

#include <wtf/MathExtras.h>

using namespace QV4;
using QtMiscUtils::toHexUpper;
using QtMiscUtils::fromHex;

static QString escape(const QString &input)
{
    QString output;
    output.reserve(input.size() * 3);
    const int length = input.length();
    for (int i = 0; i < length; ++i) {
        ushort uc = input.at(i).unicode();
        if (uc < 0x100) {
            if (   (uc > 0x60 && uc < 0x7B)
                || (uc > 0x3F && uc < 0x5B)
                || (uc > 0x2C && uc < 0x3A)
                || (uc == 0x2A)
                || (uc == 0x2B)
                || (uc == 0x5F)) {
                output.append(QChar(uc));
            } else {
                output.append('%');
                output.append(QLatin1Char(toHexUpper(uc >> 4)));
                output.append(QLatin1Char(toHexUpper(uc)));
            }
        } else {
            output.append('%');
            output.append('u');
            output.append(QLatin1Char(toHexUpper(uc >> 12)));
            output.append(QLatin1Char(toHexUpper(uc >> 8)));
            output.append(QLatin1Char(toHexUpper(uc >> 4)));
            output.append(QLatin1Char(toHexUpper(uc)));
        }
    }
    return output;
}

static QString unescape(const QString &input)
{
    QString result;
    result.reserve(input.length());
    int i = 0;
    const int length = input.length();
    while (i < length) {
        QChar c = input.at(i++);
        if ((c == '%') && (i + 1 < length)) {
            QChar a = input.at(i);
            if ((a == 'u') && (i + 4 < length)) {
                int d3 = fromHex(input.at(i+1).unicode());
                int d2 = fromHex(input.at(i+2).unicode());
                int d1 = fromHex(input.at(i+3).unicode());
                int d0 = fromHex(input.at(i+4).unicode());
                if ((d3 != -1) && (d2 != -1) && (d1 != -1) && (d0 != -1)) {
                    ushort uc = ushort((d3 << 12) | (d2 << 8) | (d1 << 4) | d0);
                    result.append(QChar(uc));
                    i += 5;
                } else {
                    result.append(c);
                }
            } else {
                int d1 = fromHex(a.unicode());
                int d0 = fromHex(input.at(i+1).unicode());
                if ((d1 != -1) && (d0 != -1)) {
                    c = (d1 << 4) | d0;
                    i += 2;
                }
                result.append(c);
            }
        } else {
            result.append(c);
        }
    }
    return result;
}

static const char uriReserved[] = ";/?:@&=+$,#";
static const char uriUnescaped[] = "-_.!~*'()";
static const char uriUnescapedReserved[] = "-_.!~*'();/?:@&=+$,#";

static void addEscapeSequence(QString &output, uchar ch)
{
    output.append(QLatin1Char('%'));
    output.append(QLatin1Char(toHexUpper(ch >> 4)));
    output.append(QLatin1Char(toHexUpper(ch & 0xf)));
}

static QString encode(const QString &input, const char *unescapedSet, bool *ok)
{
    *ok = true;
    QString output;
    const int length = input.length();
    int i = 0;
    while (i < length) {
        const QChar c = input.at(i);
        bool escape = true;
        if ((c.unicode() >= 'a' && c.unicode() <= 'z') ||
            (c.unicode() >= 'A' && c.unicode() <= 'Z') ||
            (c.unicode() >= '0' && c.unicode() <= '9')) {
            escape = false;
        } else {
            const char *r = unescapedSet;
            while (*r) {
                if (*r == c.unicode()) {
                    escape = false;
                    break;
                }
                ++r;
            }
        }
        if (escape) {
            uint uc = c.unicode();
            if ((uc >= 0xDC00) && (uc <= 0xDFFF)) {
                *ok = false;
                break;
            }
            if (!((uc < 0xD800) || (uc > 0xDBFF))) {
                ++i;
                if (i == length) {
                    *ok = false;
                    break;
                }
                const uint uc2 = input.at(i).unicode();
                if ((uc2 < 0xDC00) || (uc2 > 0xDFFF)) {
                    *ok = false;
                    break;
                }
                uc = ((uc - 0xD800) * 0x400) + (uc2 - 0xDC00) + 0x10000;
            }
            if (uc < 0x80) {
                addEscapeSequence(output, (uchar)uc);
            } else {
                if (uc < 0x0800) {
                    addEscapeSequence(output, 0xc0 | ((uchar) (uc >> 6)));
                } else {

                    if (QChar::requiresSurrogates(uc)) {
                        addEscapeSequence(output, 0xf0 | ((uchar) (uc >> 18)));
                        addEscapeSequence(output, 0x80 | (((uchar) (uc >> 12)) & 0x3f));
                    } else {
                        addEscapeSequence(output, 0xe0 | (((uchar) (uc >> 12)) & 0x3f));
                    }
                    addEscapeSequence(output, 0x80 | (((uchar) (uc >> 6)) & 0x3f));
                }
                addEscapeSequence(output, 0x80 | ((uchar) (uc&0x3f)));
            }
        } else {
            output.append(c);
        }
        ++i;
    }
    if (i != length)
        *ok = false;
    return output;
}

enum DecodeMode {
    DecodeAll,
    DecodeNonReserved
};

static QString decode(const QString &input, DecodeMode decodeMode, bool *ok)
{
    *ok = true;
    QString output;
    output.reserve(input.length());
    const int length = input.length();
    int i = 0;
    const QChar percent = QLatin1Char('%');
    while (i < length) {
        const QChar ch = input.at(i);
        if (ch == percent) {
            int start = i;
            if (i + 2 >= length)
                goto error;

            int d1 = fromHex(input.at(i+1).unicode());
            int d0 = fromHex(input.at(i+2).unicode());
            if ((d1 == -1) || (d0 == -1))
                goto error;

            int b = (d1 << 4) | d0;
            i += 2;
            if (b & 0x80) {
                int uc;
                int min_uc;
                int need;
                if ((b & 0xe0) == 0xc0) {
                    uc = b & 0x1f;
                    need = 1;
                    min_uc = 0x80;
                } else if ((b & 0xf0) == 0xe0) {
                    uc = b & 0x0f;
                    need = 2;
                    min_uc = 0x800;
                } else if ((b & 0xf8) == 0xf0) {
                    uc = b & 0x07;
                    need = 3;
                    min_uc = 0x10000;
                } else {
                    goto error;
                }

                if (i + (3 * need) >= length)
                    goto error;

                for (int j = 0; j < need; ++j) {
                    ++i;
                    if (input.at(i) != percent)
                        goto error;

                    d1 = fromHex(input.at(i+1).unicode());
                    d0 = fromHex(input.at(i+2).unicode());
                    if ((d1 == -1) || (d0 == -1))
                        goto error;

                    b = (d1 << 4) | d0;
                    if ((b & 0xC0) != 0x80)
                        goto error;

                    i += 2;
                    uc = (uc << 6) + (b & 0x3f);
                }
                if (uc < min_uc)
                    goto error;

                if (uc < 0x10000) {
                    output.append(QChar(uc));
                } else {
                    if (uc > 0x10FFFF)
                        goto error;

                    ushort l = ushort(((uc - 0x10000) & 0x3FF) + 0xDC00);
                    ushort h = ushort((((uc - 0x10000) >> 10) & 0x3FF) + 0xD800);
                    output.append(QChar(h));
                    output.append(QChar(l));
                }
            } else {
                if (decodeMode == DecodeNonReserved && b <= 0x40) {
                    const char *r = uriReserved;
                    while (*r) {
                        if (*r == b)
                            break;
                        ++r;
                    }
                    if (*r)
                        output.append(input.midRef(start, i - start + 1));
                    else
                        output.append(QChar(b));
                } else {
                    output.append(QChar(b));
                }
            }
        } else {
            output.append(ch);
        }
        ++i;
    }
    if (i != length)
        *ok = false;
    return output;
  error:
    *ok = false;
    return QString();
}

DEFINE_OBJECT_VTABLE(EvalFunction);

void Heap::EvalFunction::init(QV4::ExecutionContext *scope)
{
    Scope s(scope);
    Heap::FunctionObject::init(scope, s.engine->id_eval());
    ScopedFunctionObject f(s, this);
    f->defineReadonlyConfigurableProperty(s.engine->id_length(), Value::fromInt32(1));
}

ReturnedValue EvalFunction::evalCall(const Value *, const Value *argv, int argc, bool directCall) const
{
    if (argc < 1)
        return Encode::undefined();

    ExecutionEngine *v4 = engine();
    bool isStrict = v4->currentStackFrame->v4Function->isStrict();

    Scope scope(v4);
    ScopedContext ctx(scope, v4->currentContext());

    if (!directCall) {
        // the context for eval should be the global scope
        ctx = v4->scriptContext();
    }

    String *scode = argv[0].stringValue();
    if (!scode)
        return argv[0].asReturnedValue();

    const QString code = scode->toQString();
    bool inheritContext = !isStrict;

    Script script(ctx, QV4::Compiler::ContextType::Eval, code, QStringLiteral("eval code"));
    script.strictMode = (directCall && isStrict);
    script.inheritContext = inheritContext;
    script.parse();
    if (v4->hasException)
        return Encode::undefined();

    Function *function = script.function();
    if (!function)
        return Encode::undefined();
    function->isEval = true;

    if (function->isStrict() || isStrict) {
        ScopedFunctionObject e(scope, FunctionObject::createScriptFunction(ctx, function));
        ScopedValue thisObject(scope, directCall ? scope.engine->currentStackFrame->thisObject() : scope.engine->globalObject->asReturnedValue());
        return checkedResult(v4, e->call(thisObject, nullptr, 0));
    }

    ScopedValue thisObject(scope, scope.engine->currentStackFrame->thisObject());

    return checkedResult(v4, function->call(thisObject, nullptr, 0, ctx));
}


ReturnedValue EvalFunction::virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc)
{
    // indirect call
    return static_cast<const EvalFunction *>(f)->evalCall(thisObject, argv, argc, false);
}


static inline int toInt(const QChar &qc, int R)
{
    ushort c = qc.unicode();
    int v = -1;
    if (c >= '0' && c <= '9')
        v = c - '0';
    else if (c >= 'A' && c <= 'Z')
        v = c - 'A' + 10;
    else if (c >= 'a' && c <= 'z')
        v = c - 'a' + 10;
    if (v >= 0 && v < R)
        return v;
    else
        return -1;
}

// parseInt [15.1.2.2]
ReturnedValue GlobalFunctions::method_parseInt(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    Scope scope(b);
    ScopedValue inputString(scope, argc ? argv[0] : Value::undefinedValue());
    ScopedValue radix(scope, argc > 1 ? argv[1] : Value::undefinedValue());
    int R = radix->isUndefined() ? 0 : radix->toInt32();

    // [15.1.2.2] step by step:
    QString trimmed = inputString->toQString().trimmed(); // 1 + 2
    CHECK_EXCEPTION();

    const QChar *pos = trimmed.constData();
    const QChar *end = pos + trimmed.length();

    int sign = 1; // 3
    if (pos != end) {
        if (*pos == QLatin1Char('-'))
            sign = -1; // 4
        if (*pos == QLatin1Char('-') || *pos == QLatin1Char('+'))
            ++pos; // 5
    }
    bool stripPrefix = true; // 7
    if (R) { // 8
        if (R < 2 || R > 36)
            RETURN_RESULT(Encode(std::numeric_limits<double>::quiet_NaN())); // 8a
        if (R != 16)
            stripPrefix = false; // 8b
    } else { // 9
        R = 10; // 9a
    }
    if (stripPrefix) { // 10
        if ((end - pos >= 2)
                && (pos[0] == QLatin1Char('0'))
                && (pos[1] == QLatin1Char('x') || pos[1] == QLatin1Char('X'))) { // 10a
            pos += 2;
            R = 16;
        }
    }
    // 11: Z is progressively built below
    // 13: this is handled by the toInt function
    if (pos == end) // 12
        RETURN_RESULT(Encode(std::numeric_limits<double>::quiet_NaN()));
    bool overflow = false;
    qint64 v_overflow = 0;
    unsigned overflow_digit_count = 0;
    int d = toInt(*pos++, R);
    if (d == -1)
        RETURN_RESULT(Encode(std::numeric_limits<double>::quiet_NaN()));
    qint64 v = d;
    while (pos != end) {
        d = toInt(*pos++, R);
        if (d == -1)
            break;
        if (overflow) {
            if (overflow_digit_count == 0) {
                v_overflow = v;
                v = 0;
            }
            ++overflow_digit_count;
            v = v * R + d;
        } else {
            qint64 vNew = v * R + d;
            if (vNew < v) {
                overflow = true;
                --pos;
            } else {
                v = vNew;
            }
        }
    }

    if (overflow) {
        double result = (double) v_overflow * pow(static_cast<double>(R), static_cast<double>(overflow_digit_count));
        result += v;
        RETURN_RESULT(Encode(sign * result));
    } else {
        RETURN_RESULT(Encode(sign * (double) v)); // 15
    }
}

// parseFloat [15.1.2.3]
ReturnedValue GlobalFunctions::method_parseFloat(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    Scope scope(b);
    // [15.1.2.3] step by step:
    ScopedString inputString(scope, argc ? argv[0] : Value::undefinedValue(), ScopedString::Convert);
    CHECK_EXCEPTION();

    QString trimmed = inputString->toQString().trimmed(); // 2

    // 4:
    if (trimmed.startsWith(QLatin1String("Infinity"))
            || trimmed.startsWith(QLatin1String("+Infinity")))
        RETURN_RESULT(Encode(Q_INFINITY));
    if (trimmed.startsWith(QLatin1String("-Infinity")))
        RETURN_RESULT(Encode(-Q_INFINITY));
    QByteArray ba = trimmed.toLatin1();
    bool ok;
    const char *begin = ba.constData();
    const char *end = nullptr;
    double d = qstrtod(begin, &end, &ok);
    if (end - begin == 0)
        RETURN_RESULT(Encode(std::numeric_limits<double>::quiet_NaN())); // 3
    else
        RETURN_RESULT(Encode(d));
}

/// isNaN [15.1.2.4]
ReturnedValue GlobalFunctions::method_isNaN(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    if (!argc)
        // undefined gets converted to NaN
        RETURN_RESULT(Encode(true));

    if (argv[0].integerCompatible())
        RETURN_RESULT(Encode(false));

    double d = argv[0].toNumber();
    RETURN_RESULT(Encode((bool)std::isnan(d)));
}

/// isFinite [15.1.2.5]
ReturnedValue GlobalFunctions::method_isFinite(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    if (!argc)
        // undefined gets converted to NaN
        RETURN_RESULT(Encode(false));

    if (argv[0].integerCompatible())
        RETURN_RESULT(Encode(true));

    double d = argv[0].toNumber();
    RETURN_RESULT(Encode((bool)std::isfinite(d)));
}

/// decodeURI [15.1.3.1]
ReturnedValue GlobalFunctions::method_decodeURI(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    if (argc == 0)
        RETURN_UNDEFINED();

    ExecutionEngine *v4 = b->engine();
    QString uriString = argv[0].toQString();
    bool ok;
    QString out = decode(uriString, DecodeNonReserved, &ok);
    if (!ok) {
        Scope scope(v4);
        ScopedString s(scope, scope.engine->newString(QStringLiteral("malformed URI sequence")));
        RETURN_RESULT(scope.engine->throwURIError(s));
    }

    RETURN_RESULT(v4->newString(out));
}

/// decodeURIComponent [15.1.3.2]
ReturnedValue GlobalFunctions::method_decodeURIComponent(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    if (argc == 0)
        RETURN_UNDEFINED();

    ExecutionEngine *v4 = b->engine();
    QString uriString = argv[0].toQString();
    bool ok;
    QString out = decode(uriString, DecodeAll, &ok);
    if (!ok) {
        Scope scope(v4);
        ScopedString s(scope, scope.engine->newString(QStringLiteral("malformed URI sequence")));
        RETURN_RESULT(scope.engine->throwURIError(s));
    }

    RETURN_RESULT(v4->newString(out));
}

/// encodeURI [15.1.3.3]
ReturnedValue GlobalFunctions::method_encodeURI(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    if (argc == 0)
        RETURN_UNDEFINED();

    ExecutionEngine *v4 = b->engine();
    QString uriString = argv[0].toQString();
    bool ok;
    QString out = encode(uriString, uriUnescapedReserved, &ok);
    if (!ok) {
        Scope scope(v4);
        ScopedString s(scope, scope.engine->newString(QStringLiteral("malformed URI sequence")));
        RETURN_RESULT(scope.engine->throwURIError(s));
    }

    RETURN_RESULT(v4->newString(out));
}

/// encodeURIComponent [15.1.3.4]
ReturnedValue GlobalFunctions::method_encodeURIComponent(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    if (argc == 0)
        RETURN_UNDEFINED();

    ExecutionEngine *v4 = b->engine();
    QString uriString = argv[0].toQString();
    bool ok;
    QString out = encode(uriString, uriUnescaped, &ok);
    if (!ok) {
        Scope scope(v4);
        ScopedString s(scope, scope.engine->newString(QStringLiteral("malformed URI sequence")));
        RETURN_RESULT(scope.engine->throwURIError(s));
    }

    RETURN_RESULT(v4->newString(out));
}

ReturnedValue GlobalFunctions::method_escape(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    if (!argc)
        RETURN_RESULT(v4->newString(QStringLiteral("undefined")));

    QString str = argv[0].toQString();
    RETURN_RESULT(v4->newString(escape(str)));
}

ReturnedValue GlobalFunctions::method_unescape(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    if (!argc)
        RETURN_RESULT(v4->newString(QStringLiteral("undefined")));

    QString str = argv[0].toQString();
    RETURN_RESULT(v4->newString(unescape(str)));
}
