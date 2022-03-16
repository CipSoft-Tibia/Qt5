//===-- AMDGPUSubtarget.cpp - AMDGPU Subtarget Information ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the AMDGPU specific subclass of TargetSubtarget.
//
//===----------------------------------------------------------------------===//

#include "AMDGPUSubtarget.h"

using namespace llvm;

#define GET_SUBTARGETINFO_ENUM
#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "AMDGPUGenSubtargetInfo.inc"

AMDGPUSubtarget::AMDGPUSubtarget(StringRef TT, StringRef CPU, StringRef FS) :
  AMDGPUGenSubtargetInfo(TT, CPU, FS), mDumpCode(false) {
    InstrItins = getInstrItineraryForCPU(CPU);

  memset(CapsOverride, 0, sizeof(*CapsOverride)
      * AMDGPUDeviceInfo::MaxNumberCapabilities);
  // Default card
  StringRef GPU = CPU;
  mIs64bit = false;
  mDefaultSize[0] = 64;
  mDefaultSize[1] = 1;
  mDefaultSize[2] = 1;
  ParseSubtargetFeatures(GPU, FS);
  mDevName = GPU;
  mDevice = AMDGPUDeviceInfo::getDeviceFromName(mDevName, this, mIs64bit);
}

AMDGPUSubtarget::~AMDGPUSubtarget()
{
  delete mDevice;
}

bool
AMDGPUSubtarget::isOverride(AMDGPUDeviceInfo::Caps caps) const
{
  assert(caps < AMDGPUDeviceInfo::MaxNumberCapabilities &&
      "Caps index is out of bounds!");
  return CapsOverride[caps];
}
bool
AMDGPUSubtarget::is64bit() const 
{
  return mIs64bit;
}
bool
AMDGPUSubtarget::isTargetELF() const
{
  return false;
}
size_t
AMDGPUSubtarget::getDefaultSize(uint32_t dim) const
{
  if (dim > 3) {
    return 1;
  } else {
    return mDefaultSize[dim];
  }
}

std::string
AMDGPUSubtarget::getDataLayout() const
{
    if (!mDevice) {
        return std::string("e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16"
                "-i32:32:32-i64:64:64-f32:32:32-f64:64:64-f80:32:32"
                "-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64"
                "-v96:128:128-v128:128:128-v192:256:256-v256:256:256"
                "-v512:512:512-v1024:1024:1024-v2048:2048:2048-a0:0:64");
    }
    return mDevice->getDataLayout();
}

std::string
AMDGPUSubtarget::getDeviceName() const
{
  return mDevName;
}
const AMDGPUDevice *
AMDGPUSubtarget::device() const
{
  return mDevice;
}
