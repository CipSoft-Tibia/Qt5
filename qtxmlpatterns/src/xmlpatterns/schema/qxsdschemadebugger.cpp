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

#include "qxsdschemadebugger_p.h"

QT_BEGIN_NAMESPACE

using namespace QPatternist;

XsdSchemaDebugger::XsdSchemaDebugger(const NamePool::Ptr &namePool)
    : m_namePool(namePool)
{
}

void XsdSchemaDebugger::dumpParticle(const XsdParticle::Ptr &particle, int level)
{
    QString prefix; prefix.fill(QLatin1Char(' '), level);

    qDebug("%s min=%s max=%s", qPrintable(prefix), qPrintable(QString::number(particle->minimumOccurs())),
                               qPrintable(particle->maximumOccursUnbounded() ? QLatin1String("unbounded") : QString::number(particle->maximumOccurs())));

    if (particle->term()->isElement()) {
        qDebug("%selement (%s)", qPrintable(prefix), qPrintable(XsdElement::Ptr(particle->term())->displayName(m_namePool)));
    } else if (particle->term()->isModelGroup()) {
        const XsdModelGroup::Ptr group(particle->term());
        if (group->compositor() == XsdModelGroup::SequenceCompositor) {
            qDebug("%ssequence", qPrintable(prefix));
        } else if (group->compositor() == XsdModelGroup::AllCompositor) {
            qDebug("%sall", qPrintable(prefix));
        } else if (group->compositor() == XsdModelGroup::ChoiceCompositor) {
            qDebug("%schoice", qPrintable(prefix));
        }

        for (int i = 0; i < group->particles().count(); ++i)
            dumpParticle(group->particles().at(i), level + 5);
    } else if (particle->term()->isWildcard()) {
        XsdWildcard::Ptr wildcard(particle->term());
        qDebug("%swildcard (process=%d)", qPrintable(prefix), wildcard->processContents());
    }
}

void XsdSchemaDebugger::dumpInheritance(const SchemaType::Ptr &type, int level)
{
    QString prefix; prefix.fill(QLatin1Char(' '), level);
    qDebug("%s-->%s", qPrintable(prefix), qPrintable(type->displayName(m_namePool)));
    if (type->wxsSuperType())
        dumpInheritance(type->wxsSuperType(), ++level);
}

void XsdSchemaDebugger::dumpWildcard(const XsdWildcard::Ptr &wildcard)
{
    QVector<QString> varietyNames;
    varietyNames.append(QLatin1String("Any"));
    varietyNames.append(QLatin1String("Enumeration"));
    varietyNames.append(QLatin1String("Not"));

    QVector<QString> processContentsNames;
    processContentsNames.append(QLatin1String("Strict"));
    processContentsNames.append(QLatin1String("Lax"));
    processContentsNames.append(QLatin1String("Skip"));

    qDebug("      processContents: %s", qPrintable(processContentsNames.at((int)wildcard->processContents())));
    const XsdWildcard::NamespaceConstraint::Ptr constraint = wildcard->namespaceConstraint();
    qDebug("      variety: %s", qPrintable(varietyNames.at((int)constraint->variety())));
    if (constraint->variety() != XsdWildcard::NamespaceConstraint::Any)
        qDebug() << "      namespaces:" << constraint->namespaces();
}

void XsdSchemaDebugger::dumpType(const SchemaType::Ptr &type)
{
    if (type->isComplexType()) {
        const XsdComplexType::Ptr complexType(type);
        qDebug("\n+++ Complex Type +++");
        qDebug("Name: %s (abstract: %s)", qPrintable(complexType->displayName(m_namePool)), complexType->isAbstract() ? "yes" : "no");
        if (complexType->wxsSuperType())
            qDebug("  base type: %s", qPrintable(complexType->wxsSuperType()->displayName(m_namePool)));
        else
            qDebug("  base type: (none)");
        if (complexType->contentType()->variety() == XsdComplexType::ContentType::Empty)
            qDebug("  content type: empty");
        if (complexType->contentType()->variety() == XsdComplexType::ContentType::Simple)
            qDebug("  content type: simple");
        if (complexType->contentType()->variety() == XsdComplexType::ContentType::ElementOnly)
            qDebug("  content type: element-only");
        if (complexType->contentType()->variety() == XsdComplexType::ContentType::Mixed)
            qDebug("  content type: mixed");
        if (complexType->contentType()->variety() == XsdComplexType::ContentType::Simple) {
            if (complexType->contentType()->simpleType())
                qDebug("  simple type: %s", qPrintable(complexType->contentType()->simpleType()->displayName(m_namePool)));
            else
                qDebug("  simple type: (none)");
        }

        const XsdAttributeUse::List uses = complexType->attributeUses();
        qDebug("   %d attributes", uses.count());
        for (int i = 0; i < uses.count(); ++i) {
            qDebug("      attr: %s", qPrintable(uses.at(i)->attribute()->displayName(m_namePool)));
        }
        qDebug("   has attribute wildcard: %s", complexType->attributeWildcard() ? "yes" : "no");
        if (complexType->attributeWildcard()) {
            dumpWildcard(complexType->attributeWildcard());
        }

        if (complexType->contentType()->particle()) {
            dumpParticle(complexType->contentType()->particle(), 5);
        }
    } else {
        qDebug("\n+++ Simple Type +++");
        qDebug("Name: %s", qPrintable(type->displayName(m_namePool)));
        if (type->isDefinedBySchema()) {
            const XsdSimpleType::Ptr simpleType(type);
            if (simpleType->primitiveType())
                qDebug("  primitive type: %s", qPrintable(simpleType->primitiveType()->displayName(m_namePool)));
            else
                qDebug("  primitive type: (none)");
        }
        dumpInheritance(type, 0);
    }
}


void XsdSchemaDebugger::dumpElement(const XsdElement::Ptr &element)
{
    QStringList disallowedSubstGroup;
    if (element->disallowedSubstitutions() & XsdElement::RestrictionConstraint)
        disallowedSubstGroup << QLatin1String("restriction");
    if (element->disallowedSubstitutions() & XsdElement::ExtensionConstraint)
        disallowedSubstGroup << QLatin1String("extension");
    if (element->disallowedSubstitutions() & XsdElement::SubstitutionConstraint)
        disallowedSubstGroup << QLatin1String("substitution");


    qDebug() << "Name:" << element->displayName(m_namePool);
    qDebug() << "IsAbstract:" << (element->isAbstract() ? "yes" : "no");
    qDebug() << "Type:" << element->type()->displayName(m_namePool);
    qDebug() << "DisallowedSubstitutionGroups:" << disallowedSubstGroup.join(QLatin1String("' "));
}

void XsdSchemaDebugger::dumpAttribute(const XsdAttribute::Ptr &attribute)
{
    qDebug() << "Name:" << attribute->displayName(m_namePool);
    qDebug() << "Type:" << attribute->type()->displayName(m_namePool);
}

void XsdSchemaDebugger::dumpSchema(const XsdSchema::Ptr &schema)
{
    qDebug() << "------------------------------ Schema -------------------------------";

    // elements
    {
        qDebug() << "Global Elements:";
        const XsdElement::List elements = schema->elements();
        for (int i = 0; i < elements.count(); ++i) {
            dumpElement(elements.at(i));
        }
    }

    // attributes
    {
        qDebug() << "Global Attributes:";
        const XsdAttribute::List attributes = schema->attributes();
        for (int i = 0; i < attributes.count(); ++i) {
            dumpAttribute(attributes.at(i));
        }
    }

    // types
    {
        qDebug() << "Global Types:";
        const SchemaType::List types = schema->types();
        for (int i = 0; i < types.count(); ++i) {
            dumpType(types.at(i));
        }
    }

    // anonymous types
    {
        qDebug() << "Anonymous Types:";
        const SchemaType::List types = schema->anonymousTypes();
        for (int i = 0; i < types.count(); ++i) {
            dumpType(types.at(i));
        }
    }

    qDebug() << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++";
}

QT_END_NAMESPACE
