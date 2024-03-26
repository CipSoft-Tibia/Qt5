// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtextureimage.h"
#include "qabstracttextureimage.h"
#include "qtextureimage_p.h"
#include "qtextureimagedata_p.h"
#include "qtexturedata.h"
#include "qtexture.h"
#include "qtexture_p.h"
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <QtCore/QBuffer>
#include <qendian.h>
#include <Qt3DCore/private/qscene_p.h>
#include <Qt3DCore/qaspectengine.h>
#include <Qt3DCore/private/qdownloadhelperservice_p.h>
#include <Qt3DCore/private/qurlhelper_p.h>
#include <Qt3DRender/private/qrenderaspect_p.h>
#include <Qt3DRender/private/nodemanagers_p.h>
#include <Qt3DRender/private/managers_p.h>
#include <Qt3DRender/private/texture_p.h>
#include <qmath.h>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {

namespace {

struct DdsPixelFormat
{
    quint32 size;
    quint32 flags;
    quint32 fourCC;
    quint32 rgbBitCount;
    quint32 redMask;
    quint32 greenMask;
    quint32 blueMask;
    quint32 alphaMask;
};

struct DdsHeader
{
    char magic[4];
    quint32 size;
    quint32 flags;
    quint32 height;
    quint32 width;
    quint32 pitchOrLinearSize;
    quint32 depth;
    quint32 mipmapCount;
    quint32 reserved[11];
    DdsPixelFormat pixelFormat;
    quint32 caps;
    quint32 caps2;
    quint32 caps3;
    quint32 caps4;
    quint32 reserved2;
};

struct DdsDX10Header
{
    quint32 format;
    quint32 dimension;
    quint32 miscFlag;
    quint32 arraySize;
    quint32 miscFlags2;
};

enum DdsFlags
{
    MipmapCountFlag             = 0x20000,
};

enum PixelFormatFlag
{
    AlphaFlag                   = 0x1,
    FourCCFlag                  = 0x4,
    RGBFlag                     = 0x40,
    RGBAFlag                    = RGBFlag | AlphaFlag,
    YUVFlag                     = 0x200,
    LuminanceFlag               = 0x20000
};

enum Caps2Flags
{
    CubemapFlag                 = 0x200,
    CubemapPositiveXFlag        = 0x400,
    CubemapNegativeXFlag        = 0x800,
    CubemapPositiveYFlag        = 0x1000,
    CubemapNegativeYFlag        = 0x2000,
    CubemapPositiveZFlag        = 0x4000,
    CubemapNegativeZFlag        = 0x8000,
    AllCubemapFaceFlags         = CubemapPositiveXFlag |
    CubemapNegativeXFlag |
    CubemapPositiveYFlag |
    CubemapNegativeYFlag |
    CubemapPositiveZFlag |
    CubemapNegativeZFlag,
    VolumeFlag                  = 0x200000
};

enum DXGIFormat
{
    DXGI_FORMAT_UNKNOWN                     = 0,
    DXGI_FORMAT_R32G32B32A32_TYPELESS       = 1,
    DXGI_FORMAT_R32G32B32A32_FLOAT          = 2,
    DXGI_FORMAT_R32G32B32A32_UINT           = 3,
    DXGI_FORMAT_R32G32B32A32_SINT           = 4,
    DXGI_FORMAT_R32G32B32_TYPELESS          = 5,
    DXGI_FORMAT_R32G32B32_FLOAT             = 6,
    DXGI_FORMAT_R32G32B32_UINT              = 7,
    DXGI_FORMAT_R32G32B32_SINT              = 8,
    DXGI_FORMAT_R16G16B16A16_TYPELESS       = 9,
    DXGI_FORMAT_R16G16B16A16_FLOAT          = 10,
    DXGI_FORMAT_R16G16B16A16_UNORM          = 11,
    DXGI_FORMAT_R16G16B16A16_UINT           = 12,
    DXGI_FORMAT_R16G16B16A16_SNORM          = 13,
    DXGI_FORMAT_R16G16B16A16_SINT           = 14,
    DXGI_FORMAT_R32G32_TYPELESS             = 15,
    DXGI_FORMAT_R32G32_FLOAT                = 16,
    DXGI_FORMAT_R32G32_UINT                 = 17,
    DXGI_FORMAT_R32G32_SINT                 = 18,
    DXGI_FORMAT_R32G8X24_TYPELESS           = 19,
    DXGI_FORMAT_D32_FLOAT_S8X24_UINT        = 20,
    DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS    = 21,
    DXGI_FORMAT_X32_TYPELESS_G8X24_UINT     = 22,
    DXGI_FORMAT_R10G10B10A2_TYPELESS        = 23,
    DXGI_FORMAT_R10G10B10A2_UNORM           = 24,
    DXGI_FORMAT_R10G10B10A2_UINT            = 25,
    DXGI_FORMAT_R11G11B10_FLOAT             = 26,
    DXGI_FORMAT_R8G8B8A8_TYPELESS           = 27,
    DXGI_FORMAT_R8G8B8A8_UNORM              = 28,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB         = 29,
    DXGI_FORMAT_R8G8B8A8_UINT               = 30,
    DXGI_FORMAT_R8G8B8A8_SNORM              = 31,
    DXGI_FORMAT_R8G8B8A8_SINT               = 32,
    DXGI_FORMAT_R16G16_TYPELESS             = 33,
    DXGI_FORMAT_R16G16_FLOAT                = 34,
    DXGI_FORMAT_R16G16_UNORM                = 35,
    DXGI_FORMAT_R16G16_UINT                 = 36,
    DXGI_FORMAT_R16G16_SNORM                = 37,
    DXGI_FORMAT_R16G16_SINT                 = 38,
    DXGI_FORMAT_R32_TYPELESS                = 39,
    DXGI_FORMAT_D32_FLOAT                   = 40,
    DXGI_FORMAT_R32_FLOAT                   = 41,
    DXGI_FORMAT_R32_UINT                    = 42,
    DXGI_FORMAT_R32_SINT                    = 43,
    DXGI_FORMAT_R24G8_TYPELESS              = 44,
    DXGI_FORMAT_D24_UNORM_S8_UINT           = 45,
    DXGI_FORMAT_R24_UNORM_X8_TYPELESS       = 46,
    DXGI_FORMAT_X24_TYPELESS_G8_UINT        = 47,
    DXGI_FORMAT_R8G8_TYPELESS               = 48,
    DXGI_FORMAT_R8G8_UNORM                  = 49,
    DXGI_FORMAT_R8G8_UINT                   = 50,
    DXGI_FORMAT_R8G8_SNORM                  = 51,
    DXGI_FORMAT_R8G8_SINT                   = 52,
    DXGI_FORMAT_R16_TYPELESS                = 53,
    DXGI_FORMAT_R16_FLOAT                   = 54,
    DXGI_FORMAT_D16_UNORM                   = 55,
    DXGI_FORMAT_R16_UNORM                   = 56,
    DXGI_FORMAT_R16_UINT                    = 57,
    DXGI_FORMAT_R16_SNORM                   = 58,
    DXGI_FORMAT_R16_SINT                    = 59,
    DXGI_FORMAT_R8_TYPELESS                 = 60,
    DXGI_FORMAT_R8_UNORM                    = 61,
    DXGI_FORMAT_R8_UINT                     = 62,
    DXGI_FORMAT_R8_SNORM                    = 63,
    DXGI_FORMAT_R8_SINT                     = 64,
    DXGI_FORMAT_A8_UNORM                    = 65,
    DXGI_FORMAT_R1_UNORM                    = 66,
    DXGI_FORMAT_R9G9B9E5_SHAREDEXP          = 67,
    DXGI_FORMAT_R8G8_B8G8_UNORM             = 68,
    DXGI_FORMAT_G8R8_G8B8_UNORM             = 69,
    DXGI_FORMAT_BC1_TYPELESS                = 70,
    DXGI_FORMAT_BC1_UNORM                   = 71,
    DXGI_FORMAT_BC1_UNORM_SRGB              = 72,
    DXGI_FORMAT_BC2_TYPELESS                = 73,
    DXGI_FORMAT_BC2_UNORM                   = 74,
    DXGI_FORMAT_BC2_UNORM_SRGB              = 75,
    DXGI_FORMAT_BC3_TYPELESS                = 76,
    DXGI_FORMAT_BC3_UNORM                   = 77,
    DXGI_FORMAT_BC3_UNORM_SRGB              = 78,
    DXGI_FORMAT_BC4_TYPELESS                = 79,
    DXGI_FORMAT_BC4_UNORM                   = 80,
    DXGI_FORMAT_BC4_SNORM                   = 81,
    DXGI_FORMAT_BC5_TYPELESS                = 82,
    DXGI_FORMAT_BC5_UNORM                   = 83,
    DXGI_FORMAT_BC5_SNORM                   = 84,
    DXGI_FORMAT_B5G6R5_UNORM                = 85,
    DXGI_FORMAT_B5G5R5A1_UNORM              = 86,
    DXGI_FORMAT_B8G8R8A8_UNORM              = 87,
    DXGI_FORMAT_B8G8R8X8_UNORM              = 88,
    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM  = 89,
    DXGI_FORMAT_B8G8R8A8_TYPELESS           = 90,
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB         = 91,
    DXGI_FORMAT_B8G8R8X8_TYPELESS           = 92,
    DXGI_FORMAT_B8G8R8X8_UNORM_SRGB         = 93,
    DXGI_FORMAT_BC6H_TYPELESS               = 94,
    DXGI_FORMAT_BC6H_UF16                   = 95,
    DXGI_FORMAT_BC6H_SF16                   = 96,
    DXGI_FORMAT_BC7_TYPELESS                = 97,
    DXGI_FORMAT_BC7_UNORM                   = 98,
    DXGI_FORMAT_BC7_UNORM_SRGB              = 99,
    DXGI_FORMAT_AYUV                        = 100,
    DXGI_FORMAT_Y410                        = 101,
    DXGI_FORMAT_Y416                        = 102,
    DXGI_FORMAT_NV12                        = 103,
    DXGI_FORMAT_P010                        = 104,
    DXGI_FORMAT_P016                        = 105,
    DXGI_FORMAT_420_OPAQUE                  = 106,
    DXGI_FORMAT_YUY2                        = 107,
    DXGI_FORMAT_Y210                        = 108,
    DXGI_FORMAT_Y216                        = 109,
    DXGI_FORMAT_NV11                        = 110,
    DXGI_FORMAT_AI44                        = 111,
    DXGI_FORMAT_IA44                        = 112,
    DXGI_FORMAT_P8                          = 113,
    DXGI_FORMAT_A8P8                        = 114,
    DXGI_FORMAT_B4G4R4A4_UNORM              = 115,
    DXGI_FORMAT_P208                        = 130,
    DXGI_FORMAT_V208                        = 131,
    DXGI_FORMAT_V408                        = 132,
};

template <int a, int b, int c, int d>
struct DdsFourCC
{
    static const quint32 value = a | (b << 8) | (c << 16) | (d << 24);
};

struct FormatInfo
{
    QOpenGLTexture::PixelFormat pixelFormat;
    QOpenGLTexture::TextureFormat textureFormat;
    QOpenGLTexture::PixelType pixelType;
    int components;
    bool compressed;
};

const struct RGBAFormat
{
    quint32 redMask;
    quint32 greenMask;
    quint32 blueMask;
    quint32 alphaMask;
    FormatInfo formatInfo;

} rgbaFormats[] = {
    // unorm formats
{ 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000,   { QOpenGLTexture::RGBA,             QOpenGLTexture::RGBA8_UNorm,    QOpenGLTexture::UInt8,           4, false } },
{ 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000,   { QOpenGLTexture::BGRA,             QOpenGLTexture::RGBA8_UNorm,    QOpenGLTexture::UInt8,           4, false } },
{ 0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000,   { QOpenGLTexture::RGBA,             QOpenGLTexture::RGBA8_UNorm,    QOpenGLTexture::UInt8,           4, false } },
{ 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000,   { QOpenGLTexture::BGRA,             QOpenGLTexture::RGBA8_UNorm,    QOpenGLTexture::UInt8,           4, false } },
{ 0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000,   { QOpenGLTexture::RGB,              QOpenGLTexture::RGB8_UNorm,     QOpenGLTexture::UInt8,           3, false } },
{ 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000,   { QOpenGLTexture::BGR,              QOpenGLTexture::RGB8_UNorm,     QOpenGLTexture::UInt8,           3, false } },

// packed formats
{ 0x0000f800, 0x000007e0, 0x0000001f, 0x00000000,   { QOpenGLTexture::RGB,              QOpenGLTexture::R5G6B5,         QOpenGLTexture::UInt16_R5G6B5,   2, false } },
{ 0x00007c00, 0x000003e0, 0x0000001f, 0x00008000,   { QOpenGLTexture::RGBA,             QOpenGLTexture::RGB5A1,         QOpenGLTexture::UInt16_RGB5A1,   2, false } },
{ 0x00000f00, 0x000000f0, 0x0000000f, 0x0000f000,   { QOpenGLTexture::RGBA,             QOpenGLTexture::RGBA4,          QOpenGLTexture::UInt16_RGBA4,    2, false } },
{ 0x000000e0, 0x0000001c, 0x00000003, 0x00000000,   { QOpenGLTexture::RGB,              QOpenGLTexture::RG3B2,          QOpenGLTexture::UInt8_RG3B2,     1, false } },
{ 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000,   { QOpenGLTexture::RGBA,             QOpenGLTexture::RGB10A2,        QOpenGLTexture::UInt32_RGB10A2,  4, false } },

// luminance alpha
{ 0x000000ff, 0x000000ff, 0x000000ff, 0x00000000,   { QOpenGLTexture::Red,              QOpenGLTexture::R8_UNorm,       QOpenGLTexture::UInt8,           1, false } },
{ 0x000000ff, 0x00000000, 0x00000000, 0x00000000,   { QOpenGLTexture::Red,              QOpenGLTexture::R8_UNorm,       QOpenGLTexture::UInt8,           1, false } },
{ 0x000000ff, 0x000000ff, 0x000000ff, 0x0000ff00,   { QOpenGLTexture::RG,               QOpenGLTexture::RG8_UNorm,      QOpenGLTexture::UInt8,           2, false } },
{ 0x000000ff, 0x00000000, 0x00000000, 0x0000ff00,   { QOpenGLTexture::RG,               QOpenGLTexture::RG8_UNorm,      QOpenGLTexture::UInt8,           2, false } },
};

const struct FourCCFormat
{
    quint32 fourCC;
    FormatInfo formatInfo;
} fourCCFormats[] = {
{ DdsFourCC<'D','X','T','1'>::value,                { QOpenGLTexture::NoSourceFormat,   QOpenGLTexture::RGBA_DXT1,      QOpenGLTexture::NoPixelType,     8, true } },
{ DdsFourCC<'D','X','T','3'>::value,                { QOpenGLTexture::NoSourceFormat,   QOpenGLTexture::RGBA_DXT3,      QOpenGLTexture::NoPixelType,    16, true } },
{ DdsFourCC<'D','X','T','5'>::value,                { QOpenGLTexture::NoSourceFormat,   QOpenGLTexture::RGBA_DXT5,      QOpenGLTexture::NoPixelType,    16, true } },
{ DdsFourCC<'A','T','I','1'>::value,                { QOpenGLTexture::NoSourceFormat,   QOpenGLTexture::R_ATI1N_UNorm,  QOpenGLTexture::NoPixelType,     8, true } },
{ DdsFourCC<'A','T','I','2'>::value,                { QOpenGLTexture::NoSourceFormat,   QOpenGLTexture::RG_ATI2N_UNorm, QOpenGLTexture::NoPixelType,    16, true } },
{ /* DXGI_FORMAT_R16_FLOAT */         111,          { QOpenGLTexture::Red,              QOpenGLTexture::R16F,           QOpenGLTexture::Float16,        2,  false } },
{ /* DXGI_FORMAT_R16_FLOAT */         112,          { QOpenGLTexture::RG,               QOpenGLTexture::RG16F,          QOpenGLTexture::Float16,        4,  false } },
{ /* DXGI_FORMAT_R16G16B16A16_FLOAT */113,          { QOpenGLTexture::RGBA,             QOpenGLTexture::RGBA16F,        QOpenGLTexture::Float16,        8,  false } },
{ /* DXGI_FORMAT_R32_FLOAT */         114,          { QOpenGLTexture::Red,              QOpenGLTexture::R32F,           QOpenGLTexture::Float32,        4,  false } },
{ /* DXGI_FORMAT_R32G32_FLOAT */      115,          { QOpenGLTexture::RG,               QOpenGLTexture::RG32F,          QOpenGLTexture::Float32,        8,  false } },
{ /* DXGI_FORMAT_R32G32B32A32_FLOAT */116,          { QOpenGLTexture::RGBA,             QOpenGLTexture::RGBA32F,        QOpenGLTexture::Float32,        16, false } }
};

const struct DX10Format
{
    DXGIFormat dxgiFormat;
    FormatInfo formatInfo;
} dx10Formats[] = {
    // unorm formats
{ DXGI_FORMAT_R8_UNORM,                             { QOpenGLTexture::Red,              QOpenGLTexture::R8_UNorm,       QOpenGLTexture::UInt8,           1, false } },
{ DXGI_FORMAT_R8G8_UNORM,                           { QOpenGLTexture::RG,               QOpenGLTexture::RG8_UNorm,      QOpenGLTexture::UInt8,           2, false } },
{ DXGI_FORMAT_R8G8B8A8_UNORM,                       { QOpenGLTexture::RGBA,             QOpenGLTexture::RGBA8_UNorm,    QOpenGLTexture::UInt8,           4, false } },

{ DXGI_FORMAT_R16_UNORM,                            { QOpenGLTexture::Red,              QOpenGLTexture::R16_UNorm,      QOpenGLTexture::UInt16,          2, false } },
{ DXGI_FORMAT_R16G16_UNORM,                         { QOpenGLTexture::RG,               QOpenGLTexture::RG16_UNorm,     QOpenGLTexture::UInt16,          4, false } },
{ DXGI_FORMAT_R16G16B16A16_UNORM,                   { QOpenGLTexture::RGBA,             QOpenGLTexture::RGBA16_UNorm,   QOpenGLTexture::UInt16,          8, false } },

// snorm formats
{ DXGI_FORMAT_R8_SNORM,                             { QOpenGLTexture::Red,              QOpenGLTexture::R8_SNorm,       QOpenGLTexture::Int8,            1, false } },
{ DXGI_FORMAT_R8G8_SNORM,                           { QOpenGLTexture::RG,               QOpenGLTexture::RG8_SNorm,      QOpenGLTexture::Int8,            2, false } },
{ DXGI_FORMAT_R8G8B8A8_SNORM,                       { QOpenGLTexture::RGBA,             QOpenGLTexture::RGBA8_SNorm,    QOpenGLTexture::Int8,            4, false } },

{ DXGI_FORMAT_R16_SNORM,                            { QOpenGLTexture::Red,              QOpenGLTexture::R16_SNorm,      QOpenGLTexture::Int16,           2, false } },
{ DXGI_FORMAT_R16G16_SNORM,                         { QOpenGLTexture::RG,               QOpenGLTexture::RG16_SNorm,     QOpenGLTexture::Int16,           4, false } },
{ DXGI_FORMAT_R16G16B16A16_SNORM,                   { QOpenGLTexture::RGBA,             QOpenGLTexture::RGBA16_SNorm,   QOpenGLTexture::Int16,           8, false } },

// unsigned integer formats
{ DXGI_FORMAT_R8_UINT,                              { QOpenGLTexture::Red_Integer,      QOpenGLTexture::R8U,            QOpenGLTexture::UInt8,           1, false } },
{ DXGI_FORMAT_R8G8_UINT,                            { QOpenGLTexture::RG_Integer,       QOpenGLTexture::RG8U,           QOpenGLTexture::UInt8,           2, false } },
{ DXGI_FORMAT_R8G8B8A8_UINT,                        { QOpenGLTexture::RGBA_Integer,     QOpenGLTexture::RGBA8U,         QOpenGLTexture::UInt8,           4, false } },

{ DXGI_FORMAT_R16_UINT,                             { QOpenGLTexture::Red_Integer,      QOpenGLTexture::R16U,           QOpenGLTexture::UInt16,          2, false } },
{ DXGI_FORMAT_R16G16_UINT,                          { QOpenGLTexture::RG_Integer,       QOpenGLTexture::RG16U,          QOpenGLTexture::UInt16,          4, false } },
{ DXGI_FORMAT_R16G16B16A16_UINT,                    { QOpenGLTexture::RGBA_Integer,     QOpenGLTexture::RGBA16U,        QOpenGLTexture::UInt16,          8, false } },

{ DXGI_FORMAT_R32_UINT,                             { QOpenGLTexture::Red_Integer,      QOpenGLTexture::R32U,           QOpenGLTexture::UInt32,          4, false } },
{ DXGI_FORMAT_R32G32_UINT,                          { QOpenGLTexture::RG_Integer,       QOpenGLTexture::RG32U,          QOpenGLTexture::UInt32,          8, false } },
{ DXGI_FORMAT_R32G32B32_UINT,                       { QOpenGLTexture::RGB_Integer,      QOpenGLTexture::RGB32U,         QOpenGLTexture::UInt32,         12, false } },
{ DXGI_FORMAT_R32G32B32A32_UINT,                    { QOpenGLTexture::RGBA_Integer,     QOpenGLTexture::RGBA32U,        QOpenGLTexture::UInt32,         16, false } },

// signed integer formats
{ DXGI_FORMAT_R8_SINT,                              { QOpenGLTexture::Red_Integer,      QOpenGLTexture::R8I,            QOpenGLTexture::Int8,            1, false } },
{ DXGI_FORMAT_R8G8_SINT,                            { QOpenGLTexture::RG_Integer,       QOpenGLTexture::RG8I,           QOpenGLTexture::Int8,            2, false } },
{ DXGI_FORMAT_R8G8B8A8_SINT,                        { QOpenGLTexture::RGBA_Integer,     QOpenGLTexture::RGBA8I,         QOpenGLTexture::Int8,            4, false } },

{ DXGI_FORMAT_R16_SINT,                             { QOpenGLTexture::Red_Integer,      QOpenGLTexture::R16I,           QOpenGLTexture::Int16,           2, false } },
{ DXGI_FORMAT_R16G16_SINT,                          { QOpenGLTexture::RG_Integer,       QOpenGLTexture::RG16I,          QOpenGLTexture::Int16,           4, false } },
{ DXGI_FORMAT_R16G16B16A16_SINT,                    { QOpenGLTexture::RGBA_Integer,     QOpenGLTexture::RGBA16I,        QOpenGLTexture::Int16,           8, false } },

{ DXGI_FORMAT_R32_SINT,                             { QOpenGLTexture::Red_Integer,      QOpenGLTexture::R32I,           QOpenGLTexture::Int32,           4, false } },
{ DXGI_FORMAT_R32G32_SINT,                          { QOpenGLTexture::RG_Integer,       QOpenGLTexture::RG32I,          QOpenGLTexture::Int32,           8, false } },
{ DXGI_FORMAT_R32G32B32_SINT,                       { QOpenGLTexture::RGB_Integer,      QOpenGLTexture::RGB32I,         QOpenGLTexture::Int32,          12, false } },
{ DXGI_FORMAT_R32G32B32A32_SINT,                    { QOpenGLTexture::RGBA_Integer,     QOpenGLTexture::RGBA32I,        QOpenGLTexture::Int32,          16, false } },

// floating formats
{ DXGI_FORMAT_R16_FLOAT,                            { QOpenGLTexture::Red,              QOpenGLTexture::R16F,           QOpenGLTexture::Float16,         2, false } },
{ DXGI_FORMAT_R16G16_FLOAT,                         { QOpenGLTexture::RG,               QOpenGLTexture::RG16F,          QOpenGLTexture::Float16,         4, false } },
{ DXGI_FORMAT_R16G16B16A16_FLOAT,                   { QOpenGLTexture::RGBA,             QOpenGLTexture::RGBA16F,        QOpenGLTexture::Float16,         8, false } },

{ DXGI_FORMAT_R32_FLOAT,                            { QOpenGLTexture::Red,              QOpenGLTexture::R32F,           QOpenGLTexture::Float32,         4, false } },
{ DXGI_FORMAT_R32G32_FLOAT,                         { QOpenGLTexture::RG,               QOpenGLTexture::RG32F,          QOpenGLTexture::Float32,         8, false } },
{ DXGI_FORMAT_R32G32B32_FLOAT,                      { QOpenGLTexture::RGB,              QOpenGLTexture::RGB32F,         QOpenGLTexture::Float32,        12, false } },
{ DXGI_FORMAT_R32G32B32A32_FLOAT,                   { QOpenGLTexture::RGBA,             QOpenGLTexture::RGBA32F,        QOpenGLTexture::Float32,        16, false } },

// sRGB formats
{ DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,                  { QOpenGLTexture::RGB,              QOpenGLTexture::SRGB8,          QOpenGLTexture::UInt8,           4, false } },
{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,                  { QOpenGLTexture::RGBA,             QOpenGLTexture::SRGB8_Alpha8,   QOpenGLTexture::UInt8,           4, false } },

// packed formats
// { DXGI_FORMAT_R10G10B10A2_UNORM,                 { QOpenGLTexture::RGB10A2_UNORM, QOpenGLTexture::RGBA, QOpenGLTexture::UInt32_RGB10A2, 4, false } },
{ DXGI_FORMAT_R10G10B10A2_UINT,                     { QOpenGLTexture::RGBA_Integer,     QOpenGLTexture::RGB10A2,        QOpenGLTexture::UInt32_RGB10A2,  4, false } },
{ DXGI_FORMAT_R9G9B9E5_SHAREDEXP,                   { QOpenGLTexture::RGB,              QOpenGLTexture::RGB9E5,         QOpenGLTexture::UInt32_RGB9_E5,  4, false } },
{ DXGI_FORMAT_R11G11B10_FLOAT,                      { QOpenGLTexture::RGB,              QOpenGLTexture::RG11B10F,       QOpenGLTexture::UInt32_RG11B10F, 4, false } },
{ DXGI_FORMAT_B5G6R5_UNORM,                         { QOpenGLTexture::RGB,              QOpenGLTexture::R5G6B5,         QOpenGLTexture::UInt16_R5G6B5,   2, false } },
{ DXGI_FORMAT_B5G5R5A1_UNORM,                       { QOpenGLTexture::RGBA,             QOpenGLTexture::RGB5A1,         QOpenGLTexture::UInt16_RGB5A1,   2, false } },
{ DXGI_FORMAT_B4G4R4A4_UNORM,                       { QOpenGLTexture::RGBA,             QOpenGLTexture::RGBA4,          QOpenGLTexture::UInt16_RGBA4,    2, false } },

// swizzle formats
{ DXGI_FORMAT_B8G8R8X8_UNORM,                       { QOpenGLTexture::BGRA,             QOpenGLTexture::RGB8_UNorm,     QOpenGLTexture::UInt8,           4, false } },
{ DXGI_FORMAT_B8G8R8A8_UNORM,                       { QOpenGLTexture::BGRA,             QOpenGLTexture::RGBA8_UNorm,    QOpenGLTexture::UInt8,           4, false } },
{ DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,                  { QOpenGLTexture::BGRA,             QOpenGLTexture::SRGB8,          QOpenGLTexture::UInt8,           4, false } },
{ DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,                  { QOpenGLTexture::BGRA,             QOpenGLTexture::SRGB8_Alpha8,   QOpenGLTexture::UInt8,           4, false } },

// luminance alpha formats
{ DXGI_FORMAT_R8_UNORM,                             { QOpenGLTexture::Red,              QOpenGLTexture::R8_UNorm,       QOpenGLTexture::UInt8,           1, false } },
{ DXGI_FORMAT_R8G8_UNORM,                           { QOpenGLTexture::RG,               QOpenGLTexture::RG8_UNorm,      QOpenGLTexture::UInt8,           2, false } },
{ DXGI_FORMAT_R16_UNORM,                            { QOpenGLTexture::Red,              QOpenGLTexture::R16_UNorm,      QOpenGLTexture::UInt16,          2, false } },
{ DXGI_FORMAT_R16G16_UNORM,                         { QOpenGLTexture::RG,               QOpenGLTexture::RG16_UNorm,     QOpenGLTexture::UInt16,          4, false } },

// depth formats
{ DXGI_FORMAT_D16_UNORM,                            { QOpenGLTexture::Depth,            QOpenGLTexture::D16,            QOpenGLTexture::NoPixelType,     2, false } },
{ DXGI_FORMAT_D24_UNORM_S8_UINT,                    { QOpenGLTexture::DepthStencil,     QOpenGLTexture::D24S8,          QOpenGLTexture::NoPixelType,     4, false } },
{ DXGI_FORMAT_D32_FLOAT,                            { QOpenGLTexture::Depth,            QOpenGLTexture::D32F,           QOpenGLTexture::NoPixelType,     4, false } },
{ DXGI_FORMAT_D32_FLOAT_S8X24_UINT,                 { QOpenGLTexture::DepthStencil,     QOpenGLTexture::D32FS8X24,      QOpenGLTexture::NoPixelType,     8, false } },

// compressed formats
{ DXGI_FORMAT_BC1_UNORM,                            { QOpenGLTexture::NoSourceFormat,   QOpenGLTexture::RGBA_DXT1,      QOpenGLTexture::NoPixelType,     8, true } },
{ DXGI_FORMAT_BC2_UNORM,                            { QOpenGLTexture::NoSourceFormat,   QOpenGLTexture::RGBA_DXT3,      QOpenGLTexture::NoPixelType,    16, true } },
{ DXGI_FORMAT_BC3_UNORM,                            { QOpenGLTexture::NoSourceFormat,   QOpenGLTexture::RGBA_DXT5,      QOpenGLTexture::NoPixelType,    16, true } },
{ DXGI_FORMAT_BC4_UNORM,                            { QOpenGLTexture::NoSourceFormat,   QOpenGLTexture::R_ATI1N_UNorm,  QOpenGLTexture::NoPixelType,     8, true } },
{ DXGI_FORMAT_BC4_SNORM,                            { QOpenGLTexture::NoSourceFormat,   QOpenGLTexture::R_ATI1N_SNorm,  QOpenGLTexture::NoPixelType,     8, true } },
{ DXGI_FORMAT_BC5_UNORM,                            { QOpenGLTexture::NoSourceFormat,   QOpenGLTexture::RG_ATI2N_UNorm, QOpenGLTexture::NoPixelType,    16, true } },
{ DXGI_FORMAT_BC5_SNORM,                            { QOpenGLTexture::NoSourceFormat,   QOpenGLTexture::RG_ATI2N_SNorm, QOpenGLTexture::NoPixelType,    16, true } },
{ DXGI_FORMAT_BC6H_UF16,                            { QOpenGLTexture::NoSourceFormat,   QOpenGLTexture::RGB_BP_UNSIGNED_FLOAT, QOpenGLTexture::NoPixelType, 16, true } },
{ DXGI_FORMAT_BC6H_SF16,                            { QOpenGLTexture::NoSourceFormat,   QOpenGLTexture::RGB_BP_SIGNED_FLOAT, QOpenGLTexture::NoPixelType, 16, true } },
{ DXGI_FORMAT_BC7_UNORM,                            { QOpenGLTexture::NoSourceFormat,   QOpenGLTexture::RGB_BP_UNorm, QOpenGLTexture::NoPixelType,      16, true } },

// compressed sRGB formats
{ DXGI_FORMAT_BC1_UNORM_SRGB,                       { QOpenGLTexture::NoSourceFormat,   QOpenGLTexture::SRGB_DXT1,      QOpenGLTexture::NoPixelType,     8, true } },
{ DXGI_FORMAT_BC1_UNORM_SRGB,                       { QOpenGLTexture::NoSourceFormat,   QOpenGLTexture::SRGB_Alpha_DXT1, QOpenGLTexture::NoPixelType,    8, true } },
{ DXGI_FORMAT_BC2_UNORM_SRGB,                       { QOpenGLTexture::NoSourceFormat,   QOpenGLTexture::SRGB_Alpha_DXT3, QOpenGLTexture::NoPixelType,   16, true } },
{ DXGI_FORMAT_BC3_UNORM_SRGB,                       { QOpenGLTexture::NoSourceFormat,   QOpenGLTexture::SRGB_Alpha_DXT5, QOpenGLTexture::NoPixelType,   16, true } },
{ DXGI_FORMAT_BC7_UNORM_SRGB,                       { QOpenGLTexture::NoSourceFormat,   QOpenGLTexture::SRGB_BP_UNorm,  QOpenGLTexture::NoPixelType,    16, true } },
};

struct PkmHeader
{
    char magic[4];
    char version[2];
    quint16 textureType;
    quint16 paddedWidth;
    quint16 paddedHeight;
    quint16 width;
    quint16 height;
};

enum ImageFormat {
    GenericImageFormat = 0,
    DDS,
    PKM,
    HDR,
    KTX
};

ImageFormat imageFormatFromSuffix(const QString &suffix)
{
    if (suffix == QStringLiteral("pkm"))
        return PKM;
    if (suffix == QStringLiteral("dds"))
        return DDS;
    if (suffix == QStringLiteral("hdr"))
        return HDR;
    if (suffix == QStringLiteral("ktx"))
        return KTX;
    return GenericImageFormat;
}

// NOTE: the ktx loading code is a near-duplication of the code in qt3d-runtime, and changes
// should be kept up to date in both locations.
quint32 blockSizeForTextureFormat(QOpenGLTexture::TextureFormat format)
{
    switch (format) {
    case QOpenGLTexture::RGB8_ETC1:
    case QOpenGLTexture::RGB8_ETC2:
    case QOpenGLTexture::SRGB8_ETC2:
    case QOpenGLTexture::RGB8_PunchThrough_Alpha1_ETC2:
    case QOpenGLTexture::SRGB8_PunchThrough_Alpha1_ETC2:
    case QOpenGLTexture::R11_EAC_UNorm:
    case QOpenGLTexture::R11_EAC_SNorm:
    case QOpenGLTexture::RGB_DXT1:
        return 8;

    default:
        return 16;
    }
}

QTextureImageDataPtr setKtxFile(QIODevice *source)
{
    static const int KTX_IDENTIFIER_LENGTH = 12;
    static const char ktxIdentifier[KTX_IDENTIFIER_LENGTH] = { '\xAB', 'K', 'T', 'X', ' ', '1', '1', '\xBB', '\r', '\n', '\x1A', '\n' };
    static const quint32 platformEndianIdentifier = 0x04030201;
    static const quint32 inversePlatformEndianIdentifier = 0x01020304;

    struct KTXHeader {
        quint8 identifier[KTX_IDENTIFIER_LENGTH];
        quint32 endianness;
        quint32 glType;
        quint32 glTypeSize;
        quint32 glFormat;
        quint32 glInternalFormat;
        quint32 glBaseInternalFormat;
        quint32 pixelWidth;
        quint32 pixelHeight;
        quint32 pixelDepth;
        quint32 numberOfArrayElements;
        quint32 numberOfFaces;
        quint32 numberOfMipmapLevels;
        quint32 bytesOfKeyValueData;
    };

    KTXHeader header;
    QTextureImageDataPtr imageData;
    if (source->read(reinterpret_cast<char *>(&header), sizeof(header)) != sizeof(header)
            || qstrncmp(reinterpret_cast<char *>(header.identifier), ktxIdentifier, KTX_IDENTIFIER_LENGTH) != 0
            || (header.endianness != platformEndianIdentifier && header.endianness != inversePlatformEndianIdentifier))
    {
        return imageData;
    }

    const bool isInverseEndian = (header.endianness == inversePlatformEndianIdentifier);
    auto decode = [isInverseEndian](quint32 val) {
        return isInverseEndian ? qbswap<quint32>(val) : val;
    };

    const bool isCompressed = decode(header.glType) == 0 && decode(header.glFormat) == 0 && decode(header.glTypeSize) == 1;
    if (!isCompressed) {
        qWarning("Uncompressed ktx texture data is not supported");
        return imageData;
    }

    if (decode(header.numberOfArrayElements) != 0) {
        qWarning("Array ktx textures not supported");
        return imageData;
    }

    if (decode(header.pixelDepth) != 0) {
        qWarning("Only 2D and cube ktx textures are supported");
        return imageData;
    }

    const int bytesToSkip = decode(header.bytesOfKeyValueData);
    if (source->read(bytesToSkip).size() != bytesToSkip) {
        qWarning("Unexpected end of ktx data");
        return imageData;
    }

    const int level0Width = decode(header.pixelWidth);
    const int level0Height = decode(header.pixelHeight);
    const int faceCount = decode(header.numberOfFaces);
    const int mipMapLevels = decode(header.numberOfMipmapLevels);
    const QOpenGLTexture::TextureFormat format = QOpenGLTexture::TextureFormat(decode(header.glInternalFormat));
    const int blockSize = blockSizeForTextureFormat(format);

    // now for each mipmap level we have (arrays and 3d textures not supported here)
    // uint32 imageSize
    // for each array element
    //   for each face
    //     for each z slice
    //       compressed data
    //     padding so that each face data starts at an offset that is a multiple of 4
    // padding so that each imageSize starts at an offset that is a multiple of 4

    // assumes no depth or uncompressed textures (per above)
    auto computeMipMapLevelSize = [&] (int level) {
        const int w = qMax(level0Width >> level, 1);
        const int h = qMax(level0Height >> level, 1);
        return ((w + 3) / 4) * ((h + 3) / 4) * blockSize;
    };

    int dataSize = 0;
    for (auto i = 0; i < mipMapLevels; ++i)
        dataSize += computeMipMapLevelSize(i) * faceCount + 4; // assumes a single layer (per above)

    const QByteArray rawData = source->read(dataSize);
    if (rawData.size() < dataSize) {
        qWarning() << "Unexpected end of data in" << source;
        return imageData;
    }

    if (!source->atEnd())
        qWarning() << "Unrecognized data in" << source;

    imageData = QTextureImageDataPtr::create();
    imageData->setTarget(faceCount == 6 ? QOpenGLTexture::TargetCubeMap : QOpenGLTexture::Target2D);
    imageData->setFormat(format);
    imageData->setWidth(level0Width);
    imageData->setHeight(level0Height);
    imageData->setLayers(1);
    imageData->setDepth(1);
    imageData->setFaces(faceCount);
    imageData->setMipLevels(mipMapLevels);
    imageData->setPixelFormat(QOpenGLTexture::NoSourceFormat);
    imageData->setPixelType(QOpenGLTexture::NoPixelType);
    imageData->setData(rawData, blockSize, true);
    QTextureImageDataPrivate::get(imageData.data())->m_isKtx = true; // see note in QTextureImageDataPrivate

    return imageData;
}

QTextureImageDataPtr setPkmFile(QIODevice *source)
{
    QTextureImageDataPtr imageData;

    PkmHeader header;
    if ((source->read(reinterpret_cast<char *>(&header), sizeof header) != sizeof header)
            || (qstrncmp(header.magic, "PKM ", 4) != 0))
        return imageData;

    QOpenGLTexture::TextureFormat format = QOpenGLTexture::NoFormat;
    int blockSize = 0;

    if (header.version[0] == '2' && header.version[1] == '0') {
        switch (qFromBigEndian(header.textureType)) {
        case 0:
            format = QOpenGLTexture::RGB8_ETC1;
            blockSize = 8;
            break;

        case 1:
            format = QOpenGLTexture::RGB8_ETC2;
            blockSize = 8;
            break;

        case 3:
            format = QOpenGLTexture::RGBA8_ETC2_EAC;
            blockSize = 16;
            break;

        case 4:
            format = QOpenGLTexture::RGB8_PunchThrough_Alpha1_ETC2;
            blockSize = 8;
            break;

        case 5:
            format = QOpenGLTexture::R11_EAC_UNorm;
            blockSize = 8;
            break;

        case 6:
            format = QOpenGLTexture::RG11_EAC_UNorm;
            blockSize = 16;
            break;

        case 7:
            format = QOpenGLTexture::R11_EAC_SNorm;
            blockSize = 8;
            break;

        case 8:
            format = QOpenGLTexture::RG11_EAC_SNorm;
            blockSize = 16;
            break;
        }
    } else {
        format = QOpenGLTexture::RGB8_ETC1;
        blockSize = 8;
    }

    if (format == QOpenGLTexture::NoFormat) {
        qWarning() << "Unrecognized compression format in" << source;
        return imageData;
    }

    // get the extended (multiple of 4) width and height
    const int width = qFromBigEndian(header.paddedWidth);
    const int height = qFromBigEndian(header.paddedHeight);

    const QByteArray data = source->readAll();
    if (data.size() != (width / 4) * (height / 4) * blockSize) {
        qWarning() << "Unexpected data size in" << source;
        return imageData;
    }

    imageData = QTextureImageDataPtr::create();
    imageData->setTarget(QOpenGLTexture::Target2D);
    imageData->setFormat(format);
    imageData->setWidth(width);
    imageData->setHeight(height);
    imageData->setLayers(1);
    imageData->setDepth(1);
    imageData->setFaces(1);
    imageData->setMipLevels(1);
    imageData->setPixelFormat(QOpenGLTexture::NoSourceFormat);
    imageData->setPixelType(QOpenGLTexture::NoPixelType);
    imageData->setData(data, blockSize, true);

    return imageData;
}

QTextureImageDataPtr setDdsFile(QIODevice *source)
{
    QTextureImageDataPtr imageData;

    DdsHeader header;
    if ((source->read(reinterpret_cast<char *>(&header), sizeof header) != sizeof header)
            || (qstrncmp(header.magic, "DDS ", 4) != 0))
        return imageData;

    int layers = 1;
    const quint32 pixelFlags = qFromLittleEndian(header.pixelFormat.flags);
    const quint32 fourCC = qFromLittleEndian(header.pixelFormat.fourCC);
    const FormatInfo *formatInfo = nullptr;

    if ((pixelFlags & FourCCFlag) == FourCCFlag) {
        if (fourCC == DdsFourCC<'D', 'X', '1', '0'>::value) {
            // DX10 texture
            DdsDX10Header dx10Header;
            if (source->read(reinterpret_cast<char *>(&dx10Header), sizeof dx10Header) != sizeof dx10Header)
                return imageData;

            layers = qFromLittleEndian(dx10Header.arraySize);
            DXGIFormat format = static_cast<DXGIFormat>(qFromLittleEndian(dx10Header.format));

            for (const auto &info : dx10Formats) {
                if (info.dxgiFormat == format) {
                    formatInfo = &info.formatInfo;
                    break;
                }
            }
        } else {
            // compressed, FourCC texture
            for (const auto &info : fourCCFormats) {
                if (info.fourCC == fourCC) {
                    formatInfo = &info.formatInfo;
                    break;
                }
            }
        }
    } else {
        // uncompressed texture
        const quint32 rgbBitCount   = qFromLittleEndian(header.pixelFormat.rgbBitCount);
        const quint32 redMask       = qFromLittleEndian(header.pixelFormat.redMask);
        const quint32 greenMask     = qFromLittleEndian(header.pixelFormat.greenMask);
        const quint32 blueMask      = qFromLittleEndian(header.pixelFormat.blueMask);
        const quint32 alphaMask     = qFromLittleEndian(header.pixelFormat.alphaMask);

        for (const auto &info : rgbaFormats) {
            if (info.formatInfo.components * 8u == rgbBitCount &&
                    info.redMask == redMask && info.greenMask == greenMask &&
                    info.blueMask == blueMask && info.alphaMask == alphaMask) {
                formatInfo = &info.formatInfo;
                break;
            }
        }
    }

    if (formatInfo == nullptr) {
        qWarning() << "Unrecognized pixel format in" << source;
        return imageData;
    }

    // target
    // XXX should worry about Target1D?
    QOpenGLTexture::Target target;
    const int width = qFromLittleEndian(header.width);
    const int height = qFromLittleEndian(header.height);
    const quint32 caps2Flags = qFromLittleEndian(header.caps2);
    const int blockSize = formatInfo->components;
    const bool isCompressed = formatInfo->compressed;
    const int mipLevelCount = ((qFromLittleEndian(header.flags) & MipmapCountFlag) == MipmapCountFlag) ? qFromLittleEndian(header.mipmapCount) : 1;
    int depth;
    int faces;

    if ((caps2Flags & VolumeFlag) == VolumeFlag) {
        target = QOpenGLTexture::Target3D;
        depth = qFromLittleEndian(header.depth);
        faces = 1;
    } else if ((caps2Flags & CubemapFlag) == CubemapFlag) {
        target = layers > 1 ? QOpenGLTexture::TargetCubeMapArray : QOpenGLTexture::TargetCubeMap;
        depth = 1;
        faces = qPopulationCount(caps2Flags & AllCubemapFaceFlags);
    } else {
        target = layers > 1 ? QOpenGLTexture::Target2DArray : QOpenGLTexture::Target2D;
        depth = 1;
        faces = 1;
    }

    int layerSize = 0;
    int tmpSize = 0;

    auto computeMipMapLevelSize = [&] (int level) {
        const int w = qMax(width >> level, 1);
        const int h = qMax(height >> level, 1);
        const int d = qMax(depth >> level, 1);

        if (isCompressed)
            return ((w + 3) / 4) * ((h + 3) / 4) * blockSize * d;
        else
            return w * h * blockSize * d;
    };

    for (auto i = 0; i < mipLevelCount; ++i)
        tmpSize += computeMipMapLevelSize(i);

    layerSize = faces * tmpSize;

    // data
    const int dataSize = layers * layerSize;

    const QByteArray data = source->read(dataSize);
    if (data.size() < dataSize) {
        qWarning() << "Unexpected end of data in" << source;
        return imageData;
    }

    if (!source->atEnd())
        qWarning() << "Unrecognized data in" << source;

    imageData = QTextureImageDataPtr::create();
    imageData->setData(data,blockSize, isCompressed);

    // target
    imageData->setTarget(target);

    // mip levels
    imageData->setMipLevels(mipLevelCount);

    // texture format
    imageData->setFormat(formatInfo->textureFormat);
    imageData->setPixelType(formatInfo->pixelType);
    imageData->setPixelFormat(formatInfo->pixelFormat);

    // dimensions
    imageData->setLayers(layers);
    imageData->setDepth(depth);
    imageData->setWidth(width);
    imageData->setHeight(height);
    imageData->setFaces(faces);

    return imageData;
}

// Loads Radiance RGBE images into RGBA32F image data. RGBA is chosen over RGB
// because this allows passing such images to compute shaders (image2D).
QTextureImageDataPtr setHdrFile(QIODevice *source)
{
    QTextureImageDataPtr imageData;
    char sig[256];
    source->read(sig, 11);
    if (strncmp(sig, "#?RADIANCE\n", 11))
        return imageData;

    QByteArray buf = source->readAll();
    const char *p = buf.constData();
    const char *pEnd = p + buf.size();

    // Process lines until the empty one.
    QByteArray line;
    while (p < pEnd) {
        char c = *p++;
        if (c == '\n') {
            if (line.isEmpty())
                break;
            if (line.startsWith(QByteArrayLiteral("FORMAT="))) {
                const QByteArray format = line.mid(7).trimmed();
                if (format != QByteArrayLiteral("32-bit_rle_rgbe")) {
                    qWarning("HDR format '%s' is not supported", format.constData());
                    return imageData;
                }
            }
            line.clear();
        } else {
            line.append(c);
        }
    }
    if (p == pEnd) {
        qWarning("Malformed HDR image data at property strings");
        return imageData;
    }

    // Get the resolution string.
    while (p < pEnd) {
        char c = *p++;
        if (c == '\n')
            break;
        line.append(c);
    }
    if (p == pEnd) {
        qWarning("Malformed HDR image data at resolution string");
        return imageData;
    }

    int w = 0, h = 0;
    // We only care about the standard orientation.
    if (!sscanf(line.constData(), "-Y %d +X %d", &h, &w)) {
        qWarning("Unsupported HDR resolution string '%s'", line.constData());
        return imageData;
    }
    if (w <= 0 || h <= 0) {
        qWarning("Invalid HDR resolution");
        return imageData;
    }

    const QOpenGLTexture::TextureFormat textureFormat = QOpenGLTexture::RGBA32F;
    const QOpenGLTexture::PixelFormat pixelFormat = QOpenGLTexture::RGBA;
    const QOpenGLTexture::PixelType pixelType = QOpenGLTexture::Float32;
    const int blockSize = 4 * sizeof(float);
    QByteArray data;
    data.resize(w * h * blockSize);

    typedef unsigned char RGBE[4];
    RGBE *scanline = new RGBE[w];

    for (int y = 0; y < h; ++y) {
        if (pEnd - p < 4) {
            qWarning("Unexpected end of HDR data");
            delete[] scanline;
            return imageData;
        }

        scanline[0][0] = *p++;
        scanline[0][1] = *p++;
        scanline[0][2] = *p++;
        scanline[0][3] = *p++;

        if (scanline[0][0] == 2 && scanline[0][1] == 2 && scanline[0][2] < 128) {
            // new rle, the first pixel was a dummy
            for (int channel = 0; channel < 4; ++channel) {
                for (int x = 0; x < w && p < pEnd; ) {
                    unsigned char c = *p++;
                    if (c > 128) { // run
                        if (p < pEnd) {
                            int repCount = c & 127;
                            c = *p++;
                            while (repCount--)
                                scanline[x++][channel] = c;
                        }
                    } else { // not a run
                        while (c-- && p < pEnd)
                            scanline[x++][channel] = *p++;
                    }
                }
            }
        } else {
            // old rle
            scanline[0][0] = 2;
            int bitshift = 0;
            int x = 1;
            while (x < w && pEnd - p >= 4) {
                scanline[x][0] = *p++;
                scanline[x][1] = *p++;
                scanline[x][2] = *p++;
                scanline[x][3] = *p++;

                if (scanline[x][0] == 1 && scanline[x][1] == 1 && scanline[x][2] == 1) { // run
                    int repCount = scanline[x][3] << bitshift;
                    while (repCount--) {
                        memcpy(scanline[x], scanline[x - 1], 4);
                        ++x;
                    }
                    bitshift += 8;
                } else { // not a run
                    ++x;
                    bitshift = 0;
                }
            }
        }

        // adjust for -Y orientation
        float *fp = reinterpret_cast<float *>(data.data() + (h - 1 - y) * blockSize * w);
        for (int x = 0; x < w; ++x) {
            float d = qPow(2.0f, float(scanline[x][3]) - 128.0f);
            // r, g, b, a
            *fp++ = scanline[x][0] / 256.0f * d;
            *fp++ = scanline[x][1] / 256.0f * d;
            *fp++ = scanline[x][2] / 256.0f * d;
            *fp++ = 1.0f;
        }
    }

    delete[] scanline;

    imageData = QTextureImageDataPtr::create();
    imageData->setTarget(QOpenGLTexture::Target2D);
    imageData->setFormat(textureFormat);
    imageData->setWidth(w);
    imageData->setHeight(h);
    imageData->setLayers(1);
    imageData->setDepth(1);
    imageData->setFaces(1);
    imageData->setMipLevels(1);
    imageData->setPixelFormat(pixelFormat);
    imageData->setPixelType(pixelType);
    imageData->setData(data, blockSize, false);

    return imageData;
}

} // anonynous

QTextureImageDataPtr TextureLoadingHelper::loadTextureData(const QUrl &url, bool allow3D, bool mirrored)
{
    QTextureImageDataPtr textureData;
    if (url.isLocalFile() || url.scheme() == QLatin1String("qrc")
#ifdef Q_OS_ANDROID
            || url.scheme() == QLatin1String("assets")
#endif
            ) {
        const QString source = Qt3DCore::QUrlHelper::urlToLocalFileOrQrc(url);
        QFile f(source);
        if (!f.open(QIODevice::ReadOnly))
            qWarning() << "Failed to open" << source;
        else
            textureData = loadTextureData(&f, QFileInfo(source).suffix().toLower(), allow3D, mirrored);
    }
    return textureData;
}

QTextureImageDataPtr TextureLoadingHelper::loadTextureData(QIODevice *data, const QString& suffix,
                                                           bool allow3D, bool mirrored)
{
    QTextureImageDataPtr textureData;
    ImageFormat fmt = imageFormatFromSuffix(suffix);
    switch (fmt) {
    case DDS:
        textureData = setDdsFile(data);
        break;
    case PKM:
        textureData = setPkmFile(data);
        break;
    case HDR:
        textureData = setHdrFile(data);
        break;
    case KTX: {
        textureData = setKtxFile(data);
        break;
    }
    default: {
        QImage img;
        if (img.load(data, suffix.toLatin1())) {
            textureData = QTextureImageDataPtr::create();
            textureData->setImage(mirrored ? img.mirrored() : img);
        } else {
            qWarning() << "Failed to load textureImage data using QImage";
        }
        break;
    }
    }

    if (!allow3D && textureData && (textureData->layers() > 1 || textureData->depth() > 1))
        qWarning() << "Texture data has a 3rd dimension which wasn't expected";
    return textureData;
}

QTextureDataPtr QTextureFromSourceGenerator::operator ()()
{
    QTextureDataPtr generatedData = QTextureDataPtr::create();
    QTextureImageDataPtr textureData;

    // Note: First and Second call can be seen as operator() being called twice
    // on the same object but actually call 2 will be made on a new generator
    // which is the copy of the generator used for call 1 but with m_sourceData
    // set.
    // This is required because updating the same functor wouldn't be picked up
    // by the backend texture sharing system.
    if (!Qt3DCore::QDownloadHelperService::isLocal(m_url)) {
        if (m_sourceData.isEmpty()) {
            // first time around, trigger a download
            if (m_texture) {
                auto downloadService = Qt3DCore::QDownloadHelperService::getService(m_engine);
                Qt3DCore::QDownloadRequestPtr request(new TextureDownloadRequest(sharedFromThis(),
                                                                                 m_url,
                                                                                 m_engine,
                                                                                 m_texture));
                downloadService->submitRequest(request);
            }
            return generatedData;
        }

        // second time around, we have the data
        QT_PREPEND_NAMESPACE(QBuffer) buffer(&m_sourceData);
        if (buffer.open(QIODevice::ReadOnly)) {
            QString suffix = m_url.toString();
            suffix = suffix.right(suffix.size() - suffix.lastIndexOf(QLatin1Char('.')));

            QStringList ext(suffix);

            QMimeDatabase db;
            QMimeType mtype = db.mimeTypeForData(m_sourceData);
            if (mtype.isValid()) {
                ext << mtype.suffixes();
            }

            for (const QString &s: std::as_const(ext)) {
                textureData = TextureLoadingHelper::loadTextureData(&buffer, s, true, m_mirrored);
                if (textureData && textureData->data().size() > 0)
                    break;
            }
        }
    } else {
        textureData = TextureLoadingHelper::loadTextureData(m_url, true, m_mirrored);
    }

    // Update any properties explicitly set by the user
    if (textureData && m_format != QAbstractTexture::NoFormat && m_format != QAbstractTexture::Automatic)
        textureData->setFormat(static_cast<QOpenGLTexture::TextureFormat>(m_format));

    if (textureData && textureData->data().size() > 0) {
        generatedData->setTarget(static_cast<QAbstractTexture::Target>(textureData->target()));
        generatedData->setFormat(static_cast<QAbstractTexture::TextureFormat>(textureData->format()));
        generatedData->setWidth(textureData->width());
        generatedData->setHeight(textureData->height());
        generatedData->setDepth(textureData->depth());
        generatedData->setLayers(textureData->layers());
        generatedData->addImageData(textureData);
    }

    return generatedData;
}

TextureDownloadRequest::TextureDownloadRequest(const QTextureFromSourceGeneratorPtr &functor,
                                               const QUrl &source,
                                               Qt3DCore::QAspectEngine *engine,
                                               Qt3DCore::QNodeId texNodeId)
    : Qt3DCore::QDownloadRequest(source)
    , m_functor(functor)
    , m_engine(engine)
    , m_texNodeId(texNodeId)
{

}

// Executed in main thread
void TextureDownloadRequest::onCompleted()
{
    if (cancelled() || !succeeded())
        return;

    QRenderAspectPrivate* d_aspect = QRenderAspectPrivate::findPrivate(m_engine);
    if (!d_aspect)
        return;

    Render::TextureManager *textureManager = d_aspect->m_nodeManagers->textureManager();
    Render::Texture *texture = textureManager->lookupResource(m_texNodeId);
    if (texture == nullptr)
        return;

    QTextureFromSourceGeneratorPtr oldGenerator = qSharedPointerCast<QTextureFromSourceGenerator>(texture->dataGenerator());

    // Set raw data on functor so that it can really load something
    oldGenerator->m_sourceData = m_data;

    // Mark the texture as dirty so that the functor runs again with the downloaded data
    texture->addDirtyFlag(Render::Texture::DirtyDataGenerator);
}

/*!
    \class Qt3DRender::QTexture1D
    \inheaderfile Qt3DRender/QTexture
    \inmodule Qt3DRender
    \since 5.5
    \brief A QAbstractTexture with a Target1D target format.
 */
/*!
    \qmltype Texture1D
    \instantiates Qt3DRender::QTexture1D
    \inqmlmodule Qt3D.Render
    \since 5.5
    \brief An AbstractTexture with a Target1D target format.
 */

/*!
    Constructs a new Qt3DRender::QTexture1D instance with \a parent as parent.
 */
QTexture1D::QTexture1D(QNode *parent)
    : QAbstractTexture(Target1D, parent)
{
}

/*! \internal */
QTexture1D::~QTexture1D()
{
}

/*!
    \class Qt3DRender::QTexture1DArray
    \inheaderfile Qt3DRender/QTexture
    \inmodule Qt3DRender
    \since 5.5
    \brief A QAbstractTexture with a Target1DArray target format.
 */
/*!
    \qmltype Texture1DArray
    \instantiates Qt3DRender::QTexture1DArray
    \inqmlmodule Qt3D.Render
    \since 5.5
    \brief An AbstractTexture with a Target1DArray target format.
 */

/*!
    Constructs a new Qt3DRender::QTexture1DArray instance with \a parent as parent.
 */
QTexture1DArray::QTexture1DArray(QNode *parent)
    : QAbstractTexture(Target1DArray, parent)
{
}

/*! \internal */
QTexture1DArray::~QTexture1DArray()
{
}

/*!
    \class Qt3DRender::QTexture2D
    \inheaderfile Qt3DRender/QTexture
    \inmodule Qt3DRender
    \since 5.5
    \brief A QAbstractTexture with a Target2D target format.
 */
/*!
    \qmltype Texture2D
    \instantiates Qt3DRender::QTexture2D
    \inqmlmodule Qt3D.Render
    \since 5.5
    \brief An AbstractTexture with a Target2D target format.
 */

/*!
    Constructs a new Qt3DRender::QTexture2D instance with \a parent as parent.
 */
QTexture2D::QTexture2D(QNode *parent)
    : QAbstractTexture(Target2D, parent)
{
}

/*! \internal */
QTexture2D::~QTexture2D()
{
}

/*!
    \class Qt3DRender::QTexture2DArray
    \inheaderfile Qt3DRender/QTexture
    \inmodule Qt3DRender
    \since 5.5
    \brief A QAbstractTexture with a Target2DArray target format.
 */
/*!
    \qmltype Texture2DArray
    \instantiates Qt3DRender::QTexture2DArray
    \inqmlmodule Qt3D.Render
    \since 5.5
    \brief An AbstractTexture with a Target2DArray target format.
 */

/*!
    Constructs a new Qt3DRender::QTexture2DArray instance with \a parent as parent.
 */
QTexture2DArray::QTexture2DArray(QNode *parent)
    : QAbstractTexture(Target2DArray, parent)
{
}

/*! \internal */
QTexture2DArray::~QTexture2DArray()
{
}

/*!
    \class Qt3DRender::QTexture3D
    \inheaderfile Qt3DRender/QTexture
    \inmodule Qt3DRender
    \since 5.5
    \brief A QAbstractTexture with a Target3D target format.
 */
/*!
    \qmltype Texture3D
    \instantiates Qt3DRender::QTexture3D
    \inqmlmodule Qt3D.Render
    \since 5.5
    \brief An AbstractTexture with a Target3D target format.
 */

/*!
    Constructs a new Qt3DRender::QTexture3D instance with \a parent as parent.
 */
QTexture3D::QTexture3D(QNode *parent)
    : QAbstractTexture(Target3D, parent)
{
}

/*! \internal */
QTexture3D::~QTexture3D()
{
}

/*!
    \class Qt3DRender::QTextureCubeMap
    \inheaderfile Qt3DRender/QTexture
    \inmodule Qt3DRender
    \since 5.5
    \brief A QAbstractTexture with a TargetCubeMap target format.
 */
/*!
    \qmltype TextureCubeMap
    \instantiates Qt3DRender::QTextureCubeMap
    \inqmlmodule Qt3D.Render
    \since 5.5
    \brief An AbstractTexture with a TargetCubeMap target format.
 */

/*!
    Constructs a new Qt3DRender::QTextureCubeMap instance with \a parent as parent.
 */
QTextureCubeMap::QTextureCubeMap(QNode *parent)
    : QAbstractTexture(TargetCubeMap, parent)
{
}

/*! \internal */
QTextureCubeMap::~QTextureCubeMap()
{
}

/*!
    \class Qt3DRender::QTextureCubeMapArray
    \inheaderfile Qt3DRender/QTexture
    \inmodule Qt3DRender
    \since 5.5
    \brief A QAbstractTexture with a TargetCubeMapArray target format.
 */
/*!
    \qmltype TextureCubeMapArray
    \instantiates Qt3DRender::QTextureCubeMapArray
    \inqmlmodule Qt3D.Render
    \since 5.5
    \brief An AbstractTexture with a TargetCubeMapArray target format.
 */

/*!
    Constructs a new Qt3DRender::QTextureCubeMapArray instance with \a parent as parent.
 */
QTextureCubeMapArray::QTextureCubeMapArray(QNode *parent)
    : QAbstractTexture(TargetCubeMapArray, parent)
{
}

/*! \internal */
QTextureCubeMapArray::~QTextureCubeMapArray()
{
}

/*!
    \class Qt3DRender::QTexture2DMultisample
    \inheaderfile Qt3DRender/QTexture
    \inmodule Qt3DRender
    \since 5.5
    \brief A QAbstractTexture with a Target2DMultisample target format.
 */
/*!
    \qmltype Texture2DMultisample
    \instantiates Qt3DRender::QTexture2DMultisample
    \inqmlmodule Qt3D.Render
    \since 5.5
    \brief An AbstractTexture with a Target2DMultisample target format.
 */

/*!
    Constructs a new Qt3DRender::QTexture2DMultisample instance with \a parent as parent.
 */
QTexture2DMultisample::QTexture2DMultisample(QNode *parent)
    : QAbstractTexture(Target2DMultisample, parent)
{
}

/*! \internal */
QTexture2DMultisample::~QTexture2DMultisample()
{
}

/*!
    \class Qt3DRender::QTexture2DMultisampleArray
    \inheaderfile Qt3DRender/QTexture
    \inmodule Qt3DRender
    \since 5.5
    \brief A QAbstractTexture with a Target2DMultisampleArray target format.
 */
/*!
    \qmltype Texture2DMultisampleArray
    \instantiates Qt3DRender::QTexture2DMultisampleArray
    \inqmlmodule Qt3D.Render
    \since 5.5
    \brief An AbstractTexture with a Target2DMultisampleArray target format.
 */

/*!
    Constructs a new Qt3DRender::QTexture2DMultisampleArray instance with \a parent as parent.
 */
QTexture2DMultisampleArray::QTexture2DMultisampleArray(QNode *parent)
    : QAbstractTexture(Target2DMultisampleArray, parent)
{
}

/*! \internal */
QTexture2DMultisampleArray::~QTexture2DMultisampleArray()
{
}

/*!
    \class Qt3DRender::QTextureRectangle
    \inheaderfile Qt3DRender/QTexture
    \inmodule Qt3DRender
    \since 5.5
    \brief A QAbstractTexture with a TargetRectangle target format.
 */
/*!
    \qmltype TextureRectangle
    \instantiates Qt3DRender::QTextureRectangle
    \inqmlmodule Qt3D.Render
    \since 5.5
    \brief An AbstractTexture with a TargetRectangle target format.
 */

/*!
    Constructs a new Qt3DRender::QTextureRectangle instance with \a parent as parent.
 */
QTextureRectangle::QTextureRectangle(QNode *parent)
    : QAbstractTexture(TargetRectangle, parent)
{
}

/*! \internal */
QTextureRectangle::~QTextureRectangle()
{
}

/*!
    \class Qt3DRender::QTextureBuffer
    \inheaderfile Qt3DRender/QTexture
    \inmodule Qt3DRender
    \since 5.5
    \brief A QAbstractTexture with a TargetBuffer target format.
 */
/*!
    \qmltype TextureBuffer
    \instantiates Qt3DRender::QTextureBuffer
    \inqmlmodule Qt3D.Render
    \since 5.5
    \brief An AbstractTexture with a TargetBuffer target format.
 */

/*!
    Constructs a new Qt3DRender::QTextureBuffer instance with \a parent as parent.
 */
QTextureBuffer::QTextureBuffer(QNode *parent)
    : QAbstractTexture(TargetBuffer, parent)
{
}

/*! \internal */
QTextureBuffer::~QTextureBuffer()
{
}

QTextureLoaderPrivate::QTextureLoaderPrivate()
    : QAbstractTexturePrivate()
    , m_mirrored(true)
{
}

void QTextureLoaderPrivate::setScene(Qt3DCore::QScene *scene)
{
    QAbstractTexturePrivate::setScene(scene);
    updateGenerator();
}

void QTextureLoaderPrivate::updateGenerator()
{
    Q_Q(QTextureLoader);
    Qt3DCore::QAspectEngine *engine = m_scene ? m_scene->engine() : nullptr;
    setDataFunctor(QTextureFromSourceGeneratorPtr::create(q, engine, m_id));
}

/*!
   \class Qt3DRender::QTextureLoader
   \inheaderfile Qt3DRender/QTexture
   \inmodule Qt3DRender
   \brief Handles the texture loading and setting the texture's properties.
*/
/*!
   \qmltype TextureLoader
   \instantiates Qt3DRender::QTextureLoader
   \inqmlmodule Qt3D.Render

   \brief Handles the texture loading and setting the texture's properties.
*/
/*!
 * Constructs a new Qt3DRender::QTextureLoader instance with \a parent as parent.
 *
 * Note that by default, if not contradicted by the file metadata, the loaded texture
 * will have the following properties set:
 *  - wrapMode set to Repeat
 *  - minificationFilter set to LinearMipMapLinear
 *  - magnificationFilter set to Linear
 *  - generateMipMaps set to true
 *  - maximumAnisotropy set to 16.0f
 *  - target set to TargetAutomatic
 */
QTextureLoader::QTextureLoader(QNode *parent)
    : QAbstractTexture(*new QTextureLoaderPrivate, parent)
{
    d_func()->m_wrapMode.setX(QTextureWrapMode::Repeat);
    d_func()->m_wrapMode.setY(QTextureWrapMode::Repeat);
    d_func()->m_minFilter = LinearMipMapLinear;
    d_func()->m_magFilter = Linear;
    d_func()->m_autoMipMap = true;
    d_func()->m_maximumAnisotropy = 16.0f;
    d_func()->m_target = TargetAutomatic;

    // Regenerate the texture functor when properties we support overriding
    // from QAbstractTexture get changed.
    auto regenerate = [this] () {
        Q_D(QTextureLoader);
        if (!notificationsBlocked())    // check the change doesn't come from the backend
            d->updateGenerator();
    };
    connect(this, &QAbstractTexture::formatChanged, regenerate);
}

/*! \internal */
QTextureLoader::~QTextureLoader()
{
}

/*!
    \property QTextureLoader::source

    \brief The current texture source.
*/
/*!
    \qmlproperty url Qt3D.Render::TextureLoader::source

    This property holds the current texture source.
*/
QUrl QTextureLoader::source() const
{
    Q_D(const QTextureLoader);
    return d->m_source;
}

bool QTextureLoader::isMirrored() const
{
    Q_D(const QTextureLoader);
    return d->m_mirrored;
}

/*!
 * Sets the texture loader source to \a source.
 * \param source
 */
void QTextureLoader::setSource(const QUrl& source)
{
    Q_D(QTextureLoader);
    if (source != d->m_source) {
        d->m_source = source;

        // Reset target and format
        d->m_target = TargetAutomatic;
        setFormat(NoFormat);

        d->updateGenerator();
        const bool blocked = blockNotifications(true);
        emit sourceChanged(source);
        blockNotifications(blocked);
    }
}

/*!
  \property Qt3DRender::QTextureLoader::mirrored

  This property specifies whether the texture should be mirrored when loaded. This
  is a convenience to avoid having to manipulate images to match the origin of
  the texture coordinates used by the rendering API. By default this property
  is set to true. This has no effect when using GPU compressed texture formats.

  \warning This property results in a performance price payed at runtime when
  loading uncompressed or CPU compressed image formats such as PNG. To avoid this
  performance price it is better to set this property to false and load texture
  assets that have been pre-mirrored.

  \note OpenGL specifies the origin of texture coordinates from the lower left
  hand corner whereas DirectX uses the the upper left hand corner.

  \note When using cube map texture you'll probably want mirroring disabled as
  the cube map sampler takes a direction rather than regular texture
  coordinates.
*/

/*!
  \qmlproperty bool Qt3D.Render::TextureLoader::mirrored

  This property specifies whether the texture should be mirrored when loaded. This
  is a convenience to avoid having to manipulate images to match the origin of
  the texture coordinates used by the rendering API. By default this property
  is set to true. This has no effect when using GPU compressed texture formats.

  \warning This property results in a performance price payed at runtime when
  loading uncompressed or CPU compressed image formats such as PNG. To avoid this
  performance price it is better to set this property to false and load texture
  assets that have been pre-mirrored.

  \note OpenGL specifies the origin of texture coordinates from the lower left
  hand corner whereas DirectX uses the the upper left hand corner.

  \note When using cube map texture you'll probably want mirroring disabled as
  the cube map sampler takes a direction rather than regular texture
  coordinates.
*/

/*!
    Sets mirroring to \a mirrored.
    \note This internally triggers a call to update the data generator.
 */
void QTextureLoader::setMirrored(bool mirrored)
{
    Q_D(QTextureLoader);
    if (mirrored != d->m_mirrored) {
        d->m_mirrored = mirrored;
        d->updateGenerator();
        const bool blocked = blockNotifications(true);
        emit mirroredChanged(mirrored);
        blockNotifications(blocked);
    }
}

/*
 * Constructs a new QTextureFromSourceGenerator::QTextureFromSourceGenerator
 * instance with properties passed in via \a textureLoader
 * \param url
 */
QTextureFromSourceGenerator::QTextureFromSourceGenerator(QTextureLoader *textureLoader,
                                                         Qt3DCore::QAspectEngine *engine,
                                                         Qt3DCore::QNodeId textureId)
    : QTextureGenerator()
    , QEnableSharedFromThis<QTextureFromSourceGenerator>()
    , m_url()
    , m_status(QAbstractTexture::None)
    , m_mirrored()
    , m_texture(textureId)
    , m_engine(engine)
    , m_format(QAbstractTexture::NoFormat)
{
    Q_ASSERT(textureLoader);

    // We always get QTextureLoader's "own" additional properties
    m_url = textureLoader->source();
    m_mirrored = textureLoader->isMirrored();

    // For the properties on the base QAbstractTexture we only apply
    // those that have been explicitly set and which we support here.
    // For more control, the user can themselves use a QTexture2D and
    // create the texture images themselves, or even better, go create
    // proper texture files themselves (dds/ktx etc). This is purely a
    // convenience for some common use cases and will always be less
    // ideal than using compressed textures and generating mips offline.
    m_format = textureLoader->format();
}

QTextureFromSourceGenerator::QTextureFromSourceGenerator(const QTextureFromSourceGenerator &other)
    : QTextureGenerator()
    , QEnableSharedFromThis<QTextureFromSourceGenerator>()
    , m_url(other.m_url)
    , m_status(other.m_status)
    , m_mirrored(other.m_mirrored)
    , m_sourceData(other.m_sourceData)
    , m_texture(other.m_texture)
    , m_engine(other.m_engine)
    , m_format(other.m_format)
{
}

/*
 * Takes in a TextureGenerator via \a other and
 * \return whether generators have the same source.
 */
bool QTextureFromSourceGenerator::operator ==(const QTextureGenerator &other) const
{
    const QTextureFromSourceGenerator *otherFunctor = functor_cast<QTextureFromSourceGenerator>(&other);
    return (otherFunctor != nullptr &&
            otherFunctor->m_url == m_url &&
            otherFunctor->m_mirrored == m_mirrored &&
            otherFunctor->m_engine == m_engine &&
            otherFunctor->m_format == m_format &&
            otherFunctor->m_sourceData == m_sourceData);
}

QUrl QTextureFromSourceGenerator::url() const
{
    return m_url;
}

bool QTextureFromSourceGenerator::isMirrored() const
{
    return m_mirrored;
}

/*!
 * \class Qt3DRender::QSharedGLTexture
 * \inmodule Qt3DRender
 * \inheaderfile Qt3DRender/QTexture
 * \brief Allows to use a textureId from a separate OpenGL context in a Qt 3D scene.
 *
 * Depending on the rendering mode used by Qt 3D, the shared context will either be:
 * \list
 * \li qt_gl_global_share_context when letting Qt 3D drive the rendering. When
 * setting the attribute Qt::AA_ShareOpenGLContexts on the QApplication class,
 * this will automatically make QOpenGLWidget instances have their context shared
 * with qt_gl_global_share_context.
 * \li the shared context from the QtQuick scene. You might have to subclass
 * QWindow or use QtQuickRenderControl to have control over what that shared
 * context is though as of 5.13 it is qt_gl_global_share_context.
 * \endlist
 *
 * \since 5.13
 *
 * Any 3rd party engine that shares its context with the Qt 3D renderer can now
 * provide texture ids that will be referenced by the Qt 3D texture.
 *
 * You can omit specifying the texture properties, Qt 3D will try at runtime to
 * determine what they are. If you know them, you can of course provide them,
 * avoid additional work for Qt 3D.
 *
 * Keep in mind that if you are using custom materials and shaders, you need to
 * specify the correct sampler type to be used.
 */

/*!
    \qmltype SharedGLTexture
    \instantiates Qt3DRender::QSharedGLTexture
    \inqmlmodule Qt3D.Render
    \brief Allows to use a textureId from a separate OpenGL context in a Qt 3D scene.
    \since 5.13
*/

QSharedGLTexture::QSharedGLTexture(Qt3DCore::QNode *parent)
    : QAbstractTexture(parent)
{
    QAbstractTexturePrivate *d = static_cast<QAbstractTexturePrivate *>(Qt3DCore::QNodePrivate::get(this));
    d->m_target = TargetAutomatic;
}

QSharedGLTexture::~QSharedGLTexture()
{
}

/*!
 * \qmlproperty int SharedGLTexture::textureId
 *
 * The OpenGL texture id value that you want Qt3D to gain access to.
 */
/*!
 *\property Qt3DRender::QSharedGLTexture::textureId
 *
 * The OpenGL texture id value that you want Qt3D to gain access to.
 */
int QSharedGLTexture::textureId() const
{
    return static_cast<QAbstractTexturePrivate *>(d_ptr.get())->m_sharedTextureId;
}

void QSharedGLTexture::setTextureId(int id)
{
    QAbstractTexturePrivate *d = static_cast<QAbstractTexturePrivate *>(Qt3DCore::QNodePrivate::get(this));
    if (d->m_sharedTextureId != id) {
        d->m_sharedTextureId = id;
        emit textureIdChanged(id);
    }
}

} // namespace Qt3DRender

QT_END_NAMESPACE

#include "moc_qtexture.cpp"
