/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtXmlPatterns module of the Qt Toolkit.
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

#include "qebvtype_p.h"
#include "qgenericsequencetype_p.h"
#include "qnonetype_p.h"

#include "qcommonsequencetypes_p.h"

/* To avoid the static initialization fiasco, we put the builtin types in this compilation unit, since
 * the sequence types depends on them. */
#include "qbuiltintypes.cpp"

QT_BEGIN_NAMESPACE

using namespace QPatternist;

// STATIC DATA
#define st(var, type, card)                                             \
const SequenceType::Ptr                                                 \
CommonSequenceTypes::var(new GenericSequenceType(BuiltinTypes::type,    \
                                                 Cardinality::card()))

/* Alphabetically. */
st(ExactlyOneAnyURI,                xsAnyURI,               exactlyOne);
st(ExactlyOneAtomicType,            xsAnyAtomicType,        exactlyOne);
st(ExactlyOneAttribute,             attribute,              exactlyOne);
st(ExactlyOneBase64Binary,          xsBase64Binary,         exactlyOne);
st(ExactlyOneBoolean,               xsBoolean,              exactlyOne);
st(ExactlyOneComment,               comment,                exactlyOne);
st(ExactlyOneDateTime,              xsDateTime,             exactlyOne);
st(ExactlyOneDate,                  xsDate,                 exactlyOne);
st(ExactlyOneDayTimeDuration,       xsDayTimeDuration,      exactlyOne);
st(ExactlyOneDecimal,               xsDecimal,              exactlyOne);
st(ExactlyOneDocumentNode,          document,               exactlyOne);
st(OneOrMoreDocumentNodes,          document,               oneOrMore);
st(ExactlyOneDouble,                xsDouble,               exactlyOne);
st(ExactlyOneDuration,              xsDuration,             exactlyOne);
st(ExactlyOneElement,               element,                exactlyOne);
st(ExactlyOneFloat,                 xsFloat,                exactlyOne);
st(ExactlyOneGDay,                  xsGDay,                 exactlyOne);
st(ExactlyOneGMonthDay,             xsGMonthDay,            exactlyOne);
st(ExactlyOneGMonth,                xsGMonth,               exactlyOne);
st(ExactlyOneGYearMonth,            xsGYearMonth,           exactlyOne);
st(ExactlyOneGYear,                 xsGYear,                exactlyOne);
st(ExactlyOneHexBinary,             xsHexBinary,            exactlyOne);
st(ExactlyOneInteger,               xsInteger,              exactlyOne);
st(ExactlyOneItem,                  item,                   exactlyOne);
st(ExactlyOneNCName,                xsNCName,               exactlyOne);
st(ExactlyOneNode,                  node,                   exactlyOne);
st(ExactlyOneNumeric,               numeric,                exactlyOne);
st(ExactlyOneProcessingInstruction, pi,                     exactlyOne);
st(ExactlyOneQName,                 xsQName,                exactlyOne);
st(ExactlyOneString,                xsString,               exactlyOne);
st(ExactlyOneTextNode,              text,                   exactlyOne);
st(ExactlyOneTime,                  xsTime,                 exactlyOne);
st(ExactlyOneUntypedAtomic,         xsUntypedAtomic,        exactlyOne);
st(ExactlyOneYearMonthDuration,     xsYearMonthDuration,    exactlyOne);
st(OneOrMoreItems,                  item,                   oneOrMore);
st(ZeroOrMoreAtomicTypes,           xsAnyAtomicType,        zeroOrMore);
st(ZeroOrMoreElements,              element,                zeroOrMore);
st(ZeroOrMoreIntegers,              xsInteger,              zeroOrMore);
st(ZeroOrMoreItems,                 item,                   zeroOrMore);
st(ZeroOrMoreNodes,                 node,                   zeroOrMore);
st(ZeroOrMoreStrings,               xsString,               zeroOrMore);
st(ZeroOrOneAnyURI,                 xsAnyURI,               zeroOrOne);
st(ZeroOrOneAtomicType,             xsAnyAtomicType,        zeroOrOne);
st(ZeroOrOneBoolean,                xsBoolean,              zeroOrOne);
st(ZeroOrOneDateTime,               xsDateTime,             zeroOrOne);
st(ZeroOrOneDate,                   xsDate,                 zeroOrOne);
st(ZeroOrOneDayTimeDuration,        xsDayTimeDuration,      zeroOrOne);
st(ZeroOrOneDecimal,                xsDecimal,              zeroOrOne);
st(ZeroOrOneDocumentNode,           document,               zeroOrOne);
st(ZeroOrOneDuration,               xsDuration,             zeroOrOne);
st(ZeroOrOneInteger,                xsInteger,              zeroOrOne);
st(ZeroOrOneItem,                   item,                   zeroOrOne);
st(ZeroOrOneNCName,                 xsNCName,               zeroOrOne);
st(ZeroOrOneNode,                   node,                   zeroOrOne);
st(ZeroOrOneNumeric,                numeric,                zeroOrOne);
st(ZeroOrOneQName,                  xsQName,                zeroOrOne);
st(ZeroOrOneString,                 xsString,               zeroOrOne);
st(ZeroOrOneTextNode,               text,                   zeroOrOne);
st(ZeroOrOneTime,                   xsTime,                 zeroOrOne);
st(ZeroOrOneYearMonthDuration,      xsYearMonthDuration,    zeroOrOne);

#undef st

/* Special cases. */
const EmptySequenceType::Ptr    CommonSequenceTypes::Empty  (new EmptySequenceType());
const NoneType::Ptr             CommonSequenceTypes::None   (new NoneType());
const SequenceType::Ptr         CommonSequenceTypes::EBV    (new EBVType());


QT_END_NAMESPACE
