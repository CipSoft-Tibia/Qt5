# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(QT_ALLOW_DOWNLOAD)
    add_subdirectory(conformance)
else()
    message("Skip building the QtProtobuf conformance test. The test needs to download files from"
        " web. Please set QT_ALLOW_DOWNLOAD flag to ON, to enable the test.")
endif()
