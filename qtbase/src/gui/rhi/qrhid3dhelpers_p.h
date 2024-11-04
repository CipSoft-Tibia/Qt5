// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRHID3DHELPERS_P_H
#define QRHID3DHELPERS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <rhi/qrhi.h>

#include <QtGui/qwindow.h>

#include <dxgi1_6.h>
#include <dcomp.h>
#include <d3dcompiler.h>

#if __has_include(<dxcapi.h>)
#include <dxcapi.h>
#define QRHI_D3D12_HAS_DXC
#endif

QT_BEGIN_NAMESPACE

namespace QRhiD3D {

bool output6ForWindow(QWindow *w, IDXGIAdapter1 *adapter, IDXGIOutput6 **result);
bool outputDesc1ForWindow(QWindow *w, IDXGIAdapter1 *adapter, DXGI_OUTPUT_DESC1 *result);
float sdrWhiteLevelInNits(const DXGI_OUTPUT_DESC1 &outputDesc);

pD3DCompile resolveD3DCompile();

IDCompositionDevice *createDirectCompositionDevice();

#ifdef QRHI_D3D12_HAS_DXC
std::pair<IDxcCompiler *, IDxcLibrary *> createDxcCompiler();
#endif

void fillDriverInfo(QRhiDriverInfo *info, const DXGI_ADAPTER_DESC1 &desc);

} // namespace

QT_END_NAMESPACE

#endif
