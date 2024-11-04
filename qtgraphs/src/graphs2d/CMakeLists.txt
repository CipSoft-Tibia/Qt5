# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_extend_target(Graphs
    SOURCES
        qgraphsview.cpp qgraphsview_p.h
        qabstractseries.cpp qabstractseries.h qabstractseries_p.h
        charthelpers_p.h
        barchart/qbarseries.cpp barchart/qbarseries.h barchart/qbarseries_p.h
        barchart/qabstractbarseries.cpp barchart/qabstractbarseries.h barchart/qabstractbarseries_p.h
        barchart/qbarset.cpp barchart/qbarset.h barchart/qbarset_p.h
        linechart/qlineseries.cpp linechart/qlineseries.h linechart/qlineseries_p.h
        scatterchart/qscatterseries.cpp scatterchart/qscatterseries.h scatterchart/qscatterseries_p.h
        xychart/qxyseries.cpp xychart/qxyseries.h xychart/qxyseries_p.h
        xychart/qxypoint.cpp xychart/qxypoint_p.h
        axis/qabstractaxis.cpp axis/qabstractaxis.h axis/qabstractaxis_p.h
        axis/barcategoryaxis/qbarcategoryaxis.cpp axis/barcategoryaxis/qbarcategoryaxis.h axis/barcategoryaxis/qbarcategoryaxis_p.h
        axis/axisgrid.cpp axis/axisgrid_p.h
        axis/axisticker.cpp axis/axisticker_p.h
        axis/axisline.cpp axis/axisline_p.h
        axis/valueaxis/qvalueaxis.cpp axis/valueaxis/qvalueaxis.h axis/valueaxis/qvalueaxis_p.h
        themes/qgraphtheme.cpp themes/qgraphtheme.h
        themes/qseriestheme.cpp themes/qseriestheme.h
        qsgrenderer/barsrenderer.cpp qsgrenderer/barsrenderer_p.h
        qsgrenderer/pointrenderer.cpp qsgrenderer/pointrenderer_p.h
        qsgrenderer/axisrenderer.cpp qsgrenderer/axisrenderer_p.h
)

add_subdirectory(qml/designer)
