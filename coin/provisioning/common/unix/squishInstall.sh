#!/usr/bin/env bash
# Copyright (C) 2020 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

PROVISIONING_DIR="$(dirname "$0")/../../"
source "${BASH_SOURCE%/*}/DownloadURL.sh"
source "$PROVISIONING_DIR"/common/unix/common.sourced.sh

set -ex

# This script will fetch and extract pre-buildt squish package for Linux and Mac.
# Squish is need by Release Test Automation (RTA)

version="8.1.0"
qtBranch="68x"
installFolder="/opt"
squishFolder="$installFolder/squish"
preBuildCacheUrl="ci-files01-hki.ci.qt.io:/hdd/www/input/squish/jenkins_build/stable"
licenseFile=".squish-license"
licenseBranch="squish_license"
licenseUrl="http://ci-files01-hki.ci.qt.io/input/squish/coin/$licenseBranch/$licenseFile"
licenseSHA="e84b499a2011f9bb1a6eefc7b2338d7ae770927a"
testSuiteUrl="ci-files01-hki.ci.qt.io:/hdd/www/input/squish/coin/suite_test_squish"
testSuiteLocal="/tmp/squish_test_suite"
if uname -a |grep -q Darwin; then
    compressedFolder="prebuild-squish-$version-$qtBranch-mac-x64.tar.gz"
    sha1="03a0c713d0d328667df2e7804f2e4d507707b849"
else
    if [ "$PROVISIONING_ARCH" = arm64 ] ; then
        compressedFolder="prebuild-squish-$version-$qtBranch-linux-arm64.tar.gz"
        sha1="f6a2eb69faed64f13b164fb8d056182c41d2952c"
    else
        compressedFolder="prebuild-squish-$version-$qtBranch-linux-x64.tar.gz"
        sha1="b798417ddf4b668306cb90d551df906828644152"
    fi
fi

mountFolder="/tmp/squish"
sudo mkdir "$mountFolder"
sudo mkdir "$testSuiteLocal"

# Check which platform
if uname -a |grep -q Darwin; then
    usersGroup="staff"
elif uname -a |grep -q "el7"; then
    usersGroup="qt"
elif uname -a |grep -q "Ubuntu"; then
    usersGroup="users"
else
    usersGroup="users"
fi

targetFileMount="$mountFolder"/"$compressedFolder"

echo "Mounting Squish packages from $preBuildCacheUrl to $mountFolder"
echo "Mounting Squish test suite from $testSuiteUrl to $testSuiteLocal"
if uname -a |grep -q Darwin; then
   sudo mount -o locallocks "$preBuildCacheUrl" "$mountFolder"
   sudo mount -o locallocks "$testSuiteUrl" "$testSuiteLocal"
else
   sudo mount "$preBuildCacheUrl" "$mountFolder"
   sudo mount "$testSuiteUrl" "$testSuiteLocal"
fi
echo "Create $installFolder if needed"
if [ !  -d "$installFolder" ]; then
    sudo mkdir "$installFolder"
fi

VerifyHash "$targetFileMount" "$sha1"

echo "Uncompress $compressedFolder"
sudo tar -xzf "$targetFileMount" --directory "$installFolder"

if uname -a |grep -q Darwin; then
    sudo xattr -r -c "$squishFolder"
fi

echo "Download Squish license"
DownloadURL "$licenseUrl" "$licenseUrl" "$licenseSHA" "$HOME/$licenseFile"

echo "Changing ownerships"
sudo chown -R qt:$usersGroup "$squishFolder"
sudo chown qt:$usersGroup "$HOME/$licenseFile"


echo "Verifying Squish, available installations:"
ls -la $squishFolder
cd $squishFolder

for squishInstallation in */ ; do
  if "$squishInstallation/bin/squishrunner" --testsuite "$testSuiteLocal" | grep "Squish test run successfully" ; then
    echo "Squish in $squishInstallation tested successfully"
  else
    echo "Testing Squish in $squishInstallation failed! Squish wasn't installed correctly."
    exit 1
  fi
done

echo "Clean up installation temp dirs"
echo "- Unmounting $mountFolder"
sudo diskutil unmount force "$mountFolder" || sudo umount -f "$mountFolder" || true

echo "- Unmounting $testSuiteLocal"
sudo diskutil unmount force "$testSuiteLocal" || sudo umount -f "$testSuiteLocal" || true
