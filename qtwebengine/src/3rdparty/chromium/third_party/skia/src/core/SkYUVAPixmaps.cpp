/*
 * Copyright 2020 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/core/SkYUVAPixmaps.h"

#include "include/core/SkYUVAIndex.h"
#include "include/core/SkYUVASizeInfo.h"
#include "include/private/SkImageInfoPriv.h"

#if SK_SUPPORT_GPU
#include "include/private/GrImageContext.h"
#endif


SkYUVAPixmapInfo::SupportedDataTypes::SupportedDataTypes(const GrImageContext& context) {
#if SK_SUPPORT_GPU
    for (int n = 1; n <= 4; ++n) {
        if (context.defaultBackendFormat(DefaultColorTypeForDataType(DataType::kUnorm8, n),
                                         GrRenderable::kNo).isValid()) {
            this->enableDataType(DataType::kUnorm8, n);
        }
        if (context.defaultBackendFormat(DefaultColorTypeForDataType(DataType::kUnorm16, n),
                                         GrRenderable::kNo).isValid()) {
            this->enableDataType(DataType::kUnorm16, n);
        }
        if (context.defaultBackendFormat(DefaultColorTypeForDataType(DataType::kFloat16, n),
                                         GrRenderable::kNo).isValid()) {
            this->enableDataType(DataType::kFloat16, n);
        }
        if (context.defaultBackendFormat(DefaultColorTypeForDataType(DataType::kUnorm10_Unorm2, n),
                                         GrRenderable::kNo).isValid()) {
            this->enableDataType(DataType::kUnorm10_Unorm2, n);
        }
    }
#endif
}

void SkYUVAPixmapInfo::SupportedDataTypes::enableDataType(DataType type, int numChannels) {
    if (numChannels < 1 || numChannels > 4) {
        return;
    }
    fDataTypeSupport[static_cast<size_t>(type) + (numChannels - 1)*kDataTypeCnt] = true;
}

//////////////////////////////////////////////////////////////////////////////

std::tuple<int, SkYUVAPixmapInfo::DataType> SkYUVAPixmapInfo::NumChannelsAndDataType(
        SkColorType ct) {
    // We could allow BGR[A] color types, but then we'd have to decide whether B should be the 0th
    // or 2nd channel. Our docs currently say channel order is always R=0, G=1, B=2[, A=3].
    switch (ct) {
        case kAlpha_8_SkColorType:
        case kGray_8_SkColorType:    return std::make_tuple(1, DataType::kUnorm8 );
        case kA16_unorm_SkColorType: return std::make_tuple(1, DataType::kUnorm16);
        case kA16_float_SkColorType: return std::make_tuple(1, DataType::kFloat16);

        case kR8G8_unorm_SkColorType:   return std::make_tuple(2, DataType::kUnorm8  );
        case kR16G16_unorm_SkColorType: return std::make_tuple(2, DataType::kUnorm16 );
        case kR16G16_float_SkColorType: return std::make_tuple(2, DataType::kFloat16 );

        case kRGB_888x_SkColorType:    return std::make_tuple(3, DataType::kUnorm8          );
        case kRGB_101010x_SkColorType: return std::make_tuple(3, DataType::kUnorm10_Unorm2  );

        case kRGBA_8888_SkColorType:          return std::make_tuple(4, DataType::kUnorm8  );
        case kR16G16B16A16_unorm_SkColorType: return std::make_tuple(4, DataType::kUnorm16 );
        case kRGBA_F16_SkColorType:           return std::make_tuple(4, DataType::kFloat16 );
        case kRGBA_F16Norm_SkColorType:       return std::make_tuple(4, DataType::kFloat16 );
        case kRGBA_1010102_SkColorType:       return std::make_tuple(4, DataType::kUnorm10_Unorm2 );

        default: return std::make_tuple(0, DataType::kUnorm8 );
    }
}

SkYUVAPixmapInfo::SkYUVAPixmapInfo(const SkYUVAInfo& yuvaInfo,
                                   const SkColorType colorTypes[kMaxPlanes],
                                   const size_t rowBytes[kMaxPlanes])
        : fYUVAInfo(yuvaInfo) {
    if (yuvaInfo.dimensions().isEmpty()) {
        *this = {};
        SkASSERT(!this->isValid());
        return;
    }
    SkISize planeDimensions[4];
    int n = yuvaInfo.planeDimensions(planeDimensions);
    size_t tempRowBytes[kMaxPlanes];
    if (!rowBytes) {
        for (int i = 0; i < n; ++i) {
            tempRowBytes[i] = SkColorTypeBytesPerPixel(colorTypes[i]) * planeDimensions[i].width();
        }
        rowBytes = tempRowBytes;
    }
    bool ok = true;
    for (size_t i = 0; i < static_cast<size_t>(n); ++i) {
        fRowBytes[i] = rowBytes[i];
        fPlaneInfos[i] = SkImageInfo::Make(planeDimensions[i], colorTypes[i], kPremul_SkAlphaType);
        int numRequiredChannels = yuvaInfo.numChannelsInPlane(i);
        SkASSERT(numRequiredChannels > 0);
        auto t = NumChannelsAndDataType(colorTypes[i]);
        auto numColorTypeChannels = std::get<0>(t);
        auto colorTypeDataType = std::get<1>(t);
        ok &= i == 0 || colorTypeDataType == fDataType;
        ok &= numColorTypeChannels >= numRequiredChannels;
        ok &= fPlaneInfos[i].validRowBytes(fRowBytes[i]);
        fDataType = colorTypeDataType;
    }
    if (!ok) {
        *this = {};
        SkASSERT(!this->isValid());
    } else {
        SkASSERT(this->isValid());
    }
}

SkYUVAPixmapInfo::SkYUVAPixmapInfo(const SkYUVAInfo& yuvaInfo,
                                   DataType dataType,
                                   const size_t rowBytes[kMaxPlanes]) {
    SkColorType colorTypes[kMaxPlanes] = {};
    int numPlanes = yuvaInfo.numPlanes();
    for (int i = 0; i < numPlanes; ++i) {
        int numChannels = yuvaInfo.numChannelsInPlane(i);
        colorTypes[i] = DefaultColorTypeForDataType(dataType, numChannels);
    }
    *this = SkYUVAPixmapInfo(yuvaInfo, colorTypes, rowBytes);
}

bool SkYUVAPixmapInfo::operator==(const SkYUVAPixmapInfo& that) const {
    bool result = fYUVAInfo   == that.fYUVAInfo   &&
                  fPlaneInfos == that.fPlaneInfos &&
                  fRowBytes   == that.fRowBytes;
    SkASSERT(!result || fDataType == that.fDataType);
    return result;
}

size_t SkYUVAPixmapInfo::computeTotalBytes(size_t planeSizes[kMaxPlanes]) const {
    if (!this->isValid()) {
        if (planeSizes) {
            std::fill_n(planeSizes, kMaxPlanes, 0);
        }
        return 0;
    }
    return fYUVAInfo.computeTotalBytes(fRowBytes.data(), planeSizes);
}

bool SkYUVAPixmapInfo::initPixmapsFromSingleAllocation(void* memory,
                                                       SkPixmap pixmaps[kMaxPlanes]) const {
    if (!this->isValid()) {
        return false;
    }
    SkASSERT(pixmaps);
    char* addr = static_cast<char*>(memory);
    int n = this->numPlanes();
    for (int i = 0; i < n; ++i) {
        SkASSERT(fPlaneInfos[i].validRowBytes(fRowBytes[i]));
        pixmaps[i].reset(fPlaneInfos[i], addr, fRowBytes[i]);
        size_t planeSize = pixmaps[i].rowBytes()*pixmaps[i].height();
        SkASSERT(planeSize);
        addr += planeSize;
    }
    for (int i = n; i < kMaxPlanes; ++i) {
        pixmaps[i] = {};
    }
    return true;
}

bool SkYUVAPixmapInfo::isSupported(const SupportedDataTypes& supportedDataTypes) const {
    if (!this->isValid()) {
        return false;
    }
    return supportedDataTypes.supported(fYUVAInfo.planarConfig(), fDataType);
}

//////////////////////////////////////////////////////////////////////////////

SkYUVAPixmaps SkYUVAPixmaps::Allocate(const SkYUVAPixmapInfo& yuvaPixmapInfo) {
    if (!yuvaPixmapInfo.isValid()) {
        return {};
    }
    return SkYUVAPixmaps(yuvaPixmapInfo,
                         SkData::MakeUninitialized(yuvaPixmapInfo.computeTotalBytes()));
}

SkYUVAPixmaps SkYUVAPixmaps::FromData(const SkYUVAPixmapInfo& yuvaPixmapInfo, sk_sp<SkData> data) {
    if (!yuvaPixmapInfo.isValid()) {
        return {};
    }
    if (yuvaPixmapInfo.computeTotalBytes() > data->size()) {
        return {};
    }
    return SkYUVAPixmaps(yuvaPixmapInfo, std::move(data));
}

SkYUVAPixmaps SkYUVAPixmaps::FromExternalMemory(const SkYUVAPixmapInfo& yuvaPixmapInfo,
                                                void* memory) {
    if (!yuvaPixmapInfo.isValid()) {
        return {};
    }
    SkPixmap pixmaps[kMaxPlanes];
    yuvaPixmapInfo.initPixmapsFromSingleAllocation(memory, pixmaps);
    return SkYUVAPixmaps(yuvaPixmapInfo.yuvaInfo(), pixmaps);
}

SkYUVAPixmaps SkYUVAPixmaps::FromExternalPixmaps(const SkYUVAInfo& yuvaInfo,
                                                 const SkPixmap pixmaps[kMaxPlanes]) {
    SkColorType colorTypes[kMaxPlanes] = {};
    size_t rowBytes[kMaxPlanes] = {};
    int numPlanes = yuvaInfo.numPlanes();
    for (int i = 0; i < numPlanes; ++i) {
        colorTypes[i] = pixmaps[i].colorType();
        rowBytes[i] = pixmaps[i].rowBytes();
    }
    if (!SkYUVAPixmapInfo(yuvaInfo, colorTypes, rowBytes).isValid()) {
        return {};
    }
    return SkYUVAPixmaps(yuvaInfo, pixmaps);
}

SkYUVAPixmaps::SkYUVAPixmaps(const SkYUVAPixmapInfo& yuvaPixmapInfo, sk_sp<SkData> data)
        : fYUVAInfo(yuvaPixmapInfo.yuvaInfo())
        , fData(std::move(data)) {
    SkASSERT(yuvaPixmapInfo.isValid());
    SkASSERT(yuvaPixmapInfo.computeTotalBytes() <= fData->size());
    SkAssertResult(yuvaPixmapInfo.initPixmapsFromSingleAllocation(fData->writable_data(),
                                                                  fPlanes.data()));
}

SkYUVAPixmaps::SkYUVAPixmaps(const SkYUVAInfo& yuvaInfo, const SkPixmap pixmaps[kMaxPlanes])
        : fYUVAInfo(yuvaInfo) {
    std::copy_n(pixmaps, yuvaInfo.numPlanes(), fPlanes.data());
}

bool SkYUVAPixmaps::toLegacy(SkYUVASizeInfo* yuvaSizeInfo, SkYUVAIndex yuvaIndices[4]) const {
    if (!this->isValid()) {
        return false;
    }
    bool ok = true;
    auto getIthChannel = [&ok](SkColorType ct, int idx) -> SkColorChannel {
      switch (SkColorTypeChannelFlags(ct)) {
          case kAlpha_SkColorChannelFlag:
              ok &= idx == 0;
              return SkColorChannel::kA;
          case kGray_SkColorChannelFlag:
          case kRed_SkColorChannelFlag:
              ok &= idx == 0;
              return SkColorChannel::kR;
          case kRG_SkColorChannelFlags:
              ok &= idx < 2;
              return static_cast<SkColorChannel>(idx);
          case kRGB_SkColorChannelFlags:
              ok &= idx < 3;
              return static_cast<SkColorChannel>(idx);
          case kRGBA_SkColorChannelFlags:
              ok &= idx < 4;
              return static_cast<SkColorChannel>(idx);
          default:
              ok = false;
              return SkColorChannel::kR;
      }
    };
    SkColorType cts[] = {fPlanes[0].colorType(),
                         fPlanes[1].colorType(),
                         fPlanes[2].colorType(),
                         fPlanes[3].colorType()};
    switch (fYUVAInfo.planarConfig()) {
        case SkYUVAInfo::PlanarConfig::kY_U_V_444:
        case SkYUVAInfo::PlanarConfig::kY_U_V_422:
        case SkYUVAInfo::PlanarConfig::kY_U_V_420:
        case SkYUVAInfo::PlanarConfig::kY_U_V_440:
        case SkYUVAInfo::PlanarConfig::kY_U_V_411:
        case SkYUVAInfo::PlanarConfig::kY_U_V_410:
            yuvaIndices[SkYUVAIndex::kY_Index].fIndex =  0;
            yuvaIndices[SkYUVAIndex::kU_Index].fIndex =  1;
            yuvaIndices[SkYUVAIndex::kV_Index].fIndex =  2;
            yuvaIndices[SkYUVAIndex::kA_Index].fIndex = -1;
            yuvaIndices[SkYUVAIndex::kY_Index].fChannel = getIthChannel(cts[0], 0);
            yuvaIndices[SkYUVAIndex::kU_Index].fChannel = getIthChannel(cts[1], 0);
            yuvaIndices[SkYUVAIndex::kV_Index].fChannel = getIthChannel(cts[2], 0);
            yuvaIndices[SkYUVAIndex::kA_Index].fChannel = SkColorChannel::kR;  // arbitrary
            break;
        case SkYUVAInfo::PlanarConfig::kY_V_U_420:
            yuvaIndices[SkYUVAIndex::kY_Index].fIndex =  0;
            yuvaIndices[SkYUVAIndex::kU_Index].fIndex =  2;
            yuvaIndices[SkYUVAIndex::kV_Index].fIndex =  1;
            yuvaIndices[SkYUVAIndex::kA_Index].fIndex = -1;
            yuvaIndices[SkYUVAIndex::kY_Index].fChannel = getIthChannel(cts[0], 0);
            yuvaIndices[SkYUVAIndex::kU_Index].fChannel = getIthChannel(cts[2], 0);
            yuvaIndices[SkYUVAIndex::kV_Index].fChannel = getIthChannel(cts[1], 0);
            yuvaIndices[SkYUVAIndex::kA_Index].fChannel = SkColorChannel::kR;  // arbitrary
            break;
        case SkYUVAInfo::PlanarConfig::kY_U_V_A_4204:
            yuvaIndices[SkYUVAIndex::kY_Index].fIndex = 0;
            yuvaIndices[SkYUVAIndex::kU_Index].fIndex = 1;
            yuvaIndices[SkYUVAIndex::kV_Index].fIndex = 2;
            yuvaIndices[SkYUVAIndex::kA_Index].fIndex = 3;
            yuvaIndices[SkYUVAIndex::kY_Index].fChannel = getIthChannel(cts[0], 0);
            yuvaIndices[SkYUVAIndex::kU_Index].fChannel = getIthChannel(cts[1], 0);
            yuvaIndices[SkYUVAIndex::kV_Index].fChannel = getIthChannel(cts[2], 0);
            yuvaIndices[SkYUVAIndex::kA_Index].fChannel = getIthChannel(cts[3], 0);
            break;
        case SkYUVAInfo::PlanarConfig::kY_V_U_A_4204:
            yuvaIndices[SkYUVAIndex::kY_Index].fIndex = 0;
            yuvaIndices[SkYUVAIndex::kU_Index].fIndex = 2;
            yuvaIndices[SkYUVAIndex::kV_Index].fIndex = 1;
            yuvaIndices[SkYUVAIndex::kA_Index].fIndex = 3;
            yuvaIndices[SkYUVAIndex::kY_Index].fChannel = getIthChannel(cts[0], 0);
            yuvaIndices[SkYUVAIndex::kU_Index].fChannel = getIthChannel(cts[2], 0);
            yuvaIndices[SkYUVAIndex::kV_Index].fChannel = getIthChannel(cts[1], 0);
            yuvaIndices[SkYUVAIndex::kA_Index].fChannel = getIthChannel(cts[3], 0);
            break;
        case SkYUVAInfo::PlanarConfig::kY_UV_420:
            yuvaIndices[SkYUVAIndex::kY_Index].fIndex =  0;
            yuvaIndices[SkYUVAIndex::kU_Index].fIndex =  1;
            yuvaIndices[SkYUVAIndex::kV_Index].fIndex =  1;
            yuvaIndices[SkYUVAIndex::kA_Index].fIndex = -1;
            yuvaIndices[SkYUVAIndex::kY_Index].fChannel = getIthChannel(cts[0], 0);
            yuvaIndices[SkYUVAIndex::kU_Index].fChannel = getIthChannel(cts[1], 0);
            yuvaIndices[SkYUVAIndex::kV_Index].fChannel = getIthChannel(cts[1], 1);
            yuvaIndices[SkYUVAIndex::kA_Index].fChannel = SkColorChannel::kR;  // arbitrary
            break;
        case SkYUVAInfo::PlanarConfig::kY_VU_420:
            yuvaIndices[SkYUVAIndex::kY_Index].fIndex =  0;
            yuvaIndices[SkYUVAIndex::kU_Index].fIndex =  1;
            yuvaIndices[SkYUVAIndex::kV_Index].fIndex =  1;
            yuvaIndices[SkYUVAIndex::kA_Index].fIndex = -1;
            yuvaIndices[SkYUVAIndex::kY_Index].fChannel = getIthChannel(cts[0], 0);
            yuvaIndices[SkYUVAIndex::kU_Index].fChannel = getIthChannel(cts[1], 1);
            yuvaIndices[SkYUVAIndex::kV_Index].fChannel = getIthChannel(cts[1], 0);
            yuvaIndices[SkYUVAIndex::kA_Index].fChannel = SkColorChannel::kR;  // arbitrary
            break;
        case SkYUVAInfo::PlanarConfig::kY_UV_A_4204:
            yuvaIndices[SkYUVAIndex::kY_Index].fIndex = 0;
            yuvaIndices[SkYUVAIndex::kU_Index].fIndex = 1;
            yuvaIndices[SkYUVAIndex::kV_Index].fIndex = 1;
            yuvaIndices[SkYUVAIndex::kA_Index].fIndex = 2;
            yuvaIndices[SkYUVAIndex::kY_Index].fChannel = getIthChannel(cts[0], 0);
            yuvaIndices[SkYUVAIndex::kU_Index].fChannel = getIthChannel(cts[1], 0);
            yuvaIndices[SkYUVAIndex::kV_Index].fChannel = getIthChannel(cts[1], 1);
            yuvaIndices[SkYUVAIndex::kA_Index].fChannel = getIthChannel(cts[2], 0);
            break;
        case SkYUVAInfo::PlanarConfig::kY_VU_A_4204:
            yuvaIndices[SkYUVAIndex::kY_Index].fIndex = 0;
            yuvaIndices[SkYUVAIndex::kU_Index].fIndex = 1;
            yuvaIndices[SkYUVAIndex::kV_Index].fIndex = 1;
            yuvaIndices[SkYUVAIndex::kA_Index].fIndex = 2;
            yuvaIndices[SkYUVAIndex::kY_Index].fChannel = getIthChannel(cts[0], 0);
            yuvaIndices[SkYUVAIndex::kU_Index].fChannel = getIthChannel(cts[1], 1);
            yuvaIndices[SkYUVAIndex::kV_Index].fChannel = getIthChannel(cts[1], 0);
            yuvaIndices[SkYUVAIndex::kA_Index].fChannel = getIthChannel(cts[2], 0);
            break;
        case SkYUVAInfo::PlanarConfig::kYUV_444:
            yuvaIndices[SkYUVAIndex::kY_Index].fIndex =  0;
            yuvaIndices[SkYUVAIndex::kU_Index].fIndex =  0;
            yuvaIndices[SkYUVAIndex::kV_Index].fIndex =  0;
            yuvaIndices[SkYUVAIndex::kA_Index].fIndex = -1;
            yuvaIndices[SkYUVAIndex::kY_Index].fChannel = getIthChannel(cts[0], 0);
            yuvaIndices[SkYUVAIndex::kU_Index].fChannel = getIthChannel(cts[0], 1);
            yuvaIndices[SkYUVAIndex::kV_Index].fChannel = getIthChannel(cts[0], 2);
            yuvaIndices[SkYUVAIndex::kA_Index].fChannel = SkColorChannel::kR;  // arbitrary
            break;
        case SkYUVAInfo::PlanarConfig::kUYV_444:
            yuvaIndices[SkYUVAIndex::kY_Index].fIndex =  0;
            yuvaIndices[SkYUVAIndex::kU_Index].fIndex =  0;
            yuvaIndices[SkYUVAIndex::kV_Index].fIndex =  0;
            yuvaIndices[SkYUVAIndex::kA_Index].fIndex = -1;
            yuvaIndices[SkYUVAIndex::kY_Index].fChannel = getIthChannel(cts[0], 1);
            yuvaIndices[SkYUVAIndex::kU_Index].fChannel = getIthChannel(cts[0], 0);
            yuvaIndices[SkYUVAIndex::kV_Index].fChannel = getIthChannel(cts[0], 2);
            yuvaIndices[SkYUVAIndex::kA_Index].fChannel = SkColorChannel::kR;  // arbitrary
            break;
        case SkYUVAInfo::PlanarConfig::kYUVA_4444:
            yuvaIndices[SkYUVAIndex::kY_Index].fIndex = 0;
            yuvaIndices[SkYUVAIndex::kU_Index].fIndex = 0;
            yuvaIndices[SkYUVAIndex::kV_Index].fIndex = 0;
            yuvaIndices[SkYUVAIndex::kA_Index].fIndex = 0;
            yuvaIndices[SkYUVAIndex::kY_Index].fChannel = getIthChannel(cts[0], 0);
            yuvaIndices[SkYUVAIndex::kU_Index].fChannel = getIthChannel(cts[0], 1);
            yuvaIndices[SkYUVAIndex::kV_Index].fChannel = getIthChannel(cts[0], 2);
            yuvaIndices[SkYUVAIndex::kA_Index].fChannel = getIthChannel(cts[0], 3);
            break;
        case SkYUVAInfo::PlanarConfig::kUYVA_4444:
            yuvaIndices[SkYUVAIndex::kY_Index].fIndex = 0;
            yuvaIndices[SkYUVAIndex::kU_Index].fIndex = 0;
            yuvaIndices[SkYUVAIndex::kV_Index].fIndex = 0;
            yuvaIndices[SkYUVAIndex::kA_Index].fIndex = 0;
            yuvaIndices[SkYUVAIndex::kY_Index].fChannel = getIthChannel(cts[0], 1);
            yuvaIndices[SkYUVAIndex::kU_Index].fChannel = getIthChannel(cts[0], 0);
            yuvaIndices[SkYUVAIndex::kV_Index].fChannel = getIthChannel(cts[0], 2);
            yuvaIndices[SkYUVAIndex::kA_Index].fChannel = getIthChannel(cts[0], 3);
            break;
    }
    if (!ok) {
        return false;
    }
    if (yuvaSizeInfo) {
        yuvaSizeInfo->fOrigin = fYUVAInfo.origin();
        int n = fYUVAInfo.numPlanes();
        for (int i = 0; i < n; ++i) {
            yuvaSizeInfo->fSizes[i] = fPlanes[i].dimensions();
            yuvaSizeInfo->fWidthBytes[i] = fPlanes[i].rowBytes();
        }
    }
    return true;
}
