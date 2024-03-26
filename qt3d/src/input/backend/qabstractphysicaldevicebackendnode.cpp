// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qabstractphysicaldevicebackendnode_p.h"
#include "qabstractphysicaldevicebackendnode_p_p.h"

#include <Qt3DInput/qabstractphysicaldevice.h>
#include <Qt3DInput/qaxissetting.h>
#include <Qt3DInput/qinputaspect.h>

#include <cmath>
#include <algorithm>

#include <Qt3DInput/private/inputhandler_p.h>
#include <Qt3DInput/private/inputmanagers_p.h>
#include <Qt3DInput/private/qinputaspect_p.h>
#include <Qt3DCore/private/qabstractaspect_p.h>

QT_BEGIN_NAMESPACE

namespace {

constexpr int signum(float v)
{
    return (v > 0.0f) - (v < 0.0f);
}

}

namespace Qt3DInput {

QAbstractPhysicalDeviceBackendNodePrivate::QAbstractPhysicalDeviceBackendNodePrivate(Qt3DCore::QBackendNode::Mode mode)
    : Qt3DCore::QBackendNodePrivate(mode)
    , m_axisSettings()
    , m_inputAspect(nullptr)
{
}

void QAbstractPhysicalDeviceBackendNodePrivate::addAxisSetting(int axisIdentifier, Qt3DCore::QNodeId axisSettingsId)
{
    Input::AxisIdSetting axisIdSetting;
    axisIdSetting.m_axisIdentifier = axisIdentifier;
    axisIdSetting.m_axisSettingsId = axisSettingsId;

    // Replace if already present, otherwise append
    const auto end = m_axisSettings.end();
    for (auto it = m_axisSettings.begin(); it != end; ++it) {
        if (it->m_axisIdentifier == axisIdentifier) {
            *it = axisIdSetting;
            return;
        }
    }
    m_axisSettings.push_back(axisIdSetting);
}

void QAbstractPhysicalDeviceBackendNodePrivate::removeAxisSetting(Qt3DCore::QNodeId axisSettingsId)
{
    const auto end = m_axisSettings.end();
    for (auto it = m_axisSettings.begin(); it != end; ++it) {
        if (it->m_axisSettingsId == axisSettingsId) {
            m_axisSettings.erase(it);
            return;
        }
    }
}

Input::MovingAverage &QAbstractPhysicalDeviceBackendNodePrivate::getOrCreateFilter(int axisIdentifier)
{
    const auto end = m_axisFilters.end();
    for (auto it = m_axisFilters.begin(); it != end; ++it) {
        if (it->m_axisIdentifier == axisIdentifier)
            return it->m_filter;
    }

    Input::AxisIdFilter axisIdFilter;
    axisIdFilter.m_axisIdentifier = axisIdentifier;
    m_axisFilters.push_back(axisIdFilter);
    return m_axisFilters.last().m_filter;
}

Input::AxisSetting *QAbstractPhysicalDeviceBackendNodePrivate::getAxisSetting(Qt3DCore::QNodeId axisSettingId) const
{
    Q_Q(const QAbstractPhysicalDeviceBackendNode);
    QInputAspectPrivate *aspectPrivate = static_cast<QInputAspectPrivate *>(Qt3DCore::QAbstractAspectPrivate::get(q->inputAspect()));
    Input::InputHandler *handler = aspectPrivate->m_inputHandler.data();
    Input::AxisSetting *axisSetting = handler->axisSettingManager()->getOrCreateResource(axisSettingId);
    return axisSetting;
}

QAbstractPhysicalDeviceBackendNode::QAbstractPhysicalDeviceBackendNode(QBackendNode::Mode mode)
    : Input::BackendNode(*new QAbstractPhysicalDeviceBackendNodePrivate(mode))
{
}

QAbstractPhysicalDeviceBackendNode::QAbstractPhysicalDeviceBackendNode(QAbstractPhysicalDeviceBackendNodePrivate &dd)
    : Input::BackendNode(dd)
{
}

void QAbstractPhysicalDeviceBackendNode::cleanup()
{
    Q_D(QAbstractPhysicalDeviceBackendNode);
    QBackendNode::setEnabled(false);
    d->m_axisSettings.clear();
    d->m_axisFilters.clear();
    d->m_inputAspect = nullptr;
}

void QAbstractPhysicalDeviceBackendNode::syncFromFrontEnd(const Qt3DCore::QNode *frontEnd, bool firstTime)
{
    Q_D(QAbstractPhysicalDeviceBackendNode);
    BackendNode::syncFromFrontEnd(frontEnd, firstTime);
    const Qt3DInput::QAbstractPhysicalDevice *node = qobject_cast<const Qt3DInput::QAbstractPhysicalDevice *>(frontEnd);
    if (!node)
        return;

    auto settings = Qt3DCore::qIdsForNodes(node->axisSettings());
    std::sort(std::begin(settings), std::end(settings));
    Qt3DCore::QNodeIdVector addedSettings;
    Qt3DCore::QNodeIdVector removedSettings;
    std::set_difference(std::begin(settings), std::end(settings),
                        std::begin(d->m_currentAxisSettingIds), std::end(d->m_currentAxisSettingIds),
                        std::inserter(addedSettings, addedSettings.end()));
    std::set_difference(std::begin(d->m_currentAxisSettingIds), std::end(d->m_currentAxisSettingIds),
                        std::begin(settings), std::end(settings),
                        std::inserter(removedSettings, removedSettings.end()));
    d->m_currentAxisSettingIds = settings;

    for (const auto &axisSettingId: std::as_const(addedSettings)) {
        Input::AxisSetting *axisSetting = d->getAxisSetting(axisSettingId);
        const auto axisIds = axisSetting->axes();
        for (int axisId : axisIds)
            d->addAxisSetting(axisId, axisSettingId);
    }
    for (const auto &axisSettingId: std::as_const(removedSettings))
        d->removeAxisSetting(axisSettingId);
}

void QAbstractPhysicalDeviceBackendNode::setInputAspect(QInputAspect *aspect)
{
    Q_D(QAbstractPhysicalDeviceBackendNode);
    d->m_inputAspect = aspect;
}

QInputAspect *QAbstractPhysicalDeviceBackendNode::inputAspect() const
{
    Q_D(const QAbstractPhysicalDeviceBackendNode);
    return d->m_inputAspect;
}

float QAbstractPhysicalDeviceBackendNode::processedAxisValue(int axisIdentifier)
{
    Q_D(QAbstractPhysicalDeviceBackendNode);

    // Find axis settings for this axis (if any)
    Qt3DCore::QNodeId axisSettingId;
    const auto end = d->m_axisSettings.cend();
    for (auto it = d->m_axisSettings.cbegin(); it != end; ++it) {
        if (it->m_axisIdentifier == axisIdentifier) {
            axisSettingId = it->m_axisSettingsId;
            break;
        }
    }

    const float rawAxisValue = axisValue(axisIdentifier);
    if (axisSettingId.isNull()) {
        // No special processing. Just return the raw value
        return rawAxisValue;
    } else {
        // Process the raw value in accordance with the settings
        Input::AxisSetting *axisSetting = d->getAxisSetting(axisSettingId);
        Q_ASSERT(axisSetting);
        float val = rawAxisValue;

        // Low pass smoothing
        if (axisSetting->isSmoothEnabled()) {
            // Get the filter corresponding to this axis
            Input::MovingAverage &filter = d->getOrCreateFilter(axisIdentifier);
            filter.addSample(val);
            val = filter.average();
        }

        // Deadzone handling
        const float d = axisSetting->deadZoneRadius();
        if (!qFuzzyIsNull(d)) {
            if (std::abs(val) <= d) {
                val = 0.0f;
            } else {
                // Calculate value that goes from 0 to 1 linearly from the boundary of
                // the dead zone up to 1. That is we with a dead zone value of d, we do not
                // want a step change from 0 to d when the axis leaves the deadzone. Instead
                // we want to increase the gradient of the line so that it goes from 0 to 1
                // over the range d to 1. So instead of having y = x, the equation of the
                // line becomes
                //
                // y = x / (1-d) - d / (1-d) = (x - d) / (1 - d)
                //
                // for positive values, and
                //
                // y = x / (1-d) + d / (1-d) = (x + d) / (1 - d)
                //
                // for negative values.
                val = (val - signum(val) * d) / (1.0f - d);
            }
        }

        return val;
    }

}

} // Qt3DInput

QT_END_NAMESPACE
