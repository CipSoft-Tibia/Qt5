// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "QtGui/qbrush.h"
#include "utils_p.h"

#include <QtCore/QRegularExpression>

QT_BEGIN_NAMESPACE

Utils::ParamType Utils::preParseFormat(const QString &format, QString &preStr, QString &postStr,
                                       int &precision, char &formatSpec)
{
    static QRegularExpression formatMatcher(QStringLiteral("^([^%]*)%([\\-\\+#\\s\\d\\.lhjztL]*)([dicuoxfegXFEG])(.*)$"));
    static QRegularExpression precisionMatcher(QStringLiteral("\\.(\\d+)"));

    Utils::ParamType retVal;

    QRegularExpressionMatch formatMatch = formatMatcher.match(format, 0);

    if (formatMatch.hasMatch()) {
        preStr = formatMatch.captured(1);
        // Six and 'g' are defaults in Qt API
        precision = 6;
        if (!formatMatch.captured(2).isEmpty()) {
            QRegularExpressionMatch precisionMatch = precisionMatcher.match(formatMatch.captured(2),
                                                                            0);
            if (precisionMatch.hasMatch())
                precision = precisionMatch.captured(1).toInt();
        }
        if (formatMatch.captured(3).isEmpty())
            formatSpec = 'g';
        else
            formatSpec = formatMatch.captured(3).at(0).toLatin1();
        postStr = formatMatch.captured(4);
        retVal = mapFormatCharToParamType(formatSpec);
    } else {
        retVal = ParamTypeUnknown;
        // The out parameters are irrelevant in unknown case
    }

    return retVal;
}

Utils::ParamType Utils::mapFormatCharToParamType(char formatSpec)
{
    ParamType retVal = ParamTypeUnknown;
    if (formatSpec == 'd' || formatSpec == 'i' || formatSpec == 'c') {
        retVal = ParamTypeInt;
    } else if (formatSpec == 'u' || formatSpec == 'o'
               || formatSpec == 'x'|| formatSpec == 'X') {
        retVal = ParamTypeUInt;
    } else if (formatSpec == 'f' || formatSpec == 'F'
               || formatSpec == 'e' || formatSpec == 'E'
               || formatSpec == 'g' || formatSpec == 'G') {
        retVal = ParamTypeReal;
    }

    return retVal;
}

QString Utils::formatLabelSprintf(const QByteArray &format, Utils::ParamType paramType, qreal value)
{
    switch (paramType) {
    case ParamTypeInt:
        return QString::asprintf(format.constData(), qint64(value));
    case ParamTypeUInt:
        return QString::asprintf(format.constData(), quint64(value));
    case ParamTypeReal:
        return QString::asprintf(format.constData(), value);
    default:
        // Return format string to detect errors. Bars selection label logic also depends on this.
        return QString::fromUtf8(format);
    }
}

QString Utils::formatLabelLocalized(Utils::ParamType paramType, qreal value,
                                    const QLocale &locale, const QString &preStr, const QString &postStr,
                                    int precision, char formatSpec, const QByteArray &format)
{
    switch (paramType) {
    case ParamTypeInt:
    case ParamTypeUInt:
        return preStr + locale.toString(qint64(value)) + postStr;
    case ParamTypeReal:
        return preStr + locale.toString(value, formatSpec, precision) + postStr;
    default:
        // Return format string to detect errors. Bars selection label logic also depends on this.
        return QString::fromUtf8(format);
    }
}

QString Utils::defaultLabelFormat()
{
    static const QString defaultFormat(QStringLiteral("%.2f"));
    return defaultFormat;
}

float Utils::wrapValue(float value, float min, float max)
{
    if (value > max) {
        value = min + (value - max);

        // In case single wrap fails, jump to opposite end.
        if (value > max)
            value = min;
    }

    if (value < min) {
        value = max + (value - min);

        // In case single wrap fails, jump to opposite end.
        if (value < min)
            value = max;
    }

    return value;
}

QQuaternion Utils::calculateRotation(const QVector3D &xyzRotations)
{
    QQuaternion rotQuatX = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, xyzRotations.x());
    QQuaternion rotQuatY = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, xyzRotations.y());
    QQuaternion rotQuatZ = QQuaternion::fromAxisAndAngle(0.0f, 0.0f, 1.0f, xyzRotations.z());
    QQuaternion totalRotation = rotQuatY * rotQuatZ * rotQuatX;
    return totalRotation;
}

void Utils::verifyGradientCompleteness(QLinearGradient &gradient)
{
    // Fix the start and end stops of the gradient, to make sure it's complete (0...1)
    auto stops = gradient.stops();
    if (stops.first().first != 0.) {
        const QColor firstColor = stops.first().second;
        gradient.setColorAt(0., firstColor);
    }
    if (stops.last().first != 1.) {
        const QColor lastColor = stops.last().second;
        gradient.setColorAt(1., lastColor);
    }
}

QT_END_NAMESPACE
