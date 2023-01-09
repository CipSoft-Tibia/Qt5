/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the lottie-qt module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef BMPROPERTY_P_H
#define BMPROPERTY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QPointF>
#include <QLoggingCategory>
#include <QtMath>

#include <QDebug>

#include <QtBodymovin/private/bmconstants_p.h>
#include <QtBodymovin/private/bmlayer_p.h>
#include <QtBodymovin/private/beziereasing_p.h>

QT_BEGIN_NAMESPACE

template<typename T>
struct EasingSegment {
    bool complete = false;
    double startFrame = 0;
    double endFrame = 0;
    T startValue;
    T endValue;
    BezierEasing easing;
};

template<typename T>
class BODYMOVIN_EXPORT BMProperty
{
public:
    virtual ~BMProperty() = default;

    virtual void construct(const QJsonObject &definition)
    {
        if (definition.value(QLatin1String("s")).toVariant().toInt())
            qCWarning(lcLottieQtBodymovinParser)
                    << "Property is split into separate x and y but it is not supported";

        bool fromExpression = definition.value(QLatin1String("fromExpression")).toBool();
        m_animated = definition.value(QLatin1String("a")).toDouble() > 0;
        if (m_animated) {
            QJsonArray keyframes = definition.value(QLatin1String("k")).toArray();
            QJsonArray::const_iterator it = keyframes.constBegin();
            while (it != keyframes.constEnd()) {
                EasingSegment<T> easing = parseKeyframe((*it).toObject(),
                                                        fromExpression);
                addEasing(easing);
                ++it;
            }
            m_value = T();
        } else
            m_value = getValue(definition.value(QLatin1String("k")));
    }

    void setValue(const T& value)
    {
        m_value = value;
    }

    const T& value() const
    {
        return m_value;
    }

    virtual bool update(int frame)
    {
        if (!m_animated)
            return false;

        int adjustedFrame = qBound(m_startFrame, frame, m_endFrame);
        if (const EasingSegment<T> *easing = getEasingSegment(adjustedFrame)) {
            qreal progress;
            if (easing->endFrame == easing->startFrame)
                progress = 1;
            else
                progress = ((adjustedFrame - easing->startFrame) * 1.0) /
                        (easing->endFrame - easing->startFrame);
            qreal easedValue = easing->easing.valueForProgress(progress);
            m_value = easing->startValue + easedValue *
                    ((easing->endValue - easing->startValue));
            return true;
        }
        return false;
    }

protected:
    void addEasing(EasingSegment<T>& easing)
    {
        if (m_easingCurves.length()) {
            EasingSegment<T> prevEase = m_easingCurves.last();
            // The end value has to be hand picked to the
            // previous easing segment, as the json data does
            // not contain end values for segments
            prevEase.endFrame = easing.startFrame - 1;
            m_easingCurves.replace(m_easingCurves.length() - 1, prevEase);
        }
        m_easingCurves.push_back(easing);
    }

    const EasingSegment<T>* getEasingSegment(int frame)
    {
        // TODO: Improve with a faster search algorithm
        const EasingSegment<T> *easing = m_currentEasing;
        if (!easing || easing->startFrame < frame ||
                easing->endFrame > frame) {
            for (int i=0; i < m_easingCurves.length(); i++) {
                if (m_easingCurves.at(i).startFrame <= frame &&
                        m_easingCurves.at(i).endFrame >= frame) {
                    m_currentEasing = &m_easingCurves.at(i);
                    break;
                }
            }
        }

        if (!m_currentEasing) {
            qCWarning(lcLottieQtBodymovinParser)
                    << "Property is animated but easing cannot be found";
        }
        return m_currentEasing;
    }

    virtual EasingSegment<T> parseKeyframe(const QJsonObject keyframe,
                                           bool fromExpression)
    {
        Q_UNUSED(fromExpression);

        EasingSegment<T> easing;

        int startTime = keyframe.value(QLatin1String("t")).toVariant().toInt();

        // AE exported Bodymovin file includes the last
        // key frame but no other properties.
        // No need to process in that case
        if (!keyframe.contains(QLatin1String("s")) && !keyframe.contains(QLatin1String("e"))) {
            // In this case start time is the last frame for the property
            this->m_endFrame = startTime;
            easing.startFrame = startTime;
            easing.endFrame = startTime;
            if (m_easingCurves.length()) {
                easing.startValue = m_easingCurves.last().endValue;
                easing.endValue = m_easingCurves.last().endValue;
            }
            return easing;
        }

        if (m_startFrame > startTime)
            m_startFrame = startTime;

        easing.startValue = getValue(keyframe.value(QLatin1String("s")).toArray());
        easing.endValue = getValue(keyframe.value(QLatin1String("e")).toArray());
        easing.startFrame = startTime;

        QJsonObject easingIn = keyframe.value(QLatin1String("i")).toObject();
        QJsonObject easingOut = keyframe.value(QLatin1String("o")).toObject();

        qreal eix = easingIn.value(QLatin1String("x")).toArray().at(0).toDouble();
        qreal eiy = easingIn.value(QLatin1String("y")).toArray().at(0).toDouble();

        qreal eox = easingOut.value(QLatin1String("x")).toArray().at(0).toDouble();
        qreal eoy = easingOut.value(QLatin1String("y")).toArray().at(0).toDouble();

        QPointF c1 = QPointF(eox, eoy);
        QPointF c2 = QPointF(eix, eiy);

        easing.easing.addCubicBezierSegment(c1, c2, QPointF(1.0, 1.0));

        easing.complete = true;

        return easing;
    }

    virtual T getValue(const QJsonValue &value)
    {
        if (value.isArray())
            return getValue(value.toArray());
        else {
            QVariant val = value.toVariant();
            if (val.canConvert<T>()) {
                T t = val.value<T>();
                return t;
            }
            else
                return T();
        }
    }

    virtual T getValue(const QJsonArray &value)
    {
        QVariant val = value.at(0).toVariant();
        if (val.canConvert<T>()) {
            T t = val.value<T>();
            return t;
        }
        else
            return T();
    }

protected:
    bool m_animated = false;
    QList<EasingSegment<T>> m_easingCurves;
    const EasingSegment<T> *m_currentEasing = nullptr;
    int m_startFrame = INT_MAX;
    int m_endFrame = 0;
    T m_value;
};


template <typename T>
class BODYMOVIN_EXPORT BMProperty2D : public BMProperty<T>
{
protected:
    T getValue(const QJsonArray &value) override
    {
        if (value.count() > 1)
            return T(value.at(0).toDouble(),
                     value.at(1).toDouble());
        else
            return T();
    }

    EasingSegment<T> parseKeyframe(const QJsonObject keyframe,
                                   bool fromExpression) override
    {
        QJsonArray startValues = keyframe.value(QLatin1String("s")).toArray();
        QJsonArray endValues = keyframe.value(QLatin1String("e")).toArray();
        int startTime = keyframe.value(QLatin1String("t")).toVariant().toInt();

        EasingSegment<T> easingCurve;
        easingCurve.startFrame = startTime;

        // AE exported Bodymovin file includes the last
        // key frame but no other properties.
        // No need to process in that case
        if (startValues.isEmpty() && endValues.isEmpty()) {
            // In this case start time is the last frame for the property
            this->m_endFrame = startTime;
            easingCurve.startFrame = startTime;
            easingCurve.endFrame = startTime;
            if (this->m_easingCurves.length()) {
                easingCurve.startValue = this->m_easingCurves.last().endValue;
                easingCurve.endValue = this->m_easingCurves.last().endValue;
            }
            return easingCurve;
        }

        if (this->m_startFrame > startTime)
            this->m_startFrame = startTime;

        qreal xs, ys, xe, ye;
        // Keyframes originating from an expression use only scalar values.
        // They must be expanded for both x and y coordinates
        if (fromExpression) {
            xs = startValues.at(0).toDouble();
            ys = startValues.at(0).toDouble();
            xe = endValues.at(0).toDouble();
            ye = endValues.at(0).toDouble();
        } else {
            xs = startValues.at(0).toDouble();
            ys = startValues.at(1).toDouble();
            xe = endValues.at(0).toDouble();
            ye = endValues.at(1).toDouble();
        }
        T s(xs, ys);
        T e(xe, ye);

        QJsonObject easingIn = keyframe.value(QLatin1String("i")).toObject();
        QJsonObject easingOut = keyframe.value(QLatin1String("o")).toObject();

        easingCurve.startFrame = startTime;
        easingCurve.startValue = s;
        easingCurve.endValue = e;

        if (easingIn.value(QLatin1String("x")).isArray()) {
            QJsonArray eixArr = easingIn.value(QLatin1String("x")).toArray();
            QJsonArray eiyArr = easingIn.value(QLatin1String("y")).toArray();

            QJsonArray eoxArr = easingOut.value(QLatin1String("x")).toArray();
            QJsonArray eoyArr = easingOut.value(QLatin1String("y")).toArray();

            while (!eixArr.isEmpty() && !eiyArr.isEmpty()) {
                qreal eix = eixArr.takeAt(0).toDouble();
                qreal eiy = eiyArr.takeAt(0).toDouble();

                qreal eox =eoxArr.takeAt(0).toDouble();
                qreal eoy = eoyArr.takeAt(0).toDouble();

                QPointF c1 = QPointF(eox, eoy);
                QPointF c2 = QPointF(eix, eiy);

                easingCurve.easing.addCubicBezierSegment(c1, c2, QPointF(1.0, 1.0));
            }
        }
        else {
            qreal eix = easingIn.value(QLatin1String("x")).toDouble();
            qreal eiy = easingIn.value(QLatin1String("y")).toDouble();

            qreal eox = easingOut.value(QLatin1String("x")).toDouble();
            qreal eoy = easingOut.value(QLatin1String("y")).toDouble();

            QPointF c1 = QPointF(eox, eoy);
            QPointF c2 = QPointF(eix, eiy);

            easingCurve.easing.addCubicBezierSegment(c1, c2, QPointF(1.0, 1.0));
        }

        easingCurve.complete = true;
        return easingCurve;
    }
};

template <typename T>
class BODYMOVIN_EXPORT BMProperty4D : public BMProperty<T>
{
public:
    bool update(int frame) override
    {
        if (!this->m_animated)
            return false;

        int adjustedFrame = qBound(this->m_startFrame, frame, this->m_endFrame);
        if (const EasingSegment<T> *easing = BMProperty<T>::getEasingSegment(adjustedFrame)) {
            qreal progress = ((adjustedFrame - this->m_startFrame) * 1.0) /
                    (this->m_endFrame - this->m_startFrame);
            qreal easedValue = easing->easing.valueForProgress(progress);
            // For the time being, 4D vectors are used only for colors, and
            // the value must be restricted to between [0, 1]
            easedValue = qBound(qreal(0.0), easedValue, qreal(1.0));
            T sv = easing->startValue;
            T ev = easing->endValue;
            qreal x = sv.x() + easedValue * (ev.x() - sv.x());
            qreal y = sv.y() + easedValue * (ev.y() - sv.y());
            qreal z = sv.z() + easedValue * (ev.z() - sv.z());
            qreal w = sv.w() + easedValue * (ev.w() - sv.w());
            this->m_value = T(x, y, z, w);
        }

        return true;
    }

protected:
    T getValue(const QJsonArray &value) override
    {
        if (value.count() > 3)
            return T(value.at(0).toDouble(), value.at(1).toDouble(),
                     value.at(2).toDouble(), value.at(3).toDouble());
        else
            return T();
    }
};

QT_END_NAMESPACE

#endif // BMPROPERTY_P_H
