#!/usr/bin/env bash

# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

set -ex

# shellcheck source=../common/linux/disable_selinux.sh
source "${BASH_SOURCE%/*}/../common/linux/disable_selinux.sh"
