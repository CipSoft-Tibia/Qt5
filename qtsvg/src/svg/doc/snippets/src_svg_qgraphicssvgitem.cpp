// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QSvgRenderer *renderer = new QSvgRenderer(QLatin1String("SvgCardDeck.svg"));
QGraphicsSvgItem *black = new QGraphicsSvgItem();
QGraphicsSvgItem *red   = new QGraphicsSvgItem();

black->setSharedRenderer(renderer);
black->setElementId(QLatin1String("black_joker"));

red->setSharedRenderer(renderer);
red->setElementId(QLatin1String("red_joker"));
//! [0]
