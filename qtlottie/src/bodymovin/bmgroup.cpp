// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "bmgroup_p.h"

#include <QJsonObject>
#include <QJsonArray>

#include "bmbase_p.h"
#include "bmshape_p.h"
#include "bmtrimpath_p.h"
#include "bmbasictransform_p.h"

QT_BEGIN_NAMESPACE

BMGroup::BMGroup(const QJsonObject &definition, const QVersionNumber &version, BMBase *parent)
{
    setParent(parent);
    construct(definition, version);
}

BMBase *BMGroup::clone() const
{
    return new BMGroup(*this);
}

void BMGroup::construct(const QJsonObject &definition, const QVersionNumber &version)
{
    BMBase::parse(definition);
    if (m_hidden)
        return;

    qCDebug(lcLottieQtBodymovinParser) << "BMGroup::construct()"
                                       << m_name;

    QJsonArray groupItems = definition.value(QLatin1String("it")).toArray();
    QJsonArray::const_iterator itemIt = groupItems.constEnd();
    while (itemIt != groupItems.constBegin()) {
        itemIt--;
        BMShape *shape = BMShape::construct((*itemIt).toObject(), version, this);
        if (shape) {
            // Transform affects how group contents are drawn.
            // It must be traversed first when drawing
            if (shape->type() == BM_SHAPE_TRANS_IX)
                prependChild(shape);
            else
                appendChild(shape);
        }
    }
}

void BMGroup::updateProperties(int frame)
{
    BMShape::updateProperties(frame);

    for (BMBase *child : children()) {
        if (child->hidden())
            continue;

        BMShape *shape = static_cast<BMShape*>(child);
        if (shape->type() == BM_SHAPE_TRIM_IX) {
            BMTrimPath *trim = static_cast<BMTrimPath*>(shape);
            if (m_appliedTrim)
                m_appliedTrim->applyTrim(*trim);
            else
                m_appliedTrim = trim;
        } else if (m_appliedTrim  && shape->acceptsTrim())
            shape->applyTrim(*m_appliedTrim);
    }
}

void BMGroup::render(LottieRenderer &renderer) const
{
    qCDebug(lcLottieQtBodymovinRender) << "Group:" << name();

    renderer.saveState();

    if (m_appliedTrim && !m_appliedTrim->hidden()) {
        if (m_appliedTrim->simultaneous())
            renderer.setTrimmingState(LottieRenderer::Simultaneous);
        else
            renderer.setTrimmingState(LottieRenderer::Individual);
    } else
        renderer.setTrimmingState(LottieRenderer::Off);

   for (BMBase *child : children()) {
        if (child->hidden())
            continue;
        child->render(renderer);
   }

   if (m_appliedTrim && !m_appliedTrim->hidden()
           && !m_appliedTrim->simultaneous())
       m_appliedTrim->render(renderer);

    renderer.restoreState();
}

bool BMGroup::acceptsTrim() const
{
    return true;
}

void BMGroup::applyTrim(const BMTrimPath &trimmer)
{
    Q_ASSERT_X(!m_appliedTrim, "BMGroup", "A trim already assigned");

    m_appliedTrim = static_cast<BMTrimPath*>(trimmer.clone());
    // Setting a friendly name helps in testing
    m_appliedTrim->setName(QStringLiteral("Inherited from") + trimmer.name());

    for (BMBase *child : children()) {
        BMShape *shape = static_cast<BMShape*>(child);
        if (shape->acceptsTrim())
            shape->applyTrim(*m_appliedTrim);
    }
}

QT_END_NAMESPACE
