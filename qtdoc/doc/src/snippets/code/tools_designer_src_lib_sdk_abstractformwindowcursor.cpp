// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
    QDesignerFormWindowInterface *formWindow = 0;
    formWindow = QDesignerFormWindowInterface::findFormWindow(myWidget);

    formWindow->cursor()->setProperty(myWidget, myProperty, newValue);
//! [0]


