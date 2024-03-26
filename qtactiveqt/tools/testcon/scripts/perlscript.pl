# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

sub QAxWidget2_Click {
    $QAxWidget2->{'lineWidth'} = $QAxWidget2->{'lineWidth'} + 1;
    $MainWindow->logMacro(0, "Hello from Perl: QAxWidget2_Click", 0, "");
}

sub fatLines
{
    $QAxWidget2->{'lineWidth'} = 25;
}

sub thinLines
{
    $QAxWidget2->{'lineWidth'} = 1;
}

sub setLineWidth(width)
{
    $QAxWidget2->{'lineWidth'} = width;
}

sub getLineWidth()
{
    return $QAxWidget2->{'lineWidth'};
}
