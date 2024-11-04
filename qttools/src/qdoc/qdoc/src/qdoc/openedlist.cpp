// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "openedlist.h"

#include "atom.h"

#include <QtCore/qregularexpression.h>

QT_BEGIN_NAMESPACE

static const char roman[] = "m\2d\5c\2l\5x\2v\5i";

OpenedList::OpenedList(ListStyle style) : sty(style), ini(1), nex(0) {}

OpenedList::OpenedList(const Location &location, const QString &hint) : sty(Bullet), ini(1)
{
    static const QRegularExpression hintSyntax("^(\\W*)([0-9]+|[A-Z]+|[a-z]+)(\\W*)$");

    auto match = hintSyntax.match(hint);
    if (match.hasMatch()) {
        bool ok;
        int asNumeric = hint.toInt(&ok);
        int asRoman = fromRoman(match.captured(2));
        int asAlpha = fromAlpha(match.captured(2));

        if (ok) {
            sty = Numeric;
            ini = asNumeric;
        } else if (asRoman > 0 && asRoman != 100 && asRoman != 500) {
            sty = (hint == hint.toLower()) ? LowerRoman : UpperRoman;
            ini = asRoman;
        } else {
            sty = (hint == hint.toLower()) ? LowerAlpha : UpperAlpha;
            ini = asAlpha;
        }
        pref = match.captured(1);
        suff = match.captured(3);
    } else if (!hint.isEmpty()) {
        location.warning(QStringLiteral("Unrecognized list style '%1'").arg(hint));
    }
    nex = ini - 1;
}

QString OpenedList::styleString() const
{
    switch (style()) {
    case Bullet:
    default:
        return ATOM_LIST_BULLET;
    case Tag:
        return ATOM_LIST_TAG;
    case Value:
        return ATOM_LIST_VALUE;
    case Numeric:
        return ATOM_LIST_NUMERIC;
    case UpperAlpha:
        return ATOM_LIST_UPPERALPHA;
    case LowerAlpha:
        return ATOM_LIST_LOWERALPHA;
    case UpperRoman:
        return ATOM_LIST_UPPERROMAN;
    case LowerRoman:
        return ATOM_LIST_LOWERROMAN;
    }
}

QString OpenedList::numberString() const
{
    return QString::number(number());
    /*
    switch ( style() ) {
    case Numeric:
 return QString::number( number() );
    case UpperAlpha:
 return toAlpha( number() ).toUpper();
    case LowerAlpha:
 return toAlpha( number() );
    case UpperRoman:
 return toRoman( number() ).toUpper();
    case LowerRoman:
 return toRoman( number() );
    case Bullet:
    default:
 return "*";
    }*/
}

int OpenedList::fromAlpha(const QString &str)
{
    int n = 0;
    int u;

    for (const QChar &character : str) {
        u = character.toLower().unicode();
        if (u >= 'a' && u <= 'z') {
            n *= 26;
            n += u - 'a' + 1;
        } else {
            return 0;
        }
    }
    return n;
}

QString OpenedList::toRoman(int n)
{
    /*
      See p. 30 of Donald E. Knuth's "TeX: The Program".
    */
    QString str;
    int j = 0;
    int k;
    int u;
    int v = 1000;

    for (;;) {
        while (n >= v) {
            str += roman[j];
            n -= v;
        }

        if (n <= 0)
            break;

        k = j + 2;
        u = v / roman[k - 1];
        if (roman[k - 1] == 2) {
            k += 2;
            u /= 5;
        }
        if (n + u >= v) {
            str += roman[k];
            n += u;
        } else {
            j += 2;
            v /= roman[j - 1];
        }
    }
    return str;
}

int OpenedList::fromRoman(const QString &str)
{
    int n = 0;
    int j;
    int u;
    int v = 0;

    for (const QChar &character : str) {
        j = 0;
        u = 1000;
        while (roman[j] != 'i' && roman[j] != character.toLower()) {
            j += 2;
            u /= roman[j - 1];
        }
        if (u < v) {
            n -= u;
        } else {
            n += u;
        }
        v = u;
    }

    if (str.toLower() == toRoman(n)) {
        return n;
    } else {
        return 0;
    }
}

QT_END_NAMESPACE
