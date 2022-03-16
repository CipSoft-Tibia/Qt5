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

#include "content_main_delegate_qt.h"

#include "base/command_line.h"
#include <base/i18n/rtl.h>
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include <chrome/grit/generated_resources.h>
#include "content/public/common/content_paths.h"
#include "content/public/common/content_switches.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/ui_base_paths.h"
#include "ui/base/resource/resource_bundle.h"
#include <ui/base/webui/jstemplate_builder.h>
#include "net/grit/net_resources.h"
#include "net/base/net_module.h"
#include "services/service_manager/sandbox/switches.h"
#include "url/url_util_qt.h"

#include "content_client_qt.h"
#include "renderer/content_renderer_client_qt.h"
#include "type_conversion.h"
#include "web_engine_library_info.h"

#if defined(ARCH_CPU_ARM_FAMILY) && (defined(OS_ANDROID) || defined(OS_LINUX))
#include "base/cpu.h"
#endif

#if defined(OS_LINUX)
#include "ui/base/ui_base_switches.h"
#endif

#include <QtCore/qcoreapplication.h>

namespace QtWebEngineCore {

// The logic of this function is based on chrome/common/net/net_resource_provider.cc
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.Chromium file.
static std::string constructDirHeaderHTML()
{
    base::DictionaryValue dict;
    dict.SetString("header", l10n_util::GetStringUTF16(IDS_DIRECTORY_LISTING_HEADER));
    dict.SetString("parentDirText", l10n_util::GetStringUTF16(IDS_DIRECTORY_LISTING_PARENT));
    dict.SetString("headerName", l10n_util::GetStringUTF16(IDS_DIRECTORY_LISTING_NAME));
    dict.SetString("headerSize", l10n_util::GetStringUTF16(IDS_DIRECTORY_LISTING_SIZE));
    dict.SetString("headerDateModified",
                    l10n_util::GetStringUTF16(IDS_DIRECTORY_LISTING_DATE_MODIFIED));
    dict.SetString("language", l10n_util::GetLanguage(base::i18n::GetConfiguredLocale()));
    dict.SetString("listingParsingErrorBoxText",
                    l10n_util::GetStringFUTF16(IDS_DIRECTORY_LISTING_PARSING_ERROR_BOX_TEXT,
                    toString16(QCoreApplication::applicationName())));
    dict.SetString("textdirection", base::i18n::IsRTL() ? "rtl" : "ltr");
    std::string html = webui::GetI18nTemplateHtml(
                ui::ResourceBundle::GetSharedInstance().GetRawDataResource(IDR_DIR_HEADER_HTML),
                &dict);
    return html;
}

static base::StringPiece PlatformResourceProvider(int key) {
    if (key == IDR_DIR_HEADER_HTML) {
        static std::string html_data = constructDirHeaderHTML();
        return base::StringPiece(html_data);
    }
    return base::StringPiece();
}

// Logging logic is based on chrome/common/logging_chrome.cc:
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.Chromium file.

static logging::LoggingDestination DetermineLogMode(const base::CommandLine& command_line)
{
#ifdef NDEBUG
    bool enable_logging = false;
    const char *kInvertLoggingSwitch = switches::kEnableLogging;
#else
    bool enable_logging = true;
    const char *kInvertLoggingSwitch = switches::kDisableLogging;
#endif

    if (command_line.HasSwitch(kInvertLoggingSwitch))
        enable_logging = !enable_logging;

    if (enable_logging)
        return logging::LOG_TO_SYSTEM_DEBUG_LOG;
    else
        return logging::LOG_NONE;
}

void ContentMainDelegateQt::PreSandboxStartup()
{
#if defined(ARCH_CPU_ARM_FAMILY) && (defined(OS_ANDROID) || defined(OS_LINUX))
    // Create an instance of the CPU class to parse /proc/cpuinfo and cache
    // cpu_brand info.
    base::CPU cpu_info;
#endif

    net::NetModule::SetResourceProvider(PlatformResourceProvider);
    ui::ResourceBundle::InitSharedInstanceWithLocale(WebEngineLibraryInfo::getApplicationLocale(), 0, ui::ResourceBundle::LOAD_COMMON_RESOURCES);

    base::CommandLine* parsedCommandLine = base::CommandLine::ForCurrentProcess();
    logging::LoggingSettings settings;
    settings.logging_dest = DetermineLogMode(*parsedCommandLine);
    logging::InitLogging(settings);
    // view the logs with process/thread IDs and timestamps
    logging::SetLogItems(true, //enable_process_id
                         true, //enable_thread_id
                         true, //enable_timestamp
                         false //enable_tickcount
                        );

    if (logging::GetMinLogLevel() >= logging::LOG_INFO) {
        if (parsedCommandLine->HasSwitch(switches::kLoggingLevel)) {
            std::string logLevelValue = parsedCommandLine->GetSwitchValueASCII(switches::kLoggingLevel);
            int level = 0;
            if (base::StringToInt(logLevelValue, &level) && level >= logging::LOG_INFO && level < logging::LOG_NUM_SEVERITIES)
                logging::SetMinLogLevel(level);
        }
    }
}

content::ContentBrowserClient *ContentMainDelegateQt::CreateContentBrowserClient()
{
    m_browserClient.reset(new ContentBrowserClientQt);
    return m_browserClient.get();
}

content::ContentRendererClient *ContentMainDelegateQt::CreateContentRendererClient()
{
#if defined(OS_LINUX)
    base::CommandLine *parsedCommandLine = base::CommandLine::ForCurrentProcess();
    std::string process_type = parsedCommandLine->GetSwitchValueASCII(switches::kProcessType);
    bool no_sandbox = parsedCommandLine->HasSwitch(service_manager::switches::kNoSandbox);

    // Reload locale if the renderer process is sandboxed
    if (process_type == switches::kRendererProcess && !no_sandbox) {
        if (parsedCommandLine->HasSwitch(switches::kLang)) {
            const std::string &locale = parsedCommandLine->GetSwitchValueASCII(switches::kLang);
            ui::ResourceBundle::GetSharedInstance().ReloadLocaleResources(locale);
        }
    }
#endif

    return new ContentRendererClientQt;
}

content::ContentUtilityClient *ContentMainDelegateQt::CreateContentUtilityClient()
{
    m_utilityClient.reset(new ContentUtilityClientQt);
    return m_utilityClient.get();
}

// see icu_util.cc
#define ICU_UTIL_DATA_FILE   0
#define ICU_UTIL_DATA_SHARED 1
#define ICU_UTIL_DATA_STATIC 2

static void SafeOverridePathImpl(const char *keyName, int key, const base::FilePath &path)
{
    if (path.empty())
        return;

    // Do not create directories for overridden paths.
    if (base::PathService::OverrideAndCreateIfNeeded(key, path, false, false))
        return;

    qWarning("Path override failed for key %s and path '%s'", keyName, path.value().c_str());
}

#define SafeOverridePath(KEY, PATH) SafeOverridePathImpl(#KEY, KEY, PATH)

bool ContentMainDelegateQt::BasicStartupComplete(int *exit_code)
{
    SafeOverridePath(base::FILE_EXE, WebEngineLibraryInfo::getPath(base::FILE_EXE));
#if ICU_UTIL_DATA_IMPL == ICU_UTIL_DATA_FILE
    SafeOverridePath(base::DIR_QT_LIBRARY_DATA, WebEngineLibraryInfo::getPath(base::DIR_QT_LIBRARY_DATA));
#endif
    SafeOverridePath(ui::DIR_LOCALES, WebEngineLibraryInfo::getPath(ui::DIR_LOCALES));
#if QT_CONFIG(webengine_spellchecker)
    SafeOverridePath(base::DIR_APP_DICTIONARIES, WebEngineLibraryInfo::getPath(base::DIR_APP_DICTIONARIES));
#endif
    SetContentClient(new ContentClientQt);

    url::CustomScheme::LoadSchemes(base::CommandLine::ForCurrentProcess());

    return false;
}

} // namespace QtWebEngineCore
