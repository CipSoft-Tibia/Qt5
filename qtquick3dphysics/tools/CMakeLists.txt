# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

if(NOT WASM AND NOT IOS AND NOT ANDROID AND NOT QNX AND NOT INTEGRITY AND NOT WEBOS)
    add_subdirectory(cooker)
endif()
