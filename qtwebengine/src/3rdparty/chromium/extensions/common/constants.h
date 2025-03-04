// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_CONSTANTS_H_
#define EXTENSIONS_COMMON_CONSTANTS_H_

#include <cstddef>
#include <cstdint>

#include "base/files/file_path.h"
#include "base/strings/string_piece_forward.h"
#include "build/chromeos_buildflags.h"
#include "extensions/common/extensions_export.h"

namespace extensions {

// Scheme we serve extension content from.
constexpr char kExtensionScheme[] = "chrome-extension";

// The name of the manifest inside an extension.
constexpr base::FilePath::CharType kManifestFilename[] =
    FILE_PATH_LITERAL("manifest.json");

// The name of the differential fingerprint file inside an extension.
constexpr base::FilePath::CharType kDifferentialFingerprintFilename[] =
    FILE_PATH_LITERAL("manifest.fingerprint");

// The name of locale folder inside an extension.
constexpr base::FilePath::CharType kLocaleFolder[] =
    FILE_PATH_LITERAL("_locales");

// The name of the messages file inside an extension.
constexpr base::FilePath::CharType kMessagesFilename[] =
    FILE_PATH_LITERAL("messages.json");

// The name of the gzipped messages file inside an extension.
constexpr base::FilePath::CharType kGzippedMessagesFilename[] =
    FILE_PATH_LITERAL("messages.json.gz");

// The base directory for subdirectories with platform-specific code.
constexpr base::FilePath::CharType kPlatformSpecificFolder[] =
    FILE_PATH_LITERAL("_platform_specific");

// A directory reserved for metadata, generated either by the webstore
// or chrome.
constexpr base::FilePath::CharType kMetadataFolder[] =
    FILE_PATH_LITERAL("_metadata");

// Name of the verified contents file within the metadata folder.
constexpr base::FilePath::CharType kVerifiedContentsFilename[] =
    FILE_PATH_LITERAL("verified_contents.json");

// Name of the computed hashes file within the metadata folder.
constexpr base::FilePath::CharType kComputedHashesFilename[] =
    FILE_PATH_LITERAL("computed_hashes.json");

// Name of the indexed ruleset directory for the Declarative Net Request API.
constexpr base::FilePath::CharType kIndexedRulesetDirectory[] =
    FILE_PATH_LITERAL("generated_indexed_rulesets");

// The name of the directory inside the profile where extensions are
// installed to.
constexpr char kInstallDirectoryName[] = "Extensions";

// The name of the directory inside the profile where unpacked (e.g. from .zip
// file) extensions are installed to.
constexpr char kUnpackedInstallDirectoryName[] = "UnpackedExtensions";

// The name of a temporary directory to install an extension into for
// validation before finalizing install.
constexpr char kTempExtensionName[] = "CRX_INSTALL";

// The file to write our decoded message catalogs to, relative to the
// extension_path.
constexpr char kDecodedMessageCatalogsFilename[] = "DECODED_MESSAGE_CATALOGS";

// The filename to use for a background page generated from
// background.scripts.
constexpr char kGeneratedBackgroundPageFilename[] =
    "_generated_background_page.html";

// The URL piece between the extension ID and favicon URL.
constexpr char kFaviconSourcePath[] = "_favicon";

// Path to imported modules.
constexpr char kModulesDir[] = "_modules";

// The file extension (.crx) for extensions.
constexpr base::FilePath::CharType kExtensionFileExtension[] =
    FILE_PATH_LITERAL(".crx");

// The file extension (.pem) for private key files.
constexpr base::FilePath::CharType kExtensionKeyFileExtension[] =
    FILE_PATH_LITERAL(".pem");

// Default frequency for auto updates, if turned on (5 hours).
constexpr int kDefaultUpdateFrequencySeconds = 60 * 60 * 5;

// The name of the directory inside the profile where per-app local settings
// are stored.
constexpr base::FilePath::CharType kLocalAppSettingsDirectoryName[] =
    FILE_PATH_LITERAL("Local App Settings");

// The name of the directory inside the profile where per-extension local
// settings are stored.
constexpr base::FilePath::CharType kLocalExtensionSettingsDirectoryName[] =

    FILE_PATH_LITERAL("Local Extension Settings");

// The name of the directory inside the profile where per-app synced settings
// are stored.
constexpr base::FilePath::CharType kSyncAppSettingsDirectoryName[] =
    FILE_PATH_LITERAL("Sync App Settings");

// The name of the directory inside the profile where per-extension synced
// settings are stored.
constexpr base::FilePath::CharType kSyncExtensionSettingsDirectoryName[] =
    FILE_PATH_LITERAL("Sync Extension Settings");

// The name of the directory inside the profile where per-extension persistent
// managed settings are stored.
constexpr base::FilePath::CharType kManagedSettingsDirectoryName[] =
    FILE_PATH_LITERAL("Managed Extension Settings");

// The name of the database inside the profile where chrome-internal
// extension state resides.
constexpr base::FilePath::CharType kStateStoreName[] =
    FILE_PATH_LITERAL("Extension State");

// The name of the database inside the profile where declarative extension
// rules are stored.
constexpr base::FilePath::CharType kRulesStoreName[] =
    FILE_PATH_LITERAL("Extension Rules");

// The name of the database inside the profile where persistent dynamic user
// script metadata is stored.
constexpr base::FilePath::CharType kScriptsStoreName[] =
    FILE_PATH_LITERAL("Extension Scripts");

// Statistics are logged to UMA with these strings as part of histogram name.
// They can all be found under Extensions.Database.Open.<client>. Changing this
// needs to synchronize with histograms.xml, AND will also become incompatible
// with older browsers still reporting the previous values.
constexpr char kSettingsDatabaseUMAClientName[] = "Settings";
constexpr char kRulesDatabaseUMAClientName[] = "Rules";
constexpr char kStateDatabaseUMAClientName[] = "State";
constexpr char kScriptsDatabaseUMAClientName[] = "Scripts";

// Mime type strings
constexpr char kMimeTypeJpeg[] = "image/jpeg";
constexpr char kMimeTypePng[] = "image/png";

// The extension id of the Web Store component application.
constexpr char kWebStoreAppId[] = "ahfgeienlihckogmohjhadlkjgocpleb";

// The key used for signing some pieces of data from the webstore.
EXTENSIONS_EXPORT extern const uint8_t kWebstoreSignaturesPublicKey[];
EXTENSIONS_EXPORT extern const size_t kWebstoreSignaturesPublicKeySize;

// A preference for storing the extension's update URL data.
constexpr char kUpdateURLData[] = "update_url_data";

// Thread identifier for the main renderer thread (as opposed to a service
// worker thread).
// This is the default thread id used for extension event listeners registered
// from a non-service worker context
constexpr int kMainThreadId = 0;

// Enumeration of possible app launch sources.
// This should be kept in sync with LaunchSource in
// extensions/common/api/app_runtime.idl, and GetLaunchSourceEnum() in
// extensions/browser/api/app_runtime/app_runtime_api.cc.
// Note the enumeration is used in UMA histogram so entries
// should not be re-ordered or removed.
enum class AppLaunchSource {
  kSourceNone = 0,
  kSourceUntracked = 1,
  kSourceAppLauncher = 2,
  kSourceNewTabPage = 3,
  kSourceReload = 4,
  kSourceRestart = 5,
  kSourceLoadAndLaunch = 6,
  kSourceCommandLine = 7,
  kSourceFileHandler = 8,
  kSourceUrlHandler = 9,
  kSourceSystemTray = 10,
  kSourceAboutPage = 11,
  kSourceKeyboard = 12,
  kSourceExtensionsPage = 13,
  kSourceManagementApi = 14,
  kSourceEphemeralAppDeprecated = 15,
  kSourceBackground = 16,
  kSourceKiosk = 17,
  kSourceChromeInternal = 18,
  kSourceTest = 19,
  kSourceInstalledNotification = 20,
  kSourceContextMenu = 21,
  kSourceArc = 22,
  kSourceIntentUrl = 23,        // App launch triggered by a URL.
  kSourceRunOnOsLogin = 24,     // App launched during OS login.
  kSourceProtocolHandler = 25,  // App launch via protocol handler.
  kSourceReparenting = 26,      // APP launch via reparenting.
  kSourceAppHomePage = 27,      // App launch from chrome://apps (App Home).

  // Add any new values above this one, and update kMaxValue to the highest
  // enumerator value.
  kMaxValue = kSourceAppHomePage,
};

// This enum is used for the launch type the user wants to use for an
// application.
// Do not remove items or re-order this enum as it is used in preferences
// and histograms.
enum LaunchType {
  LAUNCH_TYPE_INVALID = -1,
  LAUNCH_TYPE_FIRST = 0,
  LAUNCH_TYPE_PINNED = LAUNCH_TYPE_FIRST,
  LAUNCH_TYPE_REGULAR = 1,
  LAUNCH_TYPE_FULLSCREEN = 2,
  LAUNCH_TYPE_WINDOW = 3,
  NUM_LAUNCH_TYPES,

  // Launch an app in the in the way a click on the NTP would,
  // if no user pref were set.  Update this constant to change
  // the default for the NTP and chrome.management.launchApp().
  LAUNCH_TYPE_DEFAULT = LAUNCH_TYPE_REGULAR
};

}  // namespace extensions

namespace extension_misc {

// Matches chrome.tabs.TAB_ID_NONE.
constexpr int kUnknownTabId = -1;

// Matches chrome.windows.WINDOW_ID_NONE.
constexpr int kUnknownWindowId = -1;

// Matches chrome.windows.WINDOW_ID_CURRENT.
constexpr int kCurrentWindowId = -2;

using ExtensionIcons = int;
constexpr ExtensionIcons EXTENSION_ICON_GIGANTOR = 512;
constexpr ExtensionIcons EXTENSION_ICON_EXTRA_LARGE = 256;
constexpr ExtensionIcons EXTENSION_ICON_LARGE = 128;
constexpr ExtensionIcons EXTENSION_ICON_MEDIUM = 48;
constexpr ExtensionIcons EXTENSION_ICON_SMALL = 32;
constexpr ExtensionIcons EXTENSION_ICON_SMALLISH = 24;
constexpr ExtensionIcons EXTENSION_ICON_BITTY = 16;
constexpr ExtensionIcons EXTENSION_ICON_INVALID = 0;

// The extension id of the ChromeVox extension.
constexpr char kChromeVoxExtensionId[] =
#if BUILDFLAG(IS_CHROMEOS)
    // The extension id for the built-in component extension.
    "mndnfokpggljbaajbnioimlmbfngpief";
#else
    // The extension id for the web store extension.
    "kgejglhpjiefppelpmljglcjbhoiplfn";
#endif

// The extension id of the PDF extension.
constexpr char kPdfExtensionId[] = "mhjfbmdgcfjbbpaeojofohoefgiehjai";

// The extension id of the Office Viewer component extension.
constexpr char kQuickOfficeComponentExtensionId[] =
    "bpmcpldpdmajfigpchkicefoigmkfalc";

// The extension id of the Office Viewer extension on the internal webstore.
constexpr char kQuickOfficeInternalExtensionId[] =
    "ehibbfinohgbchlgdbfpikodjaojhccn";

// The extension id of the Office Viewer extension.
constexpr char kQuickOfficeExtensionId[] = "gbkeegbaiigmenfmjfclcdgdpimamgkj";

// The extension id used for testing mimeHandlerPrivate.
constexpr char kMimeHandlerPrivateTestExtensionId[] =
    "oickdpebdnfbgkcaoklfcdhjniefkcji";

// The extension id of the Files Manager application.
constexpr char kFilesManagerAppId[] = "hhaomjibdihmijegdhdafkllkbggdgoj";

// The extension id of the Calculator application.
constexpr char kCalculatorAppId[] = "joodangkbfjnajiiifokapkpmhfnpleo";

// The extension id of the demo Calendar application.
constexpr char kCalendarDemoAppId[] = "fpgfohogebplgnamlafljlcidjedbdeb";

// The extension id of the GMail application.
constexpr char kGmailAppId[] = "pjkljhegncpnkpknbcohdijeoejaedia";

// The extension id of the demo Google Docs application.
constexpr char kGoogleDocsDemoAppId[] = "chdaoodbokekbiiphekbfjdmiodccljl";

// The extension id of the Google Docs PWA.
constexpr char kGoogleDocsPwaAppId[] = "cepkndkdlbllfhpfhledabdcdbidehkd";

// The extension id of the Google Drive application.
constexpr char kGoogleDriveAppId[] = "apdfllckaahabafndbhieahigkjlhalf";

// The extension id of the Google Meet PWA.
constexpr char kGoogleMeetPwaAppId[] = "dkainijpcknoofiakgccliajhbmlbhji";

// The extension id of the demo Google Sheets application.
constexpr char kGoogleSheetsDemoAppId[] = "nifkmgcdokhkjghdlgflonppnefddien";

// The extension id of the Google Sheets PWA.
constexpr char kGoogleSheetsPwaAppId[] = "hcgjdbbnhkmopplfiibmdgghhdhbiidh";

// The extension id of the demo Google Slides application.
constexpr char kGoogleSlidesDemoAppId[] = "hdmobeajeoanbanmdlabnbnlopepchip";

// The extension id of the Google Keep application.
constexpr char kGoogleKeepAppId[] = "hmjkmjkepdijhoojdojkdfohbdgmmhki";

// The extension id of the Youtube application.
constexpr char kYoutubeAppId[] = "blpcfgokakmgnkcojhhkbfbldkacnbeo";

// The extension id of the Youtube PWA.
constexpr char kYoutubePwaAppId[] = "agimnkijcaahngcdmfeangaknmldooml";

// The extension id of the Spotify PWA.
constexpr char kSpotifyAppId[] = "pjibgclleladliembfgfagdaldikeohf";

// The extension id of the BeFunky PWA.
constexpr char kBeFunkyAppId[] = "fjoomcalbeohjbnlcneddljemclcekeg";

// The extension id of the Clipchamp PWA.
constexpr char kClipchampAppId[] = "pfepfhbcedkbjdkanpimmmdjfgoddhkg";

// The extension id of the GeForce NOW PWA.
constexpr char kGeForceNowAppId[] = "egmafekfmcnknbdlbfbhafbllplmjlhn";

// The extension id of the Zoom PWA.
constexpr char kZoomAppId[] = "jldpdkiafafcejhceeincjmlkmibemgj";

// The extension id of the Sumo PWA.
constexpr char kSumoAppId[] = "mfknjekfflbfdchhohffdpkokgfbfmdc";

// The extension id of the Sumo PWA.
constexpr char kAdobeSparkAppId[] = "magefboookdoiehjohjmbjmkepngibhm";

// The extension id of the Google Docs application.
constexpr char kGoogleDocsAppId[] = "aohghmighlieiainnegkcijnfilokake";

// The extension id of the Google Sheets application.
constexpr char kGoogleSheetsAppId[] = "felcaaldnbdncclmgdcncolpebgiejap";

// The extension id of the Google Slides application.
constexpr char kGoogleSlidesAppId[] = "aapocclcgogkmnckokdopfmhonfmgoek";

#if BUILDFLAG(IS_CHROMEOS_ASH)
// The id of the testing extension allowed in the signin profile.
constexpr char kSigninProfileTestExtensionId[] =
    "mecfefiddjlmabpeilblgegnbioikfmp";

// The id of the testing extension allowed in guest mode.
constexpr char kGuestModeTestExtensionId[] = "behllobkkfkfnphdnhnkndlbkcpglgmj";

// Returns true if this app is part of the "system UI". Generally this is UI
// that that on other operating systems would be considered part of the OS,
// for example the file manager.
EXTENSIONS_EXPORT bool IsSystemUIApp(base::StringPiece extension_id);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

#if BUILDFLAG(IS_CHROMEOS)
// The extension id of the default Demo Mode Highlights app.
constexpr char kHighlightsAppId[] = "lpmakjfjcconjeehbidjclhdlpjmfjjj";

// The extension id of the default Demo Mode screensaver app.
constexpr char kScreensaverAppId[] = "mnoijifedipmbjaoekhadjcijipaijjc";

// The extension id of 2022 Demo Mode Highlights app.
constexpr char kNewAttractLoopAppId[] = "igilkdghcdehjdcpndaodgnjgdggiemm";

// The extension id of 2022 Demo Mode screensaver app.
constexpr char kNewHighlightsAppId[] = "enchmnkoajljphdmahljlebfmpkkbnkj";

// Returns true if this app is one of Demo Mode Chrome Apps, including
// attract loop and highlights apps.
EXTENSIONS_EXPORT bool IsDemoModeChromeApp(base::StringPiece extension_id);
#endif  // BUILDFLAG(IS_CHROMEOS)

// True if the id matches any of the QuickOffice extension ids.
EXTENSIONS_EXPORT bool IsQuickOfficeExtension(base::StringPiece extension_id);

// Returns if the app is managed by extension default apps. This is a hardcoded
// list of default apps for Windows/Linux/MacOS platforms that should be
// migrated from extension to web app.
// TODO(https://crbug.com/1257275): remove after deault app migration is done.
// This function is copied from
// chrome/browser/web_applications/extension_status_utils.h.
EXTENSIONS_EXPORT bool IsPreinstalledAppId(base::StringPiece app_id);

// Error message when enterprise policy blocks scripting of webpage.
constexpr char kPolicyBlockedScripting[] =
    "This page cannot be scripted due to an ExtensionsSettings policy.";

// Error message when access to incognito preferences is denied.
constexpr char kIncognitoErrorMessage[] =
    "You do not have permission to access incognito preferences.";

// Error message when setting a pref with "incognito_session_only"
// scope is denied.
constexpr char kIncognitoSessionOnlyErrorMessage[] =
    "You cannot set a preference with scope 'incognito_session_only' when no "
    "incognito window is open.";

// Error message when an invalid color is provided to an API method.
constexpr char kInvalidColorError[] =
    "The color specification could not be parsed.";

// The default block size for hashing used in content verification.
constexpr int kContentVerificationDefaultBlockSize = 4096;

}  // namespace extension_misc

#endif  // EXTENSIONS_COMMON_CONSTANTS_H_
