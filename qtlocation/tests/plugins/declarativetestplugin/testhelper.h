/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef TESTHELPER_H
#define TESTHELPER_H

#include <QObject>
#include <QSignalSpy>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSysInfo>

QT_BEGIN_NAMESPACE

class TestHelper: public QObject
{
    Q_OBJECT
public:
    TestHelper(QObject *parent = nullptr):QObject(parent){}
    Q_INVOKABLE bool waitForPolished(QQuickItem *item, int timeout = 10000) const
    {
        QSignalSpy spy(item->window(), &QQuickWindow::afterAnimating);
        return spy.wait(timeout);
    }

    Q_INVOKABLE int x86Bits() const
    {
        if ( QSysInfo::currentCpuArchitecture() == "x86_64" )
            return 64;
        else
            return 32;
    }
};

QT_END_NAMESPACE

#endif // TESTHELPER_H
