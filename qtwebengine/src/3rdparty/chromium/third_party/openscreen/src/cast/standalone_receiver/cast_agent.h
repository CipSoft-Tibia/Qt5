// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STANDALONE_RECEIVER_CAST_AGENT_H_
#define CAST_STANDALONE_RECEIVER_CAST_AGENT_H_

#include <openssl/x509.h>

#include <memory>
#include <vector>

#include "cast/common/channel/cast_socket_message_port.h"
#include "cast/common/channel/virtual_connection_manager.h"
#include "cast/common/channel/virtual_connection_router.h"
#include "cast/common/public/cast_socket.h"
#include "cast/receiver/channel/device_auth_namespace_handler.h"
#include "cast/receiver/channel/static_credentials.h"
#include "cast/receiver/public/receiver_socket_factory.h"
#include "cast/standalone_receiver/streaming_playback_controller.h"
#include "cast/streaming/environment.h"
#include "cast/streaming/receiver_session.h"
#include "platform/api/scoped_wake_lock.h"
#include "platform/api/serial_delete_ptr.h"
#include "platform/base/error.h"
#include "platform/base/interface_info.h"
#include "platform/base/tls_credentials.h"
#include "platform/impl/task_runner.h"

namespace openscreen {
namespace cast {

// This class manages sender connections, starting with listening over TLS for
// connection attempts, constructing ReceiverSessions when OFFER messages are
// received, and linking Receivers to the output decoder and SDL visualizer.
//
// Consumers of this class are expected to provide a single threaded task runner
// implementation, a network interface information struct that will be used
// both for TLS listening and UDP messaging, and a credentials provider used
// for TLS listening.
class CastAgent final : public ReceiverSocketFactory::Client,
                        public VirtualConnectionRouter::SocketErrorHandler,
                        public ReceiverSession::Client,
                        public StreamingPlaybackController::Client {
 public:
  CastAgent(
      TaskRunner* task_runner,
      const InterfaceInfo& interface,
      DeviceAuthNamespaceHandler::CredentialsProvider* credentials_provider,
      TlsCredentials tls_credentials);
  ~CastAgent();

  // Initialization occurs as part of construction, however to actually bind
  // for discovery and listening over TLS, the CastAgent must be started.
  Error Start();
  Error Stop();

  // ReceiverSocketFactory::Client overrides.
  void OnConnected(ReceiverSocketFactory* factory,
                   const IPEndpoint& endpoint,
                   std::unique_ptr<CastSocket> socket) override;
  void OnError(ReceiverSocketFactory* factory, Error error) override;

  // VirtualConnectionRouter::SocketErrorHandler overrides.
  void OnClose(CastSocket* cast_socket) override;
  void OnError(CastSocket* socket, Error error) override;

  // ReceiverSession::Client overrides.
  void OnNegotiated(const ReceiverSession* session,
                    ReceiverSession::ConfiguredReceivers receivers) override;
  void OnReceiversDestroying(const ReceiverSession* session,
                             ReceiversDestroyingReason reason) override;
  void OnError(const ReceiverSession* session, Error error) override;

  // StreamingPlaybackController::Client overrides
  void OnPlaybackError(StreamingPlaybackController* controller,
                       Error error) override;

 private:
  // Helper for stopping the current session. This is useful for when we don't
  // want to completely stop (e.g. an issue with a specific Sender) but need
  // to terminate the current connection.
  void StopCurrentSession();

  // Member variables set as part of construction.
  std::unique_ptr<Environment> environment_;
  TaskRunner* const task_runner_;
  IPEndpoint receive_endpoint_;
  DeviceAuthNamespaceHandler::CredentialsProvider* credentials_provider_;
  TlsCredentials tls_credentials_;

  // Member variables set as part of starting up.
  SerialDeletePtr<DeviceAuthNamespaceHandler> auth_handler_;
  SerialDeletePtr<TlsConnectionFactory> connection_factory_;
  VirtualConnectionManager connection_manager_;
  SerialDeletePtr<VirtualConnectionRouter> router_;
  SerialDeletePtr<CastSocketMessagePort> message_port_;
  SerialDeletePtr<ReceiverSocketFactory> socket_factory_;
  SerialDeletePtr<ScopedWakeLock> wake_lock_;

  // Member variables set as part of a sender connection.
  // NOTE: currently we only support a single sender connection and a
  // single streaming session.
  std::unique_ptr<ReceiverSession> current_session_;
  std::unique_ptr<StreamingPlaybackController> controller_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STANDALONE_RECEIVER_CAST_AGENT_H_
