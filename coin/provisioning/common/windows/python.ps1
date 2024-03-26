# Copyright (C) 2020 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

# This script installs Python $version.
# Python is required for building Qt 5 from source.
param(
    [Int32]$archVer=32,
    [string]$targetDir="C:\Python27"
)
. "$PSScriptRoot\helpers.ps1"

$version = "2.7.13"
if ( $archVer -eq 64 ) {
    $arch = ".amd64"
    $sha1 = "d9113142bae8829365c595735e1ad1f9f5e2894c"
} else {
    $arch = ""
    $sha1 = "7e3b54236dbdbea8fe2458db501176578a4d59c0"
}
$package = "C:\Windows\temp\python-$version.msi"
$externalUrl = "https://www.python.org/ftp/python/$version/python-$version" + $arch + ".msi"
$internalUrl = "\\ci-files01-hki.intra.qt.io\provisioning\windows\python-$version" + $arch + ".msi"

Write-Host "Fetching from URL..."
Download $externalUrl $internalUrl $package
Verify-Checksum $package $sha1

# Python installation is flaky, but seems to pass with second run if error occurs.
$stop = $false
[int]$retry = "0"
do {
    try {
        # /levx = e:'All error messages' v:'Verbose' x:'Extra debugging info'
        Run-Executable "msiexec" "/passive /i $package /levx C:\Windows\Temp\Python_log.log TARGETDIR=$targetDir ALLUSERS=1"
        $stop = $true
    }
    catch {
        Get-Content C:\Windows\Temp\Python_log.log -Tail 50
        if ($retry -gt 2) {
        Write-Host "Python installation failed!"
        throw
        }
        else {
            Write-Host "Couldn't install python, retrying in 30 seconds"
            Start-Sleep -s 30
            $retry = $retry + 1
        }
    }
}
while ($stop -ne $true)

# We need to change allowZip64 from 'False' to 'True' to be able to create ZIP files that use the ZIP64 extensions when the zipfile is larger than 2 GB
Write-Host "Changing allowZip64 value to 'True'..."
(Get-Content $targetDir\lib\zipfile.py) | ForEach-Object { $_ -replace "allowZip64=False", "allowZip64=True" } | Set-Content $targetDir\lib\zipfile.py
Remove "$package"

# When installing 32 bit python to 64 bit host, we want to keep only default python in path
# For cross-compilation we export some helper env variable
if (($archVer -eq 32) -And (Is64BitWinHost)) {
    Set-EnvironmentVariable "PYTHON2_32_PATH" "$targetDir"
    Set-EnvironmentVariable "PIP2_32_PATH" "$targetDir\Scripts"
} else {
    Add-Path "$targetDir;$targetDir\Scripts"
}


Run-Executable "$targetDir\python.exe" "-m ensurepip"

Write-Host "Upgrade pip to the latest version available."
Run-Executable "$targetDir\python.exe" "-m pip install --upgrade pip"

# Install python virtual env
if (IsProxyEnabled) {
    $proxy = Get-Proxy
    Write-Host "Using proxy ($proxy) with pip"
    $pip_args = "--proxy=$proxy"
}
Run-Executable "$targetDir\Scripts\pip.exe" "$pip_args install virtualenv"

# Install PyPDF2 for QSR documentation
Run-Executable "$targetDir\Scripts\pip.exe" "$pip_args install PyPDF2"

Write-Output "Python-$archVer = $version" >> ~/versions.txt
