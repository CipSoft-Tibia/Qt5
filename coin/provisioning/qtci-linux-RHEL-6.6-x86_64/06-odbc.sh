#!/usr/bin/env bash
# provides: odbc devel packages on RHEL
# version: provided by default Linux distribution repository
# needed for configure -plugin-sql-odbc in qtbase

set -ex

sudo yum install -y unixODBC-devel
