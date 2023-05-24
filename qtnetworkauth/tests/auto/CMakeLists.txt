# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from auto.pro.

# add_subdirectory(cmake) special case remove
add_subdirectory(oauth1)
add_subdirectory(oauth2)
add_subdirectory(oauth1signature)
add_subdirectory(oauthhttpserverreplyhandler)
if(QT_FEATURE_private_tests)
    add_subdirectory(abstractoauth)
endif()
