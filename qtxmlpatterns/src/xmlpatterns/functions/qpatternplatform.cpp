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

#include <QHash>

#include "qpatternistlocale_p.h"

#include "qpatternplatform_p.h"

QT_BEGIN_NAMESPACE

using namespace QPatternist;

namespace QPatternist
{
    /**
     * @short Used internally by PatternPlatform and describes
     * a flag that affects how a pattern is treated.
     *
     * The member variables aren't declared @c const, in order
     * to make the synthesized assignment operator and copy constructor work.
     *
     * @ingroup Patternist_utils
     * @author Frans Englich <frans.englich@nokia.com>
     */
    class PatternFlag
    {
    public:
        typedef QMap<QChar, PatternFlag> Hash;

        inline PatternFlag() : flag(PatternPlatform::NoFlags)
        {
        }

        inline PatternFlag(const PatternPlatform::Flag opt,
                           const QString &descr) : flag(opt),
                                                   description(descr)
        {
        }

        PatternPlatform::Flag   flag;
        QString                 description;

        static inline Hash flagDescriptions();
    };
}

static inline PatternFlag::Hash flagDescriptions()
{
    PatternFlag::Hash retval;

    retval.insert(QChar(QLatin1Char('s')),
                  PatternFlag(PatternPlatform::DotAllMode,
                              QtXmlPatterns::tr("%1 matches newline characters").arg(formatKeyword(QLatin1Char('.')))));

    retval.insert(QChar(QLatin1Char('m')),
                  PatternFlag(PatternPlatform::MultiLineMode,
                              QtXmlPatterns::tr("%1 and %2 match the start and end of a line.")
                                   .arg(formatKeyword(QLatin1Char('^')))
                                   .arg(formatKeyword(QLatin1Char('$')))));

    retval.insert(QChar(QLatin1Char('i')),
                  PatternFlag(PatternPlatform::CaseInsensitive,
                              QtXmlPatterns::tr("Matches are case insensitive")));

    retval.insert(QChar(QLatin1Char('x')),
                  PatternFlag(PatternPlatform::SimplifyWhitespace,
                              QtXmlPatterns::tr("Whitespace characters are removed, except when they appear "
                                 "in character classes")));

    return retval;
}

PatternPlatform::PatternPlatform(const qint8 flagsPosition) : m_compiledParts(NoPart),
                                                              m_flags(NoFlags),
                                                              m_flagsPosition(flagsPosition)
{
}

QRegExp PatternPlatform::pattern(const DynamicContext::Ptr &context) const
{
    if(m_compiledParts == FlagsAndPattern) /* This is the most common case. */
    {
        Q_ASSERT(m_pattern.isValid());
        return m_pattern;
    }

    QRegExp retvalPattern;
    Flags flags;

    /* Compile the flags, if necessary. */
    if(m_compiledParts.testFlag(FlagsPrecompiled))
        flags = m_flags;
    else
    {
        const Expression::Ptr flagsOp(m_operands.value(m_flagsPosition));

        if(flagsOp)
            flags = parseFlags(flagsOp->evaluateSingleton(context).stringValue(), context);
        else
            flags = NoFlags;
    }

    /* Compile the pattern, if necessary. */
    if(m_compiledParts.testFlag(PatternPrecompiled))
        retvalPattern = m_pattern;
    else
    {
        retvalPattern = parsePattern(m_operands.at(1)->evaluateSingleton(context).stringValue(),
                                     context);

    }

    applyFlags(flags, retvalPattern);

    Q_ASSERT(m_pattern.isValid());
    return retvalPattern;
}

void PatternPlatform::applyFlags(const Flags flags, QRegExp &patternP)
{
    Q_ASSERT(patternP.isValid());
    if(flags == NoFlags)
        return;

    if(flags & CaseInsensitive)
    {
        patternP.setCaseSensitivity(Qt::CaseInsensitive);
    }
    // TODO Apply the other flags, like 'x'.
}

QRegExp PatternPlatform::parsePattern(const QString &pattern,
                                      const ReportContext::Ptr &context) const
{
    return parsePattern(pattern, context, this);
}

QRegExp PatternPlatform::parsePattern(const QString &patternP,
                                      const ReportContext::Ptr &context,
                                      const SourceLocationReflection *const location)
{
    if(patternP == QLatin1String("(.)\\3") ||
       patternP == QLatin1String("\\3")    ||
       patternP == QLatin1String("(.)\\2"))
    {
        context->error(QLatin1String("We don't want to hang infinitely on K2-MatchesFunc-9, "
                                     "10 and 11."),
                       ReportContext::FOER0000, location);
        return QRegExp();
    }

    QString rewrittenPattern(patternP);

    /* We rewrite some well known patterns to QRegExp style here. Note that
     * these character classes only works in the ASCII range, and fail for
     * others. This support needs to be in QRegExp, since it's about checking
     * QChar::category(). */
    rewrittenPattern.replace(QLatin1String("[\\i-[:]]"), QLatin1String("[a-zA-Z_]"));
    rewrittenPattern.replace(QLatin1String("[\\c-[:]]"), QLatin1String("[a-zA-Z0-9_\\-\\.]"));

    QRegExp retval(rewrittenPattern, Qt::CaseSensitive, QRegExp::W3CXmlSchema11);

    if(retval.isValid())
        return retval;
    else
    {
        context->error(QtXmlPatterns::tr("%1 is an invalid regular expression pattern: %2")
                                        .arg(formatExpression(patternP), retval.errorString()),
                                   ReportContext::FORX0002, location);
        return QRegExp();
    }
}

PatternPlatform::Flags PatternPlatform::parseFlags(const QString &flags,
                                                   const DynamicContext::Ptr &context) const
{

    if(flags.isEmpty())
        return NoFlags;

    const PatternFlag::Hash flagDescrs(flagDescriptions());
    const int len = flags.length();
    Flags retval = NoFlags;

    for(int i = 0; i < len; ++i)
    {
        const QChar flag(flags.at(i));
        const Flag specified = flagDescrs.value(flag).flag;

        if(specified != NoFlags)
        {
            retval |= specified;
            continue;
        }

        /* Generate a nice error message. */
        QString message(QtXmlPatterns::tr("%1 is an invalid flag for regular expressions. Valid flags are:")
                             .arg(formatKeyword(flag)));

        /* This is formatting, so don't bother translators with it. */
        message.append(QLatin1Char('\n'));

        const PatternFlag::Hash::const_iterator end(flagDescrs.constEnd());
        PatternFlag::Hash::const_iterator it(flagDescrs.constBegin());

        for(; it != end;)
        {
            // TODO handle bidi correctly
            // TODO format this with rich text(list/table)
            message.append(formatKeyword(it.key()));
            message.append(QLatin1String(" - "));
            message.append(it.value().description);

            ++it;
            if(it != end)
                message.append(QLatin1Char('\n'));
        }

        context->error(message, ReportContext::FORX0001, this);
        return NoFlags;
    }

    return retval;
}

Expression::Ptr PatternPlatform::compress(const StaticContext::Ptr &context)
{
    const Expression::Ptr me(FunctionCall::compress(context));
    if(me != this)
        return me;

    if(m_operands.at(1)->is(IDStringValue))
    {
        const DynamicContext::Ptr dynContext(context->dynamicContext());

        m_pattern = parsePattern(m_operands.at(1)->evaluateSingleton(dynContext).stringValue(),
                                 dynContext);
        m_compiledParts |= PatternPrecompiled;
    }

    const Expression::Ptr flagOperand(m_operands.value(m_flagsPosition));

    if(!flagOperand)
    {
        m_flags = NoFlags;
        m_compiledParts |= FlagsPrecompiled;
    }
    else if(flagOperand->is(IDStringValue))
    {
        const DynamicContext::Ptr dynContext(context->dynamicContext());
        m_flags = parseFlags(flagOperand->evaluateSingleton(dynContext).stringValue(),
                             dynContext);
        m_compiledParts |= FlagsPrecompiled;
    }

    if(m_compiledParts == FlagsAndPattern)
        applyFlags(m_flags, m_pattern);

    return me;
}

QT_END_NAMESPACE
