
ifeq ($(QT_MAC_SDK_NO_VERSION_CHECK),)
    CHECK_SDK_COMMAND = /usr/bin/xcrun --sdk $(EXPORT_QMAKE_MAC_SDK) -show-sdk-version 2>/dev/null
    CURRENT_MAC_SDK_VERSION := $(shell DEVELOPER_DIR=$(EXPORT_QMAKE_XCODE_DEVELOPER_PATH) $(CHECK_SDK_COMMAND))
    ifneq ($(CURRENT_MAC_SDK_VERSION),$(EXPORT_QMAKE_MAC_SDK_VERSION))
        # We don't want to complain about out of date SDK unless the target needs to be remade.
        # This covers use-cases such as running 'make check' after moving the build to a
        # computer without Xcode or with a different Xcode version.
        TARGET_UP_TO_DATE := $(shell QT_MAC_SDK_NO_VERSION_CHECK=1 $(MAKE) --question $(QMAKE_TARGET) && echo 1 || echo 0)
        ifeq ($(TARGET_UP_TO_DATE),0)
            ifneq ($(findstring missing DEVELOPER_DIR path,$(CURRENT_MAC_SDK_VERSION)),)
                $(info The developer dir $(EXPORT_QMAKE_XCODE_DEVELOPER_PATH) is no longer valid.)
            else ifneq ($(findstring SDK "$(EXPORT_QMAKE_MAC_SDK)" cannot be located,$(CURRENT_MAC_SDK_VERSION)),)
                $(info The developer dir $(EXPORT_QMAKE_XCODE_DEVELOPER_PATH) no longer contains the $(EXPORT_QMAKE_MAC_SDK_VERSION) platform SDK.)
            else ifneq ($(CURRENT_MAC_SDK_VERSION),)
                $(info The $(EXPORT_QMAKE_MAC_SDK) platform SDK has been changed from version $(EXPORT_QMAKE_MAC_SDK_VERSION) to version $(CURRENT_MAC_SDK_VERSION).)
            else
                $(info Unknown error resolving current platform SDK version.)
            endif
            $(info This requires a fresh build of your project. Please wipe the build directory)
            ifneq ($(EXPORT__QMAKE_STASH_),)
                $(info including the qmake cache in $(EXPORT__QMAKE_STASH_))
            endif
            $(error ^)
        endif
    endif
endif
