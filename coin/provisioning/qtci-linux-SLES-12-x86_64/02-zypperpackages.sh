#!/usr/bin/env bash

#############################################################################
##
## Copyright (C) 2020 The Qt Company Ltd.
## Contact: http://www.qt.io/licensing/
##
## This file is part of the provisioning scripts of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:LGPL21$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see http://www.qt.io/terms-conditions. For further
## information use the contact form at http://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 or version 3 as published by the Free
## Software Foundation and appearing in the file LICENSE.LGPLv21 and
## LICENSE.LGPLv3 included in the packaging of this file. Please review the
## following information to ensure the GNU Lesser General Public License
## requirements will be met: https://www.gnu.org/licenses/lgpl.html and
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## As a special exception, The Qt Company gives you certain additional
## rights. These rights are described in The Qt Company LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
## $QT_END_LICENSE$
##
#############################################################################

set -ex

sudo zypper -nq install git gcc gcc-c++

sudo zypper -nq install bison flex gperf \
        zlib-devel \
        libudev-devel \
        glib2-devel \
        libopenssl-devel \
        freetype2-devel \
        fontconfig-devel \
        sqlite3-devel \
        libxkbcommon-devel \
        libxkbcommon-x11-devel

sudo zypper -nq install p7zip

# EGL support
sudo zypper -nq install Mesa-libEGL-devel Mesa-libGL-devel

# gtk3 style for QtGui/QStyle
sudo zypper -nq install gtk3-devel

# Xinput2
sudo zypper -nq install libXi-devel postgresql10 postgresql10-devel mysql-devel mysql mysql-server

# system provided XCB libraries
sudo zypper -nq install xcb-util-devel xcb-util-image-devel xcb-util-keysyms-devel \
         xcb-util-wm-devel xcb-util-renderutil-devel

# ICU
sudo zypper -nq install libicu-devel libicu52_1

# qtwebengine
sudo zypper -nq install alsa-devel dbus-1-devel \
         libXcomposite-devel libXcursor-devel libXrandr-devel libXtst-devel \
         mozilla-nspr-devel mozilla-nss-devel

# qtwebkit
sudo zypper -nq install libxml2-devel libxslt-devel

# GStreamer (qtwebkit and qtmultimedia), pulseaudio (qtmultimedia)
sudo zypper -nq install gstreamer-devel gstreamer-plugins-base-devel libpulse-devel

# cups
sudo zypper -nq install cups-devel

# speech-dispatcher
sudo zypper -nq install libspeechd-devel
# https://bugzilla.suse.com/show_bug.cgi?id=1129586
sudo mv /usr/include/speech-dispatcher/speech-dispatcher/* /usr/include/speech-dispatcher/

# ODBC support
sudo zypper -nq install unixODBC-devel unixODBC

# freetds support
sudo zypper -nq install libfreetds freetds-devel

# sqlite2 support
sudo zypper -nq install sqlite2 sqlite2-devel

gccVersion="$(gcc --version |grep gcc |cut -b 17-23)"
echo "GCC = $gccVersion" >> versions.txt
