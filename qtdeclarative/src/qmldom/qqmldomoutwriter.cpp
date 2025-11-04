// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldomoutwriter_p.h"
#include "qqmldomlinewriter_p.h"
#include "qqmldomitem_p.h"
#include "qqmldomcomments_p.h"
#include "qqmldomexternalitems_p.h"
#include "qqmldomtop_p.h"

#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

OutWriterState::OutWriterState(
        const Path &itCanonicalPath, const DomItem &it, const FileLocations::Tree &fLoc)
    : itemCanonicalPath(itCanonicalPath), item(it), currentMap(fLoc)
{
    DomItem cRegions = it.field(Fields::comments);
    if (const RegionComments *cRegionsPtr = cRegions.as<RegionComments>())
        pendingComments = cRegionsPtr->regionComments();
}

void OutWriterState::closeState(OutWriter &w)
{
    if (!pendingRegions.isEmpty()) {
        qCWarning(writeOutLog) << "PendingRegions non empty when closing item"
                               << pendingRegions.keys();
        auto iend = pendingRegions.end();
        auto it = pendingRegions.begin();
        while (it == iend) {
            w.lineWriter.endSourceLocation(it.value());
            ++it;
        }
    }
    if (!w.skipComments && !pendingComments.isEmpty())
        qCWarning(writeOutLog) << "PendingComments when closing item "
                               << item.canonicalPath().toString() << "for regions"
                               << pendingComments.keys();
}

OutWriterState &OutWriter::state(int i)
{
    return states[states.size() - 1 - i];
}

void OutWriter::itemStart(const DomItem &it)
{
    if (!topLocation->path())
        topLocation->setPath(it.canonicalPath());
    FileLocations::Tree newFLoc = topLocation;
    Path itP = it.canonicalPath();

    states.append(OutWriterState(itP, it, newFLoc));

    regionStart(MainRegion);
}

void OutWriter::itemEnd(const DomItem &it)
{
    Q_ASSERT(states.size() > 0);
    Q_ASSERT(state().item == it);
    regionEnd(MainRegion);
    state().closeState(*this);
    states.removeLast();
}

void OutWriter::regionStart(FileLocationRegion region)
{
    Q_ASSERT(!state().pendingRegions.contains(region));
    FileLocations::Tree fMap = state().currentMap;
    if (!skipComments && state().pendingComments.contains(region)) {
        state().pendingComments[region].writePre(*this, nullptr);
    }
    state().pendingRegions[region] = lineWriter.startSourceLocation(
            [region, fMap](SourceLocation l) { FileLocations::addRegion(fMap, region, l); });
}

void OutWriter::regionEnd(FileLocationRegion region)
{
    Q_ASSERT(state().pendingRegions.contains(region));
    FileLocations::Tree fMap = state().currentMap;
    lineWriter.endSourceLocation(state().pendingRegions.value(region));
    state().pendingRegions.remove(region);
    if (state().pendingComments.contains(region)) {
        if (!skipComments) {
            state().pendingComments[region].writePost(*this, nullptr);
        }
        state().pendingComments.remove(region);
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
