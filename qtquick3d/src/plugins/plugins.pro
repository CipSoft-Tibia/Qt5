TEMPLATE = subdirs

# Only build import plugins on platforms that would run tooling
!integrity:!android|android_app:!wasm:!cross_compile {
    SUBDIRS = assetimporters
}

