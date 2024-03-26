// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGHWACCEL_D3D11_P_H
#define QFFMPEGHWACCEL_D3D11_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qffmpeghwaccel_p.h"
#include <private/quniquehandle_p.h>
#include <private/qcomptr_p.h>
#include <qt_windows.h>

#include <d3d11.h>
#include <d3d11_1.h>

#if QT_CONFIG(wmf)

QT_BEGIN_NAMESPACE

class QRhi;

namespace QFFmpeg {

struct SharedTextureHandleTraits
{
    using Type = HANDLE;
    static Type invalidValue() { return nullptr; }
    static bool close(Type handle) { return CloseHandle(handle) != 0; }
};

using SharedTextureHandle = QUniqueHandle<SharedTextureHandleTraits>;

/*! \internal Utility class for synchronized transfer of a texture between two D3D devices
 *
 * This class is used to copy a texture from one device to another device. This
 * is implemented using a shared texture, along with keyed mutexes to synchronize
 * access to the texture.
 *
 * This is needed because we need to copy data from FFmpeg to RHI. FFmpeg and RHI
 * uses different D3D devices.
 */
class TextureBridge final
{
public:
    /** Copy a texture slice at position 'index' belonging to device 'dev'
     * into a shared texture */
    bool copyToSharedTex(ID3D11Device *dev, ID3D11DeviceContext *ctx,
                         const ComPtr<ID3D11Texture2D> &tex, UINT index);

    /** Obtain a copy of the texture on a second device 'dev' */
    ComPtr<ID3D11Texture2D> copyFromSharedTex(const ComPtr<ID3D11Device1> &dev,
                                              const ComPtr<ID3D11DeviceContext> &ctx);

private:
    bool ensureDestTex(const ComPtr<ID3D11Device1> &dev);
    bool ensureSrcTex(ID3D11Device *dev, const ComPtr<ID3D11Texture2D> &tex);
    bool isSrcInitialized(const ID3D11Device *dev, const ComPtr<ID3D11Texture2D> &tex) const;
    bool recreateSrc(ID3D11Device *dev, const ComPtr<ID3D11Texture2D> &tex);

    SharedTextureHandle m_sharedHandle{};

    const UINT m_srcKey = 0;
    ComPtr<ID3D11Texture2D> m_srcTex;
    ComPtr<IDXGIKeyedMutex> m_srcMutex;

    const UINT m_destKey = 1;
    ComPtr<ID3D11Texture2D> m_destTex;
    ComPtr<IDXGIKeyedMutex> m_destMutex;

    ComPtr<ID3D11Texture2D> m_outputTex;
};

class D3D11TextureConverter : public TextureConverterBackend
{
public:
    D3D11TextureConverter(QRhi *rhi);

    TextureSet *getTextures(AVFrame *frame) override;

    static void SetupDecoderTextures(AVCodecContext *s);

private:
    ComPtr<ID3D11Device1> m_rhiDevice;
    ComPtr<ID3D11DeviceContext> m_rhiCtx;
    TextureBridge m_bridge;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif

#endif
