// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qlottiegfill_p.h"

#include <QLinearGradient>
#include <QRadialGradient>
#include <QtMath>
#include <QColor>

QT_BEGIN_NAMESPACE

QLottieGFill::QLottieGFill(const QLottieGFill &other)
    : QLottieShape(other)
{
    if (m_hidden)
        return;

    m_opacity = other.m_opacity;
    m_startPoint = other.m_startPoint;
    m_endPoint = other.m_endPoint;
    m_highlightLength = other.m_highlightLength;
    m_highlightAngle = other.m_highlightAngle;
    m_colors = other.m_colors;
    if (other.gradientType() == QGradient::LinearGradient)
        m_gradient = new QLinearGradient;
    else if (other.gradientType() == QGradient::RadialGradient)
        m_gradient = new QRadialGradient;
    else {
        Q_UNREACHABLE();
    }
}

QLottieGFill::~QLottieGFill()
{
    if (m_gradient)
        delete m_gradient;
}

QLottieBase *QLottieGFill::clone() const
{
    return new QLottieGFill(*this);
}

QLottieGFill::QLottieGFill(const QJsonObject &definition, QLottieBase *parent)
{
    setParent(parent);

    QLottieBase::parse(definition);
    if (m_hidden)
        return;

    qCDebug(lcLottieQtLottieParser) << "QLottieGFill::construct():" << m_name;

    int type = definition.value(QLatin1String("t")).toVariant().toInt();
    switch (type) {
    case 1:
        m_gradient = new QLinearGradient;
        break;
    case 2:
        m_gradient = new QRadialGradient;
        break;
    default:
        qCWarning(lcLottieQtLottieParser) << "Unknown gradient fill type";
    }

    QJsonObject color = definition.value(QLatin1String("g")).toObject();
    int elementCount = color.value(QLatin1String("p")).toInt();
    bool isAnimated = color.value(QLatin1String("k")).toObject().value(QLatin1String("a")).toVariant().toBool();
    if (!isAnimated) {
        QJsonArray colorArr = color.value(QLatin1String("k")).toObject().value(QLatin1String("k")).toArray();
        for (int i = 0; i < (elementCount * 4); i += 4) {
            // p denotes the color stop percentage
            QVector4D colorVec;
            colorVec[0] = colorArr.at(i + 1).toVariant().toFloat();
            colorVec[1] = colorArr.at(i + 2).toVariant().toFloat();
            colorVec[2] = colorArr.at(i + 3).toVariant().toFloat();
            // Set gradient stop position into w of the vector
            colorVec[3] = 1.0;
            QLottieProperty4D<QVector4D> colorPos;
            colorPos.setValue(colorVec);
            qreal pos = colorArr.at(i + 0).toVariant().toFloat();
            m_colors[qRound(pos * 100)] = colorPos;
        }
        for (int i = (elementCount * 4); i < colorArr.size(); i+= 2) {
            qreal pos = colorArr.at(i).toVariant().toFloat();
            qreal opacity = colorArr.at(i + 1).toVariant().toFloat();
            QLottieProperty4D<QVector4D> colorVec = m_colors[qRound(pos * 100)];
            QVector4D color = colorVec.value();
            color[3] = opacity;
            colorVec.setValue(color);
            m_colors[qRound(pos * 100)] = colorVec;
        }
    } else {
        qCInfo(lcLottieQtLottieParser) << "Animated gradient is not supported";
    }

    QJsonObject opacity = definition.value(QLatin1String("o")).toObject();
    opacity = resolveExpression(opacity);
    m_opacity.construct(opacity);

    QJsonObject startPoint = definition.value(QLatin1String("s")).toObject();
    startPoint = resolveExpression(startPoint);
    m_startPoint.construct(startPoint);

    QJsonObject endPoint = definition.value(QLatin1String("e")).toObject();
    endPoint = resolveExpression(endPoint);
    m_endPoint.construct(endPoint);

    QJsonObject highlight = definition.value(QLatin1String("h")).toObject();
    m_highlightLength.construct(highlight);

    QJsonObject angle = definition.value(QLatin1String("a")).toObject();
    angle = resolveExpression(angle);
    m_highlightAngle.construct(angle);

    m_highlightAngle.setValue(0.0);
}

void QLottieGFill::updateProperties(int frame)
{
    QGradient::Type type = gradientType();
    if (type != QGradient::LinearGradient &&
        type != QGradient::RadialGradient)
        return;

    m_startPoint.update(frame);
    m_endPoint.update(frame);
    m_highlightLength.update(frame);
    m_highlightAngle.update(frame);
    m_opacity.update(frame);
    QHash<int, QLottieProperty4D<QVector4D>>::iterator colorIt = m_colors.begin();
    while (colorIt != m_colors.end()) {
        (*colorIt).update(frame);
        ++colorIt;
    }

    setGradient();
}

void QLottieGFill::render(QLottieRenderer &renderer) const
{
    renderer.render(*this);
}

QGradient *QLottieGFill::value() const
{
    return m_gradient;
}

QGradient::Type QLottieGFill::gradientType() const
{
    if (m_gradient)
        return m_gradient->type();
    else
        return QGradient::NoGradient;
}

QPointF QLottieGFill::startPoint() const
{
    return m_startPoint.value();
}

QPointF QLottieGFill::endPoint() const
{
    return m_endPoint.value();
}

qreal QLottieGFill::highlightLength() const
{
    return m_highlightLength.value();
}

qreal QLottieGFill::highlightAngle() const
{
    return m_highlightAngle.value();
}

qreal QLottieGFill::opacity() const
{
    return m_opacity.value();
}

void QLottieGFill::setGradient()
{
    QHash<int, QLottieProperty4D<QVector4D>>::iterator colorIt = m_colors.begin();
    while (colorIt != m_colors.end()) {
        QVector4D colorPos = (*colorIt).value();
        int pos = colorIt.key();
        qreal opacity = m_opacity.value() / 100.0;
        opacity *= static_cast<qreal>(colorPos[3]);
        QColor color;
        color.setRedF(static_cast<qreal>(colorPos[0]));
        color.setGreenF(static_cast<qreal>(colorPos[1]));
        color.setBlueF(static_cast<qreal>(colorPos[2]));
        color.setAlphaF(opacity);
        m_gradient->setColorAt(pos / 100.0, color);
        ++colorIt;
    }

    switch (gradientType()) {
    case QGradient::LinearGradient:
    {
        QLinearGradient *g = static_cast<QLinearGradient*>(m_gradient);
        g->setStart(m_startPoint.value());
        g->setFinalStop(m_endPoint.value());
        break;
    }
    case QGradient::RadialGradient:
    {
        QRadialGradient *g = static_cast<QRadialGradient*>(m_gradient);
        QLineF radLine(m_startPoint.value(), m_endPoint.value());
        g->setCenter(radLine.p1());
        g->setRadius(radLine.length());
        radLine.setAngle(radLine.angle() - m_highlightAngle.value());
        // QRadialGradient needs focalPoint to be inside (and not on) radius circle
        qreal radFraction = qMin((m_highlightLength.value() / 100.0), 0.999);
        g->setFocalPoint(radLine.pointAt(radFraction));
        break;
    }
    default:
        break;
    }
}

QT_END_NAMESPACE
