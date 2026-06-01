// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qlottierepeatertransform_p.h"

QT_BEGIN_NAMESPACE

QLottieRepeaterTransform::QLottieRepeaterTransform(const QLottieRepeaterTransform &other)
    : QLottieBasicTransform(other)
{
    m_startOpacity = other.m_startOpacity;
    m_endOpacity = other.m_endOpacity;
    m_opacities = other.m_opacities;
}

QLottieRepeaterTransform::QLottieRepeaterTransform(const QJsonObject &definition,
                                                   QLottieBase *parent)
{
    setParent(parent);
    construct(definition);
}

QLottieBase *QLottieRepeaterTransform::clone() const
{
    return new QLottieRepeaterTransform(*this);
}

void QLottieRepeaterTransform::construct(const QJsonObject &definition)
{
    qCDebug(lcLottieQtLottieParser) << "QLottieRepeaterTransform::construct():" << name();

    QLottieBasicTransform::construct(definition);
    if (m_hidden)
        return;

    QJsonObject startOpacity = definition.value(QLatin1String("so")).toObject();
    startOpacity = resolveExpression(startOpacity);
    m_startOpacity.construct(startOpacity);

    QJsonObject endOpacity = definition.value(QLatin1String("eo")).toObject();
    endOpacity = resolveExpression(endOpacity);
    m_endOpacity.construct(endOpacity);
}

void QLottieRepeaterTransform::updateProperties(int frame)
{
    QLottieBasicTransform::updateProperties(frame);

    m_startOpacity.update(frame);
    m_endOpacity.update(frame);

    m_opacities.clear();
    for (int i = 0; i < m_copies; i++) {
        qreal opacity = m_startOpacity.value() +
                (m_endOpacity.value() - m_startOpacity.value()) * i / m_copies;
        m_opacities.push_back(opacity);
    }
}

void QLottieRepeaterTransform::render(QLottieRenderer &renderer) const
{
    renderer.render(*this);
}

void QLottieRepeaterTransform::setInstanceCount(int copies)
{
    m_copies = copies;
}

qreal QLottieRepeaterTransform::opacityAtInstance(int instance) const
{
    return m_opacities.at(instance) / 100.0;
}

qreal QLottieRepeaterTransform::startOpacity() const
{
    return m_startOpacity.value();
}

qreal QLottieRepeaterTransform::endOpacity() const
{
    return m_endOpacity.value();
}

QT_END_NAMESPACE
