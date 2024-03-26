# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

. "$PSScriptRoot\helpers.ps1"

function InstallMinGW
{
    Param (
        [string] $release = $(BadParam("release file name")),
        [string] $sha1    = $(BadParam("SHA1 checksum of the file"))
    )

    $null, $null, $arch, $version, $null, $threading, $ex_handling, $build_ver, $revision = $release.split('-')

    if ($arch -eq "x86_64") { $win_arch = "Win64" }
    $envvar = "MINGW$version"
    $envvar = $envvar -replace '["."]'
    $targetdir = "C:\$envvar"

    $url_original = "https://github.com/cristianadam/mingw-builds/releases/download/v" + $version + "-" + $revision + "/" + $arch + "-" + $version + "-release-" + $threading + "-" + $ex_handling + "-" + $build_ver + "-" + $revision + ".7z"
    $url_cache = "\\ci-files01-hki.intra.qt.io\provisioning\windows\" + $release + ".7z"
    $mingwPackage = "C:\Windows\Temp\MinGW-$version.zip"
    Download $url_original $url_cache $mingwPackage
    Verify-Checksum $mingwPackage $sha1

    Extract-7Zip $mingwPackage $TARGETDIR

    Set-EnvironmentVariable "$envvar" ("$targetdir\mingw" + $win_arch.Substring($win_arch.get_Length()-2))

    Write-Host "Cleaning $mingwPackage.."
    Remove "$mingwPackage"

    Write-Output "MinGW = $version $release" >> ~\versions.txt

}
