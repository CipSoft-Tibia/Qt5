/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QSvgRenderer>

extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size) {
    static int argc = 3;
    static char arg1[] = "fuzzer";
    static char arg2[] = "-platform";
    static char arg3[] = "minimal";
    static char *argv[] = {arg1, arg2, arg3, nullptr};
    static QGuiApplication qga(argc, argv);
    static QImage image(377, 233, QImage::Format_RGB32);
    static QPainter painter(&image);
    QSvgRenderer renderer(QByteArray::fromRawData(Data, Size));
    renderer.render(&painter);
    return 0;
}
