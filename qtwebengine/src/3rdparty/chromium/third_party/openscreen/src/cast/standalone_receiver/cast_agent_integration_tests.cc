// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/certificate/cast_trust_store.h"
#include "cast/common/certificate/testing/test_helpers.h"
#include "cast/common/channel/virtual_connection_manager.h"
#include "cast/common/channel/virtual_connection_router.h"
#include "cast/receiver/channel/static_credentials.h"
#include "cast/sender/public/sender_socket_factory.h"
#include "cast/standalone_receiver/cast_agent.h"
#include "gtest/gtest.h"
#include "platform/api/serial_delete_ptr.h"
#include "platform/api/time.h"
#include "platform/impl/network_interface.h"
#include "platform/impl/platform_client_posix.h"
#include "platform/impl/task_runner.h"

namespace openscreen {
namespace cast {
namespace {

// Based heavily on SenderSocketsClient from cast_socket_e2e_test.cc.
class MockSender final : public SenderSocketFactory::Client,
                         public VirtualConnectionRouter::SocketErrorHandler {
 public:
  explicit MockSender(VirtualConnectionRouter* router) : router_(router) {}
  ~MockSender() = default;

  CastSocket* socket() const { return socket_; }

  // SenderSocketFactory::Client overrides.
  void OnConnected(SenderSocketFactory* factory,
                   const IPEndpoint& endpoint,
                   std::unique_ptr<CastSocket> socket) override {
    ASSERT_FALSE(socket_);
    OSP_LOG_INFO << "Sender connected to endpoint: " << endpoint;
    socket_ = socket.get();
    router_->TakeSocket(this, std::move(socket));
  }

  void OnError(SenderSocketFactory* factory,
               const IPEndpoint& endpoint,
               Error error) override {
    FAIL() << error;
  }

  // VirtualConnectionRouter::SocketErrorHandler overrides.
  void OnClose(CastSocket* socket) override {}
  void OnError(CastSocket* socket, Error error) override { FAIL() << error; }

 private:
  VirtualConnectionRouter* const router_;
  std::atomic<CastSocket*> socket_{nullptr};
};

class CastAgentIntegrationTest : public ::testing::Test {
 public:
  void SetUp() override {
    PlatformClientPosix::Create(std::chrono::milliseconds(50),
                                std::chrono::milliseconds(50));
    task_runner_ = reinterpret_cast<TaskRunnerImpl*>(
        PlatformClientPosix::GetInstance()->GetTaskRunner());

    sender_router_ = MakeSerialDelete<VirtualConnectionRouter>(
        task_runner_, &sender_vc_manager_);
    sender_client_ = std::make_unique<MockSender>(sender_router_.get());
    sender_factory_ = MakeSerialDelete<SenderSocketFactory>(
        task_runner_, sender_client_.get(), task_runner_);
    sender_tls_factory_ = SerialDeletePtr<TlsConnectionFactory>(
        task_runner_,
        TlsConnectionFactory::CreateFactory(sender_factory_.get(), task_runner_)
            .release());
    sender_factory_->set_factory(sender_tls_factory_.get());
  }

  void TearDown() override {
    sender_router_.reset();
    sender_tls_factory_.reset();
    sender_factory_.reset();
    PlatformClientPosix::ShutDown();
    // Must be shut down after platform client, so joined tasks
    // depending on certs are called correctly.
    CastTrustStore::ResetInstance();
  }

  void WaitAndAssertSenderSocketConnected() {
    constexpr int kMaxAttempts = 10;
    constexpr std::chrono::milliseconds kSocketWaitDelay(250);
    for (int i = 0; i < kMaxAttempts; ++i) {
      OSP_LOG_INFO << "\tChecking for CastSocket, attempt " << i + 1 << "/"
                   << kMaxAttempts;
      if (sender_client_->socket()) {
        break;
      }
      std::this_thread::sleep_for(kSocketWaitDelay);
    }
    ASSERT_TRUE(sender_client_->socket());
  }

  void AssertConnect(const IPAddress& address) {
    OSP_LOG_INFO << "Sending connect task";
    task_runner_->PostTask(
        [this, &address, port = (static_cast<uint16_t>(kDefaultCastPort))]() {
          OSP_LOG_INFO << "Calling SenderSocketFactory::Connect";
          sender_factory_->Connect(
              IPEndpoint{address, port},
              SenderSocketFactory::DeviceMediaPolicy::kNone,
              sender_router_.get());
        });
    WaitAndAssertSenderSocketConnected();
  }

  TaskRunnerImpl* task_runner_;
  // Cast socket sender components, used in conjuction to mock a Libcast sender.
  VirtualConnectionManager sender_vc_manager_;
  SerialDeletePtr<VirtualConnectionRouter> sender_router_;
  std::unique_ptr<MockSender> sender_client_;
  SerialDeletePtr<SenderSocketFactory> sender_factory_;
  SerialDeletePtr<TlsConnectionFactory> sender_tls_factory_;
};

TEST_F(CastAgentIntegrationTest, CanConnect) {
  absl::optional<InterfaceInfo> loopback = GetLoopbackInterfaceForTesting();
  ASSERT_TRUE(loopback.has_value());

  ErrorOr<GeneratedCredentials> creds =
      GenerateCredentials("Test Device Certificate");
  ASSERT_TRUE(creds.is_value());
  CastTrustStore::CreateInstanceForTest(creds.value().root_cert_der);

  auto agent = MakeSerialDelete<CastAgent>(
      task_runner_, task_runner_, loopback.value(),
      creds.value().provider.get(), creds.value().tls_credentials);
  EXPECT_TRUE(agent->Start().ok());
  AssertConnect(loopback.value().GetIpAddressV4());
  EXPECT_TRUE(agent->Stop().ok());
}

}  // namespace
}  // namespace cast
}  // namespace openscreen
