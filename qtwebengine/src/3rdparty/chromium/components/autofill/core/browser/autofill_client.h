// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOFILL_CLIENT_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOFILL_CLIENT_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "base/functional/callback_forward.h"
#include "base/i18n/rtl.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "build/build_config.h"
#include "components/autofill/core/browser/autofill_trigger_details.h"
#include "components/autofill/core/browser/country_type.h"
#include "components/autofill/core/browser/payments/legal_message_line.h"
#include "components/autofill/core/browser/payments/risk_data_loader.h"
#include "components/autofill/core/browser/ui/fast_checkout_client.h"
#include "components/autofill/core/browser/ui/popup_item_ids.h"
#include "components/autofill/core/browser/ui/popup_types.h"
#include "components/autofill/core/browser/ui/suggestion.h"
#include "components/autofill/core/common/aliases.h"
#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/form_field_data.h"
#include "components/autofill/core/common/form_interactions_flow.h"
#include "components/autofill/core/common/unique_ids.h"
#include "components/device_reauth/device_authenticator.h"
#if !defined(TOOLKIT_QT)
#include "components/plus_addresses/plus_address_service.h"
#include "components/profile_metrics/browser_profile_type.h"
#include "components/security_state/core/security_state.h"
#include "components/translate/core/browser/language_state.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#endif
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "ui/base/window_open_disposition.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"
#include "url/origin.h"

class PrefService;

namespace signin {
class IdentityManager;
}

namespace syncer {
class SyncService;
}

namespace ukm {
class UkmRecorder;
}

namespace version_info {
enum class Channel;
}

#if !BUILDFLAG(IS_IOS)
namespace webauthn {
class InternalAuthenticator;
}
#endif

namespace plus_addresses {
class PlusAddressService;
}

namespace autofill {

class AddressNormalizer;
class AutocompleteHistoryManager;
class AutofillAblationStudy;
class AutofillDriver;
class AutofillDownloadManager;
struct AutofillErrorDialogContext;
class AutofillMlPredictionModelHandler;
class AutofillOfferData;
class AutofillOfferManager;
class AutofillOptimizationGuide;
class AutofillPopupDelegate;
class AutofillProfile;
enum class AutofillProgressDialogType;
struct CardUnmaskChallengeOption;
class CardUnmaskDelegate;
struct CardUnmaskPromptOptions;
class CreditCard;
class CreditCardCvcAuthenticator;
enum class CreditCardFetchResult;
class CreditCardOtpAuthenticator;
class FormDataImporter;
class FormStructure;
class Iban;
class IbanManager;
class LogManager;
class MigratableCreditCard;
class MerchantPromoCodeManager;
struct OfferNotificationOptions;
class OtpUnmaskDelegate;
enum class OtpUnmaskResult;
class PersonalDataManager;
class SingleFieldFormFillRouter;
class StrikeDatabase;
struct Suggestion;
class TouchToFillDelegate;
struct VirtualCardEnrollmentFields;
class VirtualCardEnrollmentManager;
struct VirtualCardManualFallbackBubbleOptions;
enum class WebauthnDialogCallbackType;
enum class WebauthnDialogState;

namespace payments {
class PaymentsClient;
class MandatoryReauthManager;
}

// A client interface that needs to be supplied to the Autofill component by the
// embedder.
//
// Each client instance is associated with a given context within which an
// BrowserAutofillManager is used (e.g. a single tab), so when we say "for the
// client" below, we mean "in the execution context the client is associated
// with" (e.g. for the tab the BrowserAutofillManager is attached to).
#if !defined(TOOLKIT_QT)
class AutofillClient : public RiskDataLoader {
#else
class AutofillClient {
#endif
 public:
  enum class PaymentsRpcResult {
    // Empty result. Used for initializing variables and should generally
    // not be returned nor passed as arguments unless explicitly allowed by
    // the API.
    kNone,

    // Request succeeded.
    kSuccess,

    // Request failed; try again.
    kTryAgainFailure,

    // Request failed; don't try again.
    kPermanentFailure,

    // Unable to connect to Payments servers. Prompt user to check internet
    // connection.
    kNetworkError,

    // Request failed in retrieving virtual card information; try again.
    kVcnRetrievalTryAgainFailure,

    // Request failed in retrieving virtual card information; don't try again.
    kVcnRetrievalPermanentFailure,
  };

  // The type of the credit card the Payments RPC fetches.
  enum class PaymentsRpcCardType {
    // Unknown type.
    kUnknown = 0,
    // Server card.
    kServerCard = 1,
    // Virtual card.
    kVirtualCard = 2,
  };

  enum class SaveCardOfferUserDecision {
    // The user accepted credit card save.
    kAccepted,

    // The user explicitly declined credit card save.
    kDeclined,

    // The user ignored the credit card save prompt.
    kIgnored,
  };

  enum class SaveIbanOfferUserDecision {
    // The user accepted IBAN save.
    kAccepted,

    // The user explicitly declined IBAN save.
    kDeclined,

    // The user ignored the IBAN save prompt.
    kIgnored,
  };

  enum class UnmaskCardReason {
    // The card is being unmasked for PaymentRequest.
    kPaymentRequest,

    // The card is being unmasked for Autofill.
    kAutofill,
  };

  // Authentication methods for card unmasking.
  enum class UnmaskAuthMethod {
    kUnknown = 0,
    // Require user to unmask via CVC.
    kCvc = 1,
    // Suggest use of FIDO authenticator for card unmasking.
    kFido = 2,
  };

  enum class SaveAddressProfileOfferUserDecision {
    kUndefined,
    // No prompt is shown and no decision is needed to proceed with the process.
    kUserNotAsked,
    // The user accepted the save/update/migration flow from the initial prompt.
    kAccepted,
    // The user declined the save/update/migration flow from the initial prompt.
    kDeclined,
    // The user accepted the save/update/migration flow from the edit dialog.
    kEditAccepted,
    // The user declined the save/update/migration flow from the edit dialog.
    kEditDeclined,
    // The user selected to never migrate a `kLocalOrSyncable` profile to the
    // account storage.
    // Currently unused for new profile and update prompts.
    kNever,
    // The user ignored the prompt.
    kIgnored,
    // The save/update/migration message timed out before the user interacted.
    // This is only relevant on mobile.
    kMessageTimeout,
    // The user swipes away the save/update/migration message. This is only
    // relevant on mobile.
    kMessageDeclined,
    // The prompt is suppressed most likely because there is already another
    // prompt shown on the same tab.
    kAutoDeclined,
    kMaxValue = kAutoDeclined,
  };

  // Used for explicitly requesting the user to enter/confirm cardholder name,
  // expiration date month and year.
  struct UserProvidedCardDetails {
    std::u16string cardholder_name;
    std::u16string expiration_date_month;
    std::u16string expiration_date_year;
  };

  enum class CardSaveType {
    // Credit card is saved without the CVC.
    kCardSaveOnly = 0,
    // Credit card is saved with the CVC.
    kCardSaveWithCvc = 1,
    // Only CVC is saved.
    kCvcSaveOnly = 2,
  };

  // Used for options of upload prompt.
  struct SaveCreditCardOptions {
    SaveCreditCardOptions& with_from_dynamic_change_form(bool b) {
      from_dynamic_change_form = b;
      return *this;
    }

    SaveCreditCardOptions& with_has_non_focusable_field(bool b) {
      has_non_focusable_field = b;
      return *this;
    }

    SaveCreditCardOptions& with_should_request_name_from_user(bool b) {
      should_request_name_from_user = b;
      return *this;
    }

    SaveCreditCardOptions& with_should_request_expiration_date_from_user(
        bool b) {
      should_request_expiration_date_from_user = b;
      return *this;
    }

    SaveCreditCardOptions& with_show_prompt(bool b = true) {
      show_prompt = b;
      return *this;
    }

    SaveCreditCardOptions& with_has_multiple_legal_lines(bool b = true) {
      has_multiple_legal_lines = b;
      return *this;
    }

    SaveCreditCardOptions&
    with_same_last_four_as_server_card_but_different_expiration_date(bool b) {
      has_same_last_four_as_server_card_but_different_expiration_date = b;
      return *this;
    }

    SaveCreditCardOptions& with_card_save_type(CardSaveType b) {
      card_save_type = b;
      return *this;
    }

    bool from_dynamic_change_form = false;
    bool has_non_focusable_field = false;
    bool should_request_name_from_user = false;
    bool should_request_expiration_date_from_user = false;
    bool show_prompt = false;
    bool has_multiple_legal_lines = false;
    bool has_same_last_four_as_server_card_but_different_expiration_date =
        false;
    CardSaveType card_save_type = CardSaveType::kCardSaveOnly;
  };

  // Used for options of save (and update) address profile prompt.
  struct SaveAddressProfilePromptOptions {
    bool show_prompt = true;

    // Whether the prompt suggests migration into the user's account.
    bool is_migration_to_account = false;
  };

  // Required arguments to create a dropdown showing autofill suggestions.
  struct PopupOpenArgs {
    PopupOpenArgs();
    PopupOpenArgs(const gfx::RectF& element_bounds,
                  base::i18n::TextDirection text_direction,
                  std::vector<Suggestion> suggestions,
                  AutofillSuggestionTriggerSource trigger_source);
    PopupOpenArgs(const PopupOpenArgs&);
    PopupOpenArgs(PopupOpenArgs&&);
    ~PopupOpenArgs();
    PopupOpenArgs& operator=(const PopupOpenArgs&);
    PopupOpenArgs& operator=(PopupOpenArgs&&);

    gfx::RectF element_bounds;
    base::i18n::TextDirection text_direction =
        base::i18n::TextDirection::UNKNOWN_DIRECTION;
    std::vector<Suggestion> suggestions;
    AutofillSuggestionTriggerSource trigger_source{
        AutofillSuggestionTriggerSource::kUnspecified};
  };

  // Callback to run after local credit card save or local CVC save is offered.
  // Sends whether the prompt was accepted, declined, or ignored in
  // |user_decision|.
  typedef base::OnceCallback<void(SaveCardOfferUserDecision user_decision)>
      LocalSaveCardPromptCallback;

  // Callback to run after upload credit card save is offered. Sends whether the
  // prompt was accepted, declined, or ignored in |user_decision|, and
  // additional |user_provided_card_details| if applicable.
  typedef base::OnceCallback<void(
      SaveCardOfferUserDecision user_decision,
      const UserProvidedCardDetails& user_provided_card_details)>
      UploadSaveCardPromptCallback;

  typedef base::OnceCallback<void(const CreditCard&)> CreditCardScanCallback;

  // Callback to run if user presses the Save button in the migration dialog.
  // Will pass a vector of GUIDs of cards that the user selected to upload to
  // LocalCardMigrationManager.
  typedef base::OnceCallback<void(const std::vector<std::string>&)>
      LocalCardMigrationCallback;

  // Callback to run if the user presses the trash can button in the
  // action-required dialog. Will pass to LocalCardMigrationManager a
  // string of GUID of the card that the user selected to delete from local
  // storage.
  typedef base::RepeatingCallback<void(const std::string&)>
      MigrationDeleteCardCallback;

  // Callback to run after local IBAN save is offered. The callback runs with
  // `user_decision` indicating whether the prompt was accepted, declined,
  // or ignored. `nickname` is optionally provided by the user when IBAN local
  // save is offered, and can be nullopt.
  using LocalSaveIbanPromptCallback =
      base::OnceCallback<void(SaveIbanOfferUserDecision user_decision,
                              const absl::optional<std::u16string>& nickname)>;

  // Callback to run if the OK button or the cancel button in a
  // Webauthn dialog is clicked.
  typedef base::RepeatingCallback<void(WebauthnDialogCallbackType)>
      WebauthnDialogCallback;

  using AddressProfileSavePromptCallback =
      base::OnceCallback<void(SaveAddressProfileOfferUserDecision,
                              AutofillProfile profile)>;

#if !defined(TOOLKIT_QT)
  ~AutofillClient() override = default;
#endif

  // Returns the channel for the installation. In branded builds, this will be
  // version_info::Channel::{STABLE,BETA,DEV,CANARY}. In unbranded builds, or
  // in branded builds when the channel cannot be determined, this will be
  // version_info::Channel::UNKNOWN.
  virtual version_info::Channel GetChannel() const;

  // Returns whether the user is currently operating in an incognito context.
  virtual bool IsOffTheRecord() = 0;

  // Returns the URL loader factory associated with this driver.
  virtual scoped_refptr<network::SharedURLLoaderFactory>
  GetURLLoaderFactory() = 0;

  // Returns the AutofillDownloadManager for communication with the Autofill
  // crowdsourcing server.
  virtual AutofillDownloadManager* GetDownloadManager();

  // Gets the PersonalDataManager instance associated with the original Chrome
  // profile.
  // To distinguish between (non-)incognito mode when deciding to persist data,
  // use the client's `IsOffTheRecord()` function.
  virtual PersonalDataManager* GetPersonalDataManager() = 0;
  const PersonalDataManager* GetPersonalDataManager() const;

  // Gets the AutofillOptimizationGuide instance associated with the client.
  virtual AutofillOptimizationGuide* GetAutofillOptimizationGuide() const;

  // Gets the AutofillModelHandler instance for autofill machine learning
  // predictions associated with the client.
  virtual AutofillMlPredictionModelHandler*
  GetAutofillMlPredictionModelHandler();

  // Gets the AutocompleteHistoryManager instance associated with the client.
  virtual AutocompleteHistoryManager* GetAutocompleteHistoryManager() = 0;

  // Gets the IbanManager instance associated with the client.
  virtual IbanManager* GetIbanManager();

#if !defined(TOOLKIT_QT)
  // When the enterprise plus address feature is supported, gets the
  // KeyedService that manages that data.
  virtual plus_addresses::PlusAddressService* GetPlusAddressService();

  // Orchestrates UI for enterprise plus address creation; no-op except on
  // supported platforms.
  virtual void OfferPlusAddressCreation(
      const url::Origin& main_frame_origin,
      plus_addresses::PlusAddressCallback callback);

  // Gets the MerchantPromoCodeManager instance associated with the
  // client (can be null for unsupported platforms).
  virtual MerchantPromoCodeManager* GetMerchantPromoCodeManager();

  // Can be null on unsupported platforms.
  virtual CreditCardCvcAuthenticator* GetCvcAuthenticator();
  virtual CreditCardOtpAuthenticator* GetOtpAuthenticator();

  // Creates and returns a SingleFieldFormFillRouter using the
  // AutocompleteHistoryManager, IbanManager and MerchantPromoCodeManager
  // instances associated with the client.
  std::unique_ptr<SingleFieldFormFillRouter> CreateSingleFieldFormFillRouter();
#endif  // !defined(TOOLKIT_QT)

  // Gets the preferences associated with the client.
  virtual PrefService* GetPrefs() = 0;
  virtual const PrefService* GetPrefs() const = 0;

#if !defined(TOOLKIT_QT)
  // Gets the sync service associated with the client.
  virtual syncer::SyncService* GetSyncService() = 0;

  // Gets the IdentityManager associated with the client.
  virtual signin::IdentityManager* GetIdentityManager() = 0;

  // Gets the FormDataImporter instance owned by the client.
  virtual FormDataImporter* GetFormDataImporter() = 0;

  // Gets the payments::PaymentsClient instance owned by the client.
  virtual payments::PaymentsClient* GetPaymentsClient() = 0;

  // Gets the StrikeDatabase associated with the client. Note: Nullptr may be
  // returned so check before use.
  // TODO(crbug.com/1472094): Make sure all strike database usages check for
  // the nullptr.
  virtual StrikeDatabase* GetStrikeDatabase() = 0;

  // Gets the UKM service associated with this client (for metrics).
  virtual ukm::UkmRecorder* GetUkmRecorder() = 0;

  // Gets the UKM source id associated with this client (for metrics).
  virtual ukm::SourceId GetUkmSourceId() = 0;

  // Gets an AddressNormalizer instance (can be null).
  virtual AddressNormalizer* GetAddressNormalizer() = 0;

  // Gets an AutofillOfferManager instance (can be null for unsupported
  // platforms).
  virtual AutofillOfferManager* GetAutofillOfferManager();

  // Returns the last committed url of the primary main frame.
  virtual const GURL& GetLastCommittedPrimaryMainFrameURL() const = 0;

  // Returns the last committed origin of the primary main frame.
  virtual url::Origin GetLastCommittedPrimaryMainFrameOrigin() const = 0;

  // Gets the security level used for recording histograms for the current
  // context if possible, SECURITY_LEVEL_COUNT otherwise.
  virtual security_state::SecurityLevel GetSecurityLevelForUmaHistograms() = 0;

  // Returns the language state, if available.
  virtual const translate::LanguageState* GetLanguageState() = 0;

  // Returns the translate driver, if available, which is used to observe the
  // page language for language-dependent heuristics.
  virtual translate::TranslateDriver* GetTranslateDriver() = 0;

  // Retrieves the country code of the user from Chrome variation service.
  // If the variation service is not available, return an empty string.
  virtual GeoIpCountryCode GetVariationConfigCountryCode() const;

  // Returns the profile type of the session.
  virtual profile_metrics::BrowserProfileType GetProfileType() const;

  // Gets a FastCheckoutClient instance (can be null for unsupported platforms).
  virtual FastCheckoutClient* GetFastCheckoutClient();

#if !BUILDFLAG(IS_IOS)
  // Creates the appropriate implementation of InternalAuthenticator. May be
  // null for platforms that don't support this, in which case standard CVC
  // authentication will be used instead.
  virtual std::unique_ptr<webauthn::InternalAuthenticator>
  CreateCreditCardInternalAuthenticator(AutofillDriver* driver);
#endif

  // Causes the Autofill settings UI to be shown.
  virtual void ShowAutofillSettings(PopupType popup_type) = 0;

  // Show the OTP unmask dialog to accept user-input OTP value.
  virtual void ShowCardUnmaskOtpInputDialog(
      const CardUnmaskChallengeOption& challenge_option,
      base::WeakPtr<OtpUnmaskDelegate> delegate);

  // Invoked when we receive the server response of the OTP unmask request.
  virtual void OnUnmaskOtpVerificationResult(OtpUnmaskResult unmask_result);

  // A user has attempted to use a masked card. Prompt them for further
  // information to proceed.
  virtual void ShowUnmaskPrompt(
      const CreditCard& card,
      const CardUnmaskPromptOptions& card_unmask_prompt_options,
      base::WeakPtr<CardUnmaskDelegate> delegate) = 0;
  virtual void OnUnmaskVerificationResult(PaymentsRpcResult result) = 0;

  // Shows a dialog for the user to choose/confirm the authentication
  // to use in card unmasking.
  virtual void ShowUnmaskAuthenticatorSelectionDialog(
      const std::vector<CardUnmaskChallengeOption>& challenge_options,
      base::OnceCallback<void(const std::string&)>
          confirm_unmask_challenge_option_callback,
      base::OnceClosure cancel_unmasking_closure);
  // This should be invoked upon server accepting the authentication method, in
  // which case, we dismiss the selection dialog to open the authentication
  // dialog. |server_success| dictates whether we received a success response
  // from the server, with true representing success and false representing
  // failure. A successful server response means that the issuer has sent an OTP
  // and we can move on to the next portion of this flow.
  virtual void DismissUnmaskAuthenticatorSelectionDialog(bool server_success);

  // Returns a pointer to a VirtualCardEnrollmentManager that is owned by
  // AutofillClient. VirtualCardEnrollmentManager is used for virtual card
  // enroll and unenroll related flows. This function may return a nullptr on
  // some platforms.
  virtual VirtualCardEnrollmentManager* GetVirtualCardEnrollmentManager();

  // Shows a dialog for the user to enroll in a virtual card.
  virtual void ShowVirtualCardEnrollDialog(
      const VirtualCardEnrollmentFields& virtual_card_enrollment_fields,
      base::OnceClosure accept_virtual_card_callback,
      base::OnceClosure decline_virtual_card_callback);

  // Gets or creates a payments autofill mandatory re-auth manager. This will be
  // used to handle payments mandatory re-auth related flows.
  virtual payments::MandatoryReauthManager*
  GetOrCreatePaymentsMandatoryReauthManager();

  // Prompt the user to enable mandatory reauthentication for payment method
  // autofill. When enabled, the user will be asked to authenticate using
  // biometrics or device unlock before filling in payment method information.
  virtual void ShowMandatoryReauthOptInPrompt(
      base::OnceClosure accept_mandatory_reauth_callback,
      base::OnceClosure cancel_mandatory_reauth_callback,
      base::RepeatingClosure close_mandatory_reauth_callback);

  // Should only be called when we are sure re-showing the bubble will display a
  // confirmation bubble. If the most recent bubble was an opt-in bubble and it
  // was accepted, this will display the re-auth opt-in confirmation bubble.
  virtual void ShowMandatoryReauthOptInConfirmation();

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
  // Hides the virtual card enroll bubble and icon if it is visible.
  virtual void HideVirtualCardEnrollBubbleAndIconIfVisible();

  // Returns the list of allowed merchants and BIN ranges for virtual cards.
  virtual std::vector<std::string> GetAllowedMerchantsForVirtualCards() = 0;
  virtual std::vector<std::string> GetAllowedBinRangesForVirtualCards() = 0;

  // Runs |show_migration_dialog_closure| if the user accepts the card migration
  // offer. This causes the card migration dialog to be shown.
  virtual void ShowLocalCardMigrationDialog(
      base::OnceClosure show_migration_dialog_closure) = 0;

  // Shows a dialog with the given |legal_message_lines| and the |user_email|.
  // Runs |start_migrating_cards_callback| if the user would like the selected
  // cards in the |migratable_credit_cards| to be uploaded to cloud.
  virtual void ConfirmMigrateLocalCardToCloud(
      const LegalMessageLines& legal_message_lines,
      const std::string& user_email,
      const std::vector<MigratableCreditCard>& migratable_credit_cards,
      LocalCardMigrationCallback start_migrating_cards_callback) = 0;

  // Will show a dialog containing a error message if |has_server_error|
  // is true, or the migration results for cards in
  // |migratable_credit_cards| otherwise. If migration succeeds the dialog will
  // contain a |tip_message|. |migratable_credit_cards| will be used when
  // constructing the dialog. The dialog is invoked when the migration process
  // is finished. Runs |delete_local_card_callback| if the user chose to delete
  // one invalid card from local storage.
  virtual void ShowLocalCardMigrationResults(
      const bool has_server_error,
      const std::u16string& tip_message,
      const std::vector<MigratableCreditCard>& migratable_credit_cards,
      MigrationDeleteCardCallback delete_local_card_callback) = 0;

  // Runs `callback` once the user makes a decision with respect to the
  // offer-to-save prompt. On desktop, shows the offer-to-save bubble if
  // `should_show_prompt` is true; otherwise only shows the omnibox icon.
  virtual void ConfirmSaveIbanLocally(const Iban& iban,
                                      bool should_show_prompt,
                                      LocalSaveIbanPromptCallback callback) = 0;

  // TODO(crbug.com/991037): Find a way to merge these two functions. Shouldn't
  // use WebauthnDialogState as that state is a purely UI state (should not be
  // accessible for managers?), and some of the states |KInactive| may be
  // confusing here. Do we want to add another Enum?

  // Will show a dialog offering the option to use device's platform
  // authenticator in the future instead of CVC to verify the card being
  // unmasked. Runs |offer_dialog_callback| if the OK button or the cancel
  // button in the dialog is clicked.
  virtual void ShowWebauthnOfferDialog(
      WebauthnDialogCallback offer_dialog_callback) = 0;

  // Will show a dialog indicating the card verification is in progress. It is
  // shown after verification starts only if the WebAuthn is enabled.
  virtual void ShowWebauthnVerifyPendingDialog(
      WebauthnDialogCallback verify_pending_dialog_callback) = 0;

  // Will update the WebAuthn dialog content when there is an error fetching the
  // challenge.
  virtual void UpdateWebauthnOfferDialogWithError() = 0;

  // Will close the current visible WebAuthn dialog. Returns true if dialog was
  // visible and has been closed.
  virtual bool CloseWebauthnDialog() = 0;

  // Shows the dialog including all credit cards that are available to be used
  // as a virtual card. |candidates| must not be empty and has at least one
  // card. Runs |callback| when a card is selected.
  virtual void OfferVirtualCardOptions(
      const std::vector<CreditCard*>& candidates,
      base::OnceCallback<void(const std::string&)> callback) = 0;

#else  // BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS)
  // Display the cardholder name fix flow prompt and run the |callback| if
  // the card should be uploaded to payments with updated name from the user.
  virtual void ConfirmAccountNameFixFlow(
      base::OnceCallback<void(const std::u16string&)> callback) = 0;

  // Display the expiration date fix flow prompt with the |card| details
  // and run the |callback| if the card should be uploaded to payments with
  // updated expiration date from the user.
  virtual void ConfirmExpirationDateFixFlow(
      const CreditCard& card,
      base::OnceCallback<void(const std::u16string&, const std::u16string&)>
          callback) = 0;
#endif

  // Runs |callback| once the user makes a decision with respect to the
  // offer-to-save prompt. This includes both the save local card prompt and the
  // save CVC for a local card prompt. On desktop, shows the offer-to-save
  // bubble if |options.show_prompt| is true; otherwise only shows the omnibox
  // icon. On mobile, shows the offer-to-save infobar if |options.show_prompt|
  // is true; otherwise does not offer to save at all.
  virtual void ConfirmSaveCreditCardLocally(
      const CreditCard& card,
      AutofillClient::SaveCreditCardOptions options,
      LocalSaveCardPromptCallback callback) = 0;

  // Runs |callback| once the user makes a decision with respect to the
  // offer-to-save prompt. Displays the contents of |legal_message_lines|
  // to the user. Displays a cardholder name textfield in the bubble if
  // |options.should_request_name_from_user| is true. Displays
  // a pair of expiration date dropdowns in the bubble if
  // |should_request_expiration_date_from_user| is true. On desktop, shows the
  // offer-to-save bubble if |options.show_prompt| is true;
  // otherwise only shows the omnibox icon. On mobile, shows the offer-to-save
  // infobar if |options.show_prompt| is true; otherwise does
  // not offer to save at all.
  virtual void ConfirmSaveCreditCardToCloud(
      const CreditCard& card,
      const LegalMessageLines& legal_message_lines,
      SaveCreditCardOptions options,
      UploadSaveCardPromptCallback callback) = 0;

  // Called after credit card upload is finished. Will show upload result to
  // users. |card_saved| indicates if the card is successfully saved.
  // TODO(crbug.com/932818): This function is overridden in iOS codebase.
  // Ideally should remove it if iOS is not using it to do anything.
  virtual void CreditCardUploadCompleted(bool card_saved) = 0;

  // Will show an infobar to get user consent for Credit Card assistive filling.
  // Will run |callback| on success.
  virtual void ConfirmCreditCardFillAssist(const CreditCard& card,
                                           base::OnceClosure callback) = 0;

  // Show an edit address profile dialog, giving the user an option to alter
  // autofill profile data.
  virtual void ShowEditAddressProfileDialog(const AutofillProfile& profile) = 0;

  // Show a delete address profile dialog asking if users want to proceed with
  // deletion.
  virtual void ShowDeleteAddressProfileDialog() = 0;

  // Shows the offer-to-save (or update) address profile bubble. If
  // `original_profile` is nullptr, this renders a save prompt. Otherwise, it
  // renders an update prompt where `original_profile` is the address profile
  // that will be updated if the user accepts the update prompt. Runs `callback`
  // once the user makes a decision with respect to the offer-to-save prompt.
  // `options` carries extra configuration options for the prompt.
  virtual void ConfirmSaveAddressProfile(
      const AutofillProfile& profile,
      const AutofillProfile* original_profile,
      AutofillClient::SaveAddressProfilePromptOptions options,
      AddressProfileSavePromptCallback callback) = 0;

  // Returns true if both the platform and the device support scanning credit
  // cards. Should be called before ScanCreditCard().
  virtual bool HasCreditCardScanFeature() = 0;

  // Shows the user interface for scanning a credit card. Invokes the |callback|
  // when a credit card is scanned successfully. Should be called only if
  // HasCreditCardScanFeature() returns true.
  virtual void ScanCreditCard(CreditCardScanCallback callback) = 0;

  // Returns true if the Touch To Fill feature is both supported by platform and
  // enabled. Should be called before |ShowTouchToFillCreditCard| or
  // |HideTouchToFillCreditCard|.
  virtual bool IsTouchToFillCreditCardSupported() = 0;

  // Shows the Touch To Fill surface for filling credit card information, if
  // possible, and returns |true| on success. |delegate| will be notified of
  // events. Should be called only if |IsTouchToFillCreditCardSupported|
  // returns true.
  virtual bool ShowTouchToFillCreditCard(
      base::WeakPtr<TouchToFillDelegate> delegate,
      base::span<const autofill::CreditCard> cards_to_suggest) = 0;

  // Hides the Touch To Fill surface for filling credit card information
  // if one is currently shown. Should be called only if
  // |IsTouchToFillCreditCardSupported| returns true.
  virtual void HideTouchToFillCreditCard() = 0;
#endif  // !defined(TOOLKIT_QT)

  // Shows an Autofill popup with the given |values|, |labels|, |icons|, and
  // |identifiers| for the element at |element_bounds|. |delegate| will be
  // notified of popup events.
  virtual void ShowAutofillPopup(
      const PopupOpenArgs& open_args,
      base::WeakPtr<AutofillPopupDelegate> delegate) = 0;

  // Update the data list values shown by the Autofill popup, if visible.
  virtual void UpdateAutofillPopupDataListValues(
      const std::vector<std::u16string>& values,
      const std::vector<std::u16string>& labels) = 0;

  // Informs the client that the popup needs to be kept alive. Call before
  // |UpdatePopup| to update the open popup in-place.
  virtual void PinPopupView() = 0;

  // The returned arguments allow to reopen the dropdown with
  // `ShowAutofillPopup()` even if the controller is destroyed temporarily.
  // This function ensures that the element's bounds are transformed back to the
  // screen space-independent bounds.
  // The suggestion trigger source of the existing popup is not reused, but
  // replaced with `trigger_source`. This is because it should indicate the
  // reason for reopening the popup. Reusing the existing trigger source can
  // have unwanted implications such as re-auto-selecting the first suggestion
  // in the `AutofillSuggestionTriggerSource::kTextFieldDidReceiveKeyDown` case.
  // Note that the password manager doesn't distinguish between trigger sources.
  virtual PopupOpenArgs GetReopenPopupArgs(
      AutofillSuggestionTriggerSource trigger_source) const = 0;

  // Returns (not elided) suggestions currently held by the UI.
  virtual std::vector<Suggestion> GetPopupSuggestions() const = 0;

  // Updates the popup contents with the newly given suggestions.
  // `trigger_source` indicates the reason for updating the popup. (However, the
  // password manager makes no distinction).
  virtual void UpdatePopup(const std::vector<Suggestion>& suggestions,
                           PopupType popup_type,
                           AutofillSuggestionTriggerSource trigger_source) = 0;

  // Hide the Autofill popup if one is currently showing.
  virtual void HideAutofillPopup(PopupHidingReason reason) = 0;

#if !defined(TOOLKIT_QT)
  // TODO(crbug.com/1093057): Rename all the "domain" in this flow to origin.
  //                          The server is passing down full origin of the
  //                          urls. "Domain" is no longer accurate.
  // Notifies the client to update the offer notification when the `offer` is
  // available. `options` carries extra configuration options for the offer
  // notification.
  virtual void UpdateOfferNotification(const AutofillOfferData* offer,
                                       const OfferNotificationOptions& options);

  // Dismiss any visible offer notification on the current tab.
  virtual void DismissOfferNotification();

  // Called when the virtual card has been fetched successfully. Uses the
  // necessary information in `options` to show the manual fallback bubble.
  virtual void OnVirtualCardDataAvailable(
      const VirtualCardManualFallbackBubbleOptions& options);

  // Shows an error dialog when card retrieval errors happen. The type of error
  // dialog that is shown will match the `type` in `context`. If the
  // `server_returned_title` and `server_returned_description` in `context` are
  // both set, the error dialog that is displayed will have these fields
  // displayed for the title and description, respectively.
  virtual void ShowAutofillErrorDialog(
      const AutofillErrorDialogContext& context);

  // Show/dismiss the progress dialog which contains a throbber and a text
  // message indicating that something is in progress.
  virtual void ShowAutofillProgressDialog(
      AutofillProgressDialogType autofill_progress_dialog_type,
      base::OnceClosure cancel_callback);
  virtual void CloseAutofillProgressDialog(
      bool show_confirmation_before_closing,
      base::OnceClosure no_interactive_authentication_callback =
          base::OnceClosure());
#endif  // !defined(TOOLKIT_QT)

  // Whether the Autocomplete feature of Autofill should be enabled.
  virtual bool IsAutocompleteEnabled() const = 0;

  // Returns whether password management is enabled as per the user preferences.
  virtual bool IsPasswordManagerEnabled() = 0;

  // Pass the form structures to the password manager to choose correct username
  // and to the password generation manager to detect account creation forms.
  //
  // TODO(crbug.com/1466435): Do not use or rely on this function anymore.
  virtual void PropagateAutofillPredictionsDeprecated(
      AutofillDriver* driver,
      const std::vector<FormStructure*>& forms) = 0;

#if !defined(TOOLKIT_QT)
  // Inform the client that the form has been filled.
  virtual void DidFillOrPreviewForm(
      mojom::AutofillActionPersistence action_persistence,
      AutofillTriggerSource trigger_source,
      bool is_refill) = 0;

  // Inform the client that the field has been filled.
  virtual void DidFillOrPreviewField(
      const std::u16string& autofilled_value,
      const std::u16string& profile_full_name) = 0;

  // If the context is secure.
  virtual bool IsContextSecure() const = 0;

#endif  // !defined(TOOLKIT_QT)

  // Returns a LogManager instance. May be null for platforms that don't support
  // this.
  virtual LogManager* GetLogManager() const;

#if !defined(TOOLKIT_QT)
  virtual const AutofillAblationStudy& GetAblationStudy() const;

#if BUILDFLAG(IS_IOS)
  // Checks whether `field_id` is the last field that for which
  // AutofillAgent::queryAutofillForForm() was called. See crbug.com/1097015.
  virtual bool IsLastQueriedField(FieldGlobalId field_id) = 0;
#endif

  // Navigates to |url| in a new tab. |url| links to the promo code offer
  // details page for the offers in a promo code suggestions popup. Every offer
  // in a promo code suggestions popup links to the same offer details page.
  virtual void OpenPromoCodeOfferDetailsURL(const GURL& url) = 0;

  // Updates and returns the current form interactions flow id. This is used as
  // an approximation for keeping track of the number of user interactions with
  // related forms for logging. Example implementation: the flow id is set to a
  // GUID on the first call. That same GUID will be returned for consecutive
  // calls in the next 20 minutes. Afterwards a new GUID is set and the pattern
  // repeated.
  virtual FormInteractionsFlowId GetCurrentFormInteractionsFlowId() = 0;

  // Returns a pointer to a DeviceAuthenticator. Might be nullptr if the given
  // platform is not supported.
  virtual scoped_refptr<device_reauth::DeviceAuthenticator>
  GetDeviceAuthenticator() const;
#endif  // !defined(TOOLKIT_QT)
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOFILL_CLIENT_H_
