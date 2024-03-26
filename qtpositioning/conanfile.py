# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from conans import ConanFile
import re
from pathlib import Path
from typing import List, Dict, Any


def _parse_qt_version_by_key(key: str) -> str:
    with open(Path(__file__).parent.resolve() / ".cmake.conf") as f:
        m = re.search(fr'{key} .*"(.*)"', f.read())
    return m.group(1) if m else ""


def _get_qt_minor_version() -> str:
    return ".".join(_parse_qt_version_by_key("QT_REPO_MODULE_VERSION").split(".")[:2])


class QtPositioning(ConanFile):
    name = "qtpositioning"
    license = "LGPL-3.0, GPL-2.0+, Commercial Qt License Agreement"
    author = "The Qt Company <https://www.qt.io/contact-us>"
    url = "https://code.qt.io/cgit/qt/qtpositioning.git"
    description = "Qt Positioning support."
    topics = "qt", "qt6", "positioning"
    settings = "os", "compiler", "arch", "build_type"
    # for referencing the version number and prerelease tag and dependencies info
    exports = ".cmake.conf", "dependencies.yaml"
    exports_sources = "*", "!conan*.*"
    python_requires = f"qt-conan-common/{_get_qt_minor_version()}@qt/everywhere"
    python_requires_extend = "qt-conan-common.QtLeafModule"

    def get_qt_leaf_module_options(self) -> Dict[str, Any]:
        """Implements abstractmethod from qt-conan-common.QtLeafModule"""
        return {"force_nmea_plugin": ["yes", "no", None]}

    def get_qt_leaf_module_default_options(self) -> Dict[str, Any]:
        """Implements abstractmethod from qt-conan-common.QtLeafModule"""
        return {"force_nmea_plugin": "yes"}

    def override_qt_requirements(self) -> List[str]:
        """Implements abstractmethod from qt-conan-common.QtLeafModule"""
        requirements = ["qtbase", "qtdeclarative"]
        if self.options.force_nmea_plugin:
            requirements.append("qtserialport")
        return requirements

    def is_qt_module_feature(self, option_name: str) -> bool:
        """Implements abstractmethod from qt-conan-common.QtLeafModule"""
        if option_name == "force_nmea_plugin":
            return False
        return True
