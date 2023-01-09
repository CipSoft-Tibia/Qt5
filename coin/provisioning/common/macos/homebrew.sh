#!/bin/sh


# Will install homebrew package manager for macOS.
#     WARNING: Requires commandlinetools


set -e

. "$(dirname "$0")"/../../common/unix/DownloadURL.sh


DownloadURL  \
    http://ci-files01-hki.intra.qt.io/input/mac/homebrew-install.c744a716f9845988d01e6e238eee7117b8c366c9.rb  \
    https://raw.githubusercontent.com/Homebrew/install/c744a716f9845988d01e6e238eee7117b8c366c9/install  \
    b9782cc0b550229de77b429b56ffce04157e60486ab9df00461ccf3dad565b0a  \
    /tmp/homebrew_install
/usr/bin/ruby /tmp/homebrew_install  </dev/null

# No need to manually do `brew update`, the homebrew installer script does it.
### brew update
