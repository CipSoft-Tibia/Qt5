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

#include "renderer/content_renderer_client_qt.h"

#include "extensions/buildflags/buildflags.h"
#include "printing/buildflags/buildflags.h"
#include "renderer/content_settings_observer_qt.h"
#include "base/strings/string_split.h"
#if QT_CONFIG(webengine_spellchecker)
#include "components/spellcheck/renderer/spellcheck.h"
#include "components/spellcheck/renderer/spellcheck_provider.h"
#endif
#include "components/cdm/renderer/external_clear_key_key_system_properties.h"
#include "components/cdm/renderer/widevine_key_system_properties.h"
#include "components/error_page/common/error.h"
#include "components/error_page/common/localized_error.h"
#include "components/network_hints/renderer/web_prescient_networking_impl.h"
#if QT_CONFIG(webengine_printing_and_pdf)
#include "components/printing/renderer/print_render_frame_helper.h"
#endif
#include "components/visitedlink/renderer/visitedlink_reader.h"
#include "components/web_cache/renderer/web_cache_impl.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/child/child_thread.h"
#include "content/public/common/url_constants.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/render_view.h"
#include "media/base/key_system_properties.h"
#include "media/media_buildflags.h"
#include "mojo/public/cpp/bindings/binder_map.h"
#include "net/base/net_errors.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/platform/web_url_error.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/public/web/web_security_policy.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/webui/jstemplate_builder.h"

#if QT_CONFIG(webengine_printing_and_pdf)
#include "renderer/print_web_view_helper_delegate_qt.h"
#endif

#include "common/qt_messages.h"
#include "renderer/render_frame_observer_qt.h"
#include "renderer/web_engine_page_render_frame.h"
#include "renderer/render_configuration.h"
#include "renderer/user_resource_controller.h"
#if QT_CONFIG(webengine_webchannel)
#include "renderer/web_channel_ipc_transport.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "common/extensions/extensions_client_qt.h"
#include "extensions/extensions_renderer_client_qt.h"
#include "extensions/renderer/guest_view/mime_handler_view/mime_handler_view_container_manager.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#endif //ENABLE_EXTENSIONS

#if BUILDFLAG(ENABLE_PLUGINS)
#include "plugins/loadable_plugin_placeholder_qt.h"
#include "content/common/frame_messages.h"
#endif // ENABLE_PLUGINS

#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/connector.h"

#include "components/grit/components_resources.h"

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
#include "base/feature_list.h"
#include "content/public/renderer/key_system_support.h"
#include "media/base/media_switches.h"
#include "media/base/video_codecs.h"
#include "third_party/widevine/cdm/buildflags.h"
#include "third_party/widevine/cdm/widevine_cdm_common.h"
#endif

#if QT_CONFIG(webengine_webrtc) && QT_CONFIG(webengine_extensions)
#include "chrome/renderer/media/webrtc_logging_agent_impl.h"
#endif

#include "web_engine_library_info.h"

namespace QtWebEngineCore {

static const char kHttpErrorDomain[] = "http";

ContentRendererClientQt::ContentRendererClientQt()
{
#if BUILDFLAG(ENABLE_EXTENSIONS)
    extensions::ExtensionsClient::Set(extensions::ExtensionsClientQt::GetInstance());
    extensions::ExtensionsRendererClient::Set(ExtensionsRendererClientQt::GetInstance());
#endif
}

ContentRendererClientQt::~ContentRendererClientQt() {}

void ContentRendererClientQt::RenderThreadStarted()
{
    base::i18n::SetICUDefaultLocale(WebEngineLibraryInfo::getApplicationLocale());
    content::RenderThread *renderThread = content::RenderThread::Get();
    m_renderConfiguration.reset(new RenderConfiguration());
    m_userResourceController.reset(new UserResourceController());
    m_visitedLinkReader.reset(new visitedlink::VisitedLinkReader);
    m_webCacheImpl.reset(new web_cache::WebCacheImpl());

    renderThread->AddObserver(m_renderConfiguration.data());
    renderThread->AddObserver(m_userResourceController.data());

#if QT_CONFIG(webengine_spellchecker)
    if (!m_spellCheck)
        InitSpellCheck();
#endif

    // Allow XMLHttpRequests from qrc to file.
    // ### consider removing for Qt6
    blink::WebURL qrc(blink::KURL("qrc:"));
    blink::WebString file(blink::WebString::FromASCII("file"));
    blink::WebSecurityPolicy::AddOriginAccessAllowListEntry(
            qrc, file, blink::WebString(), 0, network::mojom::CorsDomainMatchMode::kAllowSubdomains,
            network::mojom::CorsPortMatchMode::kAllowAnyPort,
            network::mojom::CorsOriginAccessMatchPriority::kDefaultPriority);

#if BUILDFLAG(ENABLE_EXTENSIONS)
    // Allow the pdf viewer extension to access chrome resources
    blink::WebURL pdfViewerExtension(blink::KURL("chrome-extension://mhjfbmdgcfjbbpaeojofohoefgiehjai"));
    blink::WebString chromeResources(blink::WebString::FromASCII("chrome"));
    blink::WebSecurityPolicy::AddOriginAccessAllowListEntry(
            pdfViewerExtension, chromeResources, blink::WebString(), 0,
            network::mojom::CorsDomainMatchMode::kAllowSubdomains, network::mojom::CorsPortMatchMode::kAllowAnyPort,
            network::mojom::CorsOriginAccessMatchPriority::kDefaultPriority);

    ExtensionsRendererClientQt::GetInstance()->RenderThreadStarted();
#endif
}

void ContentRendererClientQt::ExposeInterfacesToBrowser(mojo::BinderMap* binders)
{
    binders->Add(m_visitedLinkReader->GetBindCallback(), base::SequencedTaskRunnerHandle::Get());

    binders->Add(base::BindRepeating(&web_cache::WebCacheImpl::BindReceiver,
                                     base::Unretained(m_webCacheImpl.get())),
                 base::SequencedTaskRunnerHandle::Get());

#if QT_CONFIG(webengine_spellchecker)
    binders->Add(base::BindRepeating(
                         [](ContentRendererClientQt *client,
                            mojo::PendingReceiver<spellcheck::mojom::SpellChecker> receiver) {
                             if (!client->m_spellCheck)
                                 client->InitSpellCheck();
                             client->m_spellCheck->BindReceiver(std::move(receiver));
                         }, this),
                 base::SequencedTaskRunnerHandle::Get());
#endif

#if QT_CONFIG(webengine_webrtc) && QT_CONFIG(webengine_extensions)
    binders->Add(base::BindRepeating(
                         [](ContentRendererClientQt *client,
                            mojo::PendingReceiver<chrome::mojom::WebRtcLoggingAgent> receiver) {
                                client->GetWebRtcLoggingAgent()->AddReceiver(std::move(receiver));
                         }, this),
                 base::SequencedTaskRunnerHandle::Get());
#endif
}

void ContentRendererClientQt::RenderFrameCreated(content::RenderFrame *render_frame)
{
    QtWebEngineCore::RenderFrameObserverQt *render_frame_observer =
            new QtWebEngineCore::RenderFrameObserverQt(render_frame, m_webCacheImpl.data());
    if (render_frame->IsMainFrame()) {
#if QT_CONFIG(webengine_webchannel)
        new WebChannelIPCTransport(render_frame);
#endif
        new WebEnginePageRenderFrame(render_frame);
    }

    m_userResourceController->renderFrameCreated(render_frame);

    new QtWebEngineCore::ContentSettingsObserverQt(render_frame);

#if QT_CONFIG(webengine_spellchecker)
    new SpellCheckProvider(render_frame, m_spellCheck.data(), this);
#endif
#if QT_CONFIG(webengine_printing_and_pdf)
    new printing::PrintRenderFrameHelper(render_frame, base::WrapUnique(new PrintWebViewHelperDelegateQt()));
#endif // QT_CONFIG(webengine_printing_and_pdf)
#if BUILDFLAG(ENABLE_EXTENSIONS)
    blink::AssociatedInterfaceRegistry *associated_interfaces = render_frame_observer->associatedInterfaces();
    associated_interfaces->AddInterface(base::BindRepeating(
        &extensions::MimeHandlerViewContainerManager::BindReceiver,
        render_frame->GetRoutingID()));

    auto registry = std::make_unique<service_manager::BinderRegistry>();
    ExtensionsRendererClientQt::GetInstance()->RenderFrameCreated(render_frame, render_frame_observer->registry());
#endif
}

void ContentRendererClientQt::RunScriptsAtDocumentStart(content::RenderFrame *render_frame)
{
#if BUILDFLAG(ENABLE_EXTENSIONS)
    ExtensionsRendererClientQt::GetInstance()->RunScriptsAtDocumentStart(render_frame);
    // |render_frame| might be dead by now.
#endif
}

void ContentRendererClientQt::RunScriptsAtDocumentEnd(content::RenderFrame *render_frame)
{
    // Check whether the render_frame has been created and has not been detached yet.
    // Otherwise the WebFrame is not available.
    RenderFrameObserverQt *render_frame_observer = RenderFrameObserverQt::Get(render_frame);

    if (render_frame_observer && !render_frame_observer->isFrameDetached())
        m_userResourceController->RunScriptsAtDocumentEnd(render_frame);

#if BUILDFLAG(ENABLE_EXTENSIONS)
    ExtensionsRendererClientQt::GetInstance()->RunScriptsAtDocumentEnd(render_frame);
    // |render_frame| might be dead by now.
#endif
}

void ContentRendererClientQt::RunScriptsAtDocumentIdle(content::RenderFrame *render_frame)
{
#if BUILDFLAG(ENABLE_EXTENSIONS)
    ExtensionsRendererClientQt::GetInstance()->RunScriptsAtDocumentIdle(render_frame);
    // |render_frame| might be dead by now.
#endif
}

bool ContentRendererClientQt::HasErrorPage(int httpStatusCode)
{
    // Use an internal error page, if we have one for the status code.
    if (!error_page::LocalizedError::HasStrings(error_page::Error::kHttpErrorDomain, httpStatusCode)) {
        return false;
    }

    return true;
}

// To tap into the chromium localized strings. Ripped from the chrome layer (highly simplified).
void ContentRendererClientQt::PrepareErrorPage(content::RenderFrame *renderFrame,
                                               const blink::WebURLError &web_error,
                                               const std::string &httpMethod,
                                               std::string *errorHtml)
{
    GetNavigationErrorStringsInternal(
            renderFrame, httpMethod,
            error_page::Error::NetError((GURL)web_error.url(), web_error.reason(), net::ResolveErrorInfo(), web_error.has_copy_in_cache()),
            errorHtml);
}

void ContentRendererClientQt::PrepareErrorPageForHttpStatusError(content::RenderFrame *renderFrame,
                                                                 const GURL &unreachable_url,
                                                                 const std::string &httpMethod,
                                                                 int http_status,
                                                                 std::string *errorHtml)
{
    GetNavigationErrorStringsInternal(renderFrame, httpMethod,
                                      error_page::Error::HttpError(unreachable_url, http_status),
                                      errorHtml);
}

void ContentRendererClientQt::GetNavigationErrorStringsInternal(content::RenderFrame *renderFrame,
                                                                const std::string &httpMethod,
                                                                const error_page::Error &error,
                                                                std::string *errorHtml)
{
    Q_UNUSED(renderFrame)
    const bool isPost = QByteArray::fromStdString(httpMethod) == QByteArrayLiteral("POST");

    if (errorHtml) {
        // Use a local error page.
        int resourceId;

        const std::string locale = content::RenderThread::Get()->GetLocale();
        // TODO(elproxy): We could potentially get better diagnostics here by first calling
        // NetErrorHelper::GetErrorStringsForDnsProbe, but that one is harder to untangle.

        error_page::LocalizedError::PageState errorPageState =
                error_page::LocalizedError::GetPageState(
                        error.reason(), error.domain(), error.url(), isPost, false,
                        error.stale_copy_in_cache(), false,
                        RenderConfiguration::is_incognito_process(), false, false, false, locale);

        resourceId = IDR_NET_ERROR_HTML;

        std::string extracted_string = ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(resourceId);
        const base::StringPiece template_html(extracted_string.data(), extracted_string.size());
        if (template_html.empty())
            NOTREACHED() << "unable to load template. ID: " << resourceId;
        else // "t" is the id of the templates root node.
            *errorHtml = webui::GetTemplatesHtml(template_html, &errorPageState.strings, "t");
    }
}

uint64_t ContentRendererClientQt::VisitedLinkHash(const char *canonicalUrl, size_t length)
{
    return m_visitedLinkReader->ComputeURLFingerprint(canonicalUrl, length);
}

bool ContentRendererClientQt::IsLinkVisited(uint64_t linkHash)
{
    return m_visitedLinkReader->IsVisited(linkHash);
}

std::unique_ptr<blink::WebPrescientNetworking> ContentRendererClientQt::CreatePrescientNetworking(content::RenderFrame *render_frame)
{
    return std::make_unique<network_hints::WebPrescientNetworkingImpl>(render_frame);
}

bool ContentRendererClientQt::IsPluginHandledExternally(content::RenderFrame *render_frame,
                                   const blink::WebElement &plugin_element,
                                   const GURL &original_url,
                                   const std::string &original_mime_type)
{
#if BUILDFLAG(ENABLE_EXTENSIONS) && BUILDFLAG(ENABLE_PLUGINS)
    bool found = false;
    content::WebPluginInfo plugin_info;
    std::string mime_type;

    render_frame->Send(new FrameHostMsg_GetPluginInfo(render_frame->GetRoutingID(), original_url,
                                                      render_frame->GetWebFrame()->Top()->GetSecurityOrigin(),
                                                      original_mime_type, &found, &plugin_info, &mime_type));

    return extensions::MimeHandlerViewContainerManager::Get(
                content::RenderFrame::FromWebFrame(
                    plugin_element.GetDocument().GetFrame()),
                true /* create_if_does_not_exist */)
                    ->CreateFrameContainer(plugin_element, original_url, mime_type, plugin_info);
#else
    return false;
#endif
}

bool ContentRendererClientQt::OverrideCreatePlugin(content::RenderFrame *render_frame,
                                                   const blink::WebPluginParams &params,
                                                   blink::WebPlugin **plugin)
{
#if BUILDFLAG(ENABLE_EXTENSIONS)
    if (!ExtensionsRendererClientQt::GetInstance()->OverrideCreatePlugin(render_frame, params))
        return false;
#endif //ENABLE_EXTENSIONS

#if BUILDFLAG(ENABLE_PLUGINS)
    content::WebPluginInfo info;
    std::string mime_type;
    bool found = false;

    render_frame->Send(new FrameHostMsg_GetPluginInfo(render_frame->GetRoutingID(), params.url,
                                                      render_frame->GetWebFrame()->Top()->GetSecurityOrigin(),
                                                      params.mime_type.Utf8(), &found, &info, &mime_type));
    if (!found)
        *plugin = LoadablePluginPlaceholderQt::CreateLoadableMissingPlugin(render_frame, params)->plugin();
    else
        *plugin = render_frame->CreatePlugin(info, params, nullptr);
#endif  // BUILDFLAG(ENABLE_PLUGINS)
    return true;
}

bool ContentRendererClientQt::IsOriginIsolatedPepperPlugin(const base::FilePath& plugin_path)
{
    return plugin_path.value() == FILE_PATH_LITERAL("internal-pdf-viewer");
}

#if QT_CONFIG(webengine_webrtc) && QT_CONFIG(webengine_extensions)
chrome::WebRtcLoggingAgentImpl *ContentRendererClientQt::GetWebRtcLoggingAgent()
{
    if (!m_webrtcLoggingAgentImpl) {
        m_webrtcLoggingAgentImpl = std::make_unique<chrome::WebRtcLoggingAgentImpl>();
    }

    return m_webrtcLoggingAgentImpl.get();
}
#endif // QT_CONFIG(webengine_webrtc) && QT_CONFIG(webengine_extensions)

void ContentRendererClientQt::GetInterface(const std::string &interface_name, mojo::ScopedMessagePipeHandle interface_pipe)
{
    content::RenderThread::Get()->BindHostReceiver(mojo::GenericPendingReceiver(interface_name, std::move(interface_pipe)));
}

// The following is based on chrome/renderer/media/chrome_key_systems.cc:
// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.Chromium file.

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
// External Clear Key (used for testing).
static void AddExternalClearKey(std::vector<std::unique_ptr<media::KeySystemProperties>> *concrete_key_systems)
{
    // TODO(xhwang): Move these into an array so we can use a for loop to add
    // supported key systems below.
    static const char kExternalClearKeyKeySystem[] =
            "org.chromium.externalclearkey";
    static const char kExternalClearKeyDecryptOnlyKeySystem[] =
            "org.chromium.externalclearkey.decryptonly";
    static const char kExternalClearKeyMessageTypeTestKeySystem[] =
            "org.chromium.externalclearkey.messagetypetest";
    static const char kExternalClearKeyFileIOTestKeySystem[] =
            "org.chromium.externalclearkey.fileiotest";
    static const char kExternalClearKeyOutputProtectionTestKeySystem[] =
            "org.chromium.externalclearkey.outputprotectiontest";
    static const char kExternalClearKeyPlatformVerificationTestKeySystem[] =
            "org.chromium.externalclearkey.platformverificationtest";
    static const char kExternalClearKeyInitializeFailKeySystem[] =
            "org.chromium.externalclearkey.initializefail";
    static const char kExternalClearKeyCrashKeySystem[] =
            "org.chromium.externalclearkey.crash";
    static const char kExternalClearKeyVerifyCdmHostTestKeySystem[] =
            "org.chromium.externalclearkey.verifycdmhosttest";
    static const char kExternalClearKeyStorageIdTestKeySystem[] =
            "org.chromium.externalclearkey.storageidtest";
    static const char kExternalClearKeyDifferentGuidTestKeySystem[] =
            "org.chromium.externalclearkey.differentguid";
    static const char kExternalClearKeyCdmProxyTestKeySystem[] =
            "org.chromium.externalclearkey.cdmproxytest";

    media::mojom::KeySystemCapabilityPtr capability;
    if (!content::IsKeySystemSupported(kExternalClearKeyKeySystem, &capability)) {
        DVLOG(1) << "External Clear Key not supported";
        return;
    }

    concrete_key_systems->emplace_back(
            new cdm::ExternalClearKeyProperties(kExternalClearKeyKeySystem));

    // Add support of decrypt-only mode in ClearKeyCdm.
    concrete_key_systems->emplace_back(
            new cdm::ExternalClearKeyProperties(kExternalClearKeyDecryptOnlyKeySystem));

    // A key system that triggers various types of messages in ClearKeyCdm.
    concrete_key_systems->emplace_back(
            new cdm::ExternalClearKeyProperties(kExternalClearKeyMessageTypeTestKeySystem));

    // A key system that triggers the FileIO test in ClearKeyCdm.
    concrete_key_systems->emplace_back(
            new cdm::ExternalClearKeyProperties(kExternalClearKeyFileIOTestKeySystem));

    // A key system that triggers the output protection test in ClearKeyCdm.
    concrete_key_systems->emplace_back(
            new cdm::ExternalClearKeyProperties(kExternalClearKeyOutputProtectionTestKeySystem));

    // A key system that triggers the platform verification test in ClearKeyCdm.
    concrete_key_systems->emplace_back(
            new cdm::ExternalClearKeyProperties(kExternalClearKeyPlatformVerificationTestKeySystem));

    // A key system that Chrome thinks is supported by ClearKeyCdm, but actually
    // will be refused by ClearKeyCdm. This is to test the CDM initialization
    // failure case.
    concrete_key_systems->emplace_back(
            new cdm::ExternalClearKeyProperties(kExternalClearKeyInitializeFailKeySystem));

    // A key system that triggers a crash in ClearKeyCdm.
    concrete_key_systems->emplace_back(
            new cdm::ExternalClearKeyProperties(kExternalClearKeyCrashKeySystem));

    // A key system that triggers the verify host files test in ClearKeyCdm.
    concrete_key_systems->emplace_back(
            new cdm::ExternalClearKeyProperties(kExternalClearKeyVerifyCdmHostTestKeySystem));

    // A key system that fetches the Storage ID in ClearKeyCdm.
    concrete_key_systems->emplace_back(
            new cdm::ExternalClearKeyProperties(kExternalClearKeyStorageIdTestKeySystem));

    // A key system that is registered with a different CDM GUID.
    concrete_key_systems->emplace_back(
            new cdm::ExternalClearKeyProperties(kExternalClearKeyDifferentGuidTestKeySystem));

    // A key system that triggers CDM Proxy test in ClearKeyCdm.
    concrete_key_systems->emplace_back(
            new cdm::ExternalClearKeyProperties(kExternalClearKeyCdmProxyTestKeySystem));
}

#if BUILDFLAG(ENABLE_WIDEVINE)
static media::SupportedCodecs GetSupportedCodecs(const std::vector<media::VideoCodec> &supported_video_codecs,
                                                 bool is_secure)
{
    media::SupportedCodecs supported_codecs = media::EME_CODEC_NONE;

    // Audio codecs are always supported because the CDM only does decrypt-only
    // for audio. The only exception is when |is_secure| is true and there's no
    // secure video decoder available, which is a signal that secure hardware
    // decryption is not available either.
    // TODO(sandersd): Distinguish these from those that are directly supported,
    // as those may offer a higher level of protection.
    if (!supported_video_codecs.empty() || !is_secure) {
        supported_codecs |= media::EME_CODEC_OPUS;
        supported_codecs |= media::EME_CODEC_VORBIS;
        supported_codecs |= media::EME_CODEC_FLAC;
#if BUILDFLAG(USE_PROPRIETARY_CODECS)
        supported_codecs |= media::EME_CODEC_AAC;
#endif // BUILDFLAG(USE_PROPRIETARY_CODECS)
    }

    // Video codecs are determined by what was registered for the CDM.
    for (const auto &codec : supported_video_codecs) {
        switch (codec) {
        case media::VideoCodec::kCodecVP8:
            supported_codecs |= media::EME_CODEC_VP8;
            break;
        case media::VideoCodec::kCodecVP9:
            supported_codecs |= media::EME_CODEC_VP9_PROFILE0;
            supported_codecs |= media::EME_CODEC_VP9_PROFILE2;
            break;
#if BUILDFLAG(USE_PROPRIETARY_CODECS)
        case media::VideoCodec::kCodecH264:
            supported_codecs |= media::EME_CODEC_AVC1;
            break;
#endif // BUILDFLAG(USE_PROPRIETARY_CODECS)
        default:
            DVLOG(1) << "Unexpected supported codec: " << GetCodecName(codec);
            break;
        }
    }

    return supported_codecs;
}

static void AddWidevine(std::vector<std::unique_ptr<media::KeySystemProperties>> *concrete_key_systems)
{
    media::mojom::KeySystemCapabilityPtr capability;
    if (!content::IsKeySystemSupported(kWidevineKeySystem, &capability)) {
        DVLOG(1) << "Widevine CDM is not currently available.";
        return;
    }

    // Codecs and encryption schemes.
    auto codecs = GetSupportedCodecs(capability->video_codecs, /*is_secure=*/false);
    const auto &encryption_schemes = capability->encryption_schemes;
    auto hw_secure_codecs = GetSupportedCodecs(capability->hw_secure_video_codecs,
                                               /*is_secure=*/true);
    const auto &hw_secure_encryption_schemes = capability->hw_secure_encryption_schemes;

    // Robustness.
    using Robustness = cdm::WidevineKeySystemProperties::Robustness;
    auto max_audio_robustness = Robustness::SW_SECURE_CRYPTO;
    auto max_video_robustness = Robustness::SW_SECURE_DECODE;

    if (base::FeatureList::IsEnabled(media::kHardwareSecureDecryption)) {
        max_audio_robustness = Robustness::HW_SECURE_CRYPTO;
        max_video_robustness = Robustness::HW_SECURE_ALL;
    }

    // Session types.
    bool cdm_supports_temporary_session = base::Contains(capability->session_types, media::CdmSessionType::kTemporary);
    if (!cdm_supports_temporary_session) {
        DVLOG(1) << "Temporary session must be supported.";
        return;
    }

    auto persistent_license_support = media::EmeSessionTypeSupport::NOT_SUPPORTED;
    auto persistent_usage_record_support = media::EmeSessionTypeSupport::NOT_SUPPORTED;

    // Others.
    auto persistent_state_support = media::EmeFeatureSupport::REQUESTABLE;
    auto distinctive_identifier_support = media::EmeFeatureSupport::NOT_SUPPORTED;

    concrete_key_systems->emplace_back(new cdm::WidevineKeySystemProperties(
                                           codecs, encryption_schemes, hw_secure_codecs,
                                           hw_secure_encryption_schemes, max_audio_robustness, max_video_robustness,
                                           persistent_license_support, persistent_usage_record_support,
                                           persistent_state_support, distinctive_identifier_support));
}
#endif // BUILDFLAG(ENABLE_WIDEVINE)
#endif // BUILDFLAG(ENABLE_LIBRARY_CDMS)

void ContentRendererClientQt::AddSupportedKeySystems(std::vector<std::unique_ptr<media::KeySystemProperties>> *key_systems)
{
#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
    if (base::FeatureList::IsEnabled(media::kExternalClearKeyForTesting))
        AddExternalClearKey(key_systems);

#if BUILDFLAG(ENABLE_WIDEVINE)
    AddWidevine(key_systems);
#endif // BUILDFLAG(ENABLE_WIDEVINE)

#endif // BUILDFLAG(ENABLE_LIBRARY_CDMS)
}

#if QT_CONFIG(webengine_spellchecker)
void ContentRendererClientQt::InitSpellCheck()
{
    m_spellCheck.reset(new SpellCheck(this));
}
#endif

void ContentRendererClientQt::WillSendRequest(blink::WebLocalFrame *frame,
                                              ui::PageTransition transition_type,
                                              const blink::WebURL &url,
                                              const net::SiteForCookies &site_for_cookies,
                                              const url::Origin *initiator_origin,
                                              GURL *new_url,
                                              bool *attach_same_site_cookies)
{
#if BUILDFLAG(ENABLE_EXTENSIONS)
    ExtensionsRendererClientQt::GetInstance()->WillSendRequest(frame, transition_type, url, /*site_for_cookies,*/
                                                               initiator_origin, new_url, attach_same_site_cookies);
    if (!new_url->is_empty())
        return;
#endif
}

bool ContentRendererClientQt::RequiresWebComponentsV0(const GURL &url)
{
    Q_UNUSED(url);
    return false;
}

} // namespace QtWebEngineCore
