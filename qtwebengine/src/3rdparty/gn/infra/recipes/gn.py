# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.
"""Recipe for building GN."""

from recipe_engine.recipe_api import Property

DEPS = [
    'recipe_engine/buildbucket',
    'recipe_engine/cipd',
    'recipe_engine/context',
    'recipe_engine/file',
    'recipe_engine/json',
    'recipe_engine/path',
    'recipe_engine/platform',
    'recipe_engine/properties',
    'recipe_engine/python',
    'recipe_engine/raw_io',
    'recipe_engine/step',
    'macos_sdk',
    'windows_sdk',
]

PROPERTIES = {
    'repository': Property(kind=str, default='https://gn.googlesource.com/gn'),
}


def RunSteps(api, repository):
  src_dir = api.path['start_dir'].join('gn')

  with api.step.nest('git'), api.context(infra_steps=True):
    api.step('init', ['git', 'init', src_dir])

    with api.context(cwd=src_dir):
      build_input = api.buildbucket.build_input
      ref = (
          build_input.gitiles_commit.id
          if build_input.gitiles_commit else 'refs/heads/master')
      # Fetch tags so `git describe` works.
      api.step('fetch', ['git', 'fetch', '--tags', repository, ref])
      api.step('checkout', ['git', 'checkout', 'FETCH_HEAD'])
      for change in build_input.gerrit_changes:
        api.step('fetch %s/%s' % (change.change, change.patchset), [
            'git', 'fetch', repository,
            'refs/changes/%s/%s/%s' %
            (str(change.change)[-2:], change.change, change.patchset)
        ])
        api.step('cherry-pick %s/%s' % (change.change, change.patchset),
                 ['git', 'cherry-pick', 'FETCH_HEAD'])

  with api.context(infra_steps=True):
    cipd_dir = api.path['start_dir'].join('cipd')
    pkgs = api.cipd.EnsureFile()
    pkgs.add_package('infra/ninja/${platform}', 'version:1.8.2')
    if api.platform.is_linux or api.platform.is_mac:
      pkgs.add_package('fuchsia/clang/${platform}',
                       'git_revision:b920a7f65b13237dc4d5b2b836b29a954fff440a')
    if api.platform.is_linux:
      pkgs.add_package('fuchsia/sysroot/${platform}',
                       'git_revision:a28dfa20af063e5ca00634024c85732e20220419',
                       'sysroot')
    api.cipd.ensure(cipd_dir, pkgs)

  # The order is important since release build will get uploaded to CIPD.
  configs = [
      {
          'name': 'debug',
          'args': ['-d']
      },
      {
          'name': 'release',
          'args': ['--use-lto', '--use-icf']
      },
  ]

  with api.macos_sdk(), api.windows_sdk():
    if api.platform.is_linux:
      sysroot = '--sysroot=%s' % cipd_dir.join('sysroot')
      env = {
          'CC': cipd_dir.join('bin', 'clang'),
          'CXX': cipd_dir.join('bin', 'clang++'),
          'AR': cipd_dir.join('bin', 'llvm-ar'),
          'CFLAGS': sysroot,
          'LDFLAGS': sysroot,
      }
    elif api.platform.is_mac:
      sysroot = '--sysroot=%s' % api.step(
          'xcrun', ['xcrun', '--show-sdk-path'],
          stdout=api.raw_io.output(name='sdk-path', add_output_log=True),
          step_test_data=
          lambda: api.raw_io.test_api.stream_output('/some/xcode/path')
      ).stdout.strip()
      stdlib = '-nostdlib++ %s' % cipd_dir.join('lib', 'libc++.a')
      env = {
          'CC': cipd_dir.join('bin', 'clang'),
          'CXX': cipd_dir.join('bin', 'clang++'),
          'AR': cipd_dir.join('bin', 'llvm-ar'),
          'CFLAGS': sysroot,
          'LDFLAGS': '%s %s' % (sysroot, stdlib),
      }
    else:
      env = {}

    for config in configs:
      with api.step.nest(config['name']):
        with api.step.nest('build'), api.context(env=env, cwd=src_dir):
          api.python(
              'generate', src_dir.join('build', 'gen.py'), args=config['args'])

          # Windows requires the environment modifications when building too.
          api.step('ninja', [cipd_dir.join('ninja'), '-C', src_dir.join('out')])

        api.step('test', [src_dir.join('out', 'gn_unittests')])

  if build_input.gerrit_changes:
    return

  cipd_pkg_name = 'gn/gn/${platform}'
  gn = 'gn' + ('.exe' if api.platform.is_win else '')

  pkg_def = api.cipd.PackageDefinition(
      package_name=cipd_pkg_name,
      package_root=src_dir.join('out'),
      install_mode='copy')
  pkg_def.add_file(src_dir.join('out', gn))
  pkg_def.add_version_file('.versions/%s.cipd_version' % gn)

  cipd_pkg_file = api.path['cleanup'].join('gn.cipd')

  api.cipd.build_from_pkg(
      pkg_def=pkg_def,
      output_package=cipd_pkg_file,
  )

  if api.buildbucket.builder_id.project == 'infra-internal':
    with api.context(cwd=src_dir):
      revision = api.step(
          'rev-parse', ['git', 'rev-parse', 'HEAD'],
          stdout=api.raw_io.output()).stdout.strip()

    cipd_pin = api.cipd.search(cipd_pkg_name, 'git_revision:' + revision)
    if cipd_pin:
      api.step('Package is up-to-date', cmd=None)
      return

    api.cipd.register(
        package_name=cipd_pkg_name,
        package_path=cipd_pkg_file,
        refs=['latest'],
        tags={
            'git_repository': repository,
            'git_revision': revision,
        },
    )


def GenTests(api):
  for platform in ('linux', 'mac', 'win'):
    yield (api.test('ci_' + platform) + api.platform.name(platform) +
           api.buildbucket.ci_build(
               project='gn',
               git_repo='gn.googlesource.com/gn',
           ))

    yield (api.test('cq_' + platform) + api.platform.name(platform) +
           api.buildbucket.try_build(
               project='gn',
               git_repo='gn.googlesource.com/gn',
           ))

  yield (api.test('cipd_exists') + api.buildbucket.ci_build(
      project='infra-internal',
      git_repo='gn.googlesource.com/gn',
      revision='a' * 40,
  ) + api.step_data('rev-parse', api.raw_io.stream_output('a' * 40)) +
         api.step_data('cipd search gn/gn/${platform} git_revision:' + 'a' * 40,
                       api.cipd.example_search('gn/gn/linux-amd64',
                                               ['git_revision:' + 'a' * 40])))

  yield (api.test('cipd_register') + api.buildbucket.ci_build(
      project='infra-internal',
      git_repo='gn.googlesource.com/gn',
      revision='a' * 40,
  ) + api.step_data('rev-parse', api.raw_io.stream_output('a' * 40)) +
         api.step_data('cipd search gn/gn/${platform} git_revision:' + 'a' * 40,
                       api.cipd.example_search('gn/gn/linux-amd64', [])))
