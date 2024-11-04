// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/chrome_signin_client_factory.h"

#ifndef TOOLKIT_QT
#include "chrome/browser/net/profile_network_context_service_factory.h"
#endif
#include "chrome/browser/profiles/profile.h"

ChromeSigninClientFactory::ChromeSigninClientFactory()
    : ProfileKeyedServiceFactory(
          "ChromeSigninClient",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              // TODO(crbug.com/1418376): Check if this service is needed in
              // Guest mode.
              .WithGuest(ProfileSelection::kOriginalOnly)
              .Build()) {
#ifndef TOOLKIT_QT
  DependsOn(ProfileNetworkContextServiceFactory::GetInstance());
#endif
}

ChromeSigninClientFactory::~ChromeSigninClientFactory() = default;

// static
SigninClient* ChromeSigninClientFactory::GetForProfile(Profile* profile) {
  return static_cast<SigninClient*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
ChromeSigninClientFactory* ChromeSigninClientFactory::GetInstance() {
  static base::NoDestructor<ChromeSigninClientFactory> instance;
  return instance.get();
}

KeyedService* ChromeSigninClientFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new ChromeSigninClient(Profile::FromBrowserContext(context));
}
