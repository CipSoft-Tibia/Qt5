#!/usr/bin/env python3
# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import pathlib
import platform
import re
from typing import Iterable, List, Optional

USE_PYTHON3 = True


def CheckChange(input_api, output_api, on_commit):
  tests = []
  results = []
  testing_env = dict(input_api.environ)
  root_path = pathlib.Path(input_api.PresubmitLocalPath())
  crossbench_test_path = root_path / "tests" / "crossbench"
  testing_env["PYTHONPATH"] = input_api.os_path.pathsep.join(
      map(str, [root_path, crossbench_test_path]))
  # ---------------------------------------------------------------------------
  modified_py_files: Optional[List[str]] = ModifiedFiles(input_api, on_commit)

  # ---------------------------------------------------------------------------
  # Validate the vpython spec:
  # ---------------------------------------------------------------------------
  if platform.system() in ("Linux", "Darwin"):
    tests += input_api.canned_checks.CheckVPythonSpec(input_api, output_api)

  # ---------------------------------------------------------------------------
  # License header checks:
  # ---------------------------------------------------------------------------
  results += input_api.canned_checks.CheckLicense(input_api, output_api)

  # ---------------------------------------------------------------------------
  # Pylint:
  # ---------------------------------------------------------------------------
  pylint_file_patterns_to_check: List[str] = PylintFilePatternsToCheck(
      on_commit, modified_py_files)
  tests += input_api.canned_checks.GetPylint(
      input_api,
      output_api,
      files_to_check=pylint_file_patterns_to_check,
      pylintrc=".pylintrc",
      version="2.17")

  # ---------------------------------------------------------------------------
  # MyPy:
  # ---------------------------------------------------------------------------
  mypy_files_to_check: List[str] = MypyFilesToCheck(input_api, on_commit,
                                                    modified_py_files)
  tests.append(
      input_api.Command(
          name="mypy",
          cmd=[
              input_api.python3_executable,
              "-m",
              "mypy",
              "--check-untyped-defs",
              "--pretty",
          ] + mypy_files_to_check,
          message=output_api.PresubmitError,
          kwargs={},
          python3=True,
      ))

  # ---------------------------------------------------------------------------
  # Unittest:
  # ---------------------------------------------------------------------------
  test_dirs_to_check, test_file_patterns_to_check = TestFilePatternsToCheck(
      on_commit, crossbench_test_path)
  for test_dir_to_check in test_dirs_to_check:
    # Skip potentially empty dirs
    if test_dir_to_check.name == "__pycache__":
      continue
    # End-to-end tests require custom setup and are not suited for presubmits.
    if "end2end" in test_dir_to_check.parts:
      continue
    tests += input_api.canned_checks.GetUnitTestsInDirectory(
        input_api,
        output_api,
        directory=test_dir_to_check,
        env=testing_env,
        files_to_check=test_file_patterns_to_check,
        skip_shebang_check=True,
        run_on_python2=False)

  # ---------------------------------------------------------------------------
  # Run all test
  # ---------------------------------------------------------------------------
  results += input_api.RunTests(tests)
  return results


def ModifiedFiles(input_api,
                  on_commit: bool,
                  filename_pattern="*.py") -> Optional[List[str]]:
  if on_commit:
    return None
  files = [file.AbsoluteLocalPath() for file in input_api.AffectedFiles()]
  files_to_check = []
  for file_path in files:
    if not input_api.fnmatch.fnmatch(file_path, filename_pattern):
      continue
    if not input_api.os_path.exists(file_path):
      continue
    file_path = input_api.os_path.relpath(file_path,
                                          input_api.PresubmitLocalPath())
    files_to_check.append(file_path)
  return files_to_check


def PylintFilePatternsToCheck(on_commit, modified_py_files) -> List[str]:
  if on_commit:
    # Test all files on commit
    return [r"^[^\.]+\.py$"]
  # By default, the pylint canned check lints all Python files together to
  # check for potential problems between dependencies. This is slow to run
  # across all of crossbench (>2 min), so only lint affected files.
  return [re.escape(file) for file in modified_py_files]


def MypyFilesToCheck(input_api, on_commit, modified_py_files) -> List[str]:
  root_path = pathlib.Path(input_api.PresubmitLocalPath())
  mypy_files_to_check = {"PRESUBMIT.py"}
  crossbench_path = root_path / "crossbench"
  if on_commit:
    mypy_files_to_check.add(str(crossbench_path))
  else:
    mypy_files_to_check.update(modified_py_files)
  # TODO: enable mypy on all tests
  result = []
  for file in mypy_files_to_check:
    if not file.startswith("tests/"):
      result.append(file)
  return result


def TestFilePatternsToCheck(on_commit, crossbench_test_path):
  # Only run test_cli to speed up the presubmit checks
  if on_commit:
    test_dirs_to_check: Iterable[pathlib.Path] = crossbench_test_path.glob("**")
    test_files_to_check = [r".*test_.*\.py$"]
  else:
    # Only check a small subset on upload
    test_dirs_to_check = [crossbench_test_path / "cli"]
    test_files_to_check = [r".*test_cli_fast_.*\.py$"]
  return test_dirs_to_check, test_files_to_check


def CheckChangeOnUpload(input_api, output_api):
  return CheckChange(input_api, output_api, on_commit=False)


def CheckChangeOnCommit(input_api, output_api):
  return CheckChange(input_api, output_api, on_commit=True)
