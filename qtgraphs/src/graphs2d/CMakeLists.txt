# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_extend_target(Graphs
    SOURCES
        qgraphsview.cpp qgraphsview_p.h
        qabstractseries.cpp qabstractseries.h qabstractseries_p.h
        charthelpers_p.h
        barchart/qbarseries.cpp barchart/qbarseries.h barchart/qbarseries_p.h
        barchart/qbarset.cpp barchart/qbarset.h barchart/qbarset_p.h
        barchart/qbarmodelmapper.cpp barchart/qbarmodelmapper.h barchart/qbarmodelmapper_p.h
        linechart/qlineseries.cpp linechart/qlineseries.h linechart/qlineseries_p.h
        piechart/qpieseries.cpp piechart/qpieseries.h piechart/qpieseries_p.h
        piechart/qpieslice.cpp piechart/qpieslice.h piechart/qpieslice_p.h
        piechart/qpiemodelmapper.cpp piechart/qpiemodelmapper.h piechart/qpiemodelmapper_p.h
        scatterchart/qscatterseries.cpp scatterchart/qscatterseries.h scatterchart/qscatterseries_p.h
        xychart/qxyseries.cpp xychart/qxyseries.h xychart/qxyseries_p.h
        xychart/qxypoint.cpp xychart/qxypoint_p.h
        xychart/qxymodelmapper.cpp xychart/qxymodelmapper.h xychart/qxymodelmapper_p.h
        axis/qabstractaxis.cpp axis/qabstractaxis.h axis/qabstractaxis_p.h
        axis/barcategoryaxis/qbarcategoryaxis.cpp axis/barcategoryaxis/qbarcategoryaxis.h axis/barcategoryaxis/qbarcategoryaxis_p.h
        axis/axisgrid.cpp axis/axisgrid_p.h
        axis/axisticker.cpp axis/axisticker_p.h
        axis/axisline.cpp axis/axisline_p.h
        axis/valueaxis/qvalueaxis.cpp axis/valueaxis/qvalueaxis.h axis/valueaxis/qvalueaxis_p.h
        qsgrenderer/barsrenderer.cpp qsgrenderer/barsrenderer_p.h
        qsgrenderer/pointrenderer.cpp qsgrenderer/pointrenderer_p.h
        qsgrenderer/axisrenderer.cpp qsgrenderer/axisrenderer_p.h
        qsgrenderer/pierenderer.cpp qsgrenderer/pierenderer_p.h
        qsgrenderer/arearenderer.cpp qsgrenderer/arearenderer_p.h
        splinechart/qsplineseries.cpp splinechart/qsplineseries.h splinechart/qsplineseries_p.h
        areachart/qareaseries.cpp areachart/qareaseries.h areachart/qareaseries_p.h
        animation/qgraphanimation.cpp animation/qgraphanimation_p.h
        animation/qsplinecontrolanimation.cpp animation/qsplinecontrolanimation_p.h
        animation/qgraphpointanimation.cpp animation/qgraphpointanimation_p.h
        animation/qgraphtransition_p.h animation/qgraphtransition.cpp
        animation/qxyseriesanimation_p.h animation/qxyseriesanimation.cpp
        axis/datetimeaxis/qdatetimeaxis.h axis/datetimeaxis/qdatetimeaxis.cpp axis/datetimeaxis/qdatetimeaxis_p.h
)

add_subdirectory(qml/designer)
