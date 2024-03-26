// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtWayland.Compositor
import QtWayland.Compositor.QtShell

WaylandCompositor {
    id: waylandCompositor

    CompositorScreen { id: screen; compositor: waylandCompositor }

    // Shell surface extension. Needed to provide a window concept for Wayland clients.
    // I.e. requests and events for maximization, minimization, resizing, closing etc.

    QtShell {
        onQtShellSurfaceCreated: screen.handleShellSurface(qtShellSurface)
    }

    // Extension for Input Method (QT_IM_MODULE) support at compositor-side
    QtTextInputMethodManager {}
}
