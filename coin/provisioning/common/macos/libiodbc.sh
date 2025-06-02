#!/usr/bin/env bash
# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

# Install libiodbc

set -ex

# shellcheck source=../unix/SetEnvVar.sh
source "${BASH_SOURCE%/*}/../unix/SetEnvVar.sh"

brew install --formula "${BASH_SOURCE%/*}/libiodbc.rb" "$@"

read -r -a arr <<< "$(brew list --versions libiodbc)"
version=${arr[1]}

SetEnvVar "ODBC_ROOT" "$(brew --prefix libiodbc)"

echo "libiodbc = $version" >> ~/versions.txt
