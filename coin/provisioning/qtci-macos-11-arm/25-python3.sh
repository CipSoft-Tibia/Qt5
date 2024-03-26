#!/usr/bin/env bash
# Copyright (C) 2021 The Qt Company Ltd.
# Copyright (C) 2017 Pelagicore AG
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

# This script installs python3

# shellcheck source=../unix/SetEnvVar.sh
source "${BASH_SOURCE%/*}/../common/unix/SetEnvVar.sh"

brew install --formula ${BASH_SOURCE%/*}/pyenv.rb

pyenv install 3.9.7

/Users/qt/.pyenv/versions/3.9.7/bin/pip3 install --user install virtualenv wheel html5lib

SetEnvVar "PYTHON3_PATH" "/Users/qt/.pyenv/versions/3.9.7/bin/"
SetEnvVar "PIP3_PATH" "/Users/qt/.pyenv/versions/3.9.7/bin/"
# Use 3.9 as a default python
SetEnvVar "PATH" "\$PYTHON3_PATH:\$PATH"

echo "python3 = 3.9.7" >> ~/versions.txt
