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

#include <QtDebug>

#include "qaxisstep_p.h"
#include "qcommonsequencetypes_p.h"
#include "qcommonvalues_p.h"
#include "qitemmappingiterator_p.h"
#include "qsequencemappingiterator_p.h"
#include "qpatternistlocale_p.h"

#include "qapplytemplate_p.h"

QT_BEGIN_NAMESPACE

using namespace QPatternist;

ApplyTemplate::ApplyTemplate(const TemplateMode::Ptr &mode,
                             const WithParam::Hash &withParams,
                             const TemplateMode::Ptr &defaultMode) : TemplateInvoker(withParams)
                                                                   , m_mode(mode)
                                                                   , m_defaultMode(defaultMode)
{
    Q_ASSERT_X(m_mode || m_defaultMode, Q_FUNC_INFO,
               "Either a mode, or the default mode must be supplied.");
}

Item ApplyTemplate::mapToItem(const QXmlNodeModelIndex &node,
                              const DynamicContext::Ptr &) const
{
    return Item(node);
}

Item::Iterator::Ptr ApplyTemplate::mapToSequence(const Item &item,
                                                 const DynamicContext::Ptr &context) const
{
    Q_UNUSED(item);
    return evaluateSequence(context);
}

TemplateMode::Ptr ApplyTemplate::effectiveMode(const DynamicContext::Ptr &context) const
{
    if(m_mode)
        return m_mode;
    else
    {
        const TemplateMode::Ptr currentMode(context->currentTemplateMode());

        if(currentMode)
            return currentMode;
        else
            return m_defaultMode;
    }
}

Template::Ptr ApplyTemplate::findTemplate(const DynamicContext::Ptr &context,
                                          const TemplateMode::Ptr &templateMode) const
{
    const int count = templateMode->templatePatterns.count();
    Template::Ptr result;
    /* It's redundant to initialize these values, but it suppresses false
     * positives with GCC. */
    PatternPriority priority = 0;
    TemplatePattern::ID id = -1;

    /* Possible optimization: detecting ambiguous rule matches could be forked off to a
     * low prioirity thread. */
    for(int i = 0; i < count; ++i)
    {
        const TemplatePattern::Ptr &candidate = templateMode->templatePatterns.at(i);
        if(candidate->matchPattern()->evaluateEBV(context))
        {
            if(result)
            {
                if(   candidate->id() != id
                   && candidate->priority() == priority
                   && candidate->templateTarget()->importPrecedence ==
                                                        result->importPrecedence)
                {
                    context->error(QtXmlPatterns::tr("Ambiguous rule match."),
                                   ReportContext::XTRE0540, this);
                }
                else
                    break;
            }
            else
            {
                result = candidate->templateTarget();
                priority = candidate->priority();
                id = candidate->id();
            }
        }
    }

    return result;
}

Item::Iterator::Ptr ApplyTemplate::evaluateSequence(const DynamicContext::Ptr &context) const
{
    const TemplateMode::Ptr templateMode(effectiveMode(context));
    const Template::Ptr &templateMatch = findTemplate(context, templateMode);

    if(templateMatch)
        return templateMatch->body->evaluateSequence(templateMatch->createContext(this, context, false));
    else
    {
        /* None of our templates matched. Proceed with a built-in. */
        const Item current(context->contextItem());
        // TODO it can be an atomic value?
        const QXmlNodeModelIndex::NodeKind kind(current.asNode().kind());

        if(kind == QXmlNodeModelIndex::Element || kind == QXmlNodeModelIndex::Document)
        {
            pDebug() << "No template match, using builtin template for element() | document-node()";

            const Item::Iterator::Ptr focusIterator(makeItemMappingIterator<Item>(ConstPtr(this),
                                                                                  current.asNode().iterate(QXmlNodeModelIndex::AxisChild),
                                                                                  context));

            const DynamicContext::Ptr focus(context->createFocus());
            focus->setFocusIterator(focusIterator);
            return makeSequenceMappingIterator<Item>(ConstPtr(this), focusIterator, focus);
        }
        return CommonValues::emptyIterator;
    }
}

Expression::Ptr ApplyTemplate::compress(const StaticContext::Ptr &context)
{
    /* If we have a mode, we will never need the default mode. */
    if(m_mode)
        m_defaultMode.reset();

    return TemplateInvoker::compress(context);
}

SequenceType::Ptr ApplyTemplate::staticType() const
{
    return CommonSequenceTypes::ZeroOrMoreItems;
}

ExpressionVisitorResult::Ptr ApplyTemplate::accept(const ExpressionVisitor::Ptr &visitor) const
{
    return visitor->visit(this);
}

Expression::Properties ApplyTemplate::properties() const
{
    return RequiresFocus | DisableElimination;
}

bool ApplyTemplate::configureRecursion(const CallTargetDescription::Ptr &sign)
{
    Q_ASSERT_X(false, Q_FUNC_INFO, "We're not expected to be called.");
    Q_UNUSED(sign);
    return false;
}

Expression::Ptr ApplyTemplate::body() const
{
    Q_ASSERT_X(false, Q_FUNC_INFO, "We're not expected to be called.");
    return Expression::Ptr();
}

CallTargetDescription::Ptr ApplyTemplate::callTargetDescription() const
{
    Q_ASSERT_X(false, Q_FUNC_INFO, "We're not expected to be called.");
    return CallTargetDescription::Ptr();
}

QT_END_NAMESPACE
