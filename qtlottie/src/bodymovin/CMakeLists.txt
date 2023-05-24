# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_module(BodymovinPrivate
    INTERNAL_MODULE
    SOURCES
        beziereasing.cpp beziereasing_p.h
        bmbase.cpp bmbase_p.h
        bmbasictransform.cpp bmbasictransform_p.h
        bmconstants_p.h
        bmellipse.cpp bmellipse_p.h
        bmfill.cpp bmfill_p.h
        bmfilleffect.cpp bmfilleffect_p.h
        bmfreeformshape.cpp bmfreeformshape_p.h
        bmgfill.cpp bmgfill_p.h
        bmglobal.h
        bmgroup.cpp bmgroup_p.h
        bmimage.cpp bmimage_p.h
        bmimagelayer.cpp bmimagelayer_p.h
        bmlayer.cpp bmlayer_p.h
        bmpathtrimmer.cpp bmpathtrimmer_p.h
        bmproperty_p.h
        bmrect.cpp bmrect_p.h
        bmrepeater.cpp bmrepeater_p.h
        bmrepeatertransform.cpp bmrepeatertransform_p.h
        bmround.cpp bmround_p.h
        bmshape.cpp bmshape_p.h
        bmshapelayer.cpp bmshapelayer_p.h
        bmshapetransform.cpp bmshapetransform_p.h
        bmspatialproperty_p.h
        bmstroke.cpp bmstroke_p.h
        bmtrimpath.cpp bmtrimpath_p.h
        lottierenderer.cpp lottierenderer_p.h
        trimpath.cpp trimpath_p.h
    DEFINES
        BODYMOVIN_LIBRARY
        QT_DEPRECATED_WARNINGS
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::GuiPrivate
)
