// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qlottierepeater_p.h"

QLottieRepeater::QLottieRepeater(const QJsonObject &definition, QLottieBase *parent)
{
    setParent(parent);
    m_transform.setParent(this);
    construct(definition);
}

QLottieBase *QLottieRepeater::clone() const
{
    return new QLottieRepeater(*this);
}

void QLottieRepeater::construct(const QJsonObject &definition)
{
    qCDebug(lcLottieQtLottieParser) << "QLottieRepeater::construct():" << m_name;

    QLottieBase::parse(definition);
    if (m_hidden)
        return;

    QJsonObject copies = definition.value(QLatin1String("c")).toObject();
    copies = resolveExpression(copies);
    m_copies.construct(copies);

    QJsonObject offset = definition.value(QLatin1String("o")).toObject();
    offset = resolveExpression(offset);
    m_offset.construct(offset);

    m_transform.construct(definition.value(QLatin1String("tr")).toObject());
}

void QLottieRepeater::updateProperties(int frame)
{
    m_copies.update(frame);
    m_offset.update(frame);
    m_transform.setInstanceCount(m_copies.value());
    m_transform.updateProperties(frame);
}

void QLottieRepeater::render(QLottieRenderer &renderer) const
{
    renderer.render(*this);
}

int QLottieRepeater::copies() const
{
    return m_copies.value();
}

qreal QLottieRepeater::offset() const
{
    return m_offset.value();
}

const QLottieRepeaterTransform &QLottieRepeater::transform() const
{
    return m_transform;
}
