#!/usr/bin/env bash
# Copyright (C) 2018 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

# provides: python development libraries
# version: provided by default Linux distribution repository
# needed to build pyside

set -ex

PROVISIONING_DIR="$(dirname "$0")/../"
. "$PROVISIONING_DIR"/common/unix/common.sourced.sh
. "$PROVISIONING_DIR"/common/unix/DownloadURL.sh


# Selected installation instructions coming from:
# https://raw.githubusercontent.com/linux-on-ibm-z/scripts/master/Python3/build_python3.sh
export PACKAGE_NAME="python"
python2Version="2.7.18"
python3Version="3.8.16"
python2Sha="678d4cf483a1c92efd347ee8e1e79326dc82810b"
python3Sha="d85dbb3774132473d8081dcb158f34a10ccad7a90b96c7e50ea4bb61f5ce4562"


function InstallPython {

    PACKAGE_VERSION=$1
    PACKAGE_SHA=$2

    $CMD_PKG_INSTALL  ncurses zlib-devel libffi-devel

    echo 'Configuration and Installation started'

    #Download Source code
    DownloadURL  \
        http://ci-files01-hki.intra.qt.io/input/python/Python-${PACKAGE_VERSION}.tar.xz  \
        https://www.python.org/ftp/${PACKAGE_NAME}/${PACKAGE_VERSION}/Python-${PACKAGE_VERSION}.tar.xz  \
        $PACKAGE_SHA
    tar -xf "Python-${PACKAGE_VERSION}.tar.xz"

    #Configure and Build
    cd "Python-${PACKAGE_VERSION}"
    ./configure --prefix=/usr/local --exec-prefix=/usr/local
    make
    sudo make install

    echo 'Installed python successfully'

    #Cleanup
    cd -
    rm "Python-${PACKAGE_VERSION}.tar.xz"

    #Verify python installation
    export PATH="/usr/local/bin:${PATH}"
    if command -V "$PACKAGE_NAME"${PACKAGE_VERSION:0:1} >/dev/null
    then
        printf -- "%s installation completed. Please check the Usage to start the service.\n" "$PACKAGE_NAME"
    else
        printf -- "Error while installing %s, exiting with 127 \n" "$PACKAGE_NAME"
        exit 127
    fi


}

InstallPython "$python2Version" "$python2Sha"
InstallPython "$python3Version" "$python3Sha"

python3 --version | fgrep "$python3Version"

pip3 install --user wheel
pip3 install --user virtualenv

# shellcheck source=../common/unix/SetEnvVar.sh
source "${BASH_SOURCE%/*}/../common/unix/SetEnvVar.sh"
SetEnvVar "PYTHON3_PATH" "/usr/local/bin"
