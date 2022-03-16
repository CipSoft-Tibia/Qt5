//===-- AMDGPUAsmPrinter.cpp - AMDGPU Assebly printer  --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// The AMDGPUAsmPrinter is used to print both assembly string and also binary
// code.  When passed an MCAsmStreamer it prints assembly and when passed
// an MCObjectStreamer it outputs binary code.
//
//===----------------------------------------------------------------------===//
//


#include "AMDGPUAsmPrinter.h"
#include "AMDGPU.h"
#include "SIMachineFunctionInfo.h"
#include "SIRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;


static AsmPrinter *createAMDGPUAsmPrinterPass(TargetMachine &tm,
                                              MCStreamer &Streamer) {
  return new AMDGPUAsmPrinter(tm, Streamer);
}

extern "C" void LLVMInitializeAMDGPUAsmPrinter() {
  TargetRegistry::RegisterAsmPrinter(TheAMDGPUTarget, createAMDGPUAsmPrinterPass);
}

/// runOnMachineFunction - We need to override this function so we can avoid
/// the call to EmitFunctionHeader(), which the MCPureStreamer can't handle.
bool AMDGPUAsmPrinter::runOnMachineFunction(MachineFunction &MF) {
  const AMDGPUSubtarget &STM = TM.getSubtarget<AMDGPUSubtarget>();
  if (STM.dumpCode()) {
    MF.dump();
  }
  SetupMachineFunction(MF);
  if (STM.device()->getGeneration() > AMDGPUDeviceInfo::HD6XXX) {
    EmitProgramInfo(MF);
  }
  OutStreamer.SwitchSection(getObjFileLowering().getTextSection());
  EmitFunctionBody();
  return false;
}

void AMDGPUAsmPrinter::EmitProgramInfo(MachineFunction &MF) {
  unsigned MaxSGPR = 0;
  unsigned MaxVGPR = 0;
  bool VCCUsed = false;
  const SIRegisterInfo * RI =
                static_cast<const SIRegisterInfo*>(TM.getRegisterInfo());

  for (MachineFunction::iterator BB = MF.begin(), BB_E = MF.end();
                                                  BB != BB_E; ++BB) {
    MachineBasicBlock &MBB = *BB;
    for (MachineBasicBlock::iterator I = MBB.begin(), E = MBB.end();
                                                    I != E; ++I) {
      MachineInstr &MI = *I;

      unsigned numOperands = MI.getNumOperands();
      for (unsigned op_idx = 0; op_idx < numOperands; op_idx++) {
        MachineOperand & MO = MI.getOperand(op_idx);
        unsigned maxUsed;
        unsigned width = 0;
        bool isSGPR = false;
        unsigned reg;
        unsigned hwReg;
        if (!MO.isReg()) {
          continue;
        }
        reg = MO.getReg();
        if (reg == AMDGPU::VCC) {
          VCCUsed = true;
          continue;
        }
        if (reg == AMDGPU::EXEC) {
          continue;
        }
        if (AMDGPU::SReg_32RegClass.contains(reg)) {
          isSGPR = true;
          width = 1;
        } else if (AMDGPU::VReg_32RegClass.contains(reg)) {
          isSGPR = false;
          width = 1;
        } else if (AMDGPU::SReg_64RegClass.contains(reg)) {
          isSGPR = true;
          width = 2;
        } else if (AMDGPU::VReg_64RegClass.contains(reg)) {
          isSGPR = false;
          width = 2;
        } else if (AMDGPU::SReg_128RegClass.contains(reg)) {
          isSGPR = true;
          width = 4;
        } else if (AMDGPU::VReg_128RegClass.contains(reg)) {
          isSGPR = false;
          width = 4;
        } else if (AMDGPU::SReg_256RegClass.contains(reg)) {
          isSGPR = true;
          width = 8;
        } else {
          assert("!Unknown register class");
        }
        hwReg = RI->getHWRegNum(reg);
        maxUsed = hwReg + width - 1;
        if (isSGPR) {
          MaxSGPR = maxUsed > MaxSGPR ? maxUsed : MaxSGPR;
        } else {
          MaxVGPR = maxUsed > MaxVGPR ? maxUsed : MaxVGPR;
        }
      }
    }
  }
  if (VCCUsed) {
    MaxSGPR += 2;
  }
  SIMachineFunctionInfo * MFI = MF.getInfo<SIMachineFunctionInfo>();
  OutStreamer.EmitIntValue(MaxSGPR + 1, 4);
  OutStreamer.EmitIntValue(MaxVGPR + 1, 4);
  OutStreamer.EmitIntValue(MFI->spi_ps_input_addr, 4);
}
