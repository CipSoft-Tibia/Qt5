/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef APPXENGINE_H
#define APPXENGINE_H

#include "runnerengine.h"
#include "runner.h"

#include <QtCore/QScopedPointer>
#include <QtCore/QString>

QT_USE_NAMESPACE

struct IAppxManifestReader;
class AppxEnginePrivate;
class AppxEngine : public RunnerEngine
{
public:
    qint64 pid() const override;
    int exitCode() const override;
    QString executable() const override;

protected:
    explicit AppxEngine(Runner *runner, AppxEnginePrivate *dd);
    ~AppxEngine();

    virtual QString extensionSdkPath() const = 0;
    virtual bool installPackage(IAppxManifestReader *reader, const QString &filePath) = 0;

    bool installDependencies();
    bool createPackage(const QString &packageFileName);
    bool sign(const QString &fileName);
    static bool getManifestFile(const QString &fileName, QString *manifest = 0);

    QScopedPointer<AppxEnginePrivate> d_ptr;
    Q_DECLARE_PRIVATE(AppxEngine)
};

#endif // APPXENGINE_H
