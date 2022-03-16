include(common.pri)
load(functions)

# Reuse the cached sdk version value from mac/sdk.prf if available
# otherwise query for it.
QMAKE_MAC_SDK_VERSION = $$eval(QMAKE_MAC_SDK.$${QMAKE_MAC_SDK}.SDKVersion)
isEmpty(QMAKE_MAC_SDK_VERSION) {
     QMAKE_MAC_SDK_VERSION = $$system("/usr/bin/xcodebuild -sdk $${QMAKE_MAC_SDK} -version SDKVersion 2>/dev/null")
     isEmpty(QMAKE_MAC_SDK_VERSION): error("Could not resolve SDK version for \'$${QMAKE_MAC_SDK}\'")
}

# chromium/build/mac/find_sdk.py expects the SDK version (mac_sdk_min) in Major.Minor format.
# If Patch version is provided it fails with "Exception: No Major.Minor.Patch+ SDK found"
QMAKE_MAC_SDK_VERSION_MAJOR_MINOR = $$section(QMAKE_MAC_SDK_VERSION, ".", 0, 1)

QMAKE_CLANG_DIR = "/usr"
QMAKE_CLANG_PATH = $$eval(QMAKE_MAC_SDK.macx-clang.$${QMAKE_MAC_SDK}.QMAKE_CXX)
!isEmpty(QMAKE_CLANG_PATH) {
    clang_dir = $$clean_path("$$dirname(QMAKE_CLANG_PATH)/../")
    exists($$clang_dir): QMAKE_CLANG_DIR = $$clang_dir
}

QMAKE_CLANG_PATH = "$${QMAKE_CLANG_DIR}/bin/clang++"
message("Using clang++ from $${QMAKE_CLANG_PATH}")
system("$${QMAKE_CLANG_PATH} --version")


gn_args += \
    is_clang=true \
    use_sysroot=false \
    clang_base_path=\"$${QMAKE_CLANG_DIR}\" \
    clang_use_chrome_plugins=false \
    mac_deployment_target=\"$${QMAKE_MACOSX_DEPLOYMENT_TARGET}\" \
    mac_sdk_min=\"$${QMAKE_MAC_SDK_VERSION_MAJOR_MINOR}\" \
    mac_views_browser=false \
    toolkit_views=false \
    use_external_popup_menu=false

qtConfig(webengine-spellchecker) {
    qtConfig(webengine-native-spellchecker): gn_args += use_browser_spellchecker=true
    else: gn_args += use_browser_spellchecker=false
} else {
    gn_args += use_browser_spellchecker=false
}
