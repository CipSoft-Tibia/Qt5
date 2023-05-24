add_library(QtQuickEffectMakerDoc INTERFACE)
target_link_libraries(QtQuickEffectMakerDoc INTERFACE Qt::Core)

qt_internal_add_docs(QtQuickEffectMakerDoc
    qtquickeffectmaker.qdocconf
)
