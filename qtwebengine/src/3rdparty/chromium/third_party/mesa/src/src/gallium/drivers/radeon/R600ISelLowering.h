//===-- R600ISelLowering.h - R600 DAG Lowering Interface -*- C++ -*--------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// R600 DAG Lowering interface definition
//
//===----------------------------------------------------------------------===//

#ifndef R600ISELLOWERING_H
#define R600ISELLOWERING_H

#include "AMDGPUISelLowering.h"

namespace llvm {

class R600InstrInfo;

class R600TargetLowering : public AMDGPUTargetLowering
{
public:
  R600TargetLowering(TargetMachine &TM);
  virtual MachineBasicBlock * EmitInstrWithCustomInserter(MachineInstr *MI,
      MachineBasicBlock * BB) const;
  virtual SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const;

private:
  const R600InstrInfo * TII;

  /// lowerImplicitParameter - Each OpenCL kernel has nine implicit parameters
  /// that are stored in the first nine dwords of a Vertex Buffer.  These
  /// implicit parameters are lowered to load instructions which retreive the
  /// values from the Vertex Buffer.
  SDValue LowerImplicitParameter(SelectionDAG &DAG, EVT VT,
                                 DebugLoc DL, unsigned DwordOffset) const;

  void lowerImplicitParameter(MachineInstr *MI, MachineBasicBlock &BB,
      MachineRegisterInfo & MRI, unsigned dword_offset) const;

  SDValue LowerBR_CC(SDValue Op, SelectionDAG &DAG) const;

  /// LowerROTL - Lower ROTL opcode to BITALIGN
  SDValue LowerROTL(SDValue Op, SelectionDAG &DAG) const;

  SDValue LowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSETCC(SDValue Op, SelectionDAG &DAG) const;

};

} // End namespace llvm;

#endif // R600ISELLOWERING_H
