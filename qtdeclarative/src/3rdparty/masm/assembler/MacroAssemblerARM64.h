/*
 * Copyright (C) 2012, 2014, 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef MacroAssemblerARM64_h
#define MacroAssemblerARM64_h

#if ENABLE(ASSEMBLER) && CPU(ARM64)

#include "ARM64Assembler.h"
#include "AbstractMacroAssembler.h"
#include <wtf/MathExtras.h>

namespace JSC {

class MacroAssemblerARM64 : public AbstractMacroAssembler<ARM64Assembler> {
#define COPIED_FROM_AbstractAssembler_H 1
#ifdef COPIED_FROM_AbstractAssembler_H
    typedef MacroAssemblerARM64 AbstractMacroAssemblerType;
    class CachedTempRegister {
        friend class DataLabelPtr;
        friend class DataLabel32;
        friend class DataLabelCompact;
//        template <typename> friend class Jump;
        friend class Label;

    public:
        CachedTempRegister(AbstractMacroAssemblerType* masm, RegisterID registerID)
            : m_masm(masm)
            , m_registerID(registerID)
            , m_value(0)
            , m_validBit(1 << static_cast<unsigned>(registerID))
        {
            ASSERT(static_cast<unsigned>(registerID) < (sizeof(unsigned) * 8));
        }

        ALWAYS_INLINE RegisterID registerIDInvalidate() { invalidate(); return m_registerID; }

        ALWAYS_INLINE RegisterID registerIDNoInvalidate() { return m_registerID; }

        bool value(intptr_t& value)
        {
            value = m_value;
            return m_masm->isTempRegisterValid(m_validBit);
        }

        void setValue(intptr_t value)
        {
            m_value = value;
            m_masm->setTempRegisterValid(m_validBit);
        }

        ALWAYS_INLINE void invalidate() { m_masm->clearTempRegisterValid(m_validBit); }

    private:
        AbstractMacroAssemblerType* m_masm;
        RegisterID m_registerID;
        intptr_t m_value;
        unsigned m_validBit;
    };

    ALWAYS_INLINE void invalidateAllTempRegisters()
    {
        m_tempRegistersValidBits = 0;
    }

    ALWAYS_INLINE bool isTempRegisterValid(unsigned registerMask)
    {
        return (m_tempRegistersValidBits & registerMask);
    }

    ALWAYS_INLINE void clearTempRegisterValid(unsigned registerMask)
    {
        m_tempRegistersValidBits &=  ~registerMask;
    }

    ALWAYS_INLINE void setTempRegisterValid(unsigned registerMask)
    {
        m_tempRegistersValidBits |= registerMask;
    }

    friend class AllowMacroScratchRegisterUsage;
    friend class DisallowMacroScratchRegisterUsage;
    unsigned m_tempRegistersValidBits;
#endif // COPIED_FROM_AbstractAssembler_H
public:
    static const RegisterID dataTempRegister = ARM64Registers::ip0;
    static const RegisterID memoryTempRegister = ARM64Registers::ip1;

#if 0
    RegisterID scratchRegister()
    {
        RELEASE_ASSERT(m_allowScratchRegister);
        return getCachedDataTempRegisterIDAndInvalidate();
    }
#endif

private:
    static const ARM64Registers::FPRegisterID fpTempRegister = ARM64Registers::q31;
    static const ARM64Assembler::SetFlags S = ARM64Assembler::S;
    static const int64_t maskHalfWord0 = 0xffffl;
    static const int64_t maskHalfWord1 = 0xffff0000l;
    static const int64_t maskUpperWord = 0xffffffff00000000l;

    // 4 instructions - 3 to load the function pointer, + blr.
    static const ptrdiff_t REPATCH_OFFSET_CALL_TO_POINTER = -16;
    
public:
    static const int PointerSize = 8;

    MacroAssemblerARM64()
        : m_dataMemoryTempRegister(this, dataTempRegister)
        , m_cachedMemoryTempRegister(this, memoryTempRegister)
        , m_makeJumpPatchable(false)
    {
    }

    typedef ARM64Assembler::LinkRecord LinkRecord;
    typedef ARM64Assembler::JumpType JumpType;
    typedef ARM64Assembler::JumpLinkType JumpLinkType;
    typedef ARM64Assembler::Condition Condition;

    static const ARM64Assembler::Condition DefaultCondition = ARM64Assembler::ConditionInvalid;
    static const ARM64Assembler::JumpType DefaultJump = ARM64Assembler::JumpNoConditionFixedSize;

    Vector<LinkRecord, 0, UnsafeVectorOverflow>& jumpsToLink() { return m_assembler.jumpsToLink(); }
    void* unlinkedCode() { return m_assembler.unlinkedCode(); }
    static bool canCompact(JumpType jumpType) { return ARM64Assembler::canCompact(jumpType); }
    static JumpLinkType computeJumpType(JumpType jumpType, const uint8_t* from, const uint8_t* to) { return ARM64Assembler::computeJumpType(jumpType, from, to); }
    static JumpLinkType computeJumpType(LinkRecord& record, const uint8_t* from, const uint8_t* to) { return ARM64Assembler::computeJumpType(record, from, to); }
    static int jumpSizeDelta(JumpType jumpType, JumpLinkType jumpLinkType) { return ARM64Assembler::jumpSizeDelta(jumpType, jumpLinkType); }
    static void link(LinkRecord& record, uint8_t* from, uint8_t* to) { return ARM64Assembler::link(record, from, to); }

    static const Scale ScalePtr = TimesEight;

    static bool isCompactPtrAlignedAddressOffset(ptrdiff_t value)
    {
        // This is the largest 32-bit access allowed, aligned to 64-bit boundary.
        return !(value & ~0x3ff8);
    }

    enum RelationalCondition {
        Equal = ARM64Assembler::ConditionEQ,
        NotEqual = ARM64Assembler::ConditionNE,
        Above = ARM64Assembler::ConditionHI,
        AboveOrEqual = ARM64Assembler::ConditionHS,
        Below = ARM64Assembler::ConditionLO,
        BelowOrEqual = ARM64Assembler::ConditionLS,
        GreaterThan = ARM64Assembler::ConditionGT,
        GreaterThanOrEqual = ARM64Assembler::ConditionGE,
        LessThan = ARM64Assembler::ConditionLT,
        LessThanOrEqual = ARM64Assembler::ConditionLE
    };

    enum ResultCondition {
        Overflow = ARM64Assembler::ConditionVS,
        Signed = ARM64Assembler::ConditionMI,
        PositiveOrZero = ARM64Assembler::ConditionPL,
        Zero = ARM64Assembler::ConditionEQ,
        NonZero = ARM64Assembler::ConditionNE
    };

    enum ZeroCondition {
        IsZero = ARM64Assembler::ConditionEQ,
        IsNonZero = ARM64Assembler::ConditionNE
    };

    enum DoubleCondition {
        // These conditions will only evaluate to true if the comparison is ordered - i.e. neither operand is NaN.
        DoubleEqual = ARM64Assembler::ConditionEQ,
        DoubleNotEqual = ARM64Assembler::ConditionVC, // Not the right flag! check for this & handle differently.
        DoubleGreaterThan = ARM64Assembler::ConditionGT,
        DoubleGreaterThanOrEqual = ARM64Assembler::ConditionGE,
        DoubleLessThan = ARM64Assembler::ConditionLO,
        DoubleLessThanOrEqual = ARM64Assembler::ConditionLS,
        // If either operand is NaN, these conditions always evaluate to true.
        DoubleEqualOrUnordered = ARM64Assembler::ConditionVS, // Not the right flag! check for this & handle differently.
        DoubleNotEqualOrUnordered = ARM64Assembler::ConditionNE,
        DoubleGreaterThanOrUnordered = ARM64Assembler::ConditionHI,
        DoubleGreaterThanOrEqualOrUnordered = ARM64Assembler::ConditionHS,
        DoubleLessThanOrUnordered = ARM64Assembler::ConditionLT,
        DoubleLessThanOrEqualOrUnordered = ARM64Assembler::ConditionLE,
    };

    static const RegisterID stackPointerRegister = ARM64Registers::sp;
    static const RegisterID framePointerRegister = ARM64Registers::fp;
    static const RegisterID linkRegister = ARM64Registers::lr;

    // FIXME: Get reasonable implementations for these
    static bool shouldBlindForSpecificArch(uint32_t value) { return value >= 0x00ffffff; }
    static bool shouldBlindForSpecificArch(uint64_t value) { return value >= 0x00ffffff; }

    // Integer operations:

    void add32(RegisterID a, RegisterID b, RegisterID dest)
    {
        ASSERT(a != ARM64Registers::sp && b != ARM64Registers::sp);
        m_assembler.add<32>(dest, a, b);
    }

    void add32(RegisterID src, RegisterID dest)
    {
        m_assembler.add<32>(dest, dest, src);
    }

    void add32(TrustedImm32 imm, RegisterID dest)
    {
        if (!imm.m_value)
            return;

        add32(imm, dest, dest);
    }

    void add32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        if (!imm.m_value) {
            move(src, dest);
            return;
        }

        if (isUInt12(imm.m_value))
            m_assembler.add<32>(dest, src, UInt12(imm.m_value));
        else if (isUInt12(-imm.m_value))
            m_assembler.sub<32>(dest, src, UInt12(-imm.m_value));
        else {
            move(imm, getCachedDataTempRegisterIDAndInvalidate());
            m_assembler.add<32>(dest, src, dataTempRegister);
        }
    }

    void add32(TrustedImm32 imm, Address address)
    {
        if (!imm.m_value)
            return;

        load32(address, getCachedDataTempRegisterIDAndInvalidate());

        if (isUInt12(imm.m_value))
            m_assembler.add<32>(dataTempRegister, dataTempRegister, UInt12(imm.m_value));
        else if (isUInt12(-imm.m_value))
            m_assembler.sub<32>(dataTempRegister, dataTempRegister, UInt12(-imm.m_value));
        else {
            move(imm, getCachedMemoryTempRegisterIDAndInvalidate());
            m_assembler.add<32>(dataTempRegister, dataTempRegister, memoryTempRegister);
        }

        store32(dataTempRegister, address);
    }

    void add32(TrustedImm32 imm, AbsoluteAddress address)
    {
        if (!imm.m_value)
            return;

        load32(address.m_ptr, getCachedDataTempRegisterIDAndInvalidate());

        if (isUInt12(imm.m_value)) {
            m_assembler.add<32>(dataTempRegister, dataTempRegister, UInt12(imm.m_value));
            store32(dataTempRegister, address.m_ptr);
            return;
        }

        if (isUInt12(-imm.m_value)) {
            m_assembler.sub<32>(dataTempRegister, dataTempRegister, UInt12(-imm.m_value));
            store32(dataTempRegister, address.m_ptr);
            return;
        }

        move(imm, getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.add<32>(dataTempRegister, dataTempRegister, memoryTempRegister);
        store32(dataTempRegister, address.m_ptr);
    }

    void add32(Address src, RegisterID dest)
    {
        load32(src, getCachedDataTempRegisterIDAndInvalidate());
        add32(dataTempRegister, dest);
    }

    void add64(RegisterID a, RegisterID b, RegisterID dest)
    {
        ASSERT(a != ARM64Registers::sp || b != ARM64Registers::sp);
        if (b == ARM64Registers::sp)
            std::swap(a, b);
        m_assembler.add<64>(dest, a, b);
    }

    void add64(RegisterID src, RegisterID dest)
    {
        if (src == ARM64Registers::sp)
            m_assembler.add<64>(dest, src, dest);
        else
            m_assembler.add<64>(dest, dest, src);
    }

    void add64(TrustedImm32 imm, RegisterID dest)
    {
        if (!imm.m_value)
            return;

        if (isUInt12(imm.m_value)) {
            m_assembler.add<64>(dest, dest, UInt12(imm.m_value));
            return;
        }
        if (isUInt12(-imm.m_value)) {
            m_assembler.sub<64>(dest, dest, UInt12(-imm.m_value));
            return;
        }

        signExtend32ToPtr(imm, getCachedDataTempRegisterIDAndInvalidate());
        m_assembler.add<64>(dest, dest, dataTempRegister);
    }

    void add64(TrustedImm64 imm, RegisterID dest)
    {
        intptr_t immediate = imm.m_value;
        if (!immediate)
            return;

        if (isUInt12(immediate)) {
            m_assembler.add<64>(dest, dest, UInt12(static_cast<int32_t>(immediate)));
            return;
        }
        if (isUInt12(-immediate)) {
            m_assembler.sub<64>(dest, dest, UInt12(static_cast<int32_t>(-immediate)));
            return;
        }

        move(imm, getCachedDataTempRegisterIDAndInvalidate());
        m_assembler.add<64>(dest, dest, dataTempRegister);
    }

    void add64(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        if (!imm.m_value) {
            move(src, dest);
            return;
        }

        if (isUInt12(imm.m_value)) {
            m_assembler.add<64>(dest, src, UInt12(imm.m_value));
            return;
        }
        if (isUInt12(-imm.m_value)) {
            m_assembler.sub<64>(dest, src, UInt12(-imm.m_value));
            return;
        }

        signExtend32ToPtr(imm, getCachedDataTempRegisterIDAndInvalidate());
        m_assembler.add<64>(dest, src, dataTempRegister);
    }

    void add64(TrustedImm32 imm, Address address)
    {
        if (!imm.m_value)
            return;

        load64(address, getCachedDataTempRegisterIDAndInvalidate());

        if (isUInt12(imm.m_value))
            m_assembler.add<64>(dataTempRegister, dataTempRegister, UInt12(imm.m_value));
        else if (isUInt12(-imm.m_value))
            m_assembler.sub<64>(dataTempRegister, dataTempRegister, UInt12(-imm.m_value));
        else {
            signExtend32ToPtr(imm, getCachedMemoryTempRegisterIDAndInvalidate());
            m_assembler.add<64>(dataTempRegister, dataTempRegister, memoryTempRegister);
        }

        store64(dataTempRegister, address);
    }

    void add64(TrustedImm32 imm, AbsoluteAddress address)
    {
        if (!imm.m_value)
            return;

        load64(address.m_ptr, getCachedDataTempRegisterIDAndInvalidate());

        if (isUInt12(imm.m_value)) {
            m_assembler.add<64>(dataTempRegister, dataTempRegister, UInt12(imm.m_value));
            store64(dataTempRegister, address.m_ptr);
            return;
        }

        if (isUInt12(-imm.m_value)) {
            m_assembler.sub<64>(dataTempRegister, dataTempRegister, UInt12(-imm.m_value));
            store64(dataTempRegister, address.m_ptr);
            return;
        }

        signExtend32ToPtr(imm, getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.add<64>(dataTempRegister, dataTempRegister, memoryTempRegister);
        store64(dataTempRegister, address.m_ptr);
    }

    void addPtrNoFlags(TrustedImm32 imm, RegisterID srcDest)
    {
        add64(imm, srcDest);
    }

    void add64(Address src, RegisterID dest)
    {
        load64(src, getCachedDataTempRegisterIDAndInvalidate());
        m_assembler.add<64>(dest, dest, dataTempRegister);
    }

    void add64(AbsoluteAddress src, RegisterID dest)
    {
        load64(src.m_ptr, getCachedDataTempRegisterIDAndInvalidate());
        m_assembler.add<64>(dest, dest, dataTempRegister);
    }

    void and32(RegisterID src, RegisterID dest)
    {
        and32(dest, src, dest);
    }

    void and32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.and_<32>(dest, op1, op2);
    }

    void and32(TrustedImm32 imm, RegisterID dest)
    {
        and32(imm, dest, dest);
    }

    void and32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        LogicalImmediate logicalImm = LogicalImmediate::create32(imm.m_value);

        if (logicalImm.isValid()) {
            m_assembler.and_<32>(dest, src, logicalImm);
            return;
        }

        move(imm, getCachedDataTempRegisterIDAndInvalidate());
        m_assembler.and_<32>(dest, src, dataTempRegister);
    }

    void and32(Address src, RegisterID dest)
    {
        load32(src, dataTempRegister);
        and32(dataTempRegister, dest);
    }

    void and64(RegisterID src, RegisterID dest)
    {
        m_assembler.and_<64>(dest, dest, src);
    }

    void and64(TrustedImm32 imm, RegisterID dest)
    {
        LogicalImmediate logicalImm = LogicalImmediate::create64(static_cast<intptr_t>(static_cast<int64_t>(imm.m_value)));

        if (logicalImm.isValid()) {
            m_assembler.and_<64>(dest, dest, logicalImm);
            return;
        }

        signExtend32ToPtr(imm, getCachedDataTempRegisterIDAndInvalidate());
        m_assembler.and_<64>(dest, dest, dataTempRegister);
    }

    void and64(TrustedImmPtr imm, RegisterID dest)
    {
        LogicalImmediate logicalImm = LogicalImmediate::create64(reinterpret_cast<uint64_t>(imm.m_value));

        if (logicalImm.isValid()) {
            m_assembler.and_<64>(dest, dest, logicalImm);
            return;
        }

        move(imm, getCachedDataTempRegisterIDAndInvalidate());
        m_assembler.and_<64>(dest, dest, dataTempRegister);
    }
    
    void countLeadingZeros32(RegisterID src, RegisterID dest)
    {
        m_assembler.clz<32>(dest, src);
    }

    void countLeadingZeros64(RegisterID src, RegisterID dest)
    {
        m_assembler.clz<64>(dest, src);
    }

    void lshift32(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.lsl<32>(dest, src, shiftAmount);
    }

    void lshift32(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.lsl<32>(dest, src, imm.m_value & 0x1f);
    }

    void lshift32(RegisterID shiftAmount, RegisterID dest)
    {
        lshift32(dest, shiftAmount, dest);
    }

    void lshift32(TrustedImm32 imm, RegisterID dest)
    {
        lshift32(dest, imm, dest);
    }

    void lshift64(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.lsl<64>(dest, src, shiftAmount);
    }

    void lshift64(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.lsl<64>(dest, src, imm.m_value & 0x3f);
    }

    void lshift64(RegisterID shiftAmount, RegisterID dest)
    {
        lshift64(dest, shiftAmount, dest);
    }

    void lshift64(TrustedImm32 imm, RegisterID dest)
    {
        lshift64(dest, imm, dest);
    }

    void mul32(RegisterID left, RegisterID right, RegisterID dest)
    {
        m_assembler.mul<32>(dest, left, right);
    }
    
    void mul32(RegisterID src, RegisterID dest)
    {
        m_assembler.mul<32>(dest, dest, src);
    }

    void mul32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        move(imm, getCachedDataTempRegisterIDAndInvalidate());
        m_assembler.mul<32>(dest, src, dataTempRegister);
    }

    void mul32(Address src, RegisterID dest)
    {
        load32(src, dataTempRegister);
        mul32(dataTempRegister, dest);
    }

    void mul64(RegisterID src, RegisterID dest)
    {
        m_assembler.mul<64>(dest, dest, src);
    }

    void mul64(RegisterID left, RegisterID right, RegisterID dest)
    {
        m_assembler.mul<64>(dest, left, right);
    }

    void div32(RegisterID dividend, RegisterID divisor, RegisterID dest)
    {
        m_assembler.sdiv<32>(dest, dividend, divisor);
    }

    void div64(RegisterID dividend, RegisterID divisor, RegisterID dest)
    {
        m_assembler.sdiv<64>(dest, dividend, divisor);
    }

    void neg32(RegisterID dest)
    {
        m_assembler.neg<32>(dest, dest);
    }

    void neg64(RegisterID dest)
    {
        m_assembler.neg<64>(dest, dest);
    }

    void or32(RegisterID src, RegisterID dest)
    {
        or32(dest, src, dest);
    }

    void or32(Address src, RegisterID dest)
    {
        load32(src, dataTempRegister);
        or32(dataTempRegister, dest);
    }

    void or32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.orr<32>(dest, op1, op2);
    }

    void or32(TrustedImm32 imm, RegisterID dest)
    {
        or32(imm, dest, dest);
    }

    void or32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        LogicalImmediate logicalImm = LogicalImmediate::create32(imm.m_value);

        if (logicalImm.isValid()) {
            m_assembler.orr<32>(dest, src, logicalImm);
            return;
        }

        ASSERT(src != dataTempRegister);
        move(imm, getCachedDataTempRegisterIDAndInvalidate());
        m_assembler.orr<32>(dest, src, dataTempRegister);
    }

    void or32(RegisterID src, AbsoluteAddress address)
    {
        load32(address.m_ptr, getCachedDataTempRegisterIDAndInvalidate());
        m_assembler.orr<32>(dataTempRegister, dataTempRegister, src);
        store32(dataTempRegister, address.m_ptr);
    }

    void or32(TrustedImm32 imm, AbsoluteAddress address)
    {
        LogicalImmediate logicalImm = LogicalImmediate::create32(imm.m_value);
        if (logicalImm.isValid()) {
            load32(address.m_ptr, getCachedDataTempRegisterIDAndInvalidate());
            m_assembler.orr<32>(dataTempRegister, dataTempRegister, logicalImm);
            store32(dataTempRegister, address.m_ptr);
        } else {
            load32(address.m_ptr, getCachedMemoryTempRegisterIDAndInvalidate());
            or32(imm, memoryTempRegister, getCachedDataTempRegisterIDAndInvalidate());
            store32(dataTempRegister, address.m_ptr);
        }
    }

    void or32(TrustedImm32 imm, Address address)
    {
        load32(address, getCachedDataTempRegisterIDAndInvalidate());
        or32(imm, dataTempRegister, dataTempRegister);
        store32(dataTempRegister, address);
    }

    void or64(RegisterID src, RegisterID dest)
    {
        or64(dest, src, dest);
    }

    void or64(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.orr<64>(dest, op1, op2);
    }

    void or64(TrustedImm32 imm, RegisterID dest)
    {
        or64(imm, dest, dest);
    }

    void or64(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        LogicalImmediate logicalImm = LogicalImmediate::create64(static_cast<intptr_t>(static_cast<int64_t>(imm.m_value)));

        if (logicalImm.isValid()) {
            m_assembler.orr<64>(dest, src, logicalImm);
            return;
        }

        signExtend32ToPtr(imm, getCachedDataTempRegisterIDAndInvalidate());
        m_assembler.orr<64>(dest, src, dataTempRegister);
    }
    
    void or64(TrustedImm64 imm, RegisterID dest)
    {
        or64(imm, dest, dest);
    }

    void or64(TrustedImm64 imm, RegisterID src, RegisterID dest)
    {
        LogicalImmediate logicalImm = LogicalImmediate::create64(static_cast<intptr_t>(static_cast<int64_t>(imm.m_value)));

        if (logicalImm.isValid()) {
            m_assembler.orr<64>(dest, src, logicalImm);
            return;
        }

        move(imm, getCachedDataTempRegisterIDAndInvalidate());
        m_assembler.orr<64>(dest, src, dataTempRegister);
    }

    void rotateRight64(TrustedImm32 imm, RegisterID srcDst)
    {
        m_assembler.ror<64>(srcDst, srcDst, imm.m_value & 63);
    }

    void rshift32(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.asr<32>(dest, src, shiftAmount);
    }

    void rshift32(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.asr<32>(dest, src, imm.m_value & 0x1f);
    }

    void rshift32(RegisterID shiftAmount, RegisterID dest)
    {
        rshift32(dest, shiftAmount, dest);
    }
    
    void rshift32(TrustedImm32 imm, RegisterID dest)
    {
        rshift32(dest, imm, dest);
    }
    
    void rshift64(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.asr<64>(dest, src, shiftAmount);
    }
    
    void rshift64(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.asr<64>(dest, src, imm.m_value & 0x3f);
    }
    
    void rshift64(RegisterID shiftAmount, RegisterID dest)
    {
        rshift64(dest, shiftAmount, dest);
    }
    
    void rshift64(TrustedImm32 imm, RegisterID dest)
    {
        rshift64(dest, imm, dest);
    }

    void sub32(RegisterID src, RegisterID dest)
    {
        m_assembler.sub<32>(dest, dest, src);
    }

    void sub32(TrustedImm32 imm, RegisterID dest)
    {
        if (isUInt12(imm.m_value)) {
            m_assembler.sub<32>(dest, dest, UInt12(imm.m_value));
            return;
        }
        if (isUInt12(-imm.m_value)) {
            m_assembler.add<32>(dest, dest, UInt12(-imm.m_value));
            return;
        }

        move(imm, getCachedDataTempRegisterIDAndInvalidate());
        m_assembler.sub<32>(dest, dest, dataTempRegister);
    }

    void sub32(TrustedImm32 imm, Address address)
    {
        load32(address, getCachedDataTempRegisterIDAndInvalidate());

        if (isUInt12(imm.m_value))
            m_assembler.sub<32>(dataTempRegister, dataTempRegister, UInt12(imm.m_value));
        else if (isUInt12(-imm.m_value))
            m_assembler.add<32>(dataTempRegister, dataTempRegister, UInt12(-imm.m_value));
        else {
            move(imm, getCachedMemoryTempRegisterIDAndInvalidate());
            m_assembler.sub<32>(dataTempRegister, dataTempRegister, memoryTempRegister);
        }

        store32(dataTempRegister, address);
    }

    void sub32(TrustedImm32 imm, AbsoluteAddress address)
    {
        load32(address.m_ptr, getCachedDataTempRegisterIDAndInvalidate());

        if (isUInt12(imm.m_value)) {
            m_assembler.sub<32>(dataTempRegister, dataTempRegister, UInt12(imm.m_value));
            store32(dataTempRegister, address.m_ptr);
            return;
        }

        if (isUInt12(-imm.m_value)) {
            m_assembler.add<32>(dataTempRegister, dataTempRegister, UInt12(-imm.m_value));
            store32(dataTempRegister, address.m_ptr);
            return;
        }

        move(imm, getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.sub<32>(dataTempRegister, dataTempRegister, memoryTempRegister);
        store32(dataTempRegister, address.m_ptr);
    }

    void sub32(Address src, RegisterID dest)
    {
        load32(src, getCachedDataTempRegisterIDAndInvalidate());
        sub32(dataTempRegister, dest);
    }

    void sub64(RegisterID src, RegisterID dest)
    {
        m_assembler.sub<64>(dest, dest, src);
    }
    
    void sub64(TrustedImm32 imm, RegisterID dest)
    {
        if (isUInt12(imm.m_value)) {
            m_assembler.sub<64>(dest, dest, UInt12(imm.m_value));
            return;
        }
        if (isUInt12(-imm.m_value)) {
            m_assembler.add<64>(dest, dest, UInt12(-imm.m_value));
            return;
        }

        signExtend32ToPtr(imm, getCachedDataTempRegisterIDAndInvalidate());
        m_assembler.sub<64>(dest, dest, dataTempRegister);
    }
    
    void sub64(TrustedImm64 imm, RegisterID dest)
    {
        intptr_t immediate = imm.m_value;

        if (isUInt12(immediate)) {
            m_assembler.sub<64>(dest, dest, UInt12(static_cast<int32_t>(immediate)));
            return;
        }
        if (isUInt12(-immediate)) {
            m_assembler.add<64>(dest, dest, UInt12(static_cast<int32_t>(-immediate)));
            return;
        }

        move(imm, getCachedDataTempRegisterIDAndInvalidate());
        m_assembler.sub<64>(dest, dest, dataTempRegister);
    }

    void urshift32(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.lsr<32>(dest, src, shiftAmount);
    }
    
    void urshift32(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.lsr<32>(dest, src, imm.m_value & 0x1f);
    }

    void urshift32(RegisterID shiftAmount, RegisterID dest)
    {
        urshift32(dest, shiftAmount, dest);
    }
    
    void urshift32(TrustedImm32 imm, RegisterID dest)
    {
        urshift32(dest, imm, dest);
    }

    void urshift64(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.lsr<64>(dest, src, shiftAmount);
    }
    
    void urshift64(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.lsr<64>(dest, src, imm.m_value & 0x3f);
    }

    void urshift64(RegisterID shiftAmount, RegisterID dest)
    {
        urshift64(dest, shiftAmount, dest);
    }
    
    void urshift64(TrustedImm32 imm, RegisterID dest)
    {
        urshift64(dest, imm, dest);
    }

    void xor32(Address src, RegisterID dest)
    {
        load32(src, dataTempRegister);
        xor32(dataTempRegister, dest);
    }

    void xor32(RegisterID src, RegisterID dest)
    {
        xor32(dest, src, dest);
    }

    void xor32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.eor<32>(dest, op1, op2);
    }

    void xor32(TrustedImm32 imm, RegisterID dest)
    {
        xor32(imm, dest, dest);
    }

    void xor32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        if (imm.m_value == -1)
            m_assembler.mvn<32>(dest, src);
        else {
            LogicalImmediate logicalImm = LogicalImmediate::create32(imm.m_value);

            if (logicalImm.isValid()) {
                m_assembler.eor<32>(dest, src, logicalImm);
                return;
            }

            move(imm, getCachedDataTempRegisterIDAndInvalidate());
            m_assembler.eor<32>(dest, src, dataTempRegister);
        }
    }

    void xor64(RegisterID src, Address address)
    {
        load64(address, getCachedDataTempRegisterIDAndInvalidate());
        m_assembler.eor<64>(dataTempRegister, dataTempRegister, src);
        store64(dataTempRegister, address);
    }

    void xor64(RegisterID src, RegisterID dest)
    {
        xor64(dest, src, dest);
    }

    void xor64(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.eor<64>(dest, op1, op2);
    }

    void xor64(TrustedImm32 imm, RegisterID dest)
    {
        xor64(imm, dest, dest);
    }

    void xor64(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        if (imm.m_value == -1)
            m_assembler.mvn<64>(dest, src);
        else {
            LogicalImmediate logicalImm = LogicalImmediate::create64(static_cast<intptr_t>(static_cast<int64_t>(imm.m_value)));

            if (logicalImm.isValid()) {
                m_assembler.eor<64>(dest, src, logicalImm);
                return;
            }

            signExtend32ToPtr(imm, getCachedDataTempRegisterIDAndInvalidate());
            m_assembler.eor<64>(dest, src, dataTempRegister);
        }
    }

    void not32(RegisterID src, RegisterID dest)
    {
        m_assembler.mvn<32>(dest, src);
    }

    void not64(RegisterID src, RegisterID dest)
    {
        m_assembler.mvn<64>(dest, src);
    }

    // Memory access operations:

    void load64(ImplicitAddress address, RegisterID dest)
    {
        if (tryLoadWithOffset<64>(dest, address.base, address.offset))
            return;

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.ldr<64>(dest, address.base, memoryTempRegister);
    }

    void load64(BaseIndex address, RegisterID dest)
    {
        if (!address.offset && (!address.scale || address.scale == 3)) {
            m_assembler.ldr<64>(dest, address.base, address.index, ARM64Assembler::UXTX, address.scale);
            return;
        }

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.add<64>(memoryTempRegister, memoryTempRegister, address.index, ARM64Assembler::UXTX, address.scale);
        m_assembler.ldr<64>(dest, address.base, memoryTempRegister);
    }

    void load64(const void* address, RegisterID dest)
    {
        load<64>(address, dest);
    }

    DataLabel32 load64WithAddressOffsetPatch(Address address, RegisterID dest)
    {
        DataLabel32 label(this);
        signExtend32ToPtrWithFixedWidth(address.offset, getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.ldr<64>(dest, address.base, memoryTempRegister, ARM64Assembler::SXTW, 0);
        return label;
    }
    
    DataLabelCompact load64WithCompactAddressOffsetPatch(Address address, RegisterID dest)
    {
        ASSERT(isCompactPtrAlignedAddressOffset(address.offset));
        DataLabelCompact label(this);
        m_assembler.ldr<64>(dest, address.base, address.offset);
        return label;
    }

    ConvertibleLoadLabel convertibleLoadPtr(Address address, RegisterID dest)
    {
        ConvertibleLoadLabel result(this);
        ASSERT(!(address.offset & ~0xff8));
        m_assembler.ldr<64>(dest, address.base, address.offset);
        return result;
    }

    void load32(ImplicitAddress address, RegisterID dest)
    {
        if (tryLoadWithOffset<32>(dest, address.base, address.offset))
            return;

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.ldr<32>(dest, address.base, memoryTempRegister);
    }

    void load32(BaseIndex address, RegisterID dest)
    {
        if (!address.offset && (!address.scale || address.scale == 2)) {
            m_assembler.ldr<32>(dest, address.base, address.index, ARM64Assembler::UXTX, address.scale);
            return;
        }

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.add<64>(memoryTempRegister, memoryTempRegister, address.index, ARM64Assembler::UXTX, address.scale);
        m_assembler.ldr<32>(dest, address.base, memoryTempRegister);
    }

    void load32(const void* address, RegisterID dest)
    {
        load<32>(address, dest);
    }

    DataLabel32 load32WithAddressOffsetPatch(Address address, RegisterID dest)
    {
        DataLabel32 label(this);
        signExtend32ToPtrWithFixedWidth(address.offset, getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.ldr<32>(dest, address.base, memoryTempRegister, ARM64Assembler::SXTW, 0);
        return label;
    }
    
    DataLabelCompact load32WithCompactAddressOffsetPatch(Address address, RegisterID dest)
    {
        ASSERT(isCompactPtrAlignedAddressOffset(address.offset));
        DataLabelCompact label(this);
        m_assembler.ldr<32>(dest, address.base, address.offset);
        return label;
    }

    void load32WithUnalignedHalfWords(BaseIndex address, RegisterID dest)
    {
        load32(address, dest);
    }

    void load16(ImplicitAddress address, RegisterID dest)
    {
        if (tryLoadWithOffset<16>(dest, address.base, address.offset))
            return;

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.ldrh(dest, address.base, memoryTempRegister);
    }
    
    void load16(BaseIndex address, RegisterID dest)
    {
        if (!address.offset && (!address.scale || address.scale == 1)) {
            m_assembler.ldrh(dest, address.base, address.index, ARM64Assembler::UXTX, address.scale);
            return;
        }

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.add<64>(memoryTempRegister, memoryTempRegister, address.index, ARM64Assembler::UXTX, address.scale);
        m_assembler.ldrh(dest, address.base, memoryTempRegister);
    }
    
    void load16(ExtendedAddress address, RegisterID dest)
    {
        moveToCachedReg(TrustedImmPtr(reinterpret_cast<void*>(address.offset)), m_cachedMemoryTempRegister);
        m_assembler.ldrh(dest, memoryTempRegister, address.base, ARM64Assembler::UXTX, 1);
        if (dest == memoryTempRegister)
            m_cachedMemoryTempRegister.invalidate();
    }

    void load16Unaligned(ImplicitAddress address, RegisterID dest)
    {
        load16(address, dest);
    }

    void load16Unaligned(BaseIndex address, RegisterID dest)
    {
        load16(address, dest);
    }

    void load16SignedExtendTo32(ImplicitAddress address, RegisterID dest)
    {
        if (tryLoadSignedWithOffset<16>(dest, address.base, address.offset))
            return;

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.ldrsh<32>(dest, address.base, memoryTempRegister);
    }

    void load16SignedExtendTo32(BaseIndex address, RegisterID dest)
    {
        if (!address.offset && (!address.scale || address.scale == 1)) {
            m_assembler.ldrsh<32>(dest, address.base, address.index, ARM64Assembler::UXTX, address.scale);
            return;
        }

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.add<64>(memoryTempRegister, memoryTempRegister, address.index, ARM64Assembler::UXTX, address.scale);
        m_assembler.ldrsh<32>(dest, address.base, memoryTempRegister);
    }

    void zeroExtend16To32(RegisterID src, RegisterID dest)
    {
        m_assembler.uxth<64>(dest, src);
    }

    void signExtend16To32(RegisterID src, RegisterID dest)
    {
        m_assembler.sxth<64>(dest, src);
    }

    void load8(ImplicitAddress address, RegisterID dest)
    {
        if (tryLoadWithOffset<8>(dest, address.base, address.offset))
            return;

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.ldrb(dest, address.base, memoryTempRegister);
    }

    void load8(BaseIndex address, RegisterID dest)
    {
        if (!address.offset && !address.scale) {
            m_assembler.ldrb(dest, address.base, address.index, ARM64Assembler::UXTX, address.scale);
            return;
        }

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.add<64>(memoryTempRegister, memoryTempRegister, address.index, ARM64Assembler::UXTX, address.scale);
        m_assembler.ldrb(dest, address.base, memoryTempRegister);
    }
    
    void load8(const void* address, RegisterID dest)
    {
        moveToCachedReg(TrustedImmPtr(address), m_cachedMemoryTempRegister);
        m_assembler.ldrb(dest, memoryTempRegister, ARM64Registers::zr);
        if (dest == memoryTempRegister)
            m_cachedMemoryTempRegister.invalidate();
    }

    void load8SignedExtendTo32(ImplicitAddress address, RegisterID dest)
    {
        if (tryLoadSignedWithOffset<8>(dest, address.base, address.offset))
            return;

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.ldrsb<32>(dest, address.base, memoryTempRegister);
    }

    void load8SignedExtendTo32(BaseIndex address, RegisterID dest)
    {
        if (!address.offset && !address.scale) {
            m_assembler.ldrsb<32>(dest, address.base, address.index, ARM64Assembler::UXTX, address.scale);
            return;
        }

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.add<64>(memoryTempRegister, memoryTempRegister, address.index, ARM64Assembler::UXTX, address.scale);
        m_assembler.ldrsb<32>(dest, address.base, memoryTempRegister);
    }

    void zeroExtend8To32(RegisterID src, RegisterID dest)
    {
        m_assembler.uxtb<64>(dest, src);
    }

    void signExtend8To32(RegisterID src, RegisterID dest)
    {
        m_assembler.sxtb<64>(dest, src);
    }

    void store64(RegisterID src, ImplicitAddress address)
    {
        if (tryStoreWithOffset<64>(src, address.base, address.offset))
            return;

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.str<64>(src, address.base, memoryTempRegister);
    }

    void store64(RegisterID src, BaseIndex address)
    {
        if (!address.offset && (!address.scale || address.scale == 3)) {
            m_assembler.str<64>(src, address.base, address.index, ARM64Assembler::UXTX, address.scale);
            return;
        }

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.add<64>(memoryTempRegister, memoryTempRegister, address.index, ARM64Assembler::UXTX, address.scale);
        m_assembler.str<64>(src, address.base, memoryTempRegister);
    }
    
    void store64(RegisterID src, const void* address)
    {
        store<64>(src, address);
    }

    void store64(TrustedImm32 imm, ImplicitAddress address)
    {
        store64(TrustedImm64(imm.m_value), address);
    }

    void store64(TrustedImm64 imm, ImplicitAddress address)
    {
        if (!imm.m_value) {
            store64(ARM64Registers::zr, address);
            return;
        }

        moveToCachedReg(imm, m_dataMemoryTempRegister);
        store64(dataTempRegister, address);
    }

    void store64(TrustedImm64 imm, BaseIndex address)
    {
        if (!imm.m_value) {
            store64(ARM64Registers::zr, address);
            return;
        }

        moveToCachedReg(imm, m_dataMemoryTempRegister);
        store64(dataTempRegister, address);
    }
    
    DataLabel32 store64WithAddressOffsetPatch(RegisterID src, Address address)
    {
        DataLabel32 label(this);
        signExtend32ToPtrWithFixedWidth(address.offset, getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.str<64>(src, address.base, memoryTempRegister, ARM64Assembler::SXTW, 0);
        return label;
    }

    void storePair64(RegisterID src1, RegisterID src2, RegisterID dest)
    {
        storePair64(src1, src2, dest, TrustedImm32(0));
    }

    void storePair64(RegisterID src1, RegisterID src2, RegisterID dest, TrustedImm32 offset)
    {
        m_assembler.stp<64>(src1, src2, dest, offset.m_value);
    }

    void store32(RegisterID src, ImplicitAddress address)
    {
        if (tryStoreWithOffset<32>(src, address.base, address.offset))
            return;

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.str<32>(src, address.base, memoryTempRegister);
    }

    void store32(RegisterID src, BaseIndex address)
    {
        if (!address.offset && (!address.scale || address.scale == 2)) {
            m_assembler.str<32>(src, address.base, address.index, ARM64Assembler::UXTX, address.scale);
            return;
        }

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.add<64>(memoryTempRegister, memoryTempRegister, address.index, ARM64Assembler::UXTX, address.scale);
        m_assembler.str<32>(src, address.base, memoryTempRegister);
    }

    void store32(RegisterID src, const void* address)
    {
        store<32>(src, address);
    }

    void store32(TrustedImm32 imm, ImplicitAddress address)
    {
        if (!imm.m_value) {
            store32(ARM64Registers::zr, address);
            return;
        }

        moveToCachedReg(imm, m_dataMemoryTempRegister);
        store32(dataTempRegister, address);
    }

    void store32(TrustedImm32 imm, BaseIndex address)
    {
        if (!imm.m_value) {
            store32(ARM64Registers::zr, address);
            return;
        }

        moveToCachedReg(imm, m_dataMemoryTempRegister);
        store32(dataTempRegister, address);
    }

    void store32(TrustedImm32 imm, const void* address)
    {
        if (!imm.m_value) {
            store32(ARM64Registers::zr, address);
            return;
        }

        moveToCachedReg(imm, m_dataMemoryTempRegister);
        store32(dataTempRegister, address);
    }

    DataLabel32 store32WithAddressOffsetPatch(RegisterID src, Address address)
    {
        DataLabel32 label(this);
        signExtend32ToPtrWithFixedWidth(address.offset, getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.str<32>(src, address.base, memoryTempRegister, ARM64Assembler::SXTW, 0);
        return label;
    }

    void store16(RegisterID src, ImplicitAddress address)
    {
        if (tryStoreWithOffset<16>(src, address.base, address.offset))
            return;

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.str<16>(src, address.base, memoryTempRegister);
    }

    void store16(RegisterID src, BaseIndex address)
    {
        if (!address.offset && (!address.scale || address.scale == 1)) {
            m_assembler.strh(src, address.base, address.index, ARM64Assembler::UXTX, address.scale);
            return;
        }

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.add<64>(memoryTempRegister, memoryTempRegister, address.index, ARM64Assembler::UXTX, address.scale);
        m_assembler.strh(src, address.base, memoryTempRegister);
    }

    void store8(RegisterID src, BaseIndex address)
    {
        if (!address.offset && !address.scale) {
            m_assembler.strb(src, address.base, address.index, ARM64Assembler::UXTX, address.scale);
            return;
        }

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.add<64>(memoryTempRegister, memoryTempRegister, address.index, ARM64Assembler::UXTX, address.scale);
        m_assembler.strb(src, address.base, memoryTempRegister);
    }

    void store8(RegisterID src, void* address)
    {
        move(TrustedImmPtr(address), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.strb(src, memoryTempRegister, 0);
    }

    void store8(RegisterID src, ImplicitAddress address)
    {
        if (tryStoreWithOffset<8>(src, address.base, address.offset))
            return;

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.str<8>(src, address.base, memoryTempRegister);
    }

    void store8(TrustedImm32 imm, void* address)
    {
        if (!imm.m_value) {
            store8(ARM64Registers::zr, address);
            return;
        }

        move(imm, getCachedDataTempRegisterIDAndInvalidate());
        store8(dataTempRegister, address);
    }

    void store8(TrustedImm32 imm, ImplicitAddress address)
    {
        if (!imm.m_value) {
            store8(ARM64Registers::zr, address);
            return;
        }

        move(imm, getCachedDataTempRegisterIDAndInvalidate());
        store8(dataTempRegister, address);
    }

    void getEffectiveAddress(BaseIndex address, RegisterID dest)
    {
        m_assembler.add<64>(dest, address.base, address.index, ARM64Assembler::LSL, address.scale);
        if (address.offset)
            add64(TrustedImm32(address.offset), dest);
    }


    // Floating-point operations:

    static bool supportsFloatingPoint() { return true; }
    static bool supportsFloatingPointTruncate() { return true; }
    static bool supportsFloatingPointSqrt() { return true; }
    static bool supportsFloatingPointAbs() { return true; }
    static bool supportsFloatingPointCeil() { return true; }

    enum BranchTruncateType { BranchIfTruncateFailed, BranchIfTruncateSuccessful };

    void absDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.fabs<64>(dest, src);
    }

    void absFloat(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.fabs<32>(dest, src);
    }

    void addDouble(FPRegisterID src, FPRegisterID dest)
    {
        addDouble(dest, src, dest);
    }

    void addDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.fadd<64>(dest, op1, op2);
    }

    void addDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, fpTempRegister);
        addDouble(fpTempRegister, dest);
    }

    void addDouble(AbsoluteAddress address, FPRegisterID dest)
    {
        loadDouble(TrustedImmPtr(address.m_ptr), fpTempRegister);
        addDouble(fpTempRegister, dest);
    }

    void addFloat(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.fadd<32>(dest, op1, op2);
    }

    void ceilDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.frintp<64>(dest, src);
    }

    void ceilFloat(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.frintp<32>(dest, src);
    }

    void floorDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.frintm<64>(dest, src);
    }

    // Convert 'src' to an integer, and places the resulting 'dest'.
    // If the result is not representable as a 32 bit value, branch.
    // May also branch for some values that are representable in 32 bits
    // (specifically, in this case, 0).
    void branchConvertDoubleToInt32(FPRegisterID src, RegisterID dest, JumpList& failureCases, FPRegisterID, bool negZeroCheck = true)
    {
        m_assembler.fcvtns<32, 64>(dest, src);

        // Convert the integer result back to float & compare to the original value - if not equal or unordered (NaN) then jump.
        m_assembler.scvtf<64, 32>(fpTempRegister, dest);
        failureCases.append(branchDouble(DoubleNotEqualOrUnordered, src, fpTempRegister));

        // Test for negative zero.
        if (negZeroCheck) {
            Jump valueIsNonZero = branchTest32(NonZero, dest);
            RegisterID scratch = getCachedMemoryTempRegisterIDAndInvalidate();
            m_assembler.fmov<64>(scratch, src);
            failureCases.append(makeTestBitAndBranch(scratch, 63, IsNonZero));
            valueIsNonZero.link(this);
        }
    }

    Jump branchDouble(DoubleCondition cond, FPRegisterID left, FPRegisterID right)
    {
        m_assembler.fcmp<64>(left, right);
        return jumpAfterFloatingPointCompare(cond);
    }

    Jump branchFloat(DoubleCondition cond, FPRegisterID left, FPRegisterID right)
    {
        m_assembler.fcmp<32>(left, right);
        return jumpAfterFloatingPointCompare(cond);
    }

    Jump branchDoubleNonZero(FPRegisterID reg, FPRegisterID)
    {
        m_assembler.fcmp_0<64>(reg);
        Jump unordered = makeBranch(ARM64Assembler::ConditionVS);
        Jump result = makeBranch(ARM64Assembler::ConditionNE);
        unordered.link(this);
        return result;
    }

    Jump branchDoubleZeroOrNaN(FPRegisterID reg, FPRegisterID)
    {
        m_assembler.fcmp_0<64>(reg);
        Jump unordered = makeBranch(ARM64Assembler::ConditionVS);
        Jump notEqual = makeBranch(ARM64Assembler::ConditionNE);
        unordered.link(this);
        // We get here if either unordered or equal.
        Jump result = jump();
        notEqual.link(this);
        return result;
    }

    Jump branchTruncateDoubleToInt32(FPRegisterID src, RegisterID dest, BranchTruncateType branchType = BranchIfTruncateFailed)
    {
        // Truncate to a 64-bit integer in dataTempRegister, copy the low 32-bit to dest.
        m_assembler.fcvtzs<64, 64>(getCachedDataTempRegisterIDAndInvalidate(), src);
        zeroExtend32ToPtr(dataTempRegister, dest);
        // Check thlow 32-bits sign extend to be equal to the full value.
        m_assembler.cmp<64>(dataTempRegister, dataTempRegister, ARM64Assembler::SXTW, 0);
        return Jump(makeBranch(branchType == BranchIfTruncateSuccessful ? Equal : NotEqual));
    }

    Jump branchTruncateDoubleToUint32(FPRegisterID src, RegisterID dest, BranchTruncateType branchType = BranchIfTruncateFailed)
    {
        // Truncate to a 64-bit unsigned integer in dataTempRegister, copy the low 32-bit to dest.
        m_assembler.fcvtzu<64, 64>(getCachedDataTempRegisterIDAndInvalidate(), src);
        zeroExtend32ToPtr(dataTempRegister, dest);
        // Check thlow 32-bits sign extend to be equal to the full value.
        m_assembler.cmp<64>(dataTempRegister, dataTempRegister, ARM64Assembler::SXTW, 0);
        return Jump(makeBranch(branchType == BranchIfTruncateSuccessful ? Equal : NotEqual));
    }

    void convertDoubleToFloat(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.fcvt<32, 64>(dest, src);
    }

    void convertFloatToDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.fcvt<64, 32>(dest, src);
    }
    
    void convertInt32ToDouble(TrustedImm32 imm, FPRegisterID dest)
    {
        move(imm, getCachedDataTempRegisterIDAndInvalidate());
        convertInt32ToDouble(dataTempRegister, dest);
    }
    
    void convertUInt32ToDouble(RegisterID src, FPRegisterID dest, RegisterID /*scratch*/)
    {
        m_assembler.ucvtf<64, 32>(dest, src);
    }

    void convertInt32ToDouble(RegisterID src, FPRegisterID dest)
    {
        m_assembler.scvtf<64, 32>(dest, src);
    }

    void convertInt32ToDouble(Address address, FPRegisterID dest)
    {
        load32(address, getCachedDataTempRegisterIDAndInvalidate());
        convertInt32ToDouble(dataTempRegister, dest);
    }

    void convertInt32ToDouble(AbsoluteAddress address, FPRegisterID dest)
    {
        load32(address.m_ptr, getCachedDataTempRegisterIDAndInvalidate());
        convertInt32ToDouble(dataTempRegister, dest);
    }
    
    void convertInt64ToDouble(RegisterID src, FPRegisterID dest)
    {
        m_assembler.scvtf<64, 64>(dest, src);
    }
    
    void divDouble(FPRegisterID src, FPRegisterID dest)
    {
        divDouble(dest, src, dest);
    }

    void divDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.fdiv<64>(dest, op1, op2);
    }

    void divFloat(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.fdiv<32>(dest, op1, op2);
    }

    void loadDouble(ImplicitAddress address, FPRegisterID dest)
    {
        if (tryLoadWithOffset<64>(dest, address.base, address.offset))
            return;

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.ldr<64>(dest, address.base, memoryTempRegister);
    }

    void loadDouble(BaseIndex address, FPRegisterID dest)
    {
        if (!address.offset && (!address.scale || address.scale == 3)) {
            m_assembler.ldr<64>(dest, address.base, address.index, ARM64Assembler::UXTX, address.scale);
            return;
        }

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.add<64>(memoryTempRegister, memoryTempRegister, address.index, ARM64Assembler::UXTX, address.scale);
        m_assembler.ldr<64>(dest, address.base, memoryTempRegister);
    }
    
    void loadDouble(TrustedImmPtr address, FPRegisterID dest)
    {
        moveToCachedReg(address, m_cachedMemoryTempRegister);
        m_assembler.ldr<64>(dest, memoryTempRegister, ARM64Registers::zr);
    }

    void loadFloat(ImplicitAddress address, FPRegisterID dest)
    {
        if (tryLoadWithOffset<32>(dest, address.base, address.offset))
            return;

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.ldr<32>(dest, address.base, memoryTempRegister);
    }

    void loadFloat(BaseIndex address, FPRegisterID dest)
    {
        if (!address.offset && (!address.scale || address.scale == 2)) {
            m_assembler.ldr<32>(dest, address.base, address.index, ARM64Assembler::UXTX, address.scale);
            return;
        }

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.add<64>(memoryTempRegister, memoryTempRegister, address.index, ARM64Assembler::UXTX, address.scale);
        m_assembler.ldr<32>(dest, address.base, memoryTempRegister);
    }

    void moveDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.fmov<64>(dest, src);
    }

    void moveZeroToDouble(FPRegisterID reg)
    {
        m_assembler.fmov<64>(reg, ARM64Registers::zr);
    }

    void moveDoubleTo64(FPRegisterID src, RegisterID dest)
    {
        m_assembler.fmov<64>(dest, src);
    }

    void moveFloatTo32(FPRegisterID src, RegisterID dest)
    {
        m_assembler.fmov<32>(dest, src);
    }

    void move64ToDouble(RegisterID src, FPRegisterID dest)
    {
        m_assembler.fmov<64>(dest, src);
    }

    void move32ToFloat(RegisterID src, FPRegisterID dest)
    {
        m_assembler.fmov<32>(dest, src);
    }

    void moveConditionallyDouble(DoubleCondition cond, FPRegisterID left, FPRegisterID right, RegisterID src, RegisterID dest)
    {
        m_assembler.fcmp<64>(left, right);
        moveConditionallyAfterFloatingPointCompare<64>(cond, src, dest);
    }

    void moveConditionallyFloat(DoubleCondition cond, FPRegisterID left, FPRegisterID right, RegisterID src, RegisterID dest)
    {
        m_assembler.fcmp<32>(left, right);
        moveConditionallyAfterFloatingPointCompare<64>(cond, src, dest);
    }

    template<int datasize>
    void moveConditionallyAfterFloatingPointCompare(DoubleCondition cond, RegisterID src, RegisterID dest)
    {
        if (cond == DoubleNotEqual) {
            Jump unordered = makeBranch(ARM64Assembler::ConditionVS);
            m_assembler.csel<datasize>(dest, src, dest, ARM64Assembler::ConditionNE);
            unordered.link(this);
            return;
        }
        if (cond == DoubleEqualOrUnordered) {
            // If the compare is unordered, src is copied to dest and the
            // next csel has all arguments equal to src.
            // If the compare is ordered, dest is unchanged and EQ decides
            // what value to set.
            m_assembler.csel<datasize>(dest, src, dest, ARM64Assembler::ConditionVS);
            m_assembler.csel<datasize>(dest, src, dest, ARM64Assembler::ConditionEQ);
            return;
        }
        m_assembler.csel<datasize>(dest, src, dest, ARM64Condition(cond));
    }

    void mulDouble(FPRegisterID src, FPRegisterID dest)
    {
        mulDouble(dest, src, dest);
    }

    void mulDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.fmul<64>(dest, op1, op2);
    }

    void mulDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, fpTempRegister);
        mulDouble(fpTempRegister, dest);
    }

    void mulFloat(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.fmul<32>(dest, op1, op2);
    }

    void andDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.vand<64>(dest, op1, op2);
    }

    void andFloat(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        andDouble(op1, op2, dest);
    }

    void negateDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.fneg<64>(dest, src);
    }

    void sqrtDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.fsqrt<64>(dest, src);
    }

    void sqrtFloat(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.fsqrt<32>(dest, src);
    }

    void storeDouble(FPRegisterID src, ImplicitAddress address)
    {
        if (tryStoreWithOffset<64>(src, address.base, address.offset))
            return;

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.str<64>(src, address.base, memoryTempRegister);
    }

    void storeDouble(FPRegisterID src, TrustedImmPtr address)
    {
        moveToCachedReg(address, m_cachedMemoryTempRegister);
        m_assembler.str<64>(src, memoryTempRegister, ARM64Registers::zr);
    }

    void storeDouble(FPRegisterID src, BaseIndex address)
    {
        if (!address.offset && (!address.scale || address.scale == 3)) {
            m_assembler.str<64>(src, address.base, address.index, ARM64Assembler::UXTX, address.scale);
            return;
        }

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.add<64>(memoryTempRegister, memoryTempRegister, address.index, ARM64Assembler::UXTX, address.scale);
        m_assembler.str<64>(src, address.base, memoryTempRegister);
    }

    void storeFloat(FPRegisterID src, ImplicitAddress address)
    {
        if (tryStoreWithOffset<32>(src, address.base, address.offset))
            return;

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.str<32>(src, address.base, memoryTempRegister);
    }
    
    void storeFloat(FPRegisterID src, BaseIndex address)
    {
        if (!address.offset && (!address.scale || address.scale == 2)) {
            m_assembler.str<32>(src, address.base, address.index, ARM64Assembler::UXTX, address.scale);
            return;
        }

        signExtend32ToPtr(TrustedImm32(address.offset), getCachedMemoryTempRegisterIDAndInvalidate());
        m_assembler.add<64>(memoryTempRegister, memoryTempRegister, address.index, ARM64Assembler::UXTX, address.scale);
        m_assembler.str<32>(src, address.base, memoryTempRegister);
    }

    void subDouble(FPRegisterID src, FPRegisterID dest)
    {
        subDouble(dest, src, dest);
    }

    void subDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.fsub<64>(dest, op1, op2);
    }

    void subDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, fpTempRegister);
        subDouble(fpTempRegister, dest);
    }

    void subFloat(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.fsub<32>(dest, op1, op2);
    }

    // Result is undefined if the value is outside of the integer range.
    void truncateDoubleToInt32(FPRegisterID src, RegisterID dest)
    {
        m_assembler.fcvtzs<32, 64>(dest, src);
    }

    void truncateDoubleToUint32(FPRegisterID src, RegisterID dest)
    {
        m_assembler.fcvtzu<32, 64>(dest, src);
    }


    // Stack manipulation operations:
    //
    // The ABI is assumed to provide a stack abstraction to memory,
    // containing machine word sized units of data. Push and pop
    // operations add and remove a single register sized unit of data
    // to or from the stack. These operations are not supported on
    // ARM64. Peek and poke operations read or write values on the
    // stack, without moving the current stack position. Additionally,
    // there are popToRestore and pushToSave operations, which are
    // designed just for quick-and-dirty saving and restoring of
    // temporary values. These operations don't claim to have any
    // ABI compatibility.
    
    void pop(RegisterID) NO_RETURN_DUE_TO_CRASH
    {
        CRASH();
    }

    void push(RegisterID) NO_RETURN_DUE_TO_CRASH
    {
        CRASH();
    }

    void push(Address) NO_RETURN_DUE_TO_CRASH
    {
        CRASH();
    }

    void push(TrustedImm32) NO_RETURN_DUE_TO_CRASH
    {
        CRASH();
    }

    void popPair(RegisterID dest1, RegisterID dest2)
    {
        m_assembler.ldp<64>(dest1, dest2, ARM64Registers::sp, PairPostIndex(16));
    }

    void pushPair(RegisterID src1, RegisterID src2)
    {
        m_assembler.stp<64>(src1, src2, ARM64Registers::sp, PairPreIndex(-16));
    }

    void popToRestore(RegisterID dest)
    {
        m_assembler.ldr<64>(dest, ARM64Registers::sp, PostIndex(16));
    }

    void pushToSave(RegisterID src)
    {
        m_assembler.str<64>(src, ARM64Registers::sp, PreIndex(-16));
    }
    
    void pushToSaveImmediateWithoutTouchingRegisters(TrustedImm32 imm)
    {
        RegisterID reg = dataTempRegister;
        pushPair(reg, reg);
        move(imm, reg);
        store64(reg, stackPointerRegister);
        load64(Address(stackPointerRegister, 8), reg);
    }

    void pushToSave(Address address)
    {
        load32(address, getCachedDataTempRegisterIDAndInvalidate());
        pushToSave(dataTempRegister);
    }

    void pushToSave(TrustedImm32 imm)
    {
        move(imm, getCachedDataTempRegisterIDAndInvalidate());
        pushToSave(dataTempRegister);
    }
    
    void popToRestore(FPRegisterID dest)
    {
        loadDouble(stackPointerRegister, dest);
        add64(TrustedImm32(16), stackPointerRegister);
    }
    
    void pushToSave(FPRegisterID src)
    {
        sub64(TrustedImm32(16), stackPointerRegister);
        storeDouble(src, stackPointerRegister);
    }

    static ptrdiff_t pushToSaveByteOffset() { return 16; }

    // Register move operations:

    void move(RegisterID src, RegisterID dest)
    {
        if (src != dest)
            m_assembler.mov<64>(dest, src);
    }

    void move(TrustedImm32 imm, RegisterID dest)
    {
        moveInternal<TrustedImm32, int32_t>(imm, dest);
    }

    void move(TrustedImmPtr imm, RegisterID dest)
    {
        moveInternal<TrustedImmPtr, intptr_t>(imm, dest);
    }

    void move(TrustedImm64 imm, RegisterID dest)
    {
        moveInternal<TrustedImm64, int64_t>(imm, dest);
    }

    void swap(RegisterID reg1, RegisterID reg2)
    {
        move(reg1, getCachedDataTempRegisterIDAndInvalidate());
        move(reg2, reg1);
        move(dataTempRegister, reg2);
    }

    void signExtend32ToPtr(TrustedImm32 imm, RegisterID dest)
    {
        move(TrustedImmPtr(reinterpret_cast<void*>(static_cast<intptr_t>(imm.m_value))), dest);
    }
    
    void signExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        m_assembler.sxtw(dest, src);
    }

    void zeroExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        m_assembler.uxtw(dest, src);
    }

    void moveConditionally32(RelationalCondition cond, RegisterID left, RegisterID right, RegisterID src, RegisterID dest)
    {
        m_assembler.cmp<32>(left, right);
        m_assembler.csel<32>(dest, src, dest, ARM64Condition(cond));
    }

    void moveConditionally64(RelationalCondition cond, RegisterID left, RegisterID right, RegisterID src, RegisterID dest)
    {
        m_assembler.cmp<64>(left, right);
        m_assembler.csel<64>(dest, src, dest, ARM64Condition(cond));
    }

    void moveConditionallyTest32(ResultCondition cond, RegisterID testReg, RegisterID mask, RegisterID src, RegisterID dest)
    {
        m_assembler.tst<32>(testReg, mask);
        m_assembler.csel<32>(dest, src, dest, ARM64Condition(cond));
    }

    void moveConditionallyTest64(ResultCondition cond, RegisterID testReg, RegisterID mask, RegisterID src, RegisterID dest)
    {
        m_assembler.tst<64>(testReg, mask);
        m_assembler.csel<64>(dest, src, dest, ARM64Condition(cond));
    }

    // Forwards / external control flow operations:
    //
    // This set of jump and conditional branch operations return a Jump
    // object which may linked at a later point, allow forwards jump,
    // or jumps that will require external linkage (after the code has been
    // relocated).
    //
    // For branches, signed <, >, <= and >= are denoted as l, g, le, and ge
    // respecitvely, for unsigned comparisons the names b, a, be, and ae are
    // used (representing the names 'below' and 'above').
    //
    // Operands to the comparision are provided in the expected order, e.g.
    // jle32(reg1, TrustedImm32(5)) will branch if the value held in reg1, when
    // treated as a signed 32bit value, is less than or equal to 5.
    //
    // jz and jnz test whether the first operand is equal to zero, and take
    // an optional second operand of a mask under which to perform the test.

    Jump branch32(RelationalCondition cond, RegisterID left, RegisterID right)
    {
        m_assembler.cmp<32>(left, right);
        return Jump(makeBranch(cond));
    }

    Jump branch32(RelationalCondition cond, RegisterID left, TrustedImm32 right)
    {
        if (isUInt12(right.m_value))
            m_assembler.cmp<32>(left, UInt12(right.m_value));
        else if (isUInt12(-right.m_value))
            m_assembler.cmn<32>(left, UInt12(-right.m_value));
        else {
            moveToCachedReg(right, m_dataMemoryTempRegister);
            m_assembler.cmp<32>(left, dataTempRegister);
        }
        return Jump(makeBranch(cond));
    }

    Jump branch32(RelationalCondition cond, RegisterID left, Address right)
    {
        load32(right, getCachedMemoryTempRegisterIDAndInvalidate());
        return branch32(cond, left, memoryTempRegister);
    }

    Jump branch32(RelationalCondition cond, Address left, RegisterID right)
    {
        load32(left, getCachedMemoryTempRegisterIDAndInvalidate());
        return branch32(cond, memoryTempRegister, right);
    }

    Jump branch32(RelationalCondition cond, Address left, TrustedImm32 right)
    {
        load32(left, getCachedMemoryTempRegisterIDAndInvalidate());
        return branch32(cond, memoryTempRegister, right);
    }

    Jump branch32(RelationalCondition cond, BaseIndex left, TrustedImm32 right)
    {
        load32(left, getCachedMemoryTempRegisterIDAndInvalidate());
        return branch32(cond, memoryTempRegister, right);
    }

    Jump branch32(RelationalCondition cond, AbsoluteAddress left, RegisterID right)
    {
        load32(left.m_ptr, getCachedDataTempRegisterIDAndInvalidate());
        return branch32(cond, dataTempRegister, right);
    }

    Jump branch32(RelationalCondition cond, AbsoluteAddress left, TrustedImm32 right)
    {
        load32(left.m_ptr, getCachedMemoryTempRegisterIDAndInvalidate());
        return branch32(cond, memoryTempRegister, right);
    }

    Jump branch64(RelationalCondition cond, RegisterID left, RegisterID right)
    {
        if (right == ARM64Registers::sp) {
            if (cond == Equal && left != ARM64Registers::sp) {
                // CMP can only use SP for the left argument, since we are testing for equality, the order
                // does not matter here.
                std::swap(left, right);
            } else {
                move(right, getCachedDataTempRegisterIDAndInvalidate());
                right = dataTempRegister;
            }
        }
        m_assembler.cmp<64>(left, right);
        return Jump(makeBranch(cond));
    }

    Jump branch64(RelationalCondition cond, RegisterID left, TrustedImm64 right)
    {
        intptr_t immediate = right.m_value;
        if (isUInt12(immediate))
            m_assembler.cmp<64>(left, UInt12(static_cast<int32_t>(immediate)));
        else if (isUInt12(-immediate))
            m_assembler.cmn<64>(left, UInt12(static_cast<int32_t>(-immediate)));
        else {
            moveToCachedReg(right, m_dataMemoryTempRegister);
            m_assembler.cmp<64>(left, dataTempRegister);
        }
        return Jump(makeBranch(cond));
    }

    Jump branch64(RelationalCondition cond, RegisterID left, Address right)
    {
        load64(right, getCachedMemoryTempRegisterIDAndInvalidate());
        return branch64(cond, left, memoryTempRegister);
    }

    Jump branch64(RelationalCondition cond, AbsoluteAddress left, RegisterID right)
    {
        load64(left.m_ptr, getCachedDataTempRegisterIDAndInvalidate());
        return branch64(cond, dataTempRegister, right);
    }

    Jump branch64(RelationalCondition cond, Address left, RegisterID right)
    {
        load64(left, getCachedMemoryTempRegisterIDAndInvalidate());
        return branch64(cond, memoryTempRegister, right);
    }

    Jump branch64(RelationalCondition cond, Address left, TrustedImm64 right)
    {
        load64(left, getCachedMemoryTempRegisterIDAndInvalidate());
        return branch64(cond, memoryTempRegister, right);
    }

    Jump branchPtr(RelationalCondition cond, BaseIndex left, RegisterID right)
    {
        load64(left, getCachedMemoryTempRegisterIDAndInvalidate());
        return branch64(cond, memoryTempRegister, right);
    }

    Jump branch8(RelationalCondition cond, Address left, TrustedImm32 right)
    {
        ASSERT(!(0xffffff00 & right.m_value));
        load8(left, getCachedMemoryTempRegisterIDAndInvalidate());
        return branch32(cond, memoryTempRegister, right);
    }

    Jump branch8(RelationalCondition cond, BaseIndex left, TrustedImm32 right)
    {
        ASSERT(!(0xffffff00 & right.m_value));
        load8(left, getCachedMemoryTempRegisterIDAndInvalidate());
        return branch32(cond, memoryTempRegister, right);
    }
    
    Jump branch8(RelationalCondition cond, AbsoluteAddress left, TrustedImm32 right)
    {
        ASSERT(!(0xffffff00 & right.m_value));
        load8(left.m_ptr, getCachedMemoryTempRegisterIDAndInvalidate());
        return branch32(cond, memoryTempRegister, right);
    }
    
    Jump branchTest32(ResultCondition cond, RegisterID reg, RegisterID mask)
    {
        m_assembler.tst<32>(reg, mask);
        return Jump(makeBranch(cond));
    }

    void test32(ResultCondition cond, RegisterID reg, TrustedImm32 mask = TrustedImm32(-1))
    {
        if (mask.m_value == -1)
            m_assembler.tst<32>(reg, reg);
        else {
            bool testedWithImmediate = false;
            if ((cond == Zero) || (cond == NonZero)) {
                LogicalImmediate logicalImm = LogicalImmediate::create32(mask.m_value);

                if (logicalImm.isValid()) {
                    m_assembler.tst<32>(reg, logicalImm);
                    testedWithImmediate = true;
                }
            }
            if (!testedWithImmediate) {
                move(mask, getCachedDataTempRegisterIDAndInvalidate());
                m_assembler.tst<32>(reg, dataTempRegister);
            }
        }
    }

    Jump branch(ResultCondition cond)
    {
        return Jump(makeBranch(cond));
    }

    Jump branchTest32(ResultCondition cond, RegisterID reg, TrustedImm32 mask = TrustedImm32(-1))
    {
        if (mask.m_value == -1) {
            if ((cond == Zero) || (cond == NonZero))
                return Jump(makeCompareAndBranch<32>(static_cast<ZeroCondition>(cond), reg));
            m_assembler.tst<32>(reg, reg);
        } else if (hasOneBitSet(mask.m_value) && ((cond == Zero) || (cond == NonZero)))
            return Jump(makeTestBitAndBranch(reg, getLSBSet(mask.m_value), static_cast<ZeroCondition>(cond)));
        else {
            if ((cond == Zero) || (cond == NonZero)) {
                LogicalImmediate logicalImm = LogicalImmediate::create32(mask.m_value);

                if (logicalImm.isValid()) {
                    m_assembler.tst<32>(reg, logicalImm);
                    return Jump(makeBranch(cond));
                }
            }

            move(mask, getCachedDataTempRegisterIDAndInvalidate());
            m_assembler.tst<32>(reg, dataTempRegister);
        }
        return Jump(makeBranch(cond));
    }

    Jump branchTest32(ResultCondition cond, Address address, TrustedImm32 mask = TrustedImm32(-1))
    {
        load32(address, getCachedMemoryTempRegisterIDAndInvalidate());
        return branchTest32(cond, memoryTempRegister, mask);
    }

    Jump branchTest32(ResultCondition cond, BaseIndex address, TrustedImm32 mask = TrustedImm32(-1))
    {
        load32(address, getCachedMemoryTempRegisterIDAndInvalidate());
        return branchTest32(cond, memoryTempRegister, mask);
    }

    Jump branchTest64(ResultCondition cond, RegisterID reg, RegisterID mask)
    {
        m_assembler.tst<64>(reg, mask);
        return Jump(makeBranch(cond));
    }

    Jump branchTest64(ResultCondition cond, RegisterID reg, TrustedImm32 mask = TrustedImm32(-1))
    {
        if (mask.m_value == -1) {
            if ((cond == Zero) || (cond == NonZero))
                return Jump(makeCompareAndBranch<64>(static_cast<ZeroCondition>(cond), reg));
            m_assembler.tst<64>(reg, reg);
        } else if (hasOneBitSet(mask.m_value) && ((cond == Zero) || (cond == NonZero)))
            return Jump(makeTestBitAndBranch(reg, getLSBSet(mask.m_value), static_cast<ZeroCondition>(cond)));
        else {
            if ((cond == Zero) || (cond == NonZero)) {
                LogicalImmediate logicalImm = LogicalImmediate::create64(mask.m_value);

                if (logicalImm.isValid()) {
                    m_assembler.tst<64>(reg, logicalImm);
                    return Jump(makeBranch(cond));
                }
            }

            signExtend32ToPtr(mask, getCachedDataTempRegisterIDAndInvalidate());
            m_assembler.tst<64>(reg, dataTempRegister);
        }
        return Jump(makeBranch(cond));
    }

    Jump branchTest64(ResultCondition cond, RegisterID reg, TrustedImm64 mask)
    {
        move(mask, getCachedDataTempRegisterIDAndInvalidate());
        return branchTest64(cond, reg, dataTempRegister);
    }

    Jump branchTest64(ResultCondition cond, Address address, RegisterID mask)
    {
        load64(address, getCachedDataTempRegisterIDAndInvalidate());
        return branchTest64(cond, dataTempRegister, mask);
    }

    Jump branchTest64(ResultCondition cond, Address address, TrustedImm32 mask = TrustedImm32(-1))
    {
        load64(address, getCachedDataTempRegisterIDAndInvalidate());
        return branchTest64(cond, dataTempRegister, mask);
    }

    Jump branchTest64(ResultCondition cond, BaseIndex address, TrustedImm32 mask = TrustedImm32(-1))
    {
        load64(address, getCachedDataTempRegisterIDAndInvalidate());
        return branchTest64(cond, dataTempRegister, mask);
    }

    Jump branchTest64(ResultCondition cond, AbsoluteAddress address, TrustedImm32 mask = TrustedImm32(-1))
    {
        load64(address.m_ptr, getCachedDataTempRegisterIDAndInvalidate());
        return branchTest64(cond, dataTempRegister, mask);
    }

    Jump branchTest8(ResultCondition cond, Address address, TrustedImm32 mask = TrustedImm32(-1))
    {
        load8(address, getCachedDataTempRegisterIDAndInvalidate());
        return branchTest32(cond, dataTempRegister, mask);
    }

    Jump branchTest8(ResultCondition cond, AbsoluteAddress address, TrustedImm32 mask = TrustedImm32(-1))
    {
        load8(address.m_ptr, getCachedDataTempRegisterIDAndInvalidate());
        return branchTest32(cond, dataTempRegister, mask);
    }

    Jump branchTest8(ResultCondition cond, ExtendedAddress address, TrustedImm32 mask = TrustedImm32(-1))
    {
        move(TrustedImmPtr(reinterpret_cast<void*>(address.offset)), getCachedDataTempRegisterIDAndInvalidate());
        m_assembler.ldrb(dataTempRegister, address.base, dataTempRegister);
        return branchTest32(cond, dataTempRegister, mask);
    }

    Jump branchTest8(ResultCondition cond, BaseIndex address, TrustedImm32 mask = TrustedImm32(-1))
    {
        load8(address, getCachedDataTempRegisterIDAndInvalidate());
        return branchTest32(cond, dataTempRegister, mask);
    }

    Jump branch32WithUnalignedHalfWords(RelationalCondition cond, BaseIndex left, TrustedImm32 right)
    {
        return branch32(cond, left, right);
    }


    // Arithmetic control flow operations:
    //
    // This set of conditional branch operations branch based
    // on the result of an arithmetic operation. The operation
    // is performed as normal, storing the result.
    //
    // * jz operations branch if the result is zero.
    // * jo operations branch if the (signed) arithmetic
    //   operation caused an overflow to occur.
    
    Jump branchAdd32(ResultCondition cond, RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.add<32, S>(dest, op1, op2);
        return Jump(makeBranch(cond));
    }

    Jump branchAdd32(ResultCondition cond, RegisterID op1, TrustedImm32 imm, RegisterID dest)
    {
        if (isUInt12(imm.m_value)) {
            m_assembler.add<32, S>(dest, op1, UInt12(imm.m_value));
            return Jump(makeBranch(cond));
        }
        if (isUInt12(-imm.m_value)) {
            m_assembler.sub<32, S>(dest, op1, UInt12(-imm.m_value));
            return Jump(makeBranch(cond));
        }

        signExtend32ToPtr(imm, getCachedDataTempRegisterIDAndInvalidate());
        return branchAdd32(cond, op1, dataTempRegister, dest);
    }

    Jump branchAdd32(ResultCondition cond, Address src, RegisterID dest)
    {
        load32(src, getCachedDataTempRegisterIDAndInvalidate());
        return branchAdd32(cond, dest, dataTempRegister, dest);
    }

    Jump branchAdd32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        return branchAdd32(cond, dest, src, dest);
    }

    Jump branchAdd32(ResultCondition cond, TrustedImm32 imm, RegisterID dest)
    {
        return branchAdd32(cond, dest, imm, dest);
    }

    Jump branchAdd32(ResultCondition cond, TrustedImm32 imm, AbsoluteAddress address)
    {
        load32(address.m_ptr, getCachedDataTempRegisterIDAndInvalidate());

        if (isUInt12(imm.m_value)) {
            m_assembler.add<32, S>(dataTempRegister, dataTempRegister, UInt12(imm.m_value));
            store32(dataTempRegister, address.m_ptr);
        } else if (isUInt12(-imm.m_value)) {
            m_assembler.sub<32, S>(dataTempRegister, dataTempRegister, UInt12(-imm.m_value));
            store32(dataTempRegister, address.m_ptr);
        } else {
            move(imm, getCachedMemoryTempRegisterIDAndInvalidate());
            m_assembler.add<32, S>(dataTempRegister, dataTempRegister, memoryTempRegister);
            store32(dataTempRegister, address.m_ptr);
        }

        return Jump(makeBranch(cond));
    }

    Jump branchAdd64(ResultCondition cond, RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.add<64, S>(dest, op1, op2);
        return Jump(makeBranch(cond));
    }

    Jump branchAdd64(ResultCondition cond, RegisterID op1, TrustedImm32 imm, RegisterID dest)
    {
        if (isUInt12(imm.m_value)) {
            m_assembler.add<64, S>(dest, op1, UInt12(imm.m_value));
            return Jump(makeBranch(cond));
        }
        if (isUInt12(-imm.m_value)) {
            m_assembler.sub<64, S>(dest, op1, UInt12(-imm.m_value));
            return Jump(makeBranch(cond));
        }

        move(imm, getCachedDataTempRegisterIDAndInvalidate());
        return branchAdd64(cond, op1, dataTempRegister, dest);
    }

    Jump branchAdd64(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        return branchAdd64(cond, dest, src, dest);
    }

    Jump branchAdd64(ResultCondition cond, TrustedImm32 imm, RegisterID dest)
    {
        return branchAdd64(cond, dest, imm, dest);
    }

    Jump branchMul32(ResultCondition cond, RegisterID src1, RegisterID src2, RegisterID scratch1, RegisterID scratch2, RegisterID dest)
    {
        ASSERT(cond != Signed);

        if (cond != Overflow) {
            m_assembler.mul<32>(dest, src1, src2);
            return branchTest32(cond, dest);
        }

        // This is a signed multiple of two 32-bit values, producing a 64-bit result.
        m_assembler.smull(dest, src1, src2);
        // Copy bits 63..32 of the result to bits 31..0 of scratch1.
        m_assembler.asr<64>(scratch1, dest, 32);
        // Splat bit 31 of the result to bits 31..0 of scratch2.
        m_assembler.asr<32>(scratch2, dest, 31);
        // After a mul32 the top 32 bits of the register should be clear.
        zeroExtend32ToPtr(dest, dest);
        // Check that bits 31..63 of the original result were all equal.
        return branch32(NotEqual, scratch2, scratch1);
    }

    Jump branchMul32(ResultCondition cond, RegisterID src1, RegisterID src2, RegisterID dest)
    {
        return branchMul32(cond, src1, src2, getCachedDataTempRegisterIDAndInvalidate(), getCachedMemoryTempRegisterIDAndInvalidate(), dest);
    }

    Jump branchMul32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        return branchMul32(cond, dest, src, dest);
    }

    Jump branchMul32(ResultCondition cond, RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        move(imm, getCachedDataTempRegisterIDAndInvalidate());
        return branchMul32(cond, dataTempRegister, src, dest);
    }

    Jump branchMul32(ResultCondition cond, TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        move(imm, dataTempRegister);
        return branchMul32(cond, dataTempRegister, src, dest);
    }

    Jump branchMul64(ResultCondition cond, RegisterID src1, RegisterID src2, RegisterID scratch1, RegisterID scratch2, RegisterID dest)
    {
        ASSERT(cond != Signed);

        // This is a signed multiple of two 64-bit values, producing a 64-bit result.
        m_assembler.mul<64>(dest, src1, src2);

        if (cond != Overflow)
            return branchTest64(cond, dest);

        // Compute bits 127..64 of the result into scratch1.
        m_assembler.smulh(scratch1, src1, src2);
        // Splat bit 63 of the result to bits 63..0 of scratch2.
        m_assembler.asr<64>(scratch2, dest, 63);
        // Check that bits 31..63 of the original result were all equal.
        return branch64(NotEqual, scratch2, scratch1);
    }

    Jump branchMul64(ResultCondition cond, RegisterID src1, RegisterID src2, RegisterID dest)
    {
        return branchMul64(cond, src1, src2, getCachedDataTempRegisterIDAndInvalidate(), getCachedMemoryTempRegisterIDAndInvalidate(), dest);
    }

    Jump branchMul64(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        return branchMul64(cond, dest, src, dest);
    }

    Jump branchNeg32(ResultCondition cond, RegisterID dest)
    {
        m_assembler.neg<32, S>(dest, dest);
        return Jump(makeBranch(cond));
    }

    Jump branchNeg64(ResultCondition cond, RegisterID srcDest)
    {
        m_assembler.neg<64, S>(srcDest, srcDest);
        return Jump(makeBranch(cond));
    }

    Jump branchSub32(ResultCondition cond, RegisterID dest)
    {
        m_assembler.neg<32, S>(dest, dest);
        return Jump(makeBranch(cond));
    }

    Jump branchSub32(ResultCondition cond, RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.sub<32, S>(dest, op1, op2);
        return Jump(makeBranch(cond));
    }

    Jump branchSub32(ResultCondition cond, RegisterID op1, TrustedImm32 imm, RegisterID dest)
    {
        if (isUInt12(imm.m_value)) {
            m_assembler.sub<32, S>(dest, op1, UInt12(imm.m_value));
            return Jump(makeBranch(cond));
        }
        if (isUInt12(-imm.m_value)) {
            m_assembler.add<32, S>(dest, op1, UInt12(-imm.m_value));
            return Jump(makeBranch(cond));
        }

        signExtend32ToPtr(imm, getCachedDataTempRegisterIDAndInvalidate());
        return branchSub32(cond, op1, dataTempRegister, dest);
    }

    Jump branchSub32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        return branchSub32(cond, dest, src, dest);
    }

    Jump branchSub32(ResultCondition cond, TrustedImm32 imm, RegisterID dest)
    {
        return branchSub32(cond, dest, imm, dest);
    }

    Jump branchSub64(ResultCondition cond, RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.sub<64, S>(dest, op1, op2);
        return Jump(makeBranch(cond));
    }

    Jump branchSub64(ResultCondition cond, RegisterID op1, TrustedImm32 imm, RegisterID dest)
    {
        if (isUInt12(imm.m_value)) {
            m_assembler.sub<64, S>(dest, op1, UInt12(imm.m_value));
            return Jump(makeBranch(cond));
        }
        if (isUInt12(-imm.m_value)) {
            m_assembler.add<64, S>(dest, op1, UInt12(-imm.m_value));
            return Jump(makeBranch(cond));
        }

        move(imm, getCachedDataTempRegisterIDAndInvalidate());
        return branchSub64(cond, op1, dataTempRegister, dest);
    }

    Jump branchSub64(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        return branchSub64(cond, dest, src, dest);
    }

    Jump branchSub64(ResultCondition cond, TrustedImm32 imm, RegisterID dest)
    {
        return branchSub64(cond, dest, imm, dest);
    }


    // Jumps, calls, returns

    ALWAYS_INLINE Call call()
    {
        AssemblerLabel pointerLabel = m_assembler.label();
        moveWithFixedWidth(TrustedImmPtr(0), getCachedDataTempRegisterIDAndInvalidate());
        invalidateAllTempRegisters();
        m_assembler.blr(dataTempRegister);
        AssemblerLabel callLabel = m_assembler.label();
        ASSERT_UNUSED(pointerLabel, ARM64Assembler::getDifferenceBetweenLabels(callLabel, pointerLabel) == REPATCH_OFFSET_CALL_TO_POINTER);
        return Call(callLabel, Call::Linkable);
    }

    ALWAYS_INLINE Call call(RegisterID target)
    {
        invalidateAllTempRegisters();
        m_assembler.blr(target);
        return Call(m_assembler.label(), Call::None);
    }

    ALWAYS_INLINE Call call(Address address)
    {
        load64(address, getCachedDataTempRegisterIDAndInvalidate());
        return call(dataTempRegister);
    }

    ALWAYS_INLINE Jump jump()
    {
        AssemblerLabel label = m_assembler.label();
        m_assembler.b();
        return Jump(label, m_makeJumpPatchable ? ARM64Assembler::JumpNoConditionFixedSize : ARM64Assembler::JumpNoCondition);
    }

    void jump(RegisterID target)
    {
        m_assembler.br(target);
    }

    void jump(Address address)
    {
        load64(address, getCachedDataTempRegisterIDAndInvalidate());
        m_assembler.br(dataTempRegister);
    }

    void jump(AbsoluteAddress address)
    {
        move(TrustedImmPtr(address.m_ptr), getCachedDataTempRegisterIDAndInvalidate());
        load64(Address(dataTempRegister), dataTempRegister);
        m_assembler.br(dataTempRegister);
    }

    ALWAYS_INLINE Call makeTailRecursiveCall(Jump oldJump)
    {
        oldJump.link(this);
        return tailRecursiveCall();
    }

    ALWAYS_INLINE Call nearCall()
    {
        m_assembler.bl();
        return Call(m_assembler.label(), Call::LinkableNear);
    }

#if 0
    ALWAYS_INLINE Call nearTailCall()
    {
        AssemblerLabel label = m_assembler.label();
        m_assembler.b();
        return Call(label, Call::LinkableNearTail);
    }
#endif

    ALWAYS_INLINE void ret()
    {
        m_assembler.ret();
    }

    ALWAYS_INLINE Call tailRecursiveCall()
    {
        // Like a normal call, but don't link.
        AssemblerLabel pointerLabel = m_assembler.label();
        moveWithFixedWidth(TrustedImmPtr(0), getCachedDataTempRegisterIDAndInvalidate());
        m_assembler.br(dataTempRegister);
        AssemblerLabel callLabel = m_assembler.label();
        ASSERT_UNUSED(pointerLabel, ARM64Assembler::getDifferenceBetweenLabels(callLabel, pointerLabel) == REPATCH_OFFSET_CALL_TO_POINTER);
        return Call(callLabel, Call::Linkable);
    }


    // Comparisons operations

    void compare32(RelationalCondition cond, RegisterID left, RegisterID right, RegisterID dest)
    {
        m_assembler.cmp<32>(left, right);
        m_assembler.cset<32>(dest, ARM64Condition(cond));
    }

    void compare32(RelationalCondition cond, Address left, RegisterID right, RegisterID dest)
    {
        load32(left, getCachedDataTempRegisterIDAndInvalidate());
        m_assembler.cmp<32>(dataTempRegister, right);
        m_assembler.cset<32>(dest, ARM64Condition(cond));
    }

    void compare32(RelationalCondition cond, RegisterID left, TrustedImm32 right, RegisterID dest)
    {
        move(right, getCachedDataTempRegisterIDAndInvalidate());
        m_assembler.cmp<32>(left, dataTempRegister);
        m_assembler.cset<32>(dest, ARM64Condition(cond));
    }

    void compare64(RelationalCondition cond, RegisterID left, RegisterID right, RegisterID dest)
    {
        m_assembler.cmp<64>(left, right);
        m_assembler.cset<32>(dest, ARM64Condition(cond));
    }
    
    void compare64(RelationalCondition cond, RegisterID left, TrustedImm32 right, RegisterID dest)
    {
        signExtend32ToPtr(right, getCachedDataTempRegisterIDAndInvalidate());
        m_assembler.cmp<64>(left, dataTempRegister);
        m_assembler.cset<32>(dest, ARM64Condition(cond));
    }

    void compare8(RelationalCondition cond, Address left, TrustedImm32 right, RegisterID dest)
    {
        load8(left, getCachedMemoryTempRegisterIDAndInvalidate());
        move(right, getCachedDataTempRegisterIDAndInvalidate());
        compare32(cond, memoryTempRegister, dataTempRegister, dest);
    }

    void test32(ResultCondition cond, RegisterID src, RegisterID mask, RegisterID dest)
    {
        m_assembler.tst<32>(src, mask);
        m_assembler.cset<32>(dest, ARM64Condition(cond));
    }

    void test32(ResultCondition cond, RegisterID src, TrustedImm32 mask, RegisterID dest)
    {
        if (mask.m_value == -1)
            m_assembler.tst<32>(src, src);
        else {
            signExtend32ToPtr(mask, getCachedDataTempRegisterIDAndInvalidate());
            m_assembler.tst<32>(src, dataTempRegister);
        }
        m_assembler.cset<32>(dest, ARM64Condition(cond));
    }

    void test32(ResultCondition cond, Address address, TrustedImm32 mask, RegisterID dest)
    {
        load32(address, getCachedMemoryTempRegisterIDAndInvalidate());
        test32(cond, memoryTempRegister, mask, dest);
    }

    void test8(ResultCondition cond, Address address, TrustedImm32 mask, RegisterID dest)
    {
        load8(address, getCachedMemoryTempRegisterIDAndInvalidate());
        test32(cond, memoryTempRegister, mask, dest);
    }

    void test64(ResultCondition cond, RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.tst<64>(op1, op2);
        m_assembler.cset<32>(dest, ARM64Condition(cond));
    }

    void test64(ResultCondition cond, RegisterID src, TrustedImm32 mask, RegisterID dest)
    {
        if (mask.m_value == -1)
            m_assembler.tst<64>(src, src);
        else {
            signExtend32ToPtr(mask, getCachedDataTempRegisterIDAndInvalidate());
            m_assembler.tst<64>(src, dataTempRegister);
        }
        m_assembler.cset<32>(dest, ARM64Condition(cond));
    }

    void setCarry(RegisterID dest)
    {
        m_assembler.cset<32>(dest, ARM64Assembler::ConditionCS);
    }

    // Patchable operations

    ALWAYS_INLINE DataLabel32 moveWithPatch(TrustedImm32 imm, RegisterID dest)
    {
        DataLabel32 label(this);
        moveWithFixedWidth(imm, dest);
        return label;
    }

    ALWAYS_INLINE DataLabelPtr moveWithPatch(TrustedImmPtr imm, RegisterID dest)
    {
        DataLabelPtr label(this);
        moveWithFixedWidth(imm, dest);
        return label;
    }

    ALWAYS_INLINE Jump branchPtrWithPatch(RelationalCondition cond, RegisterID left, DataLabelPtr& dataLabel, TrustedImmPtr initialRightValue = TrustedImmPtr(0))
    {
        dataLabel = DataLabelPtr(this);
        moveWithPatch(initialRightValue, getCachedDataTempRegisterIDAndInvalidate());
        return branch64(cond, left, dataTempRegister);
    }

    ALWAYS_INLINE Jump branchPtrWithPatch(RelationalCondition cond, Address left, DataLabelPtr& dataLabel, TrustedImmPtr initialRightValue = TrustedImmPtr(0))
    {
        dataLabel = DataLabelPtr(this);
        moveWithPatch(initialRightValue, getCachedDataTempRegisterIDAndInvalidate());
        return branch64(cond, left, dataTempRegister);
    }

    ALWAYS_INLINE Jump branch32WithPatch(RelationalCondition cond, Address left, DataLabel32& dataLabel, TrustedImm32 initialRightValue = TrustedImm32(0))
    {
        dataLabel = DataLabel32(this);
        moveWithPatch(initialRightValue, getCachedDataTempRegisterIDAndInvalidate());
        return branch32(cond, left, dataTempRegister);
    }

    PatchableJump patchableBranchPtr(RelationalCondition cond, Address left, TrustedImmPtr right)
    {
        m_makeJumpPatchable = true;
        Jump result = branch64(cond, left, TrustedImm64(right));
        m_makeJumpPatchable = false;
        return PatchableJump(result);
    }

    PatchableJump patchableBranchTest32(ResultCondition cond, RegisterID reg, TrustedImm32 mask = TrustedImm32(-1))
    {
        m_makeJumpPatchable = true;
        Jump result = branchTest32(cond, reg, mask);
        m_makeJumpPatchable = false;
        return PatchableJump(result);
    }

    PatchableJump patchableBranch32(RelationalCondition cond, RegisterID reg, TrustedImm32 imm)
    {
        m_makeJumpPatchable = true;
        Jump result = branch32(cond, reg, imm);
        m_makeJumpPatchable = false;
        return PatchableJump(result);
    }

    PatchableJump patchableBranch64(RelationalCondition cond, RegisterID reg, TrustedImm64 imm)
    {
        m_makeJumpPatchable = true;
        Jump result = branch64(cond, reg, imm);
        m_makeJumpPatchable = false;
        return PatchableJump(result);
    }

    PatchableJump patchableBranch64(RelationalCondition cond, RegisterID left, RegisterID right)
    {
        m_makeJumpPatchable = true;
        Jump result = branch64(cond, left, right);
        m_makeJumpPatchable = false;
        return PatchableJump(result);
    }

    PatchableJump patchableBranchPtrWithPatch(RelationalCondition cond, Address left, DataLabelPtr& dataLabel, TrustedImmPtr initialRightValue = TrustedImmPtr(0))
    {
        m_makeJumpPatchable = true;
        Jump result = branchPtrWithPatch(cond, left, dataLabel, initialRightValue);
        m_makeJumpPatchable = false;
        return PatchableJump(result);
    }

    PatchableJump patchableBranch32WithPatch(RelationalCondition cond, Address left, DataLabel32& dataLabel, TrustedImm32 initialRightValue = TrustedImm32(0))
    {
        m_makeJumpPatchable = true;
        Jump result = branch32WithPatch(cond, left, dataLabel, initialRightValue);
        m_makeJumpPatchable = false;
        return PatchableJump(result);
    }

    PatchableJump patchableJump()
    {
        m_makeJumpPatchable = true;
        Jump result = jump();
        m_makeJumpPatchable = false;
        return PatchableJump(result);
    }

    ALWAYS_INLINE DataLabelPtr storePtrWithPatch(TrustedImmPtr initialValue, ImplicitAddress address)
    {
        DataLabelPtr label(this);
        moveWithFixedWidth(initialValue, getCachedDataTempRegisterIDAndInvalidate());
        store64(dataTempRegister, address);
        return label;
    }

    ALWAYS_INLINE DataLabelPtr storePtrWithPatch(ImplicitAddress address)
    {
        return storePtrWithPatch(TrustedImmPtr(0), address);
    }

    static void reemitInitialMoveWithPatch(void* address, void* value)
    {
        ARM64Assembler::setPointer(static_cast<int*>(address), value, dataTempRegister, true);
    }

    // Miscellaneous operations:

    void breakpoint(uint16_t imm = 0)
    {
        m_assembler.brk(imm);
    }

    void nop()
    {
        m_assembler.nop();
    }
    
    void memoryFence()
    {
        m_assembler.dmbSY();
    }


    // Misc helper functions.

    // Invert a relational condition, e.g. == becomes !=, < becomes >=, etc.
    static RelationalCondition invert(RelationalCondition cond)
    {
        return static_cast<RelationalCondition>(ARM64Assembler::invert(static_cast<ARM64Assembler::Condition>(cond)));
    }

    static FunctionPtr readCallTarget(CodeLocationCall call)
    {
        return FunctionPtr(reinterpret_cast<void(*)()>(ARM64Assembler::readCallTarget(call.dataLocation())));
    }

    static void replaceWithJump(CodeLocationLabel instructionStart, CodeLocationLabel destination)
    {
        ARM64Assembler::replaceWithJump(instructionStart.dataLocation(), destination.dataLocation());
    }
    
    static ptrdiff_t maxJumpReplacementSize()
    {
        return ARM64Assembler::maxJumpReplacementSize();
    }

    RegisterID scratchRegisterForBlinding()
    {
        // We *do not* have a scratch register for blinding.
        RELEASE_ASSERT_NOT_REACHED();
        return getCachedDataTempRegisterIDAndInvalidate();
    }

    static bool canJumpReplacePatchableBranchPtrWithPatch() { return false; }
    static bool canJumpReplacePatchableBranch32WithPatch() { return false; }
    
    static CodeLocationLabel startOfBranchPtrWithPatchOnRegister(CodeLocationDataLabelPtr label)
    {
        return label.labelAtOffset(0);
    }
    
    static CodeLocationLabel startOfPatchableBranchPtrWithPatchOnAddress(CodeLocationDataLabelPtr)
    {
        UNREACHABLE_FOR_PLATFORM();
        return CodeLocationLabel();
    }
    
    static CodeLocationLabel startOfPatchableBranch32WithPatchOnAddress(CodeLocationDataLabel32)
    {
        UNREACHABLE_FOR_PLATFORM();
        return CodeLocationLabel();
    }
    
    static void revertJumpReplacementToBranchPtrWithPatch(CodeLocationLabel instructionStart, RegisterID, void* initialValue)
    {
        reemitInitialMoveWithPatch(instructionStart.dataLocation(), initialValue);
    }
    
    static void revertJumpReplacementToPatchableBranchPtrWithPatch(CodeLocationLabel, Address, void*)
    {
        UNREACHABLE_FOR_PLATFORM();
    }

    static void revertJumpReplacementToPatchableBranch32WithPatch(CodeLocationLabel, Address, int32_t)
    {
        UNREACHABLE_FOR_PLATFORM();
    }

    static void repatchCall(CodeLocationCall call, CodeLocationLabel destination)
    {
        ARM64Assembler::repatchPointer(call.dataLabelPtrAtOffset(REPATCH_OFFSET_CALL_TO_POINTER).dataLocation(), destination.executableAddress());
    }

    static void repatchCall(CodeLocationCall call, FunctionPtr destination)
    {
        ARM64Assembler::repatchPointer(call.dataLabelPtrAtOffset(REPATCH_OFFSET_CALL_TO_POINTER).dataLocation(), destination.executableAddress());
    }

#if ENABLE(MASM_PROBE)
    void probe(ProbeFunction, void* arg1, void* arg2);
#endif // ENABLE(MASM_PROBE)

protected:
    ALWAYS_INLINE Jump makeBranch(ARM64Assembler::Condition cond)
    {
        m_assembler.b_cond(cond);
        AssemblerLabel label = m_assembler.label();
        m_assembler.nop();
        return Jump(label, m_makeJumpPatchable ? ARM64Assembler::JumpConditionFixedSize : ARM64Assembler::JumpCondition, cond);
    }
    ALWAYS_INLINE Jump makeBranch(RelationalCondition cond) { return makeBranch(ARM64Condition(cond)); }
    ALWAYS_INLINE Jump makeBranch(ResultCondition cond) { return makeBranch(ARM64Condition(cond)); }
    ALWAYS_INLINE Jump makeBranch(DoubleCondition cond) { return makeBranch(ARM64Condition(cond)); }

    template <int dataSize>
    ALWAYS_INLINE Jump makeCompareAndBranch(ZeroCondition cond, RegisterID reg)
    {
        if (cond == IsZero)
            m_assembler.cbz<dataSize>(reg);
        else
            m_assembler.cbnz<dataSize>(reg);
        AssemblerLabel label = m_assembler.label();
        m_assembler.nop();
        return Jump(label, m_makeJumpPatchable ? ARM64Assembler::JumpCompareAndBranchFixedSize : ARM64Assembler::JumpCompareAndBranch, static_cast<ARM64Assembler::Condition>(cond), dataSize == 64, reg);
    }

    ALWAYS_INLINE Jump makeTestBitAndBranch(RegisterID reg, unsigned bit, ZeroCondition cond)
    {
        ASSERT(bit < 64);
        bit &= 0x3f;
        if (cond == IsZero)
            m_assembler.tbz(reg, bit);
        else
            m_assembler.tbnz(reg, bit);
        AssemblerLabel label = m_assembler.label();
        m_assembler.nop();
        return Jump(label, m_makeJumpPatchable ? ARM64Assembler::JumpTestBitFixedSize : ARM64Assembler::JumpTestBit, static_cast<ARM64Assembler::Condition>(cond), bit, reg);
    }

    ARM64Assembler::Condition ARM64Condition(RelationalCondition cond)
    {
        return static_cast<ARM64Assembler::Condition>(cond);
    }

    ARM64Assembler::Condition ARM64Condition(ResultCondition cond)
    {
        return static_cast<ARM64Assembler::Condition>(cond);
    }

    ARM64Assembler::Condition ARM64Condition(DoubleCondition cond)
    {
        return static_cast<ARM64Assembler::Condition>(cond);
    }
    
private:
    ALWAYS_INLINE RegisterID getCachedDataTempRegisterIDAndInvalidate()
    {
        RELEASE_ASSERT(m_allowScratchRegister);
        return m_dataMemoryTempRegister.registerIDInvalidate();
    }
    ALWAYS_INLINE RegisterID getCachedMemoryTempRegisterIDAndInvalidate()
    {
        RELEASE_ASSERT(m_allowScratchRegister);
        return m_cachedMemoryTempRegister.registerIDInvalidate();
    }

    ALWAYS_INLINE bool isInIntRange(int64_t value)
    {
        return value == ((value << 32) >> 32);
    }

    template<typename ImmediateType, typename rawType>
    void moveInternal(ImmediateType imm, RegisterID dest)
    {
        const int dataSize = sizeof(rawType) * 8;
        const int numberHalfWords = dataSize / 16;
        rawType value = bitwise_cast<rawType>(imm.m_value);
        uint16_t halfword[numberHalfWords];

        // Handle 0 and ~0 here to simplify code below
        if (!value) {
            m_assembler.movz<dataSize>(dest, 0);
            return;
        }
        if (!~value) {
            m_assembler.movn<dataSize>(dest, 0);
            return;
        }

        LogicalImmediate logicalImm = dataSize == 64 ? LogicalImmediate::create64(static_cast<uint64_t>(value)) : LogicalImmediate::create32(static_cast<uint32_t>(value));

        if (logicalImm.isValid()) {
            m_assembler.movi<dataSize>(dest, logicalImm);
            return;
        }

        // Figure out how many halfwords are 0 or FFFF, then choose movz or movn accordingly.
        int zeroOrNegateVote = 0;
        for (int i = 0; i < numberHalfWords; ++i) {
            halfword[i] = getHalfword(value, i);
            if (!halfword[i])
                zeroOrNegateVote++;
            else if (halfword[i] == 0xffff)
                zeroOrNegateVote--;
        }

        bool needToClearRegister = true;
        if (zeroOrNegateVote >= 0) {
            for (int i = 0; i < numberHalfWords; i++) {
                if (halfword[i]) {
                    if (needToClearRegister) {
                        m_assembler.movz<dataSize>(dest, halfword[i], 16*i);
                        needToClearRegister = false;
                    } else
                        m_assembler.movk<dataSize>(dest, halfword[i], 16*i);
                }
            }
        } else {
            for (int i = 0; i < numberHalfWords; i++) {
                if (halfword[i] != 0xffff) {
                    if (needToClearRegister) {
                        m_assembler.movn<dataSize>(dest, ~halfword[i], 16*i);
                        needToClearRegister = false;
                    } else
                        m_assembler.movk<dataSize>(dest, halfword[i], 16*i);
                }
            }
        }
    }

    template<int datasize>
    void loadUnsignedImmediate(RegisterID rt, RegisterID rn, unsigned pimm);

    template<int datasize>
    void loadUnscaledImmediate(RegisterID rt, RegisterID rn, int simm);

    template<int datasize>
    void loadSignedAddressedByUnsignedImmediate(RegisterID rt, RegisterID rn, unsigned pimm);

    template<int datasize>
    void loadSignedAddressedByUnscaledImmediate(RegisterID rt, RegisterID rn, int simm);

    template<int datasize>
    void storeUnsignedImmediate(RegisterID rt, RegisterID rn, unsigned pimm);

    template<int datasize>
    void storeUnscaledImmediate(RegisterID rt, RegisterID rn, int simm);

    void moveWithFixedWidth(TrustedImm32 imm, RegisterID dest)
    {
        int32_t value = imm.m_value;
        m_assembler.movz<32>(dest, getHalfword(value, 0));
        m_assembler.movk<32>(dest, getHalfword(value, 1), 16);
    }

    void moveWithFixedWidth(TrustedImmPtr imm, RegisterID dest)
    {
        intptr_t value = reinterpret_cast<intptr_t>(imm.m_value);
        m_assembler.movz<64>(dest, getHalfword(value, 0));
        m_assembler.movk<64>(dest, getHalfword(value, 1), 16);
        m_assembler.movk<64>(dest, getHalfword(value, 2), 32);
    }

    void signExtend32ToPtrWithFixedWidth(int32_t value, RegisterID dest)
    {
        if (value >= 0) {
            m_assembler.movz<32>(dest, getHalfword(value, 0));
            m_assembler.movk<32>(dest, getHalfword(value, 1), 16);
        } else {
            m_assembler.movn<32>(dest, ~getHalfword(value, 0));
            m_assembler.movk<32>(dest, getHalfword(value, 1), 16);
        }
    }

    template<int datasize>
    ALWAYS_INLINE void load(const void* address, RegisterID dest)
    {
        intptr_t currentRegisterContents;
        if (m_cachedMemoryTempRegister.value(currentRegisterContents)) {
            intptr_t addressAsInt = reinterpret_cast<intptr_t>(address);
            intptr_t addressDelta = addressAsInt - currentRegisterContents;

            if (dest == memoryTempRegister)
                m_cachedMemoryTempRegister.invalidate();

            if (isInIntRange(addressDelta)) {
                if (ARM64Assembler::canEncodeSImmOffset(addressDelta)) {
                    m_assembler.ldur<datasize>(dest,  memoryTempRegister, addressDelta);
                    return;
                }

                if (ARM64Assembler::canEncodePImmOffset<datasize>(addressDelta)) {
                    m_assembler.ldr<datasize>(dest,  memoryTempRegister, addressDelta);
                    return;
                }
            }

            if ((addressAsInt & (~maskHalfWord0)) == (currentRegisterContents & (~maskHalfWord0))) {
                m_assembler.movk<64>(memoryTempRegister, addressAsInt & maskHalfWord0, 0);
                m_cachedMemoryTempRegister.setValue(reinterpret_cast<intptr_t>(address));
                m_assembler.ldr<datasize>(dest, memoryTempRegister, ARM64Registers::zr);
                return;
            }
        }

        move(TrustedImmPtr(address), memoryTempRegister);
        if (dest == memoryTempRegister)
            m_cachedMemoryTempRegister.invalidate();
        else
            m_cachedMemoryTempRegister.setValue(reinterpret_cast<intptr_t>(address));
        m_assembler.ldr<datasize>(dest, memoryTempRegister, ARM64Registers::zr);
    }

    template<int datasize>
    ALWAYS_INLINE void store(RegisterID src, const void* address)
    {
        ASSERT(src != memoryTempRegister);
        intptr_t currentRegisterContents;
        if (m_cachedMemoryTempRegister.value(currentRegisterContents)) {
            intptr_t addressAsInt = reinterpret_cast<intptr_t>(address);
            intptr_t addressDelta = addressAsInt - currentRegisterContents;

            if (isInIntRange(addressDelta)) {
                if (ARM64Assembler::canEncodeSImmOffset(addressDelta)) {
                    m_assembler.stur<datasize>(src, memoryTempRegister, addressDelta);
                    return;
                }

                if (ARM64Assembler::canEncodePImmOffset<datasize>(addressDelta)) {
                    m_assembler.str<datasize>(src, memoryTempRegister, addressDelta);
                    return;
                }
            }

            if ((addressAsInt & (~maskHalfWord0)) == (currentRegisterContents & (~maskHalfWord0))) {
                m_assembler.movk<64>(memoryTempRegister, addressAsInt & maskHalfWord0, 0);
                m_cachedMemoryTempRegister.setValue(reinterpret_cast<intptr_t>(address));
                m_assembler.str<datasize>(src, memoryTempRegister, ARM64Registers::zr);
                return;
            }
        }

        move(TrustedImmPtr(address), memoryTempRegister);
        m_cachedMemoryTempRegister.setValue(reinterpret_cast<intptr_t>(address));
        m_assembler.str<datasize>(src, memoryTempRegister, ARM64Registers::zr);
    }

    template <int dataSize>
    ALWAYS_INLINE bool tryMoveUsingCacheRegisterContents(intptr_t immediate, CachedTempRegister& dest)
    {
#if 1
        Q_UNUSED(immediate);
        Q_UNUSED(dest);
#else
        intptr_t currentRegisterContents;
        if (dest.value(currentRegisterContents)) {
            if (currentRegisterContents == immediate)
                return true;

            LogicalImmediate logicalImm = dataSize == 64 ? LogicalImmediate::create64(static_cast<uint64_t>(immediate)) : LogicalImmediate::create32(static_cast<uint32_t>(immediate));

            if (logicalImm.isValid()) {
                m_assembler.movi<dataSize>(dest.registerIDNoInvalidate(), logicalImm);
                dest.setValue(immediate);
                return true;
            }

            if ((immediate & maskUpperWord) == (currentRegisterContents & maskUpperWord)) {
                if ((immediate & maskHalfWord1) != (currentRegisterContents & maskHalfWord1))
                    m_assembler.movk<dataSize>(dest.registerIDNoInvalidate(), (immediate & maskHalfWord1) >> 16, 16);

                if ((immediate & maskHalfWord0) != (currentRegisterContents & maskHalfWord0))
                    m_assembler.movk<dataSize>(dest.registerIDNoInvalidate(), immediate & maskHalfWord0, 0);

                dest.setValue(immediate);
                return true;
            }
        }
#endif

        return false;
    }

    void moveToCachedReg(TrustedImm32 imm, CachedTempRegister& dest)
    {
        if (tryMoveUsingCacheRegisterContents<32>(static_cast<intptr_t>(imm.m_value), dest))
            return;

        moveInternal<TrustedImm32, int32_t>(imm, dest.registerIDNoInvalidate());
        dest.setValue(imm.m_value);
    }

    void moveToCachedReg(TrustedImmPtr imm, CachedTempRegister& dest)
    {
        if (tryMoveUsingCacheRegisterContents<64>(imm.asIntptr(), dest))
            return;

        moveInternal<TrustedImmPtr, intptr_t>(imm, dest.registerIDNoInvalidate());
        dest.setValue(imm.asIntptr());
    }

    void moveToCachedReg(TrustedImm64 imm, CachedTempRegister& dest)
    {
        if (tryMoveUsingCacheRegisterContents<64>(static_cast<intptr_t>(imm.m_value), dest))
            return;

        moveInternal<TrustedImm64, int64_t>(imm, dest.registerIDNoInvalidate());
        dest.setValue(imm.m_value);
    }

    template<int datasize>
    bool tryLoadWithOffset(RegisterID rt, RegisterID rn, int32_t offset);

    template<int datasize>
    bool tryLoadSignedWithOffset(RegisterID rt, RegisterID rn, int32_t offset);

    template<int datasize>
    bool tryLoadWithOffset(FPRegisterID rt, RegisterID rn, int32_t offset);

    template<int datasize>
    bool tryStoreWithOffset(RegisterID rt, RegisterID rn, int32_t offset);

    template<int datasize>
    bool tryStoreWithOffset(FPRegisterID rt, RegisterID rn, int32_t offset);

    Jump jumpAfterFloatingPointCompare(DoubleCondition cond)
    {
        if (cond == DoubleNotEqual) {
            // ConditionNE jumps if NotEqual *or* unordered - force the unordered cases not to jump.
            Jump unordered = makeBranch(ARM64Assembler::ConditionVS);
            Jump result = makeBranch(ARM64Assembler::ConditionNE);
            unordered.link(this);
            return result;
        }
        if (cond == DoubleEqualOrUnordered) {
            Jump unordered = makeBranch(ARM64Assembler::ConditionVS);
            Jump notEqual = makeBranch(ARM64Assembler::ConditionNE);
            unordered.link(this);
            // We get here if either unordered or equal.
            Jump result = jump();
            notEqual.link(this);
            return result;
        }
        return makeBranch(cond);
    }

    template <typename, template <typename> class> friend class LinkBufferBase;
    template <typename> friend class BranchCompactingLinkBuffer;
    template <typename> friend struct BranchCompactingExecutableOffsetCalculator;
    void recordLinkOffsets(int32_t regionStart, int32_t regionEnd, int32_t offset) {return m_assembler.recordLinkOffsets(regionStart, regionEnd, offset); }
    int executableOffsetFor(int location) { return m_assembler.executableOffsetFor(location); }

    static void linkCall(void* code, Call call, FunctionPtr function)
    {
        if (!call.isFlagSet(Call::Near))
            ARM64Assembler::linkPointer(code, call.m_label.labelAtOffset(REPATCH_OFFSET_CALL_TO_POINTER), function.value());
#if 0
        else if (call.isFlagSet(Call::Tail))
            ARM64Assembler::linkJump(code, call.m_label, function.value());
#endif
        else
            ARM64Assembler::linkCall(code, call.m_label, function.value());
    }

    CachedTempRegister m_dataMemoryTempRegister;
    CachedTempRegister m_cachedMemoryTempRegister;
    bool m_makeJumpPatchable;
    bool m_allowScratchRegister = true;
};

template<int datasize>
ALWAYS_INLINE void MacroAssemblerARM64::loadUnsignedImmediate(RegisterID rt, RegisterID rn, unsigned pimm)
{
    m_assembler.ldr<datasize>(rt, rn, pimm);
}

template<int datasize>
ALWAYS_INLINE void MacroAssemblerARM64::loadUnscaledImmediate(RegisterID rt, RegisterID rn, int simm)
{
    m_assembler.ldur<datasize>(rt, rn, simm);
}

template<int datasize>
ALWAYS_INLINE void MacroAssemblerARM64::loadSignedAddressedByUnsignedImmediate(RegisterID rt, RegisterID rn, unsigned pimm)
{
    loadUnsignedImmediate<datasize>(rt, rn, pimm);
}

template<int datasize>
ALWAYS_INLINE void MacroAssemblerARM64::loadSignedAddressedByUnscaledImmediate(RegisterID rt, RegisterID rn, int simm)
{
    loadUnscaledImmediate<datasize>(rt, rn, simm);
}

template<int datasize>
ALWAYS_INLINE void MacroAssemblerARM64::storeUnsignedImmediate(RegisterID rt, RegisterID rn, unsigned pimm)
{
    m_assembler.str<datasize>(rt, rn, pimm);
}

template<int datasize>
ALWAYS_INLINE void MacroAssemblerARM64::storeUnscaledImmediate(RegisterID rt, RegisterID rn, int simm)
{
    m_assembler.stur<datasize>(rt, rn, simm);
}


// Extend the {load,store}{Unsigned,Unscaled}Immediate templated general register methods to cover all load/store sizes
template<>
ALWAYS_INLINE void MacroAssemblerARM64::loadUnsignedImmediate<8>(RegisterID rt, RegisterID rn, unsigned pimm)
{
    m_assembler.ldrb(rt, rn, pimm);
}

template<>
ALWAYS_INLINE void MacroAssemblerARM64::loadUnsignedImmediate<16>(RegisterID rt, RegisterID rn, unsigned pimm)
{
    m_assembler.ldrh(rt, rn, pimm);
}

template<>
ALWAYS_INLINE void MacroAssemblerARM64::loadSignedAddressedByUnsignedImmediate<8>(RegisterID rt, RegisterID rn, unsigned pimm)
{
    m_assembler.ldrsb<64>(rt, rn, pimm);
}

template<>
ALWAYS_INLINE void MacroAssemblerARM64::loadSignedAddressedByUnsignedImmediate<16>(RegisterID rt, RegisterID rn, unsigned pimm)
{
    m_assembler.ldrsh<64>(rt, rn, pimm);
}

template<>
ALWAYS_INLINE void MacroAssemblerARM64::loadUnscaledImmediate<8>(RegisterID rt, RegisterID rn, int simm)
{
    m_assembler.ldurb(rt, rn, simm);
}

template<>
ALWAYS_INLINE void MacroAssemblerARM64::loadUnscaledImmediate<16>(RegisterID rt, RegisterID rn, int simm)
{
    m_assembler.ldurh(rt, rn, simm);
}

template<>
ALWAYS_INLINE void MacroAssemblerARM64::loadSignedAddressedByUnscaledImmediate<8>(RegisterID rt, RegisterID rn, int simm)
{
    m_assembler.ldursb<64>(rt, rn, simm);
}

template<>
ALWAYS_INLINE void MacroAssemblerARM64::loadSignedAddressedByUnscaledImmediate<16>(RegisterID rt, RegisterID rn, int simm)
{
    m_assembler.ldursh<64>(rt, rn, simm);
}

template<>
ALWAYS_INLINE void MacroAssemblerARM64::storeUnsignedImmediate<8>(RegisterID rt, RegisterID rn, unsigned pimm)
{
    m_assembler.strb(rt, rn, pimm);
}

template<>
ALWAYS_INLINE void MacroAssemblerARM64::storeUnsignedImmediate<16>(RegisterID rt, RegisterID rn, unsigned pimm)
{
    m_assembler.strh(rt, rn, pimm);
}

template<>
ALWAYS_INLINE void MacroAssemblerARM64::storeUnscaledImmediate<8>(RegisterID rt, RegisterID rn, int simm)
{
    m_assembler.sturb(rt, rn, simm);
}

template<>
ALWAYS_INLINE void MacroAssemblerARM64::storeUnscaledImmediate<16>(RegisterID rt, RegisterID rn, int simm)
{
    m_assembler.sturh(rt, rn, simm);
}

template<int datasize>
ALWAYS_INLINE bool MacroAssemblerARM64::tryLoadSignedWithOffset(RegisterID rt, RegisterID rn, int32_t offset)
{
    if (ARM64Assembler::canEncodeSImmOffset(offset)) {
        loadSignedAddressedByUnscaledImmediate<datasize>(rt, rn, offset);
        return true;
    }
    if (ARM64Assembler::canEncodePImmOffset<datasize>(offset)) {
        loadSignedAddressedByUnsignedImmediate<datasize>(rt, rn, static_cast<unsigned>(offset));
        return true;
    }
    return false;
}

template<int datasize>
ALWAYS_INLINE bool MacroAssemblerARM64::tryStoreWithOffset(RegisterID rt, RegisterID rn, int32_t offset)
{
    if (ARM64Assembler::canEncodeSImmOffset(offset)) {
        storeUnscaledImmediate<datasize>(rt, rn, offset);
        return true;
    }
    if (ARM64Assembler::canEncodePImmOffset<datasize>(offset)) {
        storeUnsignedImmediate<datasize>(rt, rn, static_cast<unsigned>(offset));
        return true;
    }
    return false;
}

template<int datasize>
ALWAYS_INLINE bool MacroAssemblerARM64::tryStoreWithOffset(FPRegisterID rt, RegisterID rn, int32_t offset)
{
    if (ARM64Assembler::canEncodeSImmOffset(offset)) {
        m_assembler.stur<datasize>(rt, rn, offset);
        return true;
    }
    if (ARM64Assembler::canEncodePImmOffset<datasize>(offset)) {
        m_assembler.str<datasize>(rt, rn, static_cast<unsigned>(offset));
        return true;
    }
    return false;
}


template<int datasize>
ALWAYS_INLINE bool MacroAssemblerARM64::tryLoadWithOffset(RegisterID rt, RegisterID rn, int32_t offset)
{
    if (ARM64Assembler::canEncodeSImmOffset(offset)) {
        loadUnscaledImmediate<datasize>(rt, rn, offset);
        return true;
    }
    if (ARM64Assembler::canEncodePImmOffset<datasize>(offset)) {
        loadUnsignedImmediate<datasize>(rt, rn, static_cast<unsigned>(offset));
        return true;
    }
    return false;
}

template<int datasize>
ALWAYS_INLINE bool MacroAssemblerARM64::tryLoadWithOffset(FPRegisterID rt, RegisterID rn, int32_t offset)
    {
        if (ARM64Assembler::canEncodeSImmOffset(offset)) {
            m_assembler.ldur<datasize>(rt, rn, offset);
            return true;
        }
        if (ARM64Assembler::canEncodePImmOffset<datasize>(offset)) {
            m_assembler.ldr<datasize>(rt, rn, static_cast<unsigned>(offset));
            return true;
        }
        return false;
    }

} // namespace JSC

#endif // ENABLE(ASSEMBLER)

#endif // MacroAssemblerARM64_h
