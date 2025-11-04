# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import logging
import plistlib
import re
import shutil
import sys
import tempfile
from typing import TYPE_CHECKING, Final, Iterable, Optional, Tuple, Type, Union

from crossbench import path as pth
from crossbench.browsers.browser_helper import BROWSERS_CACHE
from crossbench.browsers.version import BrowserVersion, UnknownBrowserVersion
from crossbench.helper import Spinner

if TYPE_CHECKING:
  from crossbench.plt.base import Platform


class IncompatibleVersionError(ValueError):
  pass


class Downloader(abc.ABC):
  ARCHIVE_SUFFIX: str = ""
  ANY_MARKER: Final = 9999
  APP_VERSION_RE = re.compile(r"(?P<version>[\d\.ab]+)")

  @classmethod
  @abc.abstractmethod
  def _get_loader_cls(cls, browser_platform: Platform) -> Type[Downloader]:
    pass

  @classmethod
  def is_valid(cls, path_or_identifier: pth.RemotePathLike,
               browser_platform: Platform) -> bool:
    return cls._get_loader_cls(browser_platform).is_valid(
        path_or_identifier, browser_platform)

  @classmethod
  @abc.abstractmethod
  def is_valid_version(cls, path_or_identifier: str):
    pass

  @classmethod
  def load(cls,
           archive_path_or_version_identifier: Union[str, pth.LocalPath],
           browser_platform: Platform,
           cache_dir: Optional[pth.LocalPath] = None) -> pth.LocalPath:
    logging.debug("Downloading chrome %s binary for %s",
                  archive_path_or_version_identifier, browser_platform)
    loader_cls: Type[Downloader] = cls._get_loader_cls(browser_platform)
    loader: Downloader = loader_cls(archive_path_or_version_identifier, "", "",
                                    browser_platform, cache_dir)
    return loader.app_path

  def __init__(self,
               archive_path_or_version_identifier: Union[str, pth.LocalPath],
               browser_type: str,
               platform_name: str,
               browser_platform: Platform,
               cache_dir: Optional[pth.LocalPath] = None):
    assert browser_type, "Missing browser_type"
    self._browser_type = browser_type
    self._browser_platform = browser_platform
    self._platform_name = platform_name
    assert platform_name, "Missing platform_name"
    self._archive_url: str = ""
    self._archive_path: pth.LocalPath = pth.LocalPath()
    self._out_dir: pth.LocalPath = cache_dir or BROWSERS_CACHE
    self._archive_dir: pth.LocalPath = self._out_dir / "archive"
    self._archive_dir.mkdir(parents=True, exist_ok=True)
    self._app_path: pth.LocalPath = pth.LocalPath()
    self._requested_version: BrowserVersion = UnknownBrowserVersion()
    with Spinner():
      self._app_path = self.find(archive_path_or_version_identifier)
    self._validate()

  def find(
      self, archive_path_or_version_identifier: Union[str, pth.LocalPath]
  ) -> pth.LocalPath:
    if self.is_valid_version(str(archive_path_or_version_identifier)):
      self._requested_version = self._parse_version(
          str(archive_path_or_version_identifier))
      self._pre_check()
      sys.stdout.write(f"   BROWSER: Looking for {self._requested_version}\r")
      return self._load_from_version()

    self._archive_path = pth.LocalPath(archive_path_or_version_identifier)
    self._pre_check()
    if not archive_path_or_version_identifier or (
        not self._archive_path.exists()):
      raise ValueError(
          f"{self._browser_type} archive does not exist: {self._archive_path}")
    return self._load_from_archive()

  def _validate(self) -> None:
    assert self._app_path != pth.LocalPath(), "Did not set app_path"
    assert self._is_app_installed(self._app_path), (
        f"Could not extract {self._browser_type}  binary: {self._app_path}")
    logging.debug("Extracted app: %s", self._app_path)

  @property
  def app_path(self) -> pth.LocalPath:
    assert self._is_app_installed(self._app_path), "Could not download browser"
    return self._app_path

  @property
  def host_platform(self) -> Platform:
    return self._browser_platform.host_platform

  def _pre_check(self) -> None:
    pass

  def _is_app_installed(self, app_path: pth.LocalPath) -> bool:
    return self._browser_platform.search_app(app_path) is not None

  def _find_matching_installed_version(self) -> Optional[pth.LocalPath]:
    app_path: pth.LocalPath = self._installed_app_path()
    if self._is_app_installed(app_path):
      return app_path
    return None

  def _create_archive_path(self, version: BrowserVersion) -> pth.LocalPath:
    version_name = str(version).replace(" ", "_")
    return self._archive_dir / (f"{version_name}{self.ARCHIVE_SUFFIX}")

  def _load_from_version(self) -> pth.LocalPath:
    self._archive_path = self._create_archive_path(self._requested_version)
    if app_path := self._find_matching_installed_version():
      if cached_version := self._validate_installed(app_path):
        logging.info("CACHED BROWSER: %s %s", cached_version, self._app_path)
        return app_path
    self._requested_version_validation()
    if not self._try_download_version_archive():
      logging.info("CACHED DOWNLOAD: %s", self._archive_path)
    self._install_archive(self._archive_path)
    return self._installed_app_path()

  def _try_download_version_archive(self):
    if self._archive_path.exists():
      return False
    archive_version, archive_url = self._find_archive_url()
    if not archive_url:
      raise ValueError(
          f"Could not find matching version for {self._requested_version}")
    self._archive_url = archive_url
    self._archive_path = self._create_archive_path(archive_version)
    if self._archive_path.exists():
      return False
    logging.info("DOWNLOADING %s", self._archive_url)
    with tempfile.TemporaryDirectory(prefix="cb_download_") as tmp_dir_name:
      tmp_dir = pth.LocalPath(tmp_dir_name)
      self._download_archive(self._archive_url, tmp_dir)
    return True

  @abc.abstractmethod
  def _requested_version_validation(self) -> None:
    pass

  def _load_from_archive(self) -> pth.LocalPath:
    assert not self._requested_version.is_complete
    assert self._archive_path.exists()
    logging.info("EXTRACTING ARCHIVE: %s", self._archive_path)
    original_out_dir = self._out_dir
    with tempfile.TemporaryDirectory(
        prefix="cb_extract_", dir=original_out_dir) as tmpdir:
      # Extract input archive to temp dir for version extraction.
      self._out_dir = pth.LocalPath(tmpdir)
      temp_extracted_path = self._extract_unknown_version_archive()
      self._out_dir = original_out_dir
      # Install temporary extracted version
      versioned_path = self._extracted_path()
      app_path = self._installed_app_path()
      if self._is_app_installed(app_path):
        cached_version = self._validate_installed(app_path)
        logging.info("CACHED BROWSER: %s %s", cached_version, app_path)
      else:
        assert not versioned_path.exists()
        temp_extracted_path.rename(versioned_path)
    return app_path

  def _extract_unknown_version_archive(self) -> pth.LocalPath:
    tmp_app_path: pth.LocalPath = self._installed_app_path()
    temp_extracted_path = self._extracted_path()
    self._install_archive(self._archive_path)
    logging.debug("Parsing browser version: %s", tmp_app_path)
    assert self._is_app_installed(tmp_app_path), (
        f"Extraction failed, app does not exist: {tmp_app_path}")
    full_version_string = self._browser_platform.app_version(tmp_app_path)
    self._requested_version = self._parse_version(full_version_string)
    assert self._requested_version.is_complete
    return temp_extracted_path


  @abc.abstractmethod
  def _parse_version(self, version_identifier: str) -> BrowserVersion:
    pass

  def _extracted_path(self) -> pth.LocalPath:
    # TODO: support local vs remote
    return self._out_dir / str(self._requested_version).replace(" ", "_")

  @abc.abstractmethod
  def _installed_app_path(self) -> pth.LocalPath:
    pass

  def _installed_app_version(self, app_path: pth.LocalPath) -> BrowserVersion:
    raw_version = self._browser_platform.app_version(app_path)
    return self._parse_version(raw_version)

  def _validate_installed(self, app_path: pth.LocalPath) -> BrowserVersion:
    cached_version: BrowserVersion = self._installed_app_version(app_path)
    msg: str = ""
    expected_version_str: str = str(self._requested_version)
    if self._requested_version.is_complete:
      if self._requested_version.contains(cached_version):
        return cached_version
      msg = (f"Previously downloaded browser at {app_path} "
             "might have been auto-updated.\n")
    else:
      requested_milestone: int = self._requested_version.major
      logging.debug("Validating installed milestone %s", requested_milestone)
      latest_milestone_version, _ = self._find_archive_url()
      if cached_version == latest_milestone_version:
        return cached_version
      msg = (f"Previously downloaded browser at {app_path} "
             f"does not match latest milestone {requested_milestone} "
             f"version: {latest_milestone_version}.\n")
      expected_version_str = (
          f"{self._requested_version}/{latest_milestone_version}")
    msg += ("Please delete the old version and re-install/-download it.\n"
            f"Expected: {expected_version_str} Got: {cached_version}")
    logging.debug(msg)
    raise IncompatibleVersionError(msg)

  @abc.abstractmethod
  def _find_archive_url(self) -> Tuple[BrowserVersion, Optional[str]]:
    pass

  @abc.abstractmethod
  def _archive_urls(
      self, folder_url: str,
      version: BrowserVersion) -> Iterable[Tuple[BrowserVersion, str]]:
    pass

  @abc.abstractmethod
  def _download_archive(self, archive_url: str, tmp_dir: pth.LocalPath) -> None:
    pass

  @abc.abstractmethod
  def _install_archive(self, archive_path: pth.LocalPath) -> None:
    pass


class ArchiveHelper(abc.ABC):

  @classmethod
  @abc.abstractmethod
  def extract(cls, platform: Platform, archive_path: pth.LocalPath,
              dest_path: pth.LocalPath) -> pth.LocalPath:
    pass


class RPMArchiveHelper(ArchiveHelper):

  @classmethod
  def extract(cls, platform: Platform, archive_path: pth.LocalPath,
              dest_path: pth.LocalPath) -> pth.LocalPath:
    assert platform.which("rpm2cpio"), (
        "Need rpm2cpio to extract downloaded .rpm archive")
    assert platform.which("cpio"), (
        "Need cpio to extract downloaded .rpm archive")
    cpio_file = archive_path.with_suffix(".cpio")
    assert not cpio_file.exists()
    archive_path.parent.mkdir(parents=True, exist_ok=True)
    with cpio_file.open("w") as f:
      platform.sh("rpm2cpio", archive_path, stdout=f)
    assert cpio_file.is_file(), f"Could not extract archive: {archive_path}"
    assert not dest_path.exists()
    with cpio_file.open() as f:
      platform.sh(
          "cpio",
          "--extract",
          f"--directory={dest_path}",
          "--make-directories",
          stdin=f)
    cpio_file.unlink()
    if not dest_path.exists():
      raise ValueError(f"Could not extract archive to {dest_path}")
    return dest_path


class DMGArchiveHelper:

  @classmethod
  def extract(cls, platform: Platform, archive_path: pth.LocalPath,
              dest_path: pth.LocalPath) -> pth.LocalPath:
    assert platform.is_macos, "DMG are only supported on macOS."
    assert not platform.is_remote, "Remote platform not supported yet"
    result = platform.sh_stdout("hdiutil", "attach", "-plist",
                                archive_path).strip()
    data = plistlib.loads(str.encode(result))
    dmg_path: Optional[pth.LocalPath] = None
    for item in data["system-entities"]:
      mount_point = item.get("mount-point", None)
      if mount_point:
        dmg_path = pth.LocalPath(mount_point)
        if dmg_path.exists():
          break
    if not dmg_path:
      raise ValueError("Could not mount downloaded disk image")
    apps = list(dmg_path.glob("*.app"))
    assert len(apps) == 1, "Mounted disk image contains more than 1 app"
    app = apps[0]
    try:
      logging.info("COPYING BROWSER src=%s dst=%s", app, dest_path)
      shutil.copytree(app, dest_path, symlinks=True, dirs_exist_ok=False)
    finally:
      platform.sh("hdiutil", "detach", dmg_path)
    if not dest_path.exists():
      raise ValueError(f"Could not extract archive to {dest_path}")
    return dest_path
