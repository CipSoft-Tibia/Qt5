/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:FDL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Free Documentation License Usage
** Alternatively, this file may be used under the terms of the GNU Free
** Documentation License version 1.3 as published by the Free Software
** Foundation and appearing in the file included in the packaging of
** this file. Please review the following information to ensure
** the GNU Free Documentation License version 1.3 requirements
** will be met: https://www.gnu.org/licenses/fdl-1.3.html.
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.12
import QtQuick.Controls 2.12

//! [contentItem]
Tumbler {
    id: tumbler

    contentItem: PathView {
        id: pathView
        model: tumbler.model
        delegate: tumbler.delegate
        clip: true
        pathItemCount: tumbler.visibleItemCount + 1
        preferredHighlightBegin: 0.5
        preferredHighlightEnd: 0.5
        dragMargin: width / 2

        path: Path {
            startX: pathView.width / 2
            startY: -pathView.delegateHeight / 2
            PathLine {
                x: pathView.width / 2
                y: pathView.pathItemCount * pathView.delegateHeight - pathView.delegateHeight / 2
            }
        }

        property real delegateHeight: tumbler.availableHeight / tumbler.visibleItemCount
    }
}
//! [contentItem]
