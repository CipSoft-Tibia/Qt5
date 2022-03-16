/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
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

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include "camerabinimageprocessing.h"
#include "camerabinsession.h"

#if QT_CONFIG(linux_v4l)
#include "camerabinv4limageprocessing.h"
#endif

#if GST_CHECK_VERSION(1,0,0)
# include <gst/video/colorbalance.h>
#else
# include <gst/interfaces/colorbalance.h>
#endif

QT_BEGIN_NAMESPACE

CameraBinImageProcessing::CameraBinImageProcessing(CameraBinSession *session)
    : QCameraImageProcessingControl(session)
    , m_session(session)
    , m_whiteBalanceMode(QCameraImageProcessing::WhiteBalanceAuto)
#if QT_CONFIG(linux_v4l)
    , m_v4lImageControl(nullptr)
#endif
{
#if QT_CONFIG(gstreamer_photography)
    if (m_session->photography()) {
        m_mappedWbValues[GST_PHOTOGRAPHY_WB_MODE_AUTO] = QCameraImageProcessing::WhiteBalanceAuto;
        m_mappedWbValues[GST_PHOTOGRAPHY_WB_MODE_DAYLIGHT] = QCameraImageProcessing::WhiteBalanceSunlight;
        m_mappedWbValues[GST_PHOTOGRAPHY_WB_MODE_CLOUDY] = QCameraImageProcessing::WhiteBalanceCloudy;
        m_mappedWbValues[GST_PHOTOGRAPHY_WB_MODE_SUNSET] = QCameraImageProcessing::WhiteBalanceSunset;
        m_mappedWbValues[GST_PHOTOGRAPHY_WB_MODE_TUNGSTEN] = QCameraImageProcessing::WhiteBalanceTungsten;
        m_mappedWbValues[GST_PHOTOGRAPHY_WB_MODE_FLUORESCENT] = QCameraImageProcessing::WhiteBalanceFluorescent;
        unlockWhiteBalance();
    }

#if GST_CHECK_VERSION(1, 0, 0)
      m_filterMap.insert(QCameraImageProcessing::ColorFilterNone, GST_PHOTOGRAPHY_COLOR_TONE_MODE_NORMAL);
      if (m_session->photography()) {
          m_filterMap.insert(QCameraImageProcessing::ColorFilterSepia, GST_PHOTOGRAPHY_COLOR_TONE_MODE_SEPIA);
          m_filterMap.insert(QCameraImageProcessing::ColorFilterGrayscale, GST_PHOTOGRAPHY_COLOR_TONE_MODE_GRAYSCALE);
          m_filterMap.insert(QCameraImageProcessing::ColorFilterNegative, GST_PHOTOGRAPHY_COLOR_TONE_MODE_NEGATIVE);
          m_filterMap.insert(QCameraImageProcessing::ColorFilterSolarize, GST_PHOTOGRAPHY_COLOR_TONE_MODE_SOLARIZE);
#if GST_CHECK_VERSION(1, 2, 0)
          m_filterMap.insert(QCameraImageProcessing::ColorFilterPosterize, GST_PHOTOGRAPHY_COLOR_TONE_MODE_POSTERIZE);
          m_filterMap.insert(QCameraImageProcessing::ColorFilterWhiteboard, GST_PHOTOGRAPHY_COLOR_TONE_MODE_WHITEBOARD);
          m_filterMap.insert(QCameraImageProcessing::ColorFilterBlackboard, GST_PHOTOGRAPHY_COLOR_TONE_MODE_BLACKBOARD);
          m_filterMap.insert(QCameraImageProcessing::ColorFilterAqua, GST_PHOTOGRAPHY_COLOR_TONE_MODE_AQUA);
#endif
      }
#else
      m_filterMap.insert(QCameraImageProcessing::ColorFilterNone, GST_PHOTOGRAPHY_COLOUR_TONE_MODE_NORMAL);
      if (m_session->photography()) {
          m_filterMap.insert(QCameraImageProcessing::ColorFilterSepia, GST_PHOTOGRAPHY_COLOUR_TONE_MODE_SEPIA);
          m_filterMap.insert(QCameraImageProcessing::ColorFilterGrayscale, GST_PHOTOGRAPHY_COLOUR_TONE_MODE_GRAYSCALE);
          m_filterMap.insert(QCameraImageProcessing::ColorFilterNegative, GST_PHOTOGRAPHY_COLOUR_TONE_MODE_NEGATIVE);
          m_filterMap.insert(QCameraImageProcessing::ColorFilterSolarize, GST_PHOTOGRAPHY_COLOUR_TONE_MODE_SOLARIZE);
      }
#endif
#endif

#if QT_CONFIG(linux_v4l)
      m_v4lImageControl = new CameraBinV4LImageProcessing(m_session);
      connect(m_session, &CameraBinSession::statusChanged,
              m_v4lImageControl, &CameraBinV4LImageProcessing::updateParametersInfo);
#endif

    updateColorBalanceValues();
}

CameraBinImageProcessing::~CameraBinImageProcessing()
{
}

void CameraBinImageProcessing::updateColorBalanceValues()
{
    if (!GST_IS_COLOR_BALANCE(m_session->cameraBin())) {
        // Camerabin doesn't implement gstcolorbalance interface
        return;
    }

    GstColorBalance *balance = GST_COLOR_BALANCE(m_session->cameraBin());
    const GList *controls = gst_color_balance_list_channels(balance);

    const GList *item;
    GstColorBalanceChannel *channel;
    gint cur_value;
    qreal scaledValue = 0;

    for (item = controls; item; item = g_list_next (item)) {
        channel = (GstColorBalanceChannel *)item->data;
        cur_value = gst_color_balance_get_value (balance, channel);

        //map the [min_value..max_value] range to [-1.0 .. 1.0]
        if (channel->min_value != channel->max_value) {
            scaledValue = qreal(cur_value - channel->min_value) /
                    (channel->max_value - channel->min_value) * 2 - 1;
        }

        if (!g_ascii_strcasecmp (channel->label, "brightness")) {
            m_values[QCameraImageProcessingControl::BrightnessAdjustment] = scaledValue;
        } else if (!g_ascii_strcasecmp (channel->label, "contrast")) {
            m_values[QCameraImageProcessingControl::ContrastAdjustment] = scaledValue;
        } else if (!g_ascii_strcasecmp (channel->label, "saturation")) {
            m_values[QCameraImageProcessingControl::SaturationAdjustment] = scaledValue;
        }
    }
}

bool CameraBinImageProcessing::setColorBalanceValue(const QString& channel, qreal value)
{

    if (!GST_IS_COLOR_BALANCE(m_session->cameraBin())) {
        // Camerabin doesn't implement gstcolorbalance interface
        return false;
    }

    GstColorBalance *balance = GST_COLOR_BALANCE(m_session->cameraBin());
    const GList *controls = gst_color_balance_list_channels(balance);

    const GList *item;
    GstColorBalanceChannel *colorBalanceChannel;

    for (item = controls; item; item = g_list_next (item)) {
        colorBalanceChannel = (GstColorBalanceChannel *)item->data;

        if (!g_ascii_strcasecmp (colorBalanceChannel->label, channel.toLatin1())) {
            //map the [-1.0 .. 1.0] range to [min_value..max_value]
            gint scaledValue = colorBalanceChannel->min_value + qRound(
                        (value+1.0)/2.0 * (colorBalanceChannel->max_value - colorBalanceChannel->min_value));

            gst_color_balance_set_value (balance, colorBalanceChannel, scaledValue);
            return true;
        }
    }

    return false;
}

QCameraImageProcessing::WhiteBalanceMode CameraBinImageProcessing::whiteBalanceMode() const
{
    return m_whiteBalanceMode;
}

bool CameraBinImageProcessing::setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceMode mode)
{
#if QT_CONFIG(gstreamer_photography)
    if (isWhiteBalanceModeSupported(mode)) {
        m_whiteBalanceMode = mode;
#if GST_CHECK_VERSION(1, 2, 0)
        GstPhotographyWhiteBalanceMode currentMode;
        if (gst_photography_get_white_balance_mode(m_session->photography(), &currentMode)
                && currentMode != GST_PHOTOGRAPHY_WB_MODE_MANUAL)
#endif
        {
            unlockWhiteBalance();
            return true;
        }
    }
#else
    Q_UNUSED(mode);
#endif
    return false;
}

bool CameraBinImageProcessing::isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceMode mode) const
{
#if QT_CONFIG(gstreamer_photography)
    return m_mappedWbValues.values().contains(mode);
#else
    Q_UNUSED(mode);
    return false;
#endif
}

bool CameraBinImageProcessing::isParameterSupported(QCameraImageProcessingControl::ProcessingParameter parameter) const
{
#if QT_CONFIG(gstreamer_photography)
    if (parameter == QCameraImageProcessingControl::WhiteBalancePreset
            || parameter == QCameraImageProcessingControl::ColorFilter) {
        if (m_session->photography())
            return true;
    }
#endif

    if (parameter == QCameraImageProcessingControl::Contrast
            || parameter == QCameraImageProcessingControl::Brightness
            || parameter == QCameraImageProcessingControl::Saturation) {
        if (GST_IS_COLOR_BALANCE(m_session->cameraBin()))
            return true;
    }

#if QT_CONFIG(linux_v4l)
    if (m_v4lImageControl->isParameterSupported(parameter))
        return true;
#endif

    return false;
}

bool CameraBinImageProcessing::isParameterValueSupported(QCameraImageProcessingControl::ProcessingParameter parameter, const QVariant &value) const
{
    switch (parameter) {
    case ContrastAdjustment:
    case BrightnessAdjustment:
    case SaturationAdjustment: {
        const bool isGstColorBalanceValueSupported = GST_IS_COLOR_BALANCE(m_session->cameraBin())
                && qAbs(value.toReal()) <= 1.0;
#if QT_CONFIG(linux_v4l)
        if (!isGstColorBalanceValueSupported)
            return m_v4lImageControl->isParameterValueSupported(parameter, value);
#endif
        return isGstColorBalanceValueSupported;
    }
    case SharpeningAdjustment: {
#if QT_CONFIG(linux_v4l)
        return m_v4lImageControl->isParameterValueSupported(parameter, value);
#else
        return false;
#endif
    }
    case WhiteBalancePreset: {
        const QCameraImageProcessing::WhiteBalanceMode mode =
                value.value<QCameraImageProcessing::WhiteBalanceMode>();
        const bool isPhotographyWhiteBalanceSupported = isWhiteBalanceModeSupported(mode);
#if QT_CONFIG(linux_v4l)
        if (!isPhotographyWhiteBalanceSupported)
            return m_v4lImageControl->isParameterValueSupported(parameter, value);
#endif
        return isPhotographyWhiteBalanceSupported;
    }
    case ColorTemperature: {
#if QT_CONFIG(linux_v4l)
        return m_v4lImageControl->isParameterValueSupported(parameter, value);
#else
        return false;
#endif
    }
    case ColorFilter: {
        const QCameraImageProcessing::ColorFilter filter = value.value<QCameraImageProcessing::ColorFilter>();
#if QT_CONFIG(gstreamer_photography)
        return m_filterMap.contains(filter);
#else
        return filter == QCameraImageProcessing::ColorFilterNone;
#endif
    }
    default:
        break;
    }

    return false;
}

QVariant CameraBinImageProcessing::parameter(
        QCameraImageProcessingControl::ProcessingParameter parameter) const
{
    switch (parameter) {
    case QCameraImageProcessingControl::WhiteBalancePreset: {
        const QCameraImageProcessing::WhiteBalanceMode mode = whiteBalanceMode();
#if QT_CONFIG(linux_v4l)
        if (mode == QCameraImageProcessing::WhiteBalanceAuto
                || mode == QCameraImageProcessing::WhiteBalanceManual) {
            return m_v4lImageControl->parameter(parameter);
        }
#endif
        return QVariant::fromValue<QCameraImageProcessing::WhiteBalanceMode>(mode);
    }
    case QCameraImageProcessingControl::ColorTemperature: {
#if QT_CONFIG(linux_v4l)
        return m_v4lImageControl->parameter(parameter);
#else
        return QVariant();
#endif
    }
    case QCameraImageProcessingControl::ColorFilter:
#if QT_CONFIG(gstreamer_photography)
        if (GstPhotography *photography = m_session->photography()) {
#if GST_CHECK_VERSION(1, 0, 0)
            GstPhotographyColorToneMode mode = GST_PHOTOGRAPHY_COLOR_TONE_MODE_NORMAL;
            gst_photography_get_color_tone_mode(photography, &mode);
#else
            GstColourToneMode mode = GST_PHOTOGRAPHY_COLOUR_TONE_MODE_NORMAL;
            gst_photography_get_colour_tone_mode(photography, &mode);
#endif
            return QVariant::fromValue(m_filterMap.key(mode, QCameraImageProcessing::ColorFilterNone));
        }
#endif
        return QVariant::fromValue(QCameraImageProcessing::ColorFilterNone);
    default: {
        const bool isGstParameterSupported = m_values.contains(parameter);
#if QT_CONFIG(linux_v4l)
        if (!isGstParameterSupported) {
            if (parameter == QCameraImageProcessingControl::BrightnessAdjustment
                    || parameter == QCameraImageProcessingControl::ContrastAdjustment
                    || parameter == QCameraImageProcessingControl::SaturationAdjustment
                    || parameter == QCameraImageProcessingControl::SharpeningAdjustment) {
                return m_v4lImageControl->parameter(parameter);
            }
        }
#endif
        return isGstParameterSupported
                ? QVariant(m_values.value(parameter))
                : QVariant();
    }
    }
}

void CameraBinImageProcessing::setParameter(QCameraImageProcessingControl::ProcessingParameter parameter,
        const QVariant &value)
{
    switch (parameter) {
    case ContrastAdjustment: {
        if (!setColorBalanceValue("contrast", value.toReal())) {
#if QT_CONFIG(linux_v4l)
            m_v4lImageControl->setParameter(parameter, value);
#endif
        }
    }
        break;
    case BrightnessAdjustment: {
        if (!setColorBalanceValue("brightness", value.toReal())) {
#if QT_CONFIG(linux_v4l)
            m_v4lImageControl->setParameter(parameter, value);
#endif
        }
    }
        break;
    case SaturationAdjustment: {
        if (!setColorBalanceValue("saturation", value.toReal())) {
#if QT_CONFIG(linux_v4l)
            m_v4lImageControl->setParameter(parameter, value);
#endif
        }
    }
        break;
    case SharpeningAdjustment: {
#if QT_CONFIG(linux_v4l)
        m_v4lImageControl->setParameter(parameter, value);
#endif
    }
        break;
    case WhiteBalancePreset: {
        if (!setWhiteBalanceMode(value.value<QCameraImageProcessing::WhiteBalanceMode>())) {
#if QT_CONFIG(linux_v4l)
            const QCameraImageProcessing::WhiteBalanceMode mode =
                    value.value<QCameraImageProcessing::WhiteBalanceMode>();
            if (mode == QCameraImageProcessing::WhiteBalanceAuto
                    || mode == QCameraImageProcessing::WhiteBalanceManual) {
                m_v4lImageControl->setParameter(parameter, value);
                return;
            }
#endif
        }
    }
        break;
    case QCameraImageProcessingControl::ColorTemperature: {
#if QT_CONFIG(linux_v4l)
        m_v4lImageControl->setParameter(parameter, value);
#endif
        break;
    }
    case QCameraImageProcessingControl::ColorFilter:
#if QT_CONFIG(gstreamer_photography)
        if (GstPhotography *photography = m_session->photography()) {
#if GST_CHECK_VERSION(1, 0, 0)
            gst_photography_set_color_tone_mode(photography, m_filterMap.value(
                        value.value<QCameraImageProcessing::ColorFilter>(),
                        GST_PHOTOGRAPHY_COLOR_TONE_MODE_NORMAL));
#else
            gst_photography_set_colour_tone_mode(photography, m_filterMap.value(
                        value.value<QCameraImageProcessing::ColorFilter>(),
                        GST_PHOTOGRAPHY_COLOUR_TONE_MODE_NORMAL));
#endif
        }
#endif
        break;
    default:
        break;
    }

    updateColorBalanceValues();
}

#if QT_CONFIG(gstreamer_photography)
void CameraBinImageProcessing::lockWhiteBalance()
{
#if GST_CHECK_VERSION(1, 2, 0)
    if (GstPhotography *photography = m_session->photography()) {
        gst_photography_set_white_balance_mode(photography, GST_PHOTOGRAPHY_WB_MODE_MANUAL);
    }
#endif
}

void CameraBinImageProcessing::unlockWhiteBalance()
{
    if (GstPhotography *photography = m_session->photography()) {
        gst_photography_set_white_balance_mode(
                photography, m_mappedWbValues.key(m_whiteBalanceMode));
    }
}
#endif

QT_END_NAMESPACE
