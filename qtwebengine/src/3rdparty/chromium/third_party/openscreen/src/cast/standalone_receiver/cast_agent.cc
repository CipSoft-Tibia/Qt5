// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/standalone_receiver/cast_agent.h"

#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/str_cat.h"
#include "cast/common/channel/cast_socket_message_port.h"
#include "cast/common/channel/message_util.h"
#include "cast/streaming/constants.h"
#include "cast/streaming/offer_messages.h"
#include "platform/base/tls_credentials.h"
#include "platform/base/tls_listen_options.h"
#include "util/json/json_serialization.h"
#include "util/osp_logging.h"
#include "util/trace_logging.h"

namespace openscreen {
namespace cast {
namespace {

constexpr int kDefaultMaxBacklogSize = 64;
const TlsListenOptions kDefaultListenOptions{kDefaultMaxBacklogSize};

}  // namespace

CastAgent::CastAgent(
    TaskRunner* task_runner,
    const InterfaceInfo& interface,
    DeviceAuthNamespaceHandler::CredentialsProvider* credentials_provider,
    TlsCredentials tls_credentials)
    : task_runner_(task_runner),
      credentials_provider_(credentials_provider),
      tls_credentials_(std::move(tls_credentials)) {
  const IPAddress address = interface.GetIpAddressV4()
                                ? interface.GetIpAddressV4()
                                : interface.GetIpAddressV6();
  OSP_CHECK(address);
  environment_ = std::make_unique<Environment>(
      &Clock::now, task_runner_,
      IPEndpoint{address, kDefaultCastStreamingPort});
  receive_endpoint_ = IPEndpoint{address, kDefaultCastPort};
}

CastAgent::~CastAgent() = default;

Error CastAgent::Start() {
  TRACE_DEFAULT_SCOPED(TraceCategory::kStandaloneReceiver);
  OSP_CHECK(!current_session_);

  task_runner_->PostTask([this] {
    wake_lock_ = ScopedWakeLock::Create(task_runner_);

    auth_handler_ = MakeSerialDelete<DeviceAuthNamespaceHandler>(
        task_runner_, credentials_provider_);
    router_ = MakeSerialDelete<VirtualConnectionRouter>(task_runner_,
                                                        &connection_manager_);
    message_port_ =
        MakeSerialDelete<CastSocketMessagePort>(task_runner_, router_.get());
    router_->AddHandlerForLocalId(kPlatformReceiverId, auth_handler_.get());
    socket_factory_ = MakeSerialDelete<ReceiverSocketFactory>(
        task_runner_, this, router_.get());
    connection_factory_ = SerialDeletePtr<TlsConnectionFactory>(
        task_runner_,
        TlsConnectionFactory::CreateFactory(socket_factory_.get(), task_runner_)
            .release());
    connection_factory_->SetListenCredentials(tls_credentials_);
    connection_factory_->Listen(receive_endpoint_, kDefaultListenOptions);
    OSP_LOG_INFO << "Listening for connections at: " << receive_endpoint_;
  });

  return Error::None();
}

Error CastAgent::Stop() {
  task_runner_->PostTask([this] {
    router_.reset();
    connection_factory_.reset();
    controller_.reset();
    current_session_.reset();
    socket_factory_.reset();
    wake_lock_.reset();
  });
  return Error::None();
}

void CastAgent::OnConnected(ReceiverSocketFactory* factory,
                            const IPEndpoint& endpoint,
                            std::unique_ptr<CastSocket> socket) {
  TRACE_DEFAULT_SCOPED(TraceCategory::kStandaloneReceiver);
  if (current_session_) {
    OSP_LOG_WARN << "Already connected, dropping peer at: " << endpoint;
    return;
  }

  OSP_LOG_INFO << "Received connection from peer at: " << endpoint;
  message_port_->SetSocket(socket->GetWeakPtr());
  router_->TakeSocket(this, std::move(socket));
  controller_ =
      std::make_unique<StreamingPlaybackController>(task_runner_, this);
  current_session_ = std::make_unique<ReceiverSession>(
      controller_.get(), environment_.get(), message_port_.get(),
      ReceiverSession::Preferences{});
}

void CastAgent::OnError(ReceiverSocketFactory* factory, Error error) {
  OSP_LOG_ERROR << "Cast agent received socket factory error: " << error;
  StopCurrentSession();
}

void CastAgent::OnClose(CastSocket* cast_socket) {
  OSP_VLOG << "Cast agent socket closed.";
  StopCurrentSession();
}

void CastAgent::OnError(CastSocket* socket, Error error) {
  OSP_LOG_ERROR << "Cast agent received socket error: " << error;
  StopCurrentSession();
}

// Currently we don't do anything with the receiver output--the session
// is automatically linked to the playback controller when it is constructed, so
// we don't actually have to interface with the receivers. If we end up caring
// about the receiver configurations we will have to handle OnNegotiated here.
void CastAgent::OnNegotiated(const ReceiverSession* session,
                             ReceiverSession::ConfiguredReceivers receivers) {
  OSP_VLOG << "Successfully negotiated with sender.";
}

void CastAgent::OnReceiversDestroying(const ReceiverSession* session,
                                      ReceiversDestroyingReason reason) {
  const auto GetReasoning = [&] {
    switch (reason) {
      case kEndOfSession:
        return " at end of session.";
      case kRenegotiated:
        return ", to be replaced with new ones.";
    }
    return "";
  };
  OSP_VLOG << "Receiver instances destroying" << GetReasoning();
}

// Currently, we just kill the session if an error is encountered.
void CastAgent::OnError(const ReceiverSession* session, Error error) {
  OSP_LOG_ERROR << "Cast agent received receiver session error: " << error;
  StopCurrentSession();
}

void CastAgent::OnPlaybackError(StreamingPlaybackController* controller,
                                Error error) {
  OSP_LOG_ERROR << "Cast agent received playback error: " << error;
  StopCurrentSession();
}

void CastAgent::StopCurrentSession() {
  current_session_.reset();
  controller_.reset();
  router_->CloseSocket(message_port_->GetSocketId());
  message_port_->SetSocket(nullptr);
}

}  // namespace cast
}  // namespace openscreen
