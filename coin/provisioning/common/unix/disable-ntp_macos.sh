#!/usr/bin/env bash
# Copyright (C) 2018 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

set -ex

echo "Disable Network Time Protocol (NTP)"
echo "sudo launchctl unload /System/Library/LaunchDaemons/org.ntp.ntpd.plist" >> /Users/qt/.bash_profile

