// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qlottiegroup_p.h"

#include <QJsonObject>
#include <QJsonArray>

#include "qlottiebase_p.h"
#include "qlottieshape_p.h"
#include "qlottietrimpath_p.h"
#include "qlottiebasictransform_p.h"

QT_BEGIN_NAMESPACE

QLottieGroup::QLottieGroup(const QJsonObject &definition, QLottieBase *parent)
{
    setParent(parent);
    construct(definition);
}

QLottieBase *QLottieGroup::clone() const
{
    return new QLottieGroup(*this);
}

void QLottieGroup::construct(const QJsonObject &definition)
{
    QLottieBase::parse(definition);
    if (m_hidden)
        return;

    qCDebug(lcLottieQtLottieParser) << "QLottieGroup::construct()"
                                       << m_name;

    QJsonArray groupItems = definition.value(QLatin1String("it")).toArray();
    QJsonArray::const_iterator itemIt = groupItems.constEnd();
    while (itemIt != groupItems.constBegin()) {
        itemIt--;
        QLottieShape *shape = QLottieShape::construct((*itemIt).toObject(), this);
        if (shape) {
            // Transform affects how group contents are drawn.
            // It must be traversed first when drawing
            if (shape->type() == LOTTIE_SHAPE_TRANS_IX)
                prependChild(shape);
            else
                appendChild(shape);
        }
    }
}

void QLottieGroup::updateProperties(int frame)
{
    QLottieShape::updateProperties(frame);

    for (QLottieBase *child : children()) {
        if (child->hidden())
            continue;

        QLottieShape *shape = static_cast<QLottieShape*>(child);
        if (shape->type() == LOTTIE_SHAPE_TRIM_IX) {
            QLottieTrimPath *trim = static_cast<QLottieTrimPath*>(shape);
            if (m_appliedTrim)
                m_appliedTrim->applyTrim(*trim);
            else
                m_appliedTrim = trim;
        } else if (m_appliedTrim  && shape->acceptsTrim())
            shape->applyTrim(*m_appliedTrim);
    }
}

void QLottieGroup::render(QLottieRenderer &renderer) const
{
    qCDebug(lcLottieQtLottieRender) << "Group:" << name();

    renderer.saveState();

    renderer.render(*this);

    if (m_appliedTrim && !m_appliedTrim->hidden()) {
        if (m_appliedTrim->isParallel())
            renderer.setTrimmingState(QLottieRenderer::Parallel);
        else
            renderer.setTrimmingState(QLottieRenderer::Sequential);
    } else
        renderer.setTrimmingState(QLottieRenderer::Off);

   for (QLottieBase *child : children()) {
        if (child->hidden())
            continue;
        child->render(renderer);
   }

   if (m_appliedTrim && !m_appliedTrim->hidden() && !m_appliedTrim->isParallel())
       m_appliedTrim->render(renderer);

    renderer.finish(*this);

    renderer.restoreState();
}

bool QLottieGroup::acceptsTrim() const
{
    return true;
}

void QLottieGroup::applyTrim(const QLottieTrimPath &trimmer)
{
    Q_ASSERT_X(!m_appliedTrim, "QLottieGroup", "A trim already assigned");

    m_appliedTrim = static_cast<QLottieTrimPath*>(trimmer.clone());
    m_appliedTrim->setParent(parent());
    // Setting a friendly name helps in testing
    m_appliedTrim->setName(QStringLiteral("Inherited from") + trimmer.name());

    for (QLottieBase *child : children()) {
        QLottieShape *shape = static_cast<QLottieShape*>(child);
        if (shape->acceptsTrim())
            shape->applyTrim(*m_appliedTrim);
    }
}

QT_END_NAMESPACE
