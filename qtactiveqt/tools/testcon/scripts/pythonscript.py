#!/usr/bin/env python
# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

def QAxWidget2_Click():
    QAxWidget2.lineWidth = QAxWidget2.lineWidth + 1;
    MainWindow.logMacro(0, "Hello from Python: QAxWidget2_Click", 0, "");

def fatLines():
    QAxWidget2.lineWidth = 25;

def thinLines():
    QAxWidget2.lineWidth = 1;

def setLineWidth(width):
    QAxWidget2.lineWidth = width;

def getLineWidth():
    return QAxWidget2.lineWidth;
