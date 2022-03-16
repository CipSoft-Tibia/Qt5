//===-- SIMCCodeEmitter.cpp - SI Code Emitter -------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// The SI code emitter produces machine code that can be executed directly on
// the GPU device.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/AMDGPUMCTargetDesc.h"
#include "MCTargetDesc/AMDGPUMCCodeEmitter.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/raw_ostream.h"

#define LITERAL_REG 255
#define VGPR_BIT(src_idx) (1ULL << (9 * src_idx - 1))
#define SI_INSTR_FLAGS_ENCODING_MASK 0xf


// These must be kept in sync with SIInstructions.td and also the
// InstrEncodingInfo array in SIInstrInfo.cpp.
//
// NOTE: This enum is only used to identify the encoding type within LLVM,
// the actual encoding type that is part of the instruction format is different
namespace SIInstrEncodingType {
  enum Encoding {
    EXP = 0,
    LDS = 1,
    MIMG = 2,
    MTBUF = 3,
    MUBUF = 4,
    SMRD = 5,
    SOP1 = 6,
    SOP2 = 7,
    SOPC = 8,
    SOPK = 9,
    SOPP = 10,
    VINTRP = 11,
    VOP1 = 12,
    VOP2 = 13,
    VOP3 = 14,
    VOPC = 15
  };
}

using namespace llvm;

namespace {
class SIMCCodeEmitter : public  AMDGPUMCCodeEmitter {
  SIMCCodeEmitter(const SIMCCodeEmitter &); // DO NOT IMPLEMENT
  void operator=(const SIMCCodeEmitter &); // DO NOT IMPLEMENT
  const MCInstrInfo &MCII;
  const MCSubtargetInfo &STI;
  MCContext &Ctx;

public:
  SIMCCodeEmitter(const MCInstrInfo &mcii, const MCSubtargetInfo &sti,
                  MCContext &ctx)
    : MCII(mcii), STI(sti), Ctx(ctx) { }

  ~SIMCCodeEmitter() { }

  /// EncodeInstruction - Encode the instruction and write it to the OS.
  virtual void EncodeInstruction(const MCInst &MI, raw_ostream &OS,
                         SmallVectorImpl<MCFixup> &Fixups) const;

  /// getMachineOpValue - Reutrn the encoding for an MCOperand.
  virtual uint64_t getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                                     SmallVectorImpl<MCFixup> &Fixups) const;

public:

  /// GPRAlign - Encode a sequence of registers with the correct alignment.
  unsigned GPRAlign(const MCInst &MI, unsigned OpNo, unsigned shift) const;

  /// GPR2AlignEncode - Encoding for when 2 consecutive registers are used
  virtual unsigned GPR2AlignEncode(const MCInst &MI, unsigned OpNo,
                                   SmallVectorImpl<MCFixup> &Fixup) const;

  /// GPR4AlignEncode - Encoding for when 4 consectuive registers are used 
  virtual unsigned GPR4AlignEncode(const MCInst &MI, unsigned OpNo,
                                   SmallVectorImpl<MCFixup> &Fixup) const;

  /// i32LiteralEncode - Encode an i32 literal this is used as an operand
  /// for an instruction in place of a register.
  virtual uint64_t i32LiteralEncode(const MCInst &MI, unsigned OpNo,
                                   SmallVectorImpl<MCFixup> &Fixup) const;

  /// SMRDmemriEncode - Encoding for SMRD indexed loads
  virtual uint32_t SMRDmemriEncode(const MCInst &MI, unsigned OpNo,
                                   SmallVectorImpl<MCFixup> &Fixup) const;

  /// VOPPostEncode - Post-Encoder method for VOP instructions 
  virtual uint64_t VOPPostEncode(const MCInst &MI, uint64_t Value) const;

private:

  ///getEncodingType =  Return this SIInstrEncodingType for this instruction.
  unsigned getEncodingType(const MCInst &MI) const;

  ///getEncodingBytes - Get then size in bytes of this instructions encoding.
  unsigned getEncodingBytes(const MCInst &MI) const;

  /// getRegBinaryCode - Returns the hardware encoding for a register
  unsigned getRegBinaryCode(unsigned reg) const;

  /// getHWRegNum - Generated function that returns the hardware encoding for
  /// a register
  unsigned getHWRegNum(unsigned reg) const;

};

} // End anonymous namespace

MCCodeEmitter *llvm::createSIMCCodeEmitter(const MCInstrInfo &MCII,
                                           const MCSubtargetInfo &STI,
                                           MCContext &Ctx) {
  return new SIMCCodeEmitter(MCII, STI, Ctx);
}

void SIMCCodeEmitter::EncodeInstruction(const MCInst &MI, raw_ostream &OS,
                                       SmallVectorImpl<MCFixup> &Fixups) const {
  uint64_t Encoding = getBinaryCodeForInstr(MI, Fixups);
  unsigned bytes = getEncodingBytes(MI);
  for (unsigned i = 0; i < bytes; i++) {
    OS.write((uint8_t) ((Encoding >> (8 * i)) & 0xff));
  }
}

uint64_t SIMCCodeEmitter::getMachineOpValue(const MCInst &MI,
                                            const MCOperand &MO,
                                       SmallVectorImpl<MCFixup> &Fixups) const {
  if (MO.isReg()) {
    return getRegBinaryCode(MO.getReg());
  } else if (MO.isImm()) {
    return MO.getImm();
  } else if (MO.isFPImm()) {
    // XXX: Not all instructions can use inline literals
    // XXX: We should make sure this is a 32-bit constant
    return LITERAL_REG;
  } else{
    llvm_unreachable("Encoding of this operand type is not supported yet.");
  }
  return 0;
}

//===----------------------------------------------------------------------===//
// Custom Operand Encodings
//===----------------------------------------------------------------------===//

unsigned SIMCCodeEmitter::GPRAlign(const MCInst &MI, unsigned OpNo,
                                   unsigned shift) const {
  unsigned regCode = getRegBinaryCode(MI.getOperand(OpNo).getReg());
  return regCode >> shift;
  return 0;
}
unsigned SIMCCodeEmitter::GPR2AlignEncode(const MCInst &MI,
                                          unsigned OpNo ,
                                        SmallVectorImpl<MCFixup> &Fixup) const {
  return GPRAlign(MI, OpNo, 1);
}

unsigned SIMCCodeEmitter::GPR4AlignEncode(const MCInst &MI,
                                          unsigned OpNo,
                                        SmallVectorImpl<MCFixup> &Fixup) const {
  return GPRAlign(MI, OpNo, 2);
}

uint64_t SIMCCodeEmitter::i32LiteralEncode(const MCInst &MI,
                                           unsigned OpNo,
                                        SmallVectorImpl<MCFixup> &Fixup) const {
  return LITERAL_REG | (MI.getOperand(OpNo).getImm() << 32);
}

#define SMRD_OFFSET_MASK 0xff
#define SMRD_IMM_SHIFT 8
#define SMRD_SBASE_MASK 0x3f
#define SMRD_SBASE_SHIFT 9
/// SMRDmemriEncode - This function is responsibe for encoding the offset
/// and the base ptr for SMRD instructions it should return a bit string in
/// this format:
///
/// OFFSET = bits{7-0}
/// IMM    = bits{8}
/// SBASE  = bits{14-9}
///
uint32_t SIMCCodeEmitter::SMRDmemriEncode(const MCInst &MI, unsigned OpNo,
                                        SmallVectorImpl<MCFixup> &Fixup) const {
  uint32_t Encoding;

  const MCOperand &OffsetOp = MI.getOperand(OpNo + 1);

  //XXX: Use this function for SMRD loads with register offsets
  assert(OffsetOp.isImm());

  Encoding =
      (getMachineOpValue(MI, OffsetOp, Fixup) & SMRD_OFFSET_MASK)
    | (1 << SMRD_IMM_SHIFT) //XXX If the Offset is a register we shouldn't set this bit
    | ((GPR2AlignEncode(MI, OpNo, Fixup) & SMRD_SBASE_MASK) << SMRD_SBASE_SHIFT)
    ;

  return Encoding;
}

//===----------------------------------------------------------------------===//
// Post Encoder Callbacks
//===----------------------------------------------------------------------===//

uint64_t SIMCCodeEmitter::VOPPostEncode(const MCInst &MI, uint64_t Value) const{
  unsigned encodingType = getEncodingType(MI);
  unsigned numSrcOps;
  unsigned vgprBitOffset;

  if (encodingType == SIInstrEncodingType::VOP3) {
    numSrcOps = 3;
    vgprBitOffset = 32;
  } else {
    numSrcOps = 1;
    vgprBitOffset = 0;
  }

  // Add one to skip over the destination reg operand.
  for (unsigned opIdx = 1; opIdx < numSrcOps + 1; opIdx++) {
    const MCOperand &MO = MI.getOperand(opIdx);
    if (MO.isReg()) {
      unsigned reg = MI.getOperand(opIdx).getReg();
      if (AMDGPUMCRegisterClasses[AMDGPU::VReg_32RegClassID].contains(reg) ||
          AMDGPUMCRegisterClasses[AMDGPU::VReg_64RegClassID].contains(reg)) {
        Value |= (VGPR_BIT(opIdx)) << vgprBitOffset;
      }
    } else if (MO.isFPImm()) {
      union {
        float f;
        uint32_t i;
      } Imm;
      // XXX: Not all instructions can use inline literals
      // XXX: We should make sure this is a 32-bit constant
      Imm.f = MO.getFPImm();
      Value |= ((uint64_t)Imm.i) << 32;
    }
  }
  return Value;
}

//===----------------------------------------------------------------------===//
// Encoding helper functions
//===----------------------------------------------------------------------===//

unsigned SIMCCodeEmitter::getEncodingType(const MCInst &MI) const {
  return MCII.get(MI.getOpcode()).TSFlags & SI_INSTR_FLAGS_ENCODING_MASK;
}

unsigned SIMCCodeEmitter::getEncodingBytes(const MCInst &MI) const {

  // Instructions with literal constants are expanded to 64-bits, and
  // the constant is stored in bits [63:32]
  for (unsigned i = 0; i < MI.getNumOperands(); i++) {
    if (MI.getOperand(i).isFPImm()) {
      return 8;
    }
  }

  // This instruction always has a literal
  if (MI.getOpcode() == AMDGPU::S_MOV_IMM_I32) {
    return 8;
  }

  unsigned encoding_type = getEncodingType(MI);
  switch (encoding_type) {
    case SIInstrEncodingType::EXP:
    case SIInstrEncodingType::LDS:
    case SIInstrEncodingType::MUBUF:
    case SIInstrEncodingType::MTBUF:
    case SIInstrEncodingType::MIMG:
    case SIInstrEncodingType::VOP3:
      return 8;
    default:
      return 4;
  }
}


unsigned SIMCCodeEmitter::getRegBinaryCode(unsigned reg) const {
  switch (reg) {
    case AMDGPU::M0: return 124;
    case AMDGPU::SREG_LIT_0: return 128;
    default: return getHWRegNum(reg);
  }
}

#define SIRegisterInfo SIMCCodeEmitter
#include "SIRegisterGetHWRegNum.inc"
#undef SIRegisterInfo
