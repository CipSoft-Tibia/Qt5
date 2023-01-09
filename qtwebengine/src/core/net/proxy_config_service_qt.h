/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWebEngine module of the Qt Toolkit.
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

#ifndef PROXY_CONFIG_SERVICE_QT_H
#define PROXY_CONFIG_SERVICE_QT_H

#include "base/memory/ref_counted.h"
#include "base/observer_list.h"
#include "base/single_thread_task_runner.h"

#include "net/proxy_resolution/proxy_config.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/proxy_resolution/proxy_config_with_annotation.h"
#include "components/proxy_config/proxy_prefs.h"

#include <QNetworkProxy>

class PrefService;

class ProxyConfigServiceQt
    : public net::ProxyConfigService
    , public net::ProxyConfigService::Observer
{
public:
    static net::ProxyServer fromQNetworkProxy(const QNetworkProxy &);

    explicit ProxyConfigServiceQt(PrefService *prefService,
                                  const scoped_refptr<base::SingleThreadTaskRunner> &taskRunner);
    ~ProxyConfigServiceQt() override;

    // ProxyConfigService implementation:
    void AddObserver(net::ProxyConfigService::Observer *observer) override;
    void RemoveObserver(net::ProxyConfigService::Observer *observer) override;
    ConfigAvailability GetLatestProxyConfig(net::ProxyConfigWithAnnotation *config) override;
    void OnLazyPoll() override;

private:
    // ProxyConfigService::Observer implementation:
    void OnProxyConfigChanged(const net::ProxyConfigWithAnnotation &config,
                              ConfigAvailability availability) override;

    // Retrieve new proxy settings and notify observers.
    void Update();

    // Makes sure that the observer registration with the base service is set up.
    void RegisterObserver();

    std::unique_ptr<net::ProxyConfigService> m_baseService;
    base::ObserverList<net::ProxyConfigService::Observer, true>::Unchecked m_observers;

    // Keep the last state around.
    bool m_usesSystemConfiguration;
    QNetworkProxy m_qtApplicationProxy;
    net::ProxyConfig m_qtProxyConfig;

    // Indicates whether the base service registration is done.
    bool m_registeredObserver;

    // Configuration as defined by prefs.
    net::ProxyConfigWithAnnotation m_prefConfig;
    ProxyPrefs::ConfigState m_prefState;

    SEQUENCE_CHECKER(m_sequenceChecker);

    DISALLOW_COPY_AND_ASSIGN(ProxyConfigServiceQt);
};

#endif // PROXY_CONFIG_SERVICE_QT_H
