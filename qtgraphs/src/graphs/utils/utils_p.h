// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QtGraphs API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#ifndef UTILS_P_H
#define UTILS_P_H

#include <private/graphsglobal_p.h>
#include <QtGui/qimage.h>
#include <QtGui/qquaternion.h>
#include <QtCore/qlocale.h>

QT_FORWARD_DECLARE_CLASS(QLinearGradient)

QT_BEGIN_NAMESPACE

class Utils
{
public:
    enum ParamType {
        ParamTypeUnknown = 0,
        ParamTypeInt,
        ParamTypeUInt,
        ParamTypeReal
    };

    static ParamType preParseFormat(const QString &format, QString &preStr, QString &postStr,
                                    int &precision, char &formatSpec);
    static QString formatLabelSprintf(const QByteArray &format, ParamType paramType, qreal value);
    static QString formatLabelLocalized(ParamType paramType, qreal value,
                               const QLocale &locale, const QString &preStr, const QString &postStr,
                               int precision, char formatSpec, const QByteArray &format);
    static QString defaultLabelFormat();

    static float wrapValue(float value, float min, float max);
    static QQuaternion calculateRotation(const QVector3D &xyzRotations);
    static void verifyGradientCompleteness(QLinearGradient &gradient);


private:
    static ParamType mapFormatCharToParamType(char formatSpec);
};

QT_END_NAMESPACE

#endif
