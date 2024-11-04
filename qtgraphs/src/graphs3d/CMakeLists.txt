# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_configure_add_summary_section(NAME "Qt Graphs 3D")
qt_configure_add_summary_entry(ARGS "graphs-3d-bars3d")
qt_configure_add_summary_entry(ARGS "graphs-3d-scatter3d")
qt_configure_add_summary_entry(ARGS "graphs-3d-surface3d")
qt_configure_end_summary_section() # end of "Qt Graphs 3D" section

if(NOT QT_FEATURE_graphs_3d_bars3d AND NOT QT_FEATURE_graphs_3d_scatter3d AND NOT QT_FEATURE_graphs_3d_surface3d)
    message("You must have either FEATURE_graphs_3d_bars3d, FEATURE_graphs_3d_scatter3d, or FEATURE_graphs_3d_surface3d ON, or nothing will be built.")
else()
    qt_internal_extend_target(Graphs
        SOURCES
            axis/qabstract3daxis.cpp axis/qabstract3daxis.h axis/qabstract3daxis_p.h
            axis/qcategory3daxis.cpp axis/qcategory3daxis.h axis/qcategory3daxis_p.h
            axis/qlogvalue3daxisformatter.cpp axis/qlogvalue3daxisformatter.h axis/qlogvalue3daxisformatter_p.h
            axis/qvalue3daxis.cpp axis/qvalue3daxis.h axis/qvalue3daxis_p.h
            axis/qvalue3daxisformatter.cpp axis/qvalue3daxisformatter.h axis/qvalue3daxisformatter_p.h

            data/abstractitemmodelhandler.cpp data/abstractitemmodelhandler_p.h
            data/qabstract3dseries.cpp data/qabstract3dseries.h data/qabstract3dseries_p.h
            data/qabstractdataproxy.cpp data/qabstractdataproxy.h data/qabstractdataproxy_p.h
            data/qcustom3ditem.cpp data/qcustom3ditem.h data/qcustom3ditem_p.h
            data/qcustom3dlabel.cpp data/qcustom3dlabel.h data/qcustom3dlabel_p.h
            data/qcustom3dvolume.cpp data/qcustom3dvolume.h data/qcustom3dvolume_p.h

            engine/qabstract3dgraph.cpp engine/qabstract3dgraph.h
            engine/q3dscene.cpp engine/q3dscene.h engine/q3dscene_p.h

            global/graphsglobal_p.h
            global/qgraphsglobal.h

            input/q3dinputhandler.cpp input/q3dinputhandler.h input/q3dinputhandler_p.h
            input/qabstract3dinputhandler.cpp input/qabstract3dinputhandler.h input/qabstract3dinputhandler_p.h
            input/qtouch3dinputhandler.cpp input/qtouch3dinputhandler.h input/qtouch3dinputhandler_p.h

            theme/q3dtheme.cpp theme/q3dtheme.h theme/q3dtheme_p.h
            theme/thememanager.cpp theme/thememanager_p.h

            utils/qutils.h
            utils/utils.cpp utils/utils_p.h

            qml/foreigntypes_p.h
            qml/qquickgraphscolor.cpp qml/qquickgraphscolor_p.h
            qml/qquickgraphsitem.cpp qml/qquickgraphsitem_p.h
            qml/qquickgraphstexturedata.cpp qml/qquickgraphstexturedata_p.h
        NO_UNITY_BUILD_SOURCES
            # Symbol collision with, `qtouch3dinputhandler.cpp`
            # TODO: Resolve `rotationSpeed` symbol redefinition
            input/q3dinputhandler.cpp
        INCLUDE_DIRECTORIES
            axis
            data
            engine
            global
            input
            theme
            utils
            qml
    )

    qt_internal_extend_target(Graphs CONDITION QT_FEATURE_graphs_3d_bars3d
        SOURCES
            data/baritemmodelhandler.cpp data/baritemmodelhandler_p.h
            data/qbar3dseries.cpp data/qbar3dseries.h data/qbar3dseries_p.h
            data/qbardataitem.cpp data/qbardataitem.h
            data/qbardataproxy.cpp data/qbardataproxy.h data/qbardataproxy_p.h
            data/qitemmodelbardataproxy.cpp data/qitemmodelbardataproxy.h data/qitemmodelbardataproxy_p.h

            engine/barinstancing.cpp engine/barinstancing_p.h
            engine/q3dbars.cpp engine/q3dbars.h

            qml/foreigntypesbars_p.h
            qml/qquickgraphsbars.cpp qml/qquickgraphsbars_p.h
            qml/qquickgraphsbarsseries.cpp qml/qquickgraphsbarsseries_p.h
        INCLUDE_DIRECTORIES
            data
            engine
            qml
    )

    qt_internal_extend_target(Graphs CONDITION QT_FEATURE_graphs_3d_scatter3d
        SOURCES
            data/qitemmodelscatterdataproxy.cpp data/qitemmodelscatterdataproxy.h data/qitemmodelscatterdataproxy_p.h
            data/qscatter3dseries.cpp data/qscatter3dseries.h data/qscatter3dseries_p.h
            data/qscatterdataitem.cpp data/qscatterdataitem.h
            data/qscatterdataproxy.cpp data/qscatterdataproxy.h data/qscatterdataproxy_p.h
            data/scatteritemmodelhandler.cpp data/scatteritemmodelhandler_p.h

            engine/scatterinstancing.cpp engine/scatterinstancing_p.h
            engine/q3dscatter.cpp engine/q3dscatter.h

            qml/foreigntypesscatter_p.h
            qml/qquickgraphsscatter.cpp qml/qquickgraphsscatter_p.h
            qml/qquickgraphsscatterseries.cpp qml/qquickgraphsscatterseries_p.h
        INCLUDE_DIRECTORIES
            data
            engine
            qml
    )

    qt_internal_extend_target(Graphs CONDITION QT_FEATURE_graphs_3d_surface3d
        SOURCES
            data/qheightmapsurfacedataproxy.cpp data/qheightmapsurfacedataproxy.h data/qheightmapsurfacedataproxy_p.h
            data/qitemmodelsurfacedataproxy.cpp data/qitemmodelsurfacedataproxy.h data/qitemmodelsurfacedataproxy_p.h
            data/qsurface3dseries.cpp data/qsurface3dseries.h data/qsurface3dseries_p.h
            data/qsurfacedataitem.cpp data/qsurfacedataitem.h
            data/qsurfacedataproxy.cpp data/qsurfacedataproxy.h data/qsurfacedataproxy_p.h
            data/surfaceitemmodelhandler.cpp data/surfaceitemmodelhandler_p.h

            engine/q3dsurface.cpp engine/q3dsurface.h
            engine/surfaceselectioninstancing.cpp engine/surfaceselectioninstancing_p.h

            qml/foreigntypessurface_p.h
            qml/qquickgraphssurface.cpp qml/qquickgraphssurface_p.h
            qml/qquickgraphssurfaceseries.cpp qml/qquickgraphssurfaceseries_p.h
        INCLUDE_DIRECTORIES
            data
            engine
            qml
    )

    qt_internal_extend_target(Graphs CONDITION MACOS
        LIBRARIES
            Qt::GuiPrivate
        PUBLIC_LIBRARIES
            Qt::Gui
        PRIVATE_MODULE_INTERFACE
            Qt::GuiPrivate
    )

    add_subdirectory(qml/designer)
endif()

