//===-- AMDGPUTargetMachine.cpp - TargetMachine for hw codegen targets-----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// The AMDGPU target machine contains all of the hardware specific information
// needed to emit code for R600 and SI GPUs.
//
//===----------------------------------------------------------------------===//

#include "AMDGPUTargetMachine.h"
#include "AMDGPU.h"
#include "R600ISelLowering.h"
#include "R600InstrInfo.h"
#include "SIISelLowering.h"
#include "SIInstrInfo.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/CodeGen/MachineFunctionAnalysis.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/PassManager.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Scalar.h"
#include <llvm/CodeGen/Passes.h>

using namespace llvm;

extern "C" void LLVMInitializeAMDGPUTarget() {
  // Register the target
  RegisterTargetMachine<AMDGPUTargetMachine> X(TheAMDGPUTarget);
}

AMDGPUTargetMachine::AMDGPUTargetMachine(const Target &T, StringRef TT,
    StringRef CPU, StringRef FS,
  TargetOptions Options,
  Reloc::Model RM, CodeModel::Model CM,
  CodeGenOpt::Level OptLevel
)
:
  LLVMTargetMachine(T, TT, CPU, FS, Options, RM, CM, OptLevel),
  Subtarget(TT, CPU, FS),
  DataLayout(Subtarget.getDataLayout()),
  FrameLowering(TargetFrameLowering::StackGrowsUp,
      Subtarget.device()->getStackAlignment(), 0),
  IntrinsicInfo(this),
  InstrItins(&Subtarget.getInstrItineraryData()),
  mDump(false)

{
  // TLInfo uses InstrInfo so it must be initialized after.
  if (Subtarget.device()->getGeneration() <= AMDGPUDeviceInfo::HD6XXX) {
    InstrInfo = new R600InstrInfo(*this);
    TLInfo = new R600TargetLowering(*this);
  } else {
    InstrInfo = new SIInstrInfo(*this);
    TLInfo = new SITargetLowering(*this);
  }
}

AMDGPUTargetMachine::~AMDGPUTargetMachine()
{
}

namespace {
class AMDGPUPassConfig : public TargetPassConfig {
public:
  AMDGPUPassConfig(AMDGPUTargetMachine *TM, PassManagerBase &PM)
    : TargetPassConfig(TM, PM) {}

  AMDGPUTargetMachine &getAMDGPUTargetMachine() const {
    return getTM<AMDGPUTargetMachine>();
  }

  virtual bool addPreISel();
  virtual bool addInstSelector();
  virtual bool addPreRegAlloc();
  virtual bool addPostRegAlloc();
  virtual bool addPreSched2();
  virtual bool addPreEmitPass();
};
} // End of anonymous namespace

TargetPassConfig *AMDGPUTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new AMDGPUPassConfig(this, PM);
}

bool
AMDGPUPassConfig::addPreISel()
{
  const AMDGPUSubtarget &ST = TM->getSubtarget<AMDGPUSubtarget>();
  if (ST.device()->getGeneration() <= AMDGPUDeviceInfo::HD6XXX) {
    PM->add(createR600KernelParametersPass(
                     getAMDGPUTargetMachine().getTargetData()));
  }
  return false;
}

bool AMDGPUPassConfig::addInstSelector() {
  PM->add(createAMDGPUPeepholeOpt(*TM));
  PM->add(createAMDGPUISelDag(getAMDGPUTargetMachine()));
  return false;
}

bool AMDGPUPassConfig::addPreRegAlloc() {
  const AMDGPUSubtarget &ST = TM->getSubtarget<AMDGPUSubtarget>();

  if (ST.device()->getGeneration() > AMDGPUDeviceInfo::HD6XXX) {
    PM->add(createSIAssignInterpRegsPass(*TM));
  }
  PM->add(createAMDGPUConvertToISAPass(*TM));
  return false;
}

bool AMDGPUPassConfig::addPostRegAlloc() {
  return false;
}

bool AMDGPUPassConfig::addPreSched2() {

  addPass(IfConverterID);
  return false;
}

bool AMDGPUPassConfig::addPreEmitPass() {
  PM->add(createAMDGPUCFGPreparationPass(*TM));
  PM->add(createAMDGPUCFGStructurizerPass(*TM));

  const AMDGPUSubtarget &ST = TM->getSubtarget<AMDGPUSubtarget>();
  if (ST.device()->getGeneration() <= AMDGPUDeviceInfo::HD6XXX) {
    PM->add(createR600ExpandSpecialInstrsPass(*TM));
    addPass(FinalizeMachineBundlesID);
  }

  return false;
}

