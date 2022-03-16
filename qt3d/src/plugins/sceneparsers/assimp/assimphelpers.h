/****************************************************************************
**
** Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
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

#ifndef ASSIMPHELPERS_H
#define ASSIMPHELPERS_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

// ASSIMP INCLUDES
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <QtCore/QIODevice>
#include <QtCore/QMap>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {
namespace AssimpHelper {

//CUSTOM FILE STREAM
class AssimpIOStream : public Assimp::IOStream
{
public :
    AssimpIOStream(QIODevice *device);
    ~AssimpIOStream();

    size_t Read(void *pvBuffer, size_t pSize, size_t pCount) override;
    size_t Write(const void *pvBuffer, size_t pSize, size_t pCount) override;
    aiReturn Seek(size_t pOffset, aiOrigin pOrigin) override;
    size_t Tell() const override;
    size_t FileSize() const override;
    void Flush() override;

private:
    QIODevice *const m_device;
};

//CUSTOM FILE IMPORTER TO HANDLE QT RESOURCES WITHIN ASSIMP
class AssimpIOSystem : public Assimp::IOSystem
{
public :
    AssimpIOSystem();
    ~AssimpIOSystem();
    bool Exists(const char *pFile) const override;
    char getOsSeparator() const override;
    Assimp::IOStream *Open(const char *pFile, const char *pMode) override;
    void Close(Assimp::IOStream *pFile) override;

private:
    QMap<QByteArray, QIODevice::OpenMode> m_openModeMaps;
};

} // namespace AssimpHelper
} // namespace Qt3DRender

QT_END_NAMESPACE

#endif // ASSIMPHELPERS_H
