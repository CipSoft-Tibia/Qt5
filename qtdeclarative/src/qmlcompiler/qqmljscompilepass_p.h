// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLJSCOMPILEPASS_P_H
#define QQMLJSCOMPILEPASS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.


#include <private/qqmljslogger_p.h>
#include <private/qqmljsregistercontent_p.h>
#include <private/qqmljsscope_p.h>
#include <private/qqmljstyperesolver_p.h>
#include <private/qv4bytecodehandler_p.h>
#include <private/qv4compiler_p.h>
#include <private/qflatmap_p.h>

QT_BEGIN_NAMESPACE

class QQmlJSCompilePass : public QV4::Moth::ByteCodeHandler
{
    Q_DISABLE_COPY_MOVE(QQmlJSCompilePass)
public:
    enum RegisterShortcuts {
        InvalidRegister = -1,
        Accumulator = QV4::CallData::Accumulator,
        This = QV4::CallData::This,
        FirstArgument = QV4::CallData::OffsetCount
    };

    using SourceLocationTable = QV4::Compiler::Context::SourceLocationTable;

    struct VirtualRegister
    {
        QQmlJSRegisterContent content;
        bool canMove = false;
        bool affectedBySideEffects = false;

    private:
        friend bool operator==(const VirtualRegister &a, const VirtualRegister &b)
        {
            return a.content == b.content && a.canMove == b.canMove
                && a.affectedBySideEffects == b.affectedBySideEffects;
        }
    };

    // map from register index to expected type
    using VirtualRegisters = QFlatMap<int, VirtualRegister>;

    struct InstructionAnnotation
    {
        // Registers explicit read as part of the instruction.
        VirtualRegisters readRegisters;

        // Registers that have to be converted for future instructions after a jump.
        VirtualRegisters typeConversions;

        QQmlJSRegisterContent changedRegister;
        int changedRegisterIndex = InvalidRegister;
        bool hasSideEffects = false;
        bool isRename = false;
    };

    using InstructionAnnotations = QFlatMap<int, InstructionAnnotation>;

    struct Function
    {
        QQmlJSScopesById addressableScopes;
        QList<QQmlJSRegisterContent> argumentTypes;
        QList<QQmlJSRegisterContent> registerTypes;
        QQmlJSScope::ConstPtr returnType;
        QQmlJSScope::ConstPtr qmlScope;
        QByteArray code;
        const SourceLocationTable *sourceLocations = nullptr;
        bool isSignalHandler = false;
        bool isQPropertyBinding = false;
        bool isProperty = false;
    };

    struct State
    {
        VirtualRegisters registers;

        const QQmlJSRegisterContent &accumulatorIn() const
        {
            auto it = registers.find(Accumulator);
            Q_ASSERT(it != registers.end());
            return it.value().content;
        };

        const QQmlJSRegisterContent &accumulatorOut() const
        {
            Q_ASSERT(m_changedRegisterIndex == Accumulator);
            return m_changedRegister;
        };

        void setRegister(int registerIndex, QQmlJSRegisterContent content)
        {
            m_changedRegister = std::move(content);
            m_changedRegisterIndex = registerIndex;
        }

        void clearChangedRegister()
        {
            m_changedRegisterIndex = InvalidRegister;
            m_changedRegister = QQmlJSRegisterContent();
        }

        int changedRegisterIndex() const { return m_changedRegisterIndex; }
        const QQmlJSRegisterContent &changedRegister() const { return m_changedRegister; }

        void addReadRegister(int registerIndex, const QQmlJSRegisterContent &reg)
        {
            Q_ASSERT(isRename() || reg.isConversion());
            const VirtualRegister &source = registers[registerIndex];
            VirtualRegister &target = m_readRegisters[registerIndex];
            target.content = reg;
            target.canMove = source.canMove;
            target.affectedBySideEffects = source.affectedBySideEffects;
        }

        void addReadAccumulator(const QQmlJSRegisterContent &reg)
        {
            addReadRegister(Accumulator, reg);
        }

        VirtualRegisters takeReadRegisters() const { return std::move(m_readRegisters); }
        void setReadRegisters(VirtualRegisters readReagisters)
        {
            m_readRegisters = std::move(readReagisters);
        }

        QQmlJSRegisterContent readRegister(int registerIndex) const
        {
            Q_ASSERT(m_readRegisters.contains(registerIndex));
            return m_readRegisters[registerIndex].content;
        }

        bool canMoveReadRegister(int registerIndex) const
        {
            auto it = m_readRegisters.find(registerIndex);
            return it != m_readRegisters.end() && it->second.canMove;
        }

        bool isRegisterAffectedBySideEffects(int registerIndex) const
        {
            auto it = m_readRegisters.find(registerIndex);
            return it != m_readRegisters.end() && it->second.affectedBySideEffects;
        }

        QQmlJSRegisterContent readAccumulator() const
        {
            return readRegister(Accumulator);
        }

        bool readsRegister(int registerIndex) const
        {
            return m_readRegisters.contains(registerIndex);
        }

        bool hasSideEffects() const { return m_hasSideEffects; }
        void setHasSideEffects(bool hasSideEffects) {
            m_hasSideEffects = hasSideEffects;
            if (!hasSideEffects)
                return;

            for (auto it = registers.begin(), end = registers.end(); it != end; ++it)
                it.value().affectedBySideEffects = true;
        }

        bool isRename() const { return m_isRename; }
        void setIsRename(bool isRename) { m_isRename = isRename; }

        int renameSourceRegisterIndex() const
        {
            Q_ASSERT(m_isRename);
            Q_ASSERT(m_readRegisters.size() == 1);
            return m_readRegisters.begin().key();
        }

    private:
        VirtualRegisters m_readRegisters;
        QQmlJSRegisterContent m_changedRegister;
        int m_changedRegisterIndex = InvalidRegister;
        bool m_hasSideEffects = false;
        bool m_isRename = false;
    };

    QQmlJSCompilePass(const QV4::Compiler::JSUnitGenerator *jsUnitGenerator,
                      const QQmlJSTypeResolver *typeResolver, QQmlJSLogger *logger)
        : m_jsUnitGenerator(jsUnitGenerator)
        , m_typeResolver(typeResolver)
        , m_logger(logger)
    {}

protected:
    const QV4::Compiler::JSUnitGenerator *m_jsUnitGenerator = nullptr;
    const QQmlJSTypeResolver *m_typeResolver = nullptr;
    QQmlJSLogger *m_logger = nullptr;

    const Function *m_function = nullptr;
    QQmlJS::DiagnosticMessage *m_error = nullptr;

    int firstRegisterIndex() const
    {
        return FirstArgument + m_function->argumentTypes.size();
    }

    bool isArgument(int registerIndex) const
    {
        return registerIndex >= FirstArgument && registerIndex < firstRegisterIndex();
    }

    QQmlJSRegisterContent argumentType(int registerIndex) const
    {
        Q_ASSERT(isArgument(registerIndex));
        return m_function->argumentTypes[registerIndex - FirstArgument];
    }


    State initialState(const Function *function)
    {
        State state;
        for (int i = 0, end = function->argumentTypes.size(); i < end; ++i) {
            state.registers[FirstArgument + i].content = function->argumentTypes.at(i);
            Q_ASSERT(state.registers[FirstArgument + i].content.isValid());
        }
        for (int i = 0, end = function->registerTypes.size(); i != end; ++i)
            state.registers[firstRegisterIndex() + i].content = function->registerTypes[i];
        return state;
    }

    State nextStateFromAnnotations(
            const State &oldState, const InstructionAnnotations &annotations)
    {
        State newState;

        const auto instruction = annotations.find(currentInstructionOffset());
        newState.registers = oldState.registers;

        // Usually the initial accumulator type is the output of the previous instruction, but ...
        if (oldState.changedRegisterIndex() != InvalidRegister) {
            newState.registers[oldState.changedRegisterIndex()].affectedBySideEffects = false;
            newState.registers[oldState.changedRegisterIndex()].content
                    = oldState.changedRegister();
        }

        if (instruction == annotations.constEnd())
            return newState;

        newState.setHasSideEffects(instruction->second.hasSideEffects);
        newState.setReadRegisters(instruction->second.readRegisters);
        newState.setIsRename(instruction->second.isRename);

        for (auto it = instruction->second.typeConversions.begin(),
             end = instruction->second.typeConversions.end(); it != end; ++it) {
            Q_ASSERT(it.key() != InvalidRegister);
            newState.registers[it.key()] = it.value();
        }

        if (instruction->second.changedRegisterIndex != InvalidRegister) {
            newState.setRegister(instruction->second.changedRegisterIndex,
                                 instruction->second.changedRegister);
        }

        return newState;
    }

    QQmlJS::SourceLocation sourceLocation(int instructionOffset) const
    {
        Q_ASSERT(m_function);
        Q_ASSERT(m_function->sourceLocations);
        const auto &entries = m_function->sourceLocations->entries;
        auto item = std::lower_bound(entries.begin(), entries.end(), instructionOffset,
                                     [](auto entry, uint offset) { return entry.offset < offset; });

        Q_ASSERT(item != entries.end());
        return item->location;
    }

    QQmlJS::SourceLocation currentSourceLocation() const
    {
        return sourceLocation(currentInstructionOffset());
    }

    void setError(const QString &message, int instructionOffset)
    {
        Q_ASSERT(m_error);
        if (m_error->isValid())
            return;
        m_error->message = message;
        m_error->loc = sourceLocation(instructionOffset);
    }

    void setError(const QString &message)
    {
        setError(message, currentInstructionOffset());
    }

    static bool instructionManipulatesContext(QV4::Moth::Instr::Type type)
    {
        using Type = QV4::Moth::Instr::Type;
        switch (type) {
        case Type::PopContext:
        case Type::PopScriptContext:
        case Type::CreateCallContext:
        case Type::CreateCallContext_Wide:
        case Type::PushCatchContext:
        case Type::PushCatchContext_Wide:
        case Type::PushWithContext:
        case Type::PushWithContext_Wide:
        case Type::PushBlockContext:
        case Type::PushBlockContext_Wide:
        case Type::CloneBlockContext:
        case Type::CloneBlockContext_Wide:
        case Type::PushScriptContext:
        case Type::PushScriptContext_Wide:
            return true;
        default:
            break;
        }
        return false;
    }

    // Stub out all the methods so that passes can choose to only implement part of them.
    void generate_Add(int) override {}
    void generate_As(int) override {}
    void generate_BitAnd(int) override {}
    void generate_BitAndConst(int) override {}
    void generate_BitOr(int) override {}
    void generate_BitOrConst(int) override {}
    void generate_BitXor(int) override {}
    void generate_BitXorConst(int) override {}
    void generate_CallGlobalLookup(int, int, int) override {}
    void generate_CallName(int, int, int) override {}
    void generate_CallPossiblyDirectEval(int, int) override {}
    void generate_CallProperty(int, int, int, int) override {}
    void generate_CallPropertyLookup(int, int, int, int) override {}
    void generate_CallQmlContextPropertyLookup(int, int, int) override {}
    void generate_CallValue(int, int, int) override {}
    void generate_CallWithReceiver(int, int, int, int) override {}
    void generate_CallWithSpread(int, int, int, int) override {}
    void generate_CheckException() override {}
    void generate_CloneBlockContext() override {}
    void generate_CmpEq(int) override {}
    void generate_CmpEqInt(int) override {}
    void generate_CmpEqNull() override {}
    void generate_CmpGe(int) override {}
    void generate_CmpGt(int) override {}
    void generate_CmpIn(int) override {}
    void generate_CmpInstanceOf(int) override {}
    void generate_CmpLe(int) override {}
    void generate_CmpLt(int) override {}
    void generate_CmpNe(int) override {}
    void generate_CmpNeInt(int) override {}
    void generate_CmpNeNull() override {}
    void generate_CmpStrictEqual(int) override {}
    void generate_CmpStrictNotEqual(int) override {}
    void generate_Construct(int, int, int) override {}
    void generate_ConstructWithSpread(int, int, int) override {}
    void generate_ConvertThisToObject() override {}
    void generate_CreateCallContext() override {}
    void generate_CreateClass(int, int, int) override {}
    void generate_CreateMappedArgumentsObject() override {}
    void generate_CreateRestParameter(int) override {}
    void generate_CreateUnmappedArgumentsObject() override {}
    void generate_DeadTemporalZoneCheck(int) override {}
    void generate_Debug() override {}
    void generate_DeclareVar(int, int) override {}
    void generate_Decrement() override {}
    void generate_DefineArray(int, int) override {}
    void generate_DefineObjectLiteral(int, int, int) override {}
    void generate_DeleteName(int) override {}
    void generate_DeleteProperty(int, int) override {}
    void generate_DestructureRestElement() override {}
    void generate_Div(int) override {}
    void generate_Exp(int) override {}
    void generate_GetException() override {}
    void generate_GetIterator(int) override {}
    void generate_GetLookup(int) override {}
    void generate_GetOptionalLookup(int, int) override {}
    void generate_GetTemplateObject(int) override {}
    void generate_Increment() override {}
    void generate_InitializeBlockDeadTemporalZone(int, int) override {}
    void generate_IteratorClose(int) override {}
    void generate_IteratorNext(int, int) override {}
    void generate_IteratorNextForYieldStar(int, int) override {}
    void generate_Jump(int) override {}
    void generate_JumpFalse(int) override {}
    void generate_JumpNoException(int) override {}
    void generate_JumpNotUndefined(int) override {}
    void generate_JumpTrue(int) override {}
    void generate_LoadClosure(int) override {}
    void generate_LoadConst(int) override {}
    void generate_LoadElement(int) override {}
    void generate_LoadFalse() override {}
    void generate_LoadGlobalLookup(int) override {}
    void generate_LoadImport(int) override {}
    void generate_LoadInt(int) override {}
    void generate_LoadLocal(int) override {}
    void generate_LoadName(int) override {}
    void generate_LoadNull() override {}
    void generate_LoadOptionalProperty(int, int) override {}
    void generate_LoadProperty(int) override {}
    void generate_LoadQmlContextPropertyLookup(int) override {}
    void generate_LoadReg(int) override {}
    void generate_LoadRuntimeString(int) override {}
    void generate_LoadScopedLocal(int, int) override {}
    void generate_LoadSuperConstructor() override {}
    void generate_LoadSuperProperty(int) override {}
    void generate_LoadTrue() override {}
    void generate_LoadUndefined() override {}
    void generate_LoadZero() override {}
    void generate_Mod(int) override {}
    void generate_MoveConst(int, int) override {}
    void generate_MoveReg(int, int) override {}
    void generate_MoveRegExp(int, int) override {}
    void generate_Mul(int) override {}
    void generate_PopContext() override {}
    void generate_PopScriptContext() override {}
    void generate_PushBlockContext(int) override {}
    void generate_PushCatchContext(int, int) override {}
    void generate_PushScriptContext(int) override {}
    void generate_PushWithContext() override {}
    void generate_Resume(int) override {}
    void generate_Ret() override {}
    void generate_SetException() override {}
    void generate_SetLookup(int, int) override {}
    void generate_SetUnwindHandler(int) override {}
    void generate_Shl(int) override {}
    void generate_ShlConst(int) override {}
    void generate_Shr(int) override {}
    void generate_ShrConst(int) override {}
    void generate_StoreElement(int, int) override {}
    void generate_StoreLocal(int) override {}
    void generate_StoreNameSloppy(int) override {}
    void generate_StoreNameStrict(int) override {}
    void generate_StoreProperty(int, int) override {}
    void generate_StoreReg(int) override {}
    void generate_StoreScopedLocal(int, int) override {}
    void generate_StoreSuperProperty(int) override {}
    void generate_Sub(int) override {}
    void generate_TailCall(int, int, int, int) override {}
    void generate_ThrowException() override {}
    void generate_ThrowOnNullOrUndefined() override {}
    void generate_ToObject() override {}
    void generate_TypeofName(int) override {}
    void generate_TypeofValue() override {}
    void generate_UCompl() override {}
    void generate_UMinus() override {}
    void generate_UNot() override {}
    void generate_UPlus() override {}
    void generate_UShr(int) override {}
    void generate_UShrConst(int) override {}
    void generate_UnwindDispatch() override {}
    void generate_UnwindToLabel(int, int) override {}
    void generate_Yield() override {}
    void generate_YieldStar() override {}
};

QT_END_NAMESPACE

#endif // QQMLJSCOMPILEPASS_P_H
