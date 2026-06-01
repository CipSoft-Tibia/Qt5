# This file is used to manage the dependencies. It is
# used by gclient to determine what version of each dependency to check out, and
# where.
#
# For more information, please refer to the official documentation:
#   https://sites.google.com/a/chromium.org/dev/developers/how-tos/get-the-code
#
# When adding a new dependency, please update the top-level .gitignore file
# to list the dependency's destination directory.
#
# -----------------------------------------------------------------------------
# Rolling deps
# -----------------------------------------------------------------------------
# All repositories in this file are git-based, using Chromium git mirrors where
# necessary (e.g., a git mirror is used when the source project is SVN-based).
# To update the revision that Chromium pulls for a given dependency:
#
#  # Create and switch to a new branch
#  git new-branch depsroll
#  # Run roll-dep (provided by depot_tools) giving the dep's path and optionally
#  # a regex that will match the line in this file that contains the current
#  # revision. The script ALWAYS rolls the dependency to the latest revision
#  # in origin/master. The path for the dep should start with src/.
#  roll-dep src/third_party/foo_package/src foo_package.git
#  # You should now have a modified DEPS file; commit and upload as normal
#  git commit -aspv_he
#  git cl upload
#
# For more on the syntax and semantics of this file, see:
#   https://bit.ly/chromium-gclient-conditionals
#
# which is a bit incomplete but the best documentation we have at the
# moment.

# We expect all git dependencies specified in this file to be in sync with git
# submodules (gitlinks).
git_dependencies = 'SYNC'

use_relative_paths = True

vars = {
  'chromium_tsproxy_git': 'https://chromium.googlesource.com/external/github.com/catchpoint/WebPageTest.tsproxy.git',
  'chromium_webpagereplay_git': 'https://chromium.googlesource.com/webpagereplay',

  # This variable is overridden in Chromium's DEPS file.
  'build_with_chromium': False,

  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling tsproxy
  # and whatever else without interference from each other.
  'tsproxy_revision': '7915adec656341bfab173484e1e0ca661eea1627',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling tsproxy
  # and whatever else without interference from each other.
  'webpagereplay_revision': '80f08d7a3457ca7f9678e5ae4bda4aefe72bb40e',
}

# Only these hosts are allowed for dependencies in this DEPS file.
# If you need to add a new host, contact chrome infrastructure team.
allowed_hosts = [
  'chromium.googlesource.com',
]

deps = {
  'third_party/tsproxy': Var('chromium_tsproxy_git') + '@' + Var('tsproxy_revision'),
  'third_party/webpagereplay': {
    'url': Var('chromium_webpagereplay_git') + '@' + Var('webpagereplay_revision'),
    'condition': 'not build_with_chromium',
  }
}