#!/usr/bin/env bash
# Copyright (C) 2020 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

set -ex

sudo sed -i "s#baseurl=.*#baseurl=http://repo-clones.ci.qt.io/repos/opensuse/distribution/leap/15.3/repo/oss/#g" /etc/zypp/repos.d/repo-oss.repo
sudo sed -i "s#baseurl=.*#baseurl=http://repo-clones.ci.qt.io/repos/opensuse/distribution/leap/15.3/repo/non-oss/#g" /etc/zypp/repos.d/repo-non-oss.repo
sudo sed -i "s#baseurl=.*#baseurl=http://repo-clones.ci.qt.io/repos/opensuse/update/leap/15.3/oss/#g" /etc/zypp/repos.d/repo-update.repo
sudo sed -i "s#baseurl=.*#baseurl=http://repo-clones.ci.qt.io/repos/opensuse/update/leap/15.3/non-oss/#g" /etc/zypp/repos.d/repo-update-non-oss.repo

sudo zypper lr -u
