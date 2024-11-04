// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/payments/mandatory_reauth_manager.h"

#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/data_model/credit_card.h"
#include "components/autofill/core/browser/metrics/payments/mandatory_reauth_metrics.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/strings/grit/components_chromium_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace autofill::payments {

using autofill_metrics::LogMandatoryReauthOfferOptInDecision;
using autofill_metrics::MandatoryReauthOfferOptInDecision;

MandatoryReauthManager::MandatoryReauthManager(AutofillClient* client)
    : client_(client) {}
MandatoryReauthManager::~MandatoryReauthManager() = default;

void MandatoryReauthManager::Authenticate(
    device_reauth::DeviceAuthRequester requester,
    device_reauth::DeviceAuthenticator::AuthenticateCallback callback) {
  device_authenticator_ = client_->GetDeviceAuthenticator();
  CHECK(device_authenticator_);
  device_authenticator_->Authenticate(
      requester,
      base::BindOnce(&MandatoryReauthManager::OnAuthenticationCompleted,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)),
      /*use_last_valid_auth=*/true);
}

void MandatoryReauthManager::AuthenticateWithMessage(
    const std::u16string& message,
    device_reauth::DeviceAuthenticator::AuthenticateCallback callback) {
  device_authenticator_ = client_->GetDeviceAuthenticator();
  CHECK(device_authenticator_);
  device_authenticator_->AuthenticateWithMessage(
      message,
      base::BindOnce(&MandatoryReauthManager::OnAuthenticationCompleted,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void MandatoryReauthManager::OnAuthenticationCompleted(
    device_reauth::DeviceAuthenticator::AuthenticateCallback callback,
    bool success) {
  device_authenticator_.reset();
  std::move(callback).Run(success);
}

bool MandatoryReauthManager::ShouldOfferOptin(
    const absl::optional<CreditCard>& card_extracted_from_form,
    const absl::optional<absl::variant<FormDataImporter::CardGuid,
                                       FormDataImporter::CardLastFourDigits>>&
        card_identifier_if_non_interactive_authentication_flow_completed,
    FormDataImporter::CreditCardImportType import_type) {
  opt_in_source_ = autofill_metrics::MandatoryReauthOptInOrOutSource::kUnknown;
  // We should not offer to update a user pref in off the record mode.
  if (client_->IsOffTheRecord()) {
    LogMandatoryReauthOfferOptInDecision(
        MandatoryReauthOfferOptInDecision::kIncognitoMode);
    return false;
  }

  // If the user prefs denote that we should not display the re-auth opt-in
  // bubble, return that we should not offer mandatory re-auth opt-in.
  // Pref-related decision logging also occurs within this function call.
  if (!client_->GetPersonalDataManager()
           ->ShouldShowPaymentMethodsMandatoryReauthPromo()) {
    return false;
  }

  // If the device authenticator is not present or we can not authenticate with
  // biometric or screen lock, there will be no way to re-auth if the user
  // enrolls, so return that we should not offer mandatory re-auth opt-in.
  if (scoped_refptr<device_reauth::DeviceAuthenticator> device_authenticator =
          client_->GetDeviceAuthenticator();
      !device_authenticator ||
      !device_authenticator->CanAuthenticateWithBiometricOrScreenLock()) {
    LogMandatoryReauthOfferOptInDecision(
        MandatoryReauthOfferOptInDecision::kNoSupportedReauthMethod);
    return false;
  }

  // If we did not extract any card from the form, then we should not offer
  // re-auth opt-in, as the user submitted a form without a card. It could be
  // confusing to offer payments autofill functionalities when there was no card
  // submitted.
  if (!card_extracted_from_form.has_value()) {
    LogMandatoryReauthOfferOptInDecision(
        MandatoryReauthOfferOptInDecision::kNoCardExtractedFromForm);
    return false;
  }

  // If `card_identifier_if_non_interactive_authentication_flow_completed` is
  // not present, this can mean one of two things: 1) No card was autofilled 2)
  // All autofilled cards went through an interactive authentication flow. In
  // the first case it makes no sense to show a reauth proposal because this is
  // not an autofill moment. In the second case, we don't want to show an opt-in
  // prompt because the user never experienced non-interactive authentication,
  // and actually just went through an interactive authentication. Displaying a
  // prompt to enable re-authentication could be confusing.
  if (!card_identifier_if_non_interactive_authentication_flow_completed
           .has_value()) {
    LogMandatoryReauthOfferOptInDecision(
        MandatoryReauthOfferOptInDecision::
            kWentThroughInteractiveAuthentication);
    return false;
  }

  // We want to offer re-auth if the most recent payments autofill was a
  // non-interactive authentication.
  // `card_identifier_if_non_interactive_authentication_flow_completed` is set
  // when a non-interactive authentication occurs, and
  // `card_extracted_from_form` contains the card details of the card extracted
  // from the form. Thus, below contains extra logic to check that
  // `card_extracted_from_form` matches
  // `card_identifier_if_non_interactive_authentication_flow_completed` to
  // ensure they are the same card, which implies that the most recent payments
  // autofill was a non-interactive authentication.
  switch (import_type) {
    case FormDataImporter::CreditCardImportType::kLocalCard: {
      // From `import_type` we know that the submitted card exists as a local
      // card in the PersonalDataManager. If
      // `card_identifier_if_non_interactive_authentication_flow_completed`
      // holds no card GUID, that means that the card that was most recently
      // filled with non-interactive authentication was not a local card, so we
      // should not offer re-auth. This is possible when a user goes through a
      // non-interactive authentication flow with a card that is not a local
      // card, then types in a local card manually into the form.
      if (!absl::holds_alternative<FormDataImporter::CardGuid>(
              card_identifier_if_non_interactive_authentication_flow_completed
                  .value())) {
        LogMandatoryReauthOfferOptInDecision(
            MandatoryReauthOfferOptInDecision::kManuallyFilledLocalCard);
        return false;
      }
      opt_in_source_ =
          autofill_metrics::MandatoryReauthOptInOrOutSource::kCheckoutLocalCard;
      bool is_card_match = LastFilledCardMatchesSubmittedCard(
          absl::get<FormDataImporter::CardGuid>(
              card_identifier_if_non_interactive_authentication_flow_completed
                  .value()),
          card_extracted_from_form.value());
      LogMandatoryReauthOfferOptInDecision(
          is_card_match ? MandatoryReauthOfferOptInDecision::kOffered
                        : MandatoryReauthOfferOptInDecision::
                              kNoStoredCardForExtractedCard);
      return is_card_match;
    }
    case FormDataImporter::CreditCardImportType::kServerCard: {
      // From `import_type` we know that the submitted card exists as a server
      // card in the PersonalDataManager. If
      // `card_identifier_if_non_interactive_authentication_flow_completed`
      // holds no card GUID, that means that the card that was most recently
      // filled with non-interactive authentication was not a server card, so we
      // should not offer re-auth. This is possible when a user goes through a
      // non-interactive authentication flow with a card that is not a server
      // card, then types in a server card manually into the form.
      if (!absl::holds_alternative<FormDataImporter::CardGuid>(
              card_identifier_if_non_interactive_authentication_flow_completed
                  .value())) {
        LogMandatoryReauthOfferOptInDecision(
            MandatoryReauthOfferOptInDecision::kManuallyFilledServerCard);
        return false;
      }

      for (CreditCard* local_card :
           client_->GetPersonalDataManager()->GetLocalCreditCards()) {
        if (local_card->IsLocalOrServerDuplicateOf(
                card_extracted_from_form.value())) {
          // We found a matching local card for this server card. We then need
          // to check that the local card version of this card was the card most
          // recently filled into the form with non-interactive authentication,
          // as we should show the opt-in prompt in this case.
          bool is_local_card_last_filled_card =
              LastFilledCardMatchesSubmittedCard(
                  absl::get<FormDataImporter::CardGuid>(
                      card_identifier_if_non_interactive_authentication_flow_completed
                          .value()),
                  *local_card);

          // We should only use local card for metrics if the last filled card
          // was the local card, otherwise the last filled card is a server card
          // which is not supported.
          opt_in_source_ =
              is_local_card_last_filled_card
                  ? autofill_metrics::MandatoryReauthOptInOrOutSource::
                        kCheckoutLocalCard
                  : autofill_metrics::MandatoryReauthOptInOrOutSource::kUnknown;

          // If `is_local_card_last_filled_card` is true, we should offer
          // re-auth opt-in, so log that and return true. Otherwise we must have
          // filled the server card (not local card), which is not supported, so
          // log that and return false. Returning true implies we should offer
          // re-auth opt-in, returning false implies we should not.
          LogMandatoryReauthOfferOptInDecision(
              is_local_card_last_filled_card
                  ? MandatoryReauthOfferOptInDecision::kOffered
                  : MandatoryReauthOfferOptInDecision::kUnsupportedCardType);
          return is_local_card_last_filled_card;
        }
      }

      // We could not find a matching local card for this server card, so we
      // should not offer re-auth opt-in as there is no re-auth functionality
      // for server cards.
      LogMandatoryReauthOfferOptInDecision(
          MandatoryReauthOfferOptInDecision::kUnsupportedCardType);
      return false;
    }
    case FormDataImporter::CreditCardImportType::kVirtualCard: {
      // From `import_type` we know that the submitted card exists as a virtual
      // card in the fetched virtual cards cache. If
      // `card_identifier_if_non_interactive_authentication_flow_completed`
      // holds no card last four digits, that means that the card that was most
      // recently filled with non-interactive authentication was not a virtual
      // card, so we should not offer re-auth. This is possible when a user goes
      // through a non-interactive authentication flow with a card that is not a
      // virtual card, then types in a virtual card manually into the form.
      if (!absl::holds_alternative<FormDataImporter::CardLastFourDigits>(
              card_identifier_if_non_interactive_authentication_flow_completed
                  .value())) {
        LogMandatoryReauthOfferOptInDecision(
            MandatoryReauthOfferOptInDecision::kManuallyFilledVirtualCard);
        return false;
      }

      opt_in_source_ = autofill_metrics::MandatoryReauthOptInOrOutSource::
          kCheckoutVirtualCard;
      // If we have extracted a virtual card, we must check the last four digits
      // of the virtual card green pathed against the last four digits of the
      // card extracted from the form, as we do not store virtual cards in the
      // autofill table, so the card extracted from the form will not have a
      // GUID.
      bool is_card_match =
          base::UTF8ToUTF16(
              absl::get<FormDataImporter::CardLastFourDigits>(
                  card_identifier_if_non_interactive_authentication_flow_completed
                      .value())
                  .value()) == card_extracted_from_form->LastFourDigits();
      LogMandatoryReauthOfferOptInDecision(
          is_card_match ? MandatoryReauthOfferOptInDecision::kOffered
                        : MandatoryReauthOfferOptInDecision::
                              kNoStoredCardForExtractedCard);
      return is_card_match;
    }
    case FormDataImporter::CreditCardImportType::kNewCard:
    case FormDataImporter::CreditCardImportType::kNoCard:
      // We should not offer mandatory re-auth opt-in for new cards or undefined
      // cards.
      LogMandatoryReauthOfferOptInDecision(
          MandatoryReauthOfferOptInDecision::kUnsupportedCardType);
      return false;
  }
}

void MandatoryReauthManager::StartOptInFlow() {
  client_->ShowMandatoryReauthOptInPrompt(
      base::BindOnce(&MandatoryReauthManager::OnUserAcceptedOptInPrompt,
                     weak_ptr_factory_.GetWeakPtr()),
      base::BindOnce(&MandatoryReauthManager::OnUserCancelledOptInPrompt,
                     weak_ptr_factory_.GetWeakPtr()),
      base::BindRepeating(&MandatoryReauthManager::OnUserClosedOptInPrompt,
                          weak_ptr_factory_.GetWeakPtr()));
}

void MandatoryReauthManager::OnUserAcceptedOptInPrompt() {
  autofill_metrics::LogMandatoryReauthOptInOrOutUpdateEvent(
      opt_in_source_,
      /*opt_in=*/true,
      autofill_metrics::MandatoryReauthAuthenticationFlowEvent::kFlowStarted);
#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
  AuthenticateWithMessage(
      l10n_util::GetStringUTF16(IDS_PAYMENTS_AUTOFILL_MANDATORY_REAUTH_PROMPT),
      base::BindOnce(
          &MandatoryReauthManager::OnOptInAuthenticationStepCompleted,
          weak_ptr_factory_.GetWeakPtr()));
#elif BUILDFLAG(IS_ANDROID)
  // TODO(crbug.com/1427216): Convert this to
  // DeviceAuthenticator::AuthenticateWithMessage() with the correct message
  // once it is supported. Currently, the message is "Verify it's you".
  Authenticate(device_reauth::DeviceAuthRequester::kPaymentsAutofillOptIn,
               base::BindOnce(
                   &MandatoryReauthManager::OnOptInAuthenticationStepCompleted,
                   weak_ptr_factory_.GetWeakPtr()));
#else
  NOTREACHED_NORETURN();
#endif
}

void MandatoryReauthManager::OnOptInAuthenticationStepCompleted(bool success) {
  autofill_metrics::LogMandatoryReauthOptInOrOutUpdateEvent(
      opt_in_source_,
      /*opt_in=*/true,
      success ? autofill_metrics::MandatoryReauthAuthenticationFlowEvent::
                    kFlowSucceeded
              : autofill_metrics::MandatoryReauthAuthenticationFlowEvent::
                    kFlowFailed);
  if (success) {
    client_->GetPersonalDataManager()->SetPaymentMethodsMandatoryReauthEnabled(
        /*enabled=*/true);
    client_->ShowMandatoryReauthOptInConfirmation();
  } else {
    client_->GetPersonalDataManager()
        ->IncrementPaymentMethodsMandatoryReauthPromoShownCounter();
  }
}

void MandatoryReauthManager::OnUserCancelledOptInPrompt() {
  client_->GetPersonalDataManager()->SetPaymentMethodsMandatoryReauthEnabled(
      /*enabled=*/false);
}

void MandatoryReauthManager::OnUserClosedOptInPrompt() {
  client_->GetPersonalDataManager()
      ->IncrementPaymentMethodsMandatoryReauthPromoShownCounter();
}

MandatoryReauthAuthenticationMethod
MandatoryReauthManager::GetAuthenticationMethod() {
  scoped_refptr<device_reauth::DeviceAuthenticator> device_authenticator =
      client_->GetDeviceAuthenticator();
  if (!device_authenticator) {
    return MandatoryReauthAuthenticationMethod::kUnknown;
  }
  // Order matters here.
  if (device_authenticator->CanAuthenticateWithBiometrics()) {
    return MandatoryReauthAuthenticationMethod::kBiometric;
  }
  if (device_authenticator->CanAuthenticateWithBiometricOrScreenLock()) {
    return MandatoryReauthAuthenticationMethod::kScreenLock;
  }
  return MandatoryReauthAuthenticationMethod::kUnsupportedMethod;
}

bool MandatoryReauthManager::LastFilledCardMatchesSubmittedCard(
    FormDataImporter::CardGuid guid_of_last_filled_card,
    const CreditCard& card_extracted_from_form) {
  // Get the card stored with the same GUID as the most recent card filled
  // into the form. If we do not have a card stored, then that means the
  // user deleted it after filling the form but before submitting. Thus we
  // should return that we should not offer re-auth opt-in.
  CreditCard* stored_card =
      client_->GetPersonalDataManager()->GetCreditCardByGUID(
          guid_of_last_filled_card.value());
  if (!stored_card) {
    return false;
  }

  return stored_card->MatchingCardDetails(card_extracted_from_form);
}

}  // namespace autofill::payments
