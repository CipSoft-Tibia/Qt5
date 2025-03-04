// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DEVTOOLS_DEVTOOLS_UI_BINDINGS_H_
#define CHROME_BROWSER_DEVTOOLS_DEVTOOLS_UI_BINDINGS_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/containers/unique_ptr_adapters.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#if defined(AIDA_SCOPE)
#include "chrome/browser/devtools/aida_client.h"
#endif
#if !defined(TOOLKIT_QT)
#include "chrome/browser/devtools/device/devtools_android_bridge.h"
#endif  //! defined(TOOLKIT_QT)
#include "chrome/browser/devtools/devtools_embedder_message_dispatcher.h"
#include "chrome/browser/devtools/devtools_file_helper.h"
#include "chrome/browser/devtools/devtools_file_system_indexer.h"
#if !defined(TOOLKIT_QT)
#include "chrome/browser/devtools/devtools_infobar_delegate.h"
#endif  //! defined(TOOLKIT_QT)
#include "chrome/browser/devtools/devtools_settings.h"
#if !defined(TOOLKIT_QT)
#include "chrome/browser/devtools/devtools_targets_ui.h"
#endif  //! defined(TOOLKIT_QT)
#include "components/prefs/pref_change_registrar.h"
#include "components/signin/public/identity_manager/access_token_fetcher.h"
#include "components/signin/public/identity_manager/access_token_info.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_frontend_host.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "ui/gfx/geometry/size.h"

class DevToolsAndroidBridge;
class Profile;
class PortForwardingStatusSerializer;

namespace content {
class NavigationHandle;
class WebContents;
}

namespace infobars {
class ContentInfoBarManager;
}

// Base implementation of DevTools bindings around front-end.
class DevToolsUIBindings : public DevToolsEmbedderMessageDispatcher::Delegate,
#if !defined(TOOLKIT_QT)
                           public DevToolsAndroidBridge::DeviceCountListener,
#endif  //! defined(TOOLKIT_QT)
                           public content::DevToolsAgentHostClient,
                           public DevToolsFileHelper::Delegate {
 public:
  class Delegate {
   public:
    virtual ~Delegate() {}
    virtual void ActivateWindow() = 0;
    virtual void CloseWindow() = 0;
    virtual void Inspect(scoped_refptr<content::DevToolsAgentHost> host) = 0;
    virtual void SetInspectedPageBounds(const gfx::Rect& rect) = 0;
    virtual void InspectElementCompleted() = 0;
    virtual void SetIsDocked(bool is_docked) = 0;
    virtual void OpenInNewTab(const std::string& url) = 0;
    virtual void SetWhitelistedShortcuts(const std::string& message) = 0;
    virtual void SetEyeDropperActive(bool active) = 0;
    virtual void OpenNodeFrontend() = 0;

    virtual void InspectedContentsClosing() = 0;
    virtual void OnLoadCompleted() = 0;
    virtual void ReadyForTest() = 0;
    virtual void ConnectionReady() = 0;
    virtual void SetOpenNewWindowForPopups(bool value) = 0;
#if !defined(TOOLKIT_QT)
    virtual infobars::ContentInfoBarManager* GetInfoBarManager() = 0;
#endif  //! defined(TOOLKIT_QT)
    virtual void RenderProcessGone(bool crashed) = 0;
    virtual void ShowCertificateViewer(const std::string& cert_chain) = 0;
  };

  static DevToolsUIBindings* ForWebContents(content::WebContents* web_contents);

  static GURL SanitizeFrontendURL(const GURL& url);
  static bool IsValidFrontendURL(const GURL& url);
  static bool IsValidRemoteFrontendURL(const GURL& url);

  explicit DevToolsUIBindings(content::WebContents* web_contents);

  DevToolsUIBindings(const DevToolsUIBindings&) = delete;
  DevToolsUIBindings& operator=(const DevToolsUIBindings&) = delete;

  ~DevToolsUIBindings() override;

  std::string GetTypeForMetrics() override;

  content::WebContents* web_contents() { return web_contents_; }
  Profile* profile() { return profile_; }
  content::DevToolsAgentHost* agent_host() { return agent_host_.get(); }

  // Takes ownership over the |delegate|.
  void SetDelegate(Delegate* delegate);
  void CallClientMethod(
      const std::string& object_name,
      const std::string& method_name,
      base::Value arg1 = {},
      base::Value arg2 = {},
      base::Value arg3 = {},
      base::OnceCallback<void(base::Value)> completion_callback =
          base::OnceCallback<void(base::Value)>());
  void AttachTo(const scoped_refptr<content::DevToolsAgentHost>& agent_host);
  void AttachViaBrowserTarget(
      const scoped_refptr<content::DevToolsAgentHost>& agent_host);
  void Detach();
  bool IsAttachedTo(content::DevToolsAgentHost* agent_host);

  static base::Value::Dict GetSyncInformationForProfile(Profile* profile);

 private:
  using DevToolsUIBindingsList = std::vector<DevToolsUIBindings*>;

  void HandleMessageFromDevToolsFrontend(base::Value::Dict message);

  // content::DevToolsAgentHostClient implementation.
  void DispatchProtocolMessage(content::DevToolsAgentHost* agent_host,
                               base::span<const uint8_t> message) override;
  void AgentHostClosed(content::DevToolsAgentHost* agent_host) override;

  // DevToolsEmbedderMessageDispatcher::Delegate implementation.
  void ActivateWindow() override;
  void CloseWindow() override;
  void LoadCompleted() override;
  void SetInspectedPageBounds(const gfx::Rect& rect) override;
  void InspectElementCompleted() override;
  void InspectedURLChanged(const std::string& url) override;
  void LoadNetworkResource(DispatchCallback callback,
                           const std::string& url,
                           const std::string& headers,
                           int stream_id) override;
  void SetIsDocked(DispatchCallback callback, bool is_docked) override;
  void OpenInNewTab(const std::string& url) override;
  void ShowItemInFolder(const std::string& file_system_path) override;
  void SaveToFile(const std::string& url,
                  const std::string& content,
                  bool save_as) override;
  void AppendToFile(const std::string& url,
                    const std::string& content) override;
  void RequestFileSystems() override;
  void AddFileSystem(const std::string& type) override;
  void RemoveFileSystem(const std::string& file_system_path) override;
  void UpgradeDraggedFileSystemPermissions(
      const std::string& file_system_url) override;
  void IndexPath(int index_request_id,
                 const std::string& file_system_path,
                 const std::string& excluded_folders) override;
  void StopIndexing(int index_request_id) override;
  void SearchInPath(int search_request_id,
                    const std::string& file_system_path,
                    const std::string& query) override;
  void SetWhitelistedShortcuts(const std::string& message) override;
  void SetEyeDropperActive(bool active) override;
  void ShowCertificateViewer(const std::string& cert_chain) override;
  void ZoomIn() override;
  void ZoomOut() override;
  void ResetZoom() override;
  void SetDevicesDiscoveryConfig(
      bool discover_usb_devices,
      bool port_forwarding_enabled,
      const std::string& port_forwarding_config,
      bool network_discovery_enabled,
      const std::string& network_discovery_config) override;
  void SetDevicesUpdatesEnabled(bool enabled) override;
  void PerformActionOnRemotePage(const std::string& page_id,
                                 const std::string& action) override;
  void OpenRemotePage(const std::string& browser_id,
                      const std::string& url) override;
  void OpenNodeFrontend() override;
  void DispatchProtocolMessageFromDevToolsFrontend(
      const std::string& message) override;
  void RecordCountHistogram(const std::string& name,
                            int sample,
                            int min,
                            int exclusive_max,
                            int buckets) override;
  void RecordEnumeratedHistogram(const std::string& name,
                                 int sample,
                                 int boundary_value) override;
  void RecordPerformanceHistogram(const std::string& name,
                                  double duration) override;
  void RecordUserMetricsAction(const std::string& name) override;
  void SendJsonRequest(DispatchCallback callback,
                       const std::string& browser_id,
                       const std::string& url) override;
  void RegisterPreference(const std::string& name,
                          const RegisterOptions& options) override;
  void GetPreferences(DispatchCallback callback) override;
  void GetPreference(DispatchCallback callback,
                     const std::string& name) override;
  void SetPreference(const std::string& name,
                     const std::string& value) override;
  void RemovePreference(const std::string& name) override;
  void ClearPreferences() override;
  void GetSyncInformation(DispatchCallback callback) override;
  void Reattach(DispatchCallback callback) override;
  void ReadyForTest() override;
  void ConnectionReady() override;
  void SetOpenNewWindowForPopups(bool value) override;
  void RegisterExtensionsAPI(const std::string& origin,
                             const std::string& script) override;
  void ShowSurvey(DispatchCallback callback,
                  const std::string& trigger) override;
  void CanShowSurvey(DispatchCallback callback,
                     const std::string& trigger) override;
#if defined(AIDA_SCOPE)
  void DoAidaConversation(DispatchCallback callback,
                          const std::string& request) override;
#endif

  void EnableRemoteDeviceCounter(bool enable);

  void SendMessageAck(int request_id,
                      const base::Value* arg1);
  void InnerAttach();

#if !defined(TOOLKIT_QT)
  // DevToolsAndroidBridge::DeviceCountListener override:
  void DeviceCountChanged(int count) override;

  // Forwards discovered devices to frontend.
  virtual void DevicesUpdated(const std::string& source,
                              const base::Value& targets);
#endif  //! defined(TOOLKIT_QT)

  void ReadyToCommitNavigation(content::NavigationHandle* navigation_handle);
  void DocumentOnLoadCompletedInPrimaryMainFrame();
  void PrimaryPageChanged();
  void FrontendLoaded();

  void JsonReceived(DispatchCallback callback,
                    int result,
                    const std::string& message);
  void DevicesDiscoveryConfigUpdated();
  void SendPortForwardingStatus(base::Value status);

  // DevToolsFileHelper::Delegate overrides.
  void FileSystemAdded(
      const std::string& error,
      const DevToolsFileHelper::FileSystem* file_system) override;
  void FileSystemRemoved(const std::string& file_system_path) override;
  void FilePathsChanged(const std::vector<std::string>& changed_paths,
                        const std::vector<std::string>& added_paths,
                        const std::vector<std::string>& removed_paths) override;

  // DevToolsFileHelper callbacks.
  void FileSavedAs(const std::string& url, const std::string& file_system_path);
  void CanceledFileSaveAs(const std::string& url);
  void AppendedTo(const std::string& url);
  void IndexingTotalWorkCalculated(int request_id,
                                   const std::string& file_system_path,
                                   int total_work);
  void IndexingWorked(int request_id,
                      const std::string& file_system_path,
                      int worked);
  void IndexingDone(int request_id, const std::string& file_system_path);
  void SearchCompleted(int request_id,
                       const std::string& file_system_path,
                       const std::vector<std::string>& file_paths);
#if !defined(TOOLKIT_QT)
  void ShowDevToolsInfoBar(const std::u16string& message,
                           DevToolsInfoBarDelegate::Callback callback);

#endif  //! defined(TOOLKIT_QT)
  // Extensions support.
  void AddDevToolsExtensionsToClient();

  static DevToolsUIBindingsList& GetDevToolsUIBindings();

#if defined(AIDA_SCOPE)
  void OnAidaConverstaionResponse(DispatchCallback callback,
                                  const std::string& response);
#endif
  class FrontendWebContentsObserver;
  std::unique_ptr<FrontendWebContentsObserver> frontend_contents_observer_;

  Profile* profile_;
#if !defined(TOOLKIT_QT)
  DevToolsAndroidBridge* android_bridge_;
#endif  //! defined(TOOLKIT_QT)
  content::WebContents* web_contents_;
  std::unique_ptr<Delegate> delegate_;
  scoped_refptr<content::DevToolsAgentHost> agent_host_;
  std::unique_ptr<content::DevToolsFrontendHost> frontend_host_;
  std::unique_ptr<DevToolsFileHelper> file_helper_;
  scoped_refptr<DevToolsFileSystemIndexer> file_system_indexer_;
  typedef std::map<
      int,
      scoped_refptr<DevToolsFileSystemIndexer::FileSystemIndexingJob> >
      IndexingJobsMap;
  IndexingJobsMap indexing_jobs_;

  bool devices_updates_enabled_;
  bool frontend_loaded_;
#if !defined(TOOLKIT_QT)
  std::unique_ptr<DevToolsTargetsUIHandler> remote_targets_handler_;
  std::unique_ptr<PortForwardingStatusSerializer> port_status_serializer_;
#endif  //! defined(TOOLKIT_QT)
  PrefChangeRegistrar pref_change_registrar_;
  std::unique_ptr<DevToolsEmbedderMessageDispatcher>
      embedder_message_dispatcher_;
  GURL url_;

  class NetworkResourceLoader;
  std::set<std::unique_ptr<NetworkResourceLoader>, base::UniquePtrComparator>
      loaders_;

  using ExtensionsAPIs = std::map<std::string, std::string>;
  ExtensionsAPIs extensions_api_;
  std::string initial_target_id_;

  DevToolsSettings settings_;

#if defined(AIDA_SCOPE)
  std::unique_ptr<AidaClient> aida_client_;
#endif
  base::WeakPtrFactory<DevToolsUIBindings> weak_factory_{this};
};

#endif  // CHROME_BROWSER_DEVTOOLS_DEVTOOLS_UI_BINDINGS_H_
