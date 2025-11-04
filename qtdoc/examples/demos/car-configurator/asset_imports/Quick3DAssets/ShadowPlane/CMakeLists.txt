qt_add_library(Ast_ShadowPlane STATIC)

qt_add_qml_module(Ast_ShadowPlane
    URI "Quick3DAssets.ShadowPlane"
    CLASS_NAME "ShadowPlanePlugin"
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
    QML_FILES
        ShadowPlane.qml
)
