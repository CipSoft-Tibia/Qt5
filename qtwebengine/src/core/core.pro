include($$QTWEBENGINE_OUT_ROOT/src/buildtools/qtbuildtools-config.pri)
QT_FOR_CONFIG += buildtools-private
TEMPLATE = subdirs

# core_headers is a dummy module to syncqt the headers so we can
# use them by later targets
core_headers.file = core_headers.pro
core_api.file = api/core_api.pro

core_lipo.file = core_lipo.pro
core_lipo.depends = gn_run

# This will take the compile output of ninja, and link+deploy the final binary.
core_module.file = core_module.pro
core_module.depends = core_api
isUniversal(): core_module.depends += core_lipo

core_generator.file = core_generator.pro
core_generator.depends = core_headers

# core_gn_generator.pro is a dummy .pro file that is used by qmake
# to generate our main BUILD.gn file

gn_run.file = gn_run.pro
gn_run.depends = core_generator

core_api.depends = gn_run

# A fake project for qt creator
core_project.file = core_project.pro
core_project.depends = gn_run

!qtConfig(webengine-core-support):qtConfig(build-qtwebengine-core):!build_pass {
    !qtwebengine_makeCheckWebEngineCoreError() {
        errorbuild.commands = @echo $$shell_quote("QtWebEngineCore module will not be built. $${skipBuildReason}")
    } else {
        errorbuild.commands = @echo $$shell_quote("QtWebEngineCore module will not be built for unknown reason, please open a bug report at https://bugreports.qt.io")
    }
    errorbuild.CONFIG = phony
    QMAKE_EXTRA_TARGETS += errorbuild
    first.depends += errorbuild
    QMAKE_EXTRA_TARGETS += first
} else {
    SUBDIRS += \
        core_headers \
        core_generator \
        gn_run \
        core_api \
        core_module

    isUniversal(): SUBDIRS += core_lipo

    false: SUBDIRS += core_project
}
