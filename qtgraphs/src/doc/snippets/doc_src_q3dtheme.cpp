// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGraphs/Q3DBars>
#include <QtGraphs/Q3DTheme>

int main(int argc, char **argv)
{
    //! [0]
    Q3DTheme *theme = new Q3DTheme(Q3DTheme::Theme::Qt);
    //! [0]

    //! [1]
    Q3DTheme *theme = new Q3DTheme(Q3DTheme::Theme::Qt);
    theme->setBackgroundEnabled(false);
    theme->setLabelBackgroundEnabled(false);
    //! [1]

    //! [2]
    Q3DTheme *theme = new Q3DTheme();
    theme->setAmbientLightStrength(0.3f);
    theme->setBackgroundColor(QColor(QRgb(0x99ca53)));
    theme->setBackgroundEnabled(true);
    QList<QColor> colors = { QColor(QRgb(0x209fdf)) };
    theme->setBaseColors(colors);
    theme->setColorStyle(Q3DTheme::ColorStyle::Uniform);
    theme->setFont(QFont(QStringLiteral("Impact"), 35));
    theme->setGridEnabled(true);
    theme->setGridLineColor(QColor(QRgb(0x99ca53)));
    theme->setLabelBackgroundColor(QColor(0xf6, 0xa6, 0x25, 0xa0));
    theme->setLabelBackgroundEnabled(true);
    theme->setLabelBorderEnabled(true);
    theme->setLabelTextColor(QColor(QRgb(0x404044)));
    theme->setLightColor(Qt::white);
    theme->setLightStrength(6.0f);
    theme->setMultiHighlightColor(QColor(QRgb(0x6d5fd5)));
    theme->setSingleHighlightColor(QColor(QRgb(0xf6a625)));
    theme->setWindowColor(QColor(QRgb(0xffffff)));
    //! [2]

    //! [3]
    Q3DBars *graph = new Q3DBars();
    graph->activeTheme()->setType(Q3DTheme::Theme::PrimaryColors);
    QList<QColor> color = { QColor(Qt::red) };
    graph->activeTheme()->setBaseColors(color);
    graph->activeTheme()->setSingleHighlightColor(Qt::yellow);
    //! [3]
}

//! [4]
Scatter3D {
    ...
    theme: Theme3D { type: Theme3D.Theme.Retro }
    ...
}
//! [4]

//! [5]
Bars3D {
    ...
    theme: Theme3D {
        type: Theme3D.Theme.Retro
        labelBorderEnabled: true
        font.pointSize: 35
        labelBackgroundEnabled: false
    }
    ...
}
//! [5]

//! [6]
Surface3D {
    ...
    theme: Theme3D {
        ambientLightStrength: 0.5
        backgroundColor: "red"
        backgroundEnabled: true
        baseColors: ["blue"]
        colorStyle: Theme3D.ColorStyle.Uniform
        font.family: "Lucida Handwriting"
        font.pointSize: 35
        gridEnabled: false
        gridLineColor: "black"
        labelBackgroundColor: "black"
        labelBackgroundEnabled: true
        labelBorderEnabled: false
        labelTextColor: "white"
        lightColor: "yellow"
        lightStrength: 0.4
        multiHighlightColor: "green"
        singleHighlightColor: "darkRed"
        windowColor: "white"
    }
    ...
}
//! [6]
