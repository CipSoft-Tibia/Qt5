//===-- R600InstrInfo.cpp - R600 Instruction Information ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// R600 Implementation of TargetInstrInfo.
//
//===----------------------------------------------------------------------===//

#include "R600InstrInfo.h"
#include "AMDGPUTargetMachine.h"
#include "AMDGPUSubtarget.h"
#include "R600Defines.h"
#include "R600RegisterInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "AMDILUtilityFunctions.h"

#define GET_INSTRINFO_CTOR
#include "AMDGPUGenDFAPacketizer.inc"

using namespace llvm;

R600InstrInfo::R600InstrInfo(AMDGPUTargetMachine &tm)
  : AMDGPUInstrInfo(tm),
    RI(tm, *this),
    TM(tm)
  { }

const R600RegisterInfo &R600InstrInfo::getRegisterInfo() const
{
  return RI;
}

bool R600InstrInfo::isTrig(const MachineInstr &MI) const
{
  return get(MI.getOpcode()).TSFlags & R600_InstFlag::TRIG;
}

bool R600InstrInfo::isVector(const MachineInstr &MI) const
{
  return get(MI.getOpcode()).TSFlags & R600_InstFlag::VECTOR;
}

void
R600InstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MI, DebugLoc DL,
                           unsigned DestReg, unsigned SrcReg,
                           bool KillSrc) const
{
  if (AMDGPU::R600_Reg128RegClass.contains(DestReg)
      && AMDGPU::R600_Reg128RegClass.contains(SrcReg)) {
    for (unsigned I = 0; I < 4; I++) {
      unsigned SubRegIndex = RI.getSubRegFromChannel(I);
      BuildMI(MBB, MI, DL, get(AMDGPU::MOV))
              .addReg(RI.getSubReg(DestReg, SubRegIndex), RegState::Define)
              .addReg(RI.getSubReg(SrcReg, SubRegIndex))
              .addImm(0) // Flag
              .addReg(0) // PREDICATE_BIT
              .addReg(DestReg, RegState::Define | RegState::Implicit);
    }
  } else {

    /* We can't copy vec4 registers */
    assert(!AMDGPU::R600_Reg128RegClass.contains(DestReg)
           && !AMDGPU::R600_Reg128RegClass.contains(SrcReg));

    BuildMI(MBB, MI, DL, get(AMDGPU::MOV), DestReg)
      .addReg(SrcReg, getKillRegState(KillSrc))
      .addImm(0) // Flag
      .addReg(0); // PREDICATE_BIT
  }
}

MachineInstr * R600InstrInfo::getMovImmInstr(MachineFunction *MF,
                                             unsigned DstReg, int64_t Imm) const
{
  MachineInstr * MI = MF->CreateMachineInstr(get(AMDGPU::MOV), DebugLoc());
  MachineInstrBuilder(MI).addReg(DstReg, RegState::Define);
  MachineInstrBuilder(MI).addReg(AMDGPU::ALU_LITERAL_X);
  MachineInstrBuilder(MI).addImm(Imm);
  MachineInstrBuilder(MI).addReg(0); // PREDICATE_BIT

  return MI;
}

unsigned R600InstrInfo::getIEQOpcode() const
{
  return AMDGPU::SETE_INT;
}

bool R600InstrInfo::isMov(unsigned Opcode) const
{


  switch(Opcode) {
  default: return false;
  case AMDGPU::MOV:
  case AMDGPU::MOV_IMM_F32:
  case AMDGPU::MOV_IMM_I32:
    return true;
  }
}

// Some instructions act as place holders to emulate operations that the GPU
// hardware does automatically. This function can be used to check if
// an opcode falls into this category.
bool R600InstrInfo::isPlaceHolderOpcode(unsigned Opcode) const
{
  switch (Opcode) {
  default: return false;
  case AMDGPU::RETURN:
  case AMDGPU::MASK_WRITE:
  case AMDGPU::RESERVE_REG:
    return true;
  }
}

bool R600InstrInfo::isReductionOp(unsigned Opcode) const
{
  switch(Opcode) {
    default: return false;
    case AMDGPU::DOT4_r600:
    case AMDGPU::DOT4_eg:
      return true;
  }
}

bool R600InstrInfo::isCubeOp(unsigned Opcode) const
{
  switch(Opcode) {
    default: return false;
    case AMDGPU::CUBE_r600_pseudo:
    case AMDGPU::CUBE_r600_real:
    case AMDGPU::CUBE_eg_pseudo:
    case AMDGPU::CUBE_eg_real:
      return true;
  }
}

DFAPacketizer *R600InstrInfo::CreateTargetScheduleState(const TargetMachine *TM,
    const ScheduleDAG *DAG) const
{
  const InstrItineraryData *II = TM->getInstrItineraryData();
  return TM->getSubtarget<AMDGPUSubtarget>().createDFAPacketizer(II);
}

static bool
isPredicateSetter(unsigned Opcode)
{
  switch (Opcode) {
  case AMDGPU::PRED_X:
    return true;
  default:
    return false;
  }
}

static MachineInstr *
findFirstPredicateSetterFrom(MachineBasicBlock &MBB,
                             MachineBasicBlock::iterator I)
{
  while (I != MBB.begin()) {
    --I;
    MachineInstr *MI = I;
    if (isPredicateSetter(MI->getOpcode()))
      return MI;
  }

  return NULL;
}

bool
R600InstrInfo::AnalyzeBranch(MachineBasicBlock &MBB,
                             MachineBasicBlock *&TBB,
                             MachineBasicBlock *&FBB,
                             SmallVectorImpl<MachineOperand> &Cond,
                             bool AllowModify) const
{
  // Most of the following comes from the ARM implementation of AnalyzeBranch

  // If the block has no terminators, it just falls into the block after it.
  MachineBasicBlock::iterator I = MBB.end();
  if (I == MBB.begin())
    return false;
  --I;
  while (I->isDebugValue()) {
    if (I == MBB.begin())
      return false;
    --I;
  }
  if (static_cast<MachineInstr *>(I)->getOpcode() != AMDGPU::JUMP) {
    return false;
  }

  // Get the last instruction in the block.
  MachineInstr *LastInst = I;

  // If there is only one terminator instruction, process it.
  unsigned LastOpc = LastInst->getOpcode();
  if (I == MBB.begin() ||
      static_cast<MachineInstr *>(--I)->getOpcode() != AMDGPU::JUMP) {
    if (LastOpc == AMDGPU::JUMP) {
      if(!isPredicated(LastInst)) {
        TBB = LastInst->getOperand(0).getMBB();
        return false;
      } else {
        MachineInstr *predSet = I;
        while (!isPredicateSetter(predSet->getOpcode())) {
          predSet = --I;
        }
        TBB = LastInst->getOperand(0).getMBB();
        Cond.push_back(predSet->getOperand(1));
        Cond.push_back(predSet->getOperand(2));
        Cond.push_back(MachineOperand::CreateReg(AMDGPU::PRED_SEL_ONE, false));
        return false;
      }
    }
    return true;  // Can't handle indirect branch.
  }

  // Get the instruction before it if it is a terminator.
  MachineInstr *SecondLastInst = I;
  unsigned SecondLastOpc = SecondLastInst->getOpcode();

  // If the block ends with a B and a Bcc, handle it.
  if (SecondLastOpc == AMDGPU::JUMP &&
      isPredicated(SecondLastInst) &&
      LastOpc == AMDGPU::JUMP &&
      !isPredicated(LastInst)) {
    MachineInstr *predSet = --I;
    while (!isPredicateSetter(predSet->getOpcode())) {
      predSet = --I;
    }
    TBB = SecondLastInst->getOperand(0).getMBB();
    FBB = LastInst->getOperand(0).getMBB();
    Cond.push_back(predSet->getOperand(1));
    Cond.push_back(predSet->getOperand(2));
    Cond.push_back(MachineOperand::CreateReg(AMDGPU::PRED_SEL_ONE, false));
    return false;
  }

  // Otherwise, can't handle this.
  return true;
}

int R600InstrInfo::getBranchInstr(const MachineOperand &op) const {
  const MachineInstr *MI = op.getParent();

  switch (MI->getDesc().OpInfo->RegClass) {
  default: // FIXME: fallthrough??
  case AMDGPU::GPRI32RegClassID: return AMDGPU::BRANCH_COND_i32;
  case AMDGPU::GPRF32RegClassID: return AMDGPU::BRANCH_COND_f32;
  };
}

unsigned
R600InstrInfo::InsertBranch(MachineBasicBlock &MBB,
                            MachineBasicBlock *TBB,
                            MachineBasicBlock *FBB,
                            const SmallVectorImpl<MachineOperand> &Cond,
                            DebugLoc DL) const
{
  assert(TBB && "InsertBranch must not be told to insert a fallthrough");

  if (FBB == 0) {
    if (Cond.empty()) {
      BuildMI(&MBB, DL, get(AMDGPU::JUMP)).addMBB(TBB).addReg(0);
      return 1;
    } else {
      MachineInstr *PredSet = findFirstPredicateSetterFrom(MBB, MBB.end());
      assert(PredSet && "No previous predicate !");
      addFlag(PredSet, 1, MO_FLAG_PUSH);
      PredSet->getOperand(2).setImm(Cond[1].getImm());

      BuildMI(&MBB, DL, get(AMDGPU::JUMP))
             .addMBB(TBB)
             .addReg(AMDGPU::PREDICATE_BIT, RegState::Kill);
      return 1;
    }
  } else {
    MachineInstr *PredSet = findFirstPredicateSetterFrom(MBB, MBB.end());
    assert(PredSet && "No previous predicate !");
    addFlag(PredSet, 1, MO_FLAG_PUSH);
    PredSet->getOperand(2).setImm(Cond[1].getImm());
    BuildMI(&MBB, DL, get(AMDGPU::JUMP))
            .addMBB(TBB)
            .addReg(AMDGPU::PREDICATE_BIT, RegState::Kill);
    BuildMI(&MBB, DL, get(AMDGPU::JUMP)).addMBB(FBB).addReg(0);
    return 2;
  }
}

unsigned
R600InstrInfo::RemoveBranch(MachineBasicBlock &MBB) const
{

  // Note : we leave PRED* instructions there.
  // They may be needed when predicating instructions.

  MachineBasicBlock::iterator I = MBB.end();

  if (I == MBB.begin()) {
    return 0;
  }
  --I;
  switch (I->getOpcode()) {
  default:
    return 0;
  case AMDGPU::JUMP:
    if (isPredicated(I)) {
      MachineInstr *predSet = findFirstPredicateSetterFrom(MBB, I);
      clearFlag(predSet, 1, MO_FLAG_PUSH);
    }
    I->eraseFromParent();
    break;
  }
  I = MBB.end();

  if (I == MBB.begin()) {
    return 1;
  }
  --I;
  switch (I->getOpcode()) {
    // FIXME: only one case??
  default:
    return 1;
  case AMDGPU::JUMP:
    if (isPredicated(I)) {
      MachineInstr *predSet = findFirstPredicateSetterFrom(MBB, I);
      clearFlag(predSet, 1, MO_FLAG_PUSH);
    }
    I->eraseFromParent();
    break;
  }
  return 2;
}

bool
R600InstrInfo::isPredicated(const MachineInstr *MI) const
{
  int idx = MI->findFirstPredOperandIdx();
  if (idx < 0)
    return false;

  unsigned Reg = MI->getOperand(idx).getReg();
  switch (Reg) {
  default: return false;
  case AMDGPU::PRED_SEL_ONE:
  case AMDGPU::PRED_SEL_ZERO:
  case AMDGPU::PREDICATE_BIT:
    return true;
  }
}

bool
R600InstrInfo::isPredicable(MachineInstr *MI) const
{
  return AMDGPUInstrInfo::isPredicable(MI);
}


bool
R600InstrInfo::isProfitableToIfCvt(MachineBasicBlock &MBB,
                                   unsigned NumCyles,
                                   unsigned ExtraPredCycles,
                                   const BranchProbability &Probability) const{
  return true;
}

bool
R600InstrInfo::isProfitableToIfCvt(MachineBasicBlock &TMBB,
                                   unsigned NumTCycles,
                                   unsigned ExtraTCycles,
                                   MachineBasicBlock &FMBB,
                                   unsigned NumFCycles,
                                   unsigned ExtraFCycles,
                                   const BranchProbability &Probability) const
{
  return true;
}

bool
R600InstrInfo::isProfitableToDupForIfCvt(MachineBasicBlock &MBB,
                                         unsigned NumCyles,
                                         const BranchProbability &Probability)
                                         const
{
  return true;
}

bool
R600InstrInfo::isProfitableToUnpredicate(MachineBasicBlock &TMBB,
                                         MachineBasicBlock &FMBB) const
{
  return false;
}


bool
R600InstrInfo::ReverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const
{
  MachineOperand &MO = Cond[1];
  switch (MO.getImm()) {
  case OPCODE_IS_ZERO_INT:
    MO.setImm(OPCODE_IS_NOT_ZERO_INT);
    break;
  case OPCODE_IS_NOT_ZERO_INT:
    MO.setImm(OPCODE_IS_ZERO_INT);
    break;
  case OPCODE_IS_ZERO:
    MO.setImm(OPCODE_IS_NOT_ZERO);
    break;
  case OPCODE_IS_NOT_ZERO:
    MO.setImm(OPCODE_IS_ZERO);
    break;
  default:
    return true;
  }

  MachineOperand &MO2 = Cond[2];
  switch (MO2.getReg()) {
  case AMDGPU::PRED_SEL_ZERO:
    MO2.setReg(AMDGPU::PRED_SEL_ONE);
    break;
  case AMDGPU::PRED_SEL_ONE:
    MO2.setReg(AMDGPU::PRED_SEL_ZERO);
    break;
  default:
    return true;
  }
  return false;
}

bool
R600InstrInfo::DefinesPredicate(MachineInstr *MI,
                                std::vector<MachineOperand> &Pred) const
{
  return isPredicateSetter(MI->getOpcode());
}


bool
R600InstrInfo::SubsumesPredicate(const SmallVectorImpl<MachineOperand> &Pred1,
                       const SmallVectorImpl<MachineOperand> &Pred2) const
{
  return false;
}


bool
R600InstrInfo::PredicateInstruction(MachineInstr *MI,
                      const SmallVectorImpl<MachineOperand> &Pred) const
{
  int PIdx = MI->findFirstPredOperandIdx();

  if (PIdx != -1) {
    MachineOperand &PMO = MI->getOperand(PIdx);
    PMO.setReg(Pred[2].getReg());
    MachineInstrBuilder(MI).addReg(AMDGPU::PREDICATE_BIT, RegState::Implicit);
    return true;
  }

  return false;
}

int R600InstrInfo::getInstrLatency(const InstrItineraryData *ItinData,
                                   const MachineInstr *MI,
                                   unsigned *PredCost) const
{
  if (PredCost)
    *PredCost = 2;
  return 2;
}

//===----------------------------------------------------------------------===//
// Instruction flag getters/setters
//===----------------------------------------------------------------------===//

bool R600InstrInfo::hasFlagOperand(const MachineInstr &MI) const
{
  return GET_FLAG_OPERAND_IDX(get(MI.getOpcode()).TSFlags) != 0;
}

MachineOperand &R600InstrInfo::getFlagOp(MachineInstr *MI) const
{
  unsigned FlagIndex = GET_FLAG_OPERAND_IDX(get(MI->getOpcode()).TSFlags);
  assert(FlagIndex != 0 &&
         "Instruction flags not supported for this instruction");
  MachineOperand &FlagOp = MI->getOperand(FlagIndex);
  assert(FlagOp.isImm());
  return FlagOp;
}

void R600InstrInfo::addFlag(MachineInstr *MI, unsigned Operand,
                            unsigned Flag) const
{
  MachineOperand &FlagOp = getFlagOp(MI);
  FlagOp.setImm(FlagOp.getImm() | (Flag << (NUM_MO_FLAGS * Operand)));
}

void R600InstrInfo::clearFlag(MachineInstr *MI, unsigned Operand,
                              unsigned Flag) const
{
  MachineOperand &FlagOp = getFlagOp(MI);
  unsigned InstFlags = FlagOp.getImm();
  InstFlags &= ~(Flag << (NUM_MO_FLAGS * Operand));
  FlagOp.setImm(InstFlags);
}
