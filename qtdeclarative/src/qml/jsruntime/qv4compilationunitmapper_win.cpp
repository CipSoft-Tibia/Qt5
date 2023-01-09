/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qv4compilationunitmapper_p.h"

#include "qv4executablecompilationunit_p.h"
#include <QScopeGuard>
#include <QFileInfo>
#include <QDateTime>
#include <qt_windows.h>

QT_BEGIN_NAMESPACE

using namespace QV4;

CompiledData::Unit *CompilationUnitMapper::open(const QString &cacheFileName, const QDateTime &sourceTimeStamp, QString *errorString)
{
    close();

    // ### TODO: fix up file encoding/normalization/unc handling once QFileSystemEntry
    // is exported from QtCore.
    HANDLE handle =
#if defined(Q_OS_WINRT)
        CreateFile2(reinterpret_cast<const wchar_t*>(cacheFileName.constData()),
                   GENERIC_READ | GENERIC_EXECUTE, FILE_SHARE_READ,
                   OPEN_EXISTING, nullptr);
#else
        CreateFile(reinterpret_cast<const wchar_t*>(cacheFileName.constData()),
                   GENERIC_READ | GENERIC_EXECUTE, FILE_SHARE_READ,
                   nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                   nullptr);
#endif
    if (handle == INVALID_HANDLE_VALUE) {
        *errorString = qt_error_string(GetLastError());
        return nullptr;
    }

    auto fileHandleCleanup = qScopeGuard([handle]{
        CloseHandle(handle);
    });

    CompiledData::Unit header;
    DWORD bytesRead;
    if (!ReadFile(handle, reinterpret_cast<char *>(&header), sizeof(header), &bytesRead, nullptr)) {
        *errorString = qt_error_string(GetLastError());
        return nullptr;
    }

    if (bytesRead != sizeof(header)) {
        *errorString = QStringLiteral("File too small for the header fields");
        return nullptr;
    }

    if (!ExecutableCompilationUnit::verifyHeader(&header, sourceTimeStamp, errorString))
        return nullptr;

    // Data structure and qt version matched, so now we can access the rest of the file safely.

    HANDLE fileMappingHandle = CreateFileMapping(handle, 0, PAGE_READONLY, 0, 0, 0);
    if (!fileMappingHandle) {
        *errorString = qt_error_string(GetLastError());
        return nullptr;
    }

    auto mappingCleanup = qScopeGuard([fileMappingHandle]{
        CloseHandle(fileMappingHandle);
    });

    dataPtr = MapViewOfFile(fileMappingHandle, FILE_MAP_READ, 0, 0, 0);
    if (!dataPtr) {
        *errorString = qt_error_string(GetLastError());
        return nullptr;
    }

    return reinterpret_cast<CompiledData::Unit*>(dataPtr);
}

void CompilationUnitMapper::close()
{
    if (dataPtr != nullptr) {
        // Do not unmap cache files that are built with the StaticData flag. That's the majority of
        // them and it's necessary to benefit from the QString literal optimization. There might
        // still be QString instances around that point into that memory area. The memory is backed
        // on the disk, so the kernel is free to release the pages and all that remains is the
        // address space allocation.
        if (!(reinterpret_cast<CompiledData::Unit*>(dataPtr)->flags & CompiledData::Unit::StaticData))
            UnmapViewOfFile(dataPtr);
    }
    dataPtr = nullptr;
}

QT_END_NAMESPACE
