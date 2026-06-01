// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldomoutwriter_p.h"
#include "qqmldomlinewriter_p.h"
#include "qqmldomitem_p.h"
#include "qqmldomcomments_p.h"

#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

static inline OutWriter::RegionToCommentMap extractComments(const DomItem &it)
{
    OutWriter::RegionToCommentMap comments;
    if (const RegionComments *cRegionsPtr = it.field(Fields::comments).as<RegionComments>()) {
        comments = cRegionsPtr->regionComments();
    }
    return comments;
}

void OutWriter::itemStart(const DomItem &it)
{
    pendingComments.push(extractComments(it));
    regionStart(MainRegion);
}

void OutWriter::itemEnd()
{
    Q_ASSERT(!pendingComments.isEmpty());
    regionEnd(MainRegion);
    pendingComments.pop();
}

void OutWriter::regionStart(FileLocationRegion region)
{
    const auto &comments = pendingComments.top();
    if (!skipComments && comments.contains(region)) {
        comments[region].writePre(*this);
    }
}

void OutWriter::regionEnd(FileLocationRegion region)
{
    auto &comments = pendingComments.top();
    if (comments.contains(region)) {
        if (!skipComments) {
            comments[region].writePost(*this);
        }
        comments.remove(region);
    }
}

/*!
\internal
Helper method for writeRegion(FileLocationRegion region) that allows to use
\c{writeRegion(ColonTokenRegion);} instead of having to write out the more error-prone
\c{writeRegion(ColonTokenRegion, ":");} for tokens and keywords.
*/
OutWriter &OutWriter::writeRegion(FileLocationRegion region)
{
    using namespace Qt::Literals::StringLiterals;
    QString codeForRegion;
    switch (region) {
    case ComponentKeywordRegion:
        codeForRegion = u"component"_s;
        break;
    case IdColonTokenRegion:
    case ColonTokenRegion:
        codeForRegion = u":"_s;
        break;
    case ImportTokenRegion:
        codeForRegion = u"import"_s;
        break;
    case AsTokenRegion:
        codeForRegion = u"as"_s;
        break;
    case OnTokenRegion:
        codeForRegion = u"on"_s;
        break;
    case IdTokenRegion:
        codeForRegion = u"id"_s;
        break;
    case LeftBraceRegion:
        codeForRegion = u"{"_s;
        break;
    case RightBraceRegion:
        codeForRegion = u"}"_s;
        break;
    case LeftBracketRegion:
        codeForRegion = u"["_s;
        break;
    case RightBracketRegion:
        codeForRegion = u"]"_s;
        break;
    case LeftParenthesisRegion:
        codeForRegion = u"("_s;
        break;
    case RightParenthesisRegion:
        codeForRegion = u")"_s;
        break;
    case EnumKeywordRegion:
        codeForRegion = u"enum"_s;
        break;
    case DefaultKeywordRegion:
        codeForRegion = u"default"_s;
        break;
    case RequiredKeywordRegion:
        codeForRegion = u"required"_s;
        break;
    case ReadonlyKeywordRegion:
        codeForRegion = u"readonly"_s;
        break;
    case PropertyKeywordRegion:
        codeForRegion = u"property"_s;
        break;
    case FunctionKeywordRegion:
        codeForRegion = u"function"_s;
        break;
    case SignalKeywordRegion:
        codeForRegion = u"signal"_s;
        break;
    case ReturnKeywordRegion:
        codeForRegion = u"return"_s;
        break;
    case EllipsisTokenRegion:
        codeForRegion = u"..."_s;
        break;
    case EqualTokenRegion:
        codeForRegion = u"="_s;
        break;
    case PragmaKeywordRegion:
        codeForRegion = u"pragma"_s;
        break;
    case CommaTokenRegion:
        codeForRegion = u","_s;
        break;
    case ForKeywordRegion:
        codeForRegion = u"for"_s;
        break;
    case ElseKeywordRegion:
        codeForRegion = u"else"_s;
        break;
    case DoKeywordRegion:
        codeForRegion = u"do"_s;
        break;
    case WhileKeywordRegion:
        codeForRegion = u"while"_s;
        break;
    case TryKeywordRegion:
        codeForRegion = u"try"_s;
        break;
    case CatchKeywordRegion:
        codeForRegion = u"catch"_s;
        break;
    case FinallyKeywordRegion:
        codeForRegion = u"finally"_s;
        break;
    case CaseKeywordRegion:
        codeForRegion = u"case"_s;
        break;
    case ThrowKeywordRegion:
        codeForRegion = u"throw"_s;
        break;
    case ContinueKeywordRegion:
        codeForRegion = u"continue"_s;
        break;
    case BreakKeywordRegion:
        codeForRegion = u"break"_s;
        break;
    case QuestionMarkTokenRegion:
        codeForRegion = u"?"_s;
        break;
    case SemicolonTokenRegion:
        codeForRegion = u";"_s;
        break;
    case IfKeywordRegion:
        codeForRegion = u"if"_s;
        break;
    case SwitchKeywordRegion:
        codeForRegion = u"switch"_s;
        break;
    case YieldKeywordRegion:
        codeForRegion = u"yield"_s;
        break;
    case NewKeywordRegion:
        codeForRegion = u"new"_s;
        break;
    case ThisKeywordRegion:
        codeForRegion = u"this"_s;
        break;
    case SuperKeywordRegion:
        codeForRegion = u"super"_s;
        break;
    case StarTokenRegion:
        codeForRegion = u"*"_s;
        break;
    case DollarLeftBraceTokenRegion:
        codeForRegion = u"${"_s;
        break;
    case LeftBacktickTokenRegion:
    case RightBacktickTokenRegion:
        codeForRegion = u"`"_s;
        break;
    case FinalKeywordRegion:
        codeForRegion = u"final"_s;
        break;
    // not keywords:
    case ImportUriRegion:
    case IdNameRegion:
    case IdentifierRegion:
    case PragmaValuesRegion:
    case MainRegion:
    case OnTargetRegion:
    case TypeIdentifierRegion:
    case TypeModifierRegion:
    case FirstSemicolonTokenRegion:
    case SecondSemicolonRegion:
    case InOfTokenRegion:
    case OperatorTokenRegion:
    case VersionRegion:
    case EnumValueRegion:
        Q_ASSERT_X(false, "regionToString", "Using regionToString on a value or an identifier!");
        return *this;
    }

    return writeRegion(region, codeForRegion);
}

OutWriter &OutWriter::writeRegion(FileLocationRegion region, QStringView toWrite)
{
    regionStart(region);
    lineWriter.write(toWrite);
    regionEnd(region);
    return *this;
}

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE
