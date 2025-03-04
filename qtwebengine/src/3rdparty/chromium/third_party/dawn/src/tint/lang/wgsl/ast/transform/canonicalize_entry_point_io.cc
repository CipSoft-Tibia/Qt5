// Copyright 2021 The Tint Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/tint/lang/wgsl/ast/transform/canonicalize_entry_point_io.h"

#include <algorithm>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "src/tint/lang/core/builtin_value.h"
#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/wgsl/ast/disable_validation_attribute.h"
#include "src/tint/lang/wgsl/ast/transform/unshadow.h"
#include "src/tint/lang/wgsl/program/clone_context.h"
#include "src/tint/lang/wgsl/program/program_builder.h"
#include "src/tint/lang/wgsl/resolver/resolve.h"
#include "src/tint/lang/wgsl/sem/function.h"

using namespace tint::core::number_suffixes;  // NOLINT
using namespace tint::core::fluent_types;     // NOLINT

TINT_INSTANTIATE_TYPEINFO(tint::ast::transform::CanonicalizeEntryPointIO);
TINT_INSTANTIATE_TYPEINFO(tint::ast::transform::CanonicalizeEntryPointIO::Config);
TINT_INSTANTIATE_TYPEINFO(tint::ast::transform::CanonicalizeEntryPointIO::HLSLWaveIntrinsic);

namespace tint::ast::transform {

CanonicalizeEntryPointIO::CanonicalizeEntryPointIO() = default;
CanonicalizeEntryPointIO::~CanonicalizeEntryPointIO() = default;

namespace {

/// Info for a struct member
struct MemberInfo {
    /// The struct member item
    const StructMember* member;
    /// The struct member location if provided
    std::optional<uint32_t> location;
    /// The struct member index if provided
    std::optional<uint32_t> index;
};

/// FXC is sensitive to field order in structures, this is used by StructMemberComparator to ensure
/// that FXC is happy with the order of emitted fields.
uint32_t BuiltinOrder(core::BuiltinValue builtin) {
    switch (builtin) {
        case core::BuiltinValue::kPosition:
            return 1;
        case core::BuiltinValue::kVertexIndex:
            return 2;
        case core::BuiltinValue::kInstanceIndex:
            return 3;
        case core::BuiltinValue::kFrontFacing:
            return 4;
        case core::BuiltinValue::kFragDepth:
            return 5;
        case core::BuiltinValue::kLocalInvocationId:
            return 6;
        case core::BuiltinValue::kLocalInvocationIndex:
            return 7;
        case core::BuiltinValue::kGlobalInvocationId:
            return 8;
        case core::BuiltinValue::kWorkgroupId:
            return 9;
        case core::BuiltinValue::kNumWorkgroups:
            return 10;
        case core::BuiltinValue::kSampleIndex:
            return 11;
        case core::BuiltinValue::kSampleMask:
            return 12;
        case core::BuiltinValue::kPointSize:
            return 13;
        default:
            break;
    }
    return 0;
}

// Returns true if `attr` is a shader IO attribute.
bool IsShaderIOAttribute(const Attribute* attr) {
    return attr->IsAnyOf<BuiltinAttribute, InterpolateAttribute, InvariantAttribute,
                         LocationAttribute, IndexAttribute>();
}

}  // namespace

/// PIMPL state for the transform
struct CanonicalizeEntryPointIO::State {
    /// OutputValue represents a shader result that the wrapper function produces.
    struct OutputValue {
        /// The name of the output value.
        std::string name;
        /// The type of the output value.
        Type type;
        /// The shader IO attributes.
        tint::Vector<const Attribute*, 8> attributes;
        /// The value itself.
        const Expression* value;
        /// The output location.
        std::optional<uint32_t> location;
        /// The output index.
        std::optional<uint32_t> index;
    };

    /// The clone context.
    program::CloneContext& ctx;
    /// The program builder
    ProgramBuilder& b;
    /// The transform config.
    CanonicalizeEntryPointIO::Config const cfg;
    /// The entry point function (AST).
    const Function* func_ast;
    /// The entry point function (SEM).
    const sem::Function* func_sem;

    /// The new entry point wrapper function's parameters.
    tint::Vector<const Parameter*, 8> wrapper_ep_parameters;

    /// The members of the wrapper function's struct parameter.
    tint::Vector<MemberInfo, 8> wrapper_struct_param_members;
    /// The name of the wrapper function's struct parameter.
    Symbol wrapper_struct_param_name;
    /// The parameters that will be passed to the original function.
    tint::Vector<const Expression*, 8> inner_call_parameters;
    /// The members of the wrapper function's struct return type.
    tint::Vector<MemberInfo, 8> wrapper_struct_output_members;
    /// The wrapper function output values.
    tint::Vector<OutputValue, 8> wrapper_output_values;
    /// The body of the wrapper function.
    tint::Vector<const Statement*, 8> wrapper_body;
    /// Input names used by the entrypoint
    std::unordered_set<std::string> input_names;
    /// A map of cloned attribute to builtin value
    Hashmap<const BuiltinAttribute*, core::BuiltinValue, 16> builtin_attrs;
    /// A map of builtin values to HLSL wave intrinsic functions.
    Hashmap<core::BuiltinValue, Symbol, 2> wave_intrinsics;

    /// Constructor
    /// @param context the clone context
    /// @param builder the program builder
    /// @param config the transform config
    /// @param function the entry point function
    State(program::CloneContext& context,
          ProgramBuilder& builder,
          const CanonicalizeEntryPointIO::Config& config,
          const Function* function)
        : ctx(context),
          b(builder),
          cfg(config),
          func_ast(function),
          func_sem(ctx.src->Sem().Get(function)) {}

    /// Clones the attributes from @p in and adds it to @p out. If @p in is a builtin attribute,
    /// then builtin_attrs is updated with the builtin information.
    /// @param in the attribute to clone
    /// @param out the output Attributes
    template <size_t N>
    void CloneAttribute(const Attribute* in, tint::Vector<const Attribute*, N>& out) {
        auto* cloned = ctx.Clone(in);
        out.Push(cloned);
        if (auto* builtin = in->As<BuiltinAttribute>()) {
            builtin_attrs.Add(cloned->As<BuiltinAttribute>(), ctx.src->Sem().Get(builtin)->Value());
        }
    }

    /// Clones the shader IO attributes from @p in.
    /// @param in the attributes to clone
    /// @param do_interpolate whether to clone InterpolateAttribute
    /// @return the cloned attributes
    template <size_t N>
    auto CloneShaderIOAttributes(const tint::Vector<const Attribute*, N> in, bool do_interpolate) {
        tint::Vector<const Attribute*, N> out;
        for (auto* attr : in) {
            if (IsShaderIOAttribute(attr) &&
                (do_interpolate || !attr->template Is<InterpolateAttribute>())) {
                CloneAttribute(attr, out);
            }
        }
        return out;
    }

    /// @param attr the input attribute
    /// @returns the builtin value of the attribute
    core::BuiltinValue BuiltinOf(const BuiltinAttribute* attr) {
        if (attr->generation_id == b.ID()) {
            // attr belongs to the target program.
            // Obtain the builtin value from #builtin_attrs.
            if (auto builtin = builtin_attrs.Get(attr)) {
                return *builtin;
            }
        } else {
            // attr belongs to the source program.
            // Obtain the builtin value from the semantic info.
            return ctx.src->Sem().Get(attr)->Value();
        }
        TINT_ICE() << "could not obtain builtin value from attribute";
        return core::BuiltinValue::kUndefined;
    }

    /// @param attrs the input attribute list
    /// @returns the builtin value if any of the attributes in @p attrs is a builtin attribute,
    /// otherwise core::BuiltinValue::kUndefined
    core::BuiltinValue BuiltinOf(VectorRef<const Attribute*> attrs) {
        if (auto* builtin = GetAttribute<BuiltinAttribute>(attrs)) {
            return BuiltinOf(builtin);
        }
        return core::BuiltinValue::kUndefined;
    }

    /// Create or return a symbol for the wrapper function's struct parameter.
    /// @returns the symbol for the struct parameter
    Symbol InputStructSymbol() {
        if (!wrapper_struct_param_name.IsValid()) {
            wrapper_struct_param_name = b.Sym();
        }
        return wrapper_struct_param_name;
    }

    /// Call a wave intrinsic function for the subgroup builtin contained in @p attrs, if any.
    /// @param attrs the attribute list that may contain a subgroup builtin
    /// @returns an expression that calls a HLSL wave intrinsic, or nullptr
    const ast::CallExpression* CallWaveIntrinsic(VectorRef<const Attribute*> attrs) {
        if (cfg.shader_style != ShaderStyle::kHlsl) {
            return nullptr;
        }

        // Helper to make a wave intrinsic.
        auto make_intrinsic = [&](const char* name, HLSLWaveIntrinsic::Op op) {
            auto symbol = b.Symbols().New(name);
            b.Func(symbol, Empty, b.ty.u32(), nullptr,
                   Vector{b.ASTNodes().Create<HLSLWaveIntrinsic>(b.ID(), b.AllocateNodeID(), op),
                          b.Disable(DisabledValidation::kFunctionHasNoBody)});
            return symbol;
        };

        // Get or create the intrinsic function.
        auto builtin = BuiltinOf(attrs);
        auto intrinsic = wave_intrinsics.GetOrCreate(builtin, [&] {
            if (builtin == core::BuiltinValue::kSubgroupInvocationId) {
                return make_intrinsic("__WaveGetLaneIndex",
                                      HLSLWaveIntrinsic::Op::kWaveGetLaneIndex);
            }
            if (builtin == core::BuiltinValue::kSubgroupSize) {
                return make_intrinsic("__WaveGetLaneCount",
                                      HLSLWaveIntrinsic::Op::kWaveGetLaneCount);
            }
            return Symbol();
        });
        if (!intrinsic) {
            return nullptr;
        }

        // Call the intrinsic function.
        return b.Call(intrinsic);
    }

    /// Add a shader input to the entry point.
    /// @param name the name of the shader input
    /// @param type the type of the shader input
    /// @param location the location if provided
    /// @param attrs the attributes to apply to the shader input
    /// @returns an expression which evaluates to the value of the shader input
    const Expression* AddInput(std::string name,
                               const core::type::Type* type,
                               std::optional<uint32_t> location,
                               tint::Vector<const Attribute*, 8> attrs) {
        auto ast_type = CreateASTTypeFor(ctx, type);

        auto builtin_attr = BuiltinOf(attrs);

        if (cfg.shader_style == ShaderStyle::kSpirv || cfg.shader_style == ShaderStyle::kGlsl) {
            // Vulkan requires that integer user-defined fragment inputs are always decorated with
            // `Flat`. See:
            // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/StandaloneSpirv.html#VUID-StandaloneSpirv-Flat-04744
            // TODO(crbug.com/tint/1224): Remove this once a flat interpolation attribute is
            // required for integers.
            if (func_ast->PipelineStage() == PipelineStage::kFragment &&
                type->is_integer_scalar_or_vector() && !HasAttribute<InterpolateAttribute>(attrs) &&
                (HasAttribute<LocationAttribute>(attrs) ||
                 cfg.shader_style == ShaderStyle::kSpirv)) {
                attrs.Push(b.Interpolate(core::InterpolationType::kFlat,
                                         core::InterpolationSampling::kUndefined));
            }

            // Disable validation for use of the `input` address space.
            attrs.Push(b.Disable(DisabledValidation::kIgnoreAddressSpace));

            // In GLSL, if it's a builtin, override the name with the
            // corresponding gl_ builtin name
            if (cfg.shader_style == ShaderStyle::kGlsl &&
                builtin_attr != core::BuiltinValue::kUndefined) {
                name = GLSLBuiltinToString(builtin_attr, func_ast->PipelineStage(),
                                           core::AddressSpace::kIn);
            }
            auto symbol = b.Symbols().New(name);

            // Create the global variable and use its value for the shader input.
            const Expression* value = b.Expr(symbol);

            if (builtin_attr != core::BuiltinValue::kUndefined) {
                if (cfg.shader_style == ShaderStyle::kGlsl) {
                    value = FromGLSLBuiltin(builtin_attr, value, ast_type);
                } else if (builtin_attr == core::BuiltinValue::kSampleMask) {
                    // Vulkan requires the type of a SampleMask builtin to be an array.
                    // Declare it as array<u32, 1> and then load the first element.
                    ast_type = b.ty.array(ast_type, 1_u);
                    value = b.IndexAccessor(value, 0_i);
                }
            }
            b.GlobalVar(symbol, ast_type, core::AddressSpace::kIn, std::move(attrs));
            return value;
        } else if (cfg.shader_style == ShaderStyle::kMsl &&
                   builtin_attr != core::BuiltinValue::kUndefined) {
            // If this input is a builtin and we are targeting MSL, then add it to the
            // parameter list and pass it directly to the inner function.
            Symbol symbol = input_names.emplace(name).second ? b.Symbols().Register(name)
                                                             : b.Symbols().New(name);
            wrapper_ep_parameters.Push(b.Param(symbol, ast_type, std::move(attrs)));
            return b.Expr(symbol);
        } else {
            // Otherwise, move it to the new structure member list.
            Symbol symbol = input_names.emplace(name).second ? b.Symbols().Register(name)
                                                             : b.Symbols().New(name);
            wrapper_struct_param_members.Push(
                {b.Member(symbol, ast_type, std::move(attrs)), location, std::nullopt});
            return b.MemberAccessor(InputStructSymbol(), symbol);
        }
    }

    /// Add a shader output to the entry point.
    /// @param name the name of the shader output
    /// @param type the type of the shader output
    /// @param location the location if provided
    /// @param index the index if provided
    /// @param attrs the attributes to apply to the shader output
    /// @param value the value of the shader output
    void AddOutput(std::string name,
                   const core::type::Type* type,
                   std::optional<uint32_t> location,
                   std::optional<uint32_t> index,
                   tint::Vector<const Attribute*, 8> attrs,
                   const Expression* value) {
        auto builtin_attr = BuiltinOf(attrs);
        // Vulkan requires that integer user-defined vertex outputs are always decorated with
        // `Flat`.
        // TODO(crbug.com/tint/1224): Remove this once a flat interpolation attribute is required
        // for integers.
        if (cfg.shader_style == ShaderStyle::kSpirv &&
            func_ast->PipelineStage() == PipelineStage::kVertex &&
            type->is_integer_scalar_or_vector() && HasAttribute<LocationAttribute>(attrs) &&
            !HasAttribute<InterpolateAttribute>(attrs)) {
            attrs.Push(b.Interpolate(core::InterpolationType::kFlat,
                                     core::InterpolationSampling::kUndefined));
        }

        // In GLSL, if it's a builtin, override the name with the
        // corresponding gl_ builtin name
        if (cfg.shader_style == ShaderStyle::kGlsl) {
            if (builtin_attr != core::BuiltinValue::kUndefined) {
                name = GLSLBuiltinToString(builtin_attr, func_ast->PipelineStage(),
                                           core::AddressSpace::kOut);
                value = ToGLSLBuiltin(builtin_attr, value, type);
            }
        }

        OutputValue output;
        output.name = name;
        output.type = CreateASTTypeFor(ctx, type);
        output.attributes = std::move(attrs);
        output.value = value;
        output.location = location;
        output.index = index;
        wrapper_output_values.Push(output);
    }

    /// Process a non-struct parameter.
    /// This creates a new object for the shader input, moving the shader IO
    /// attributes to it. It also adds an expression to the list of parameters
    /// that will be passed to the original function.
    /// @param param the original function parameter
    void ProcessNonStructParameter(const sem::Parameter* param) {
        if (auto* wave_intrinsic_call = CallWaveIntrinsic(param->Declaration()->attributes)) {
            inner_call_parameters.Push(wave_intrinsic_call);
            for (auto* attr : param->Declaration()->attributes) {
                ctx.Remove(param->Declaration()->attributes, attr);
            }
            return;
        }

        // Do not add interpolation attributes on vertex input
        bool do_interpolate = func_ast->PipelineStage() != PipelineStage::kVertex;
        // Remove the shader IO attributes from the inner function parameter, and attach them to the
        // new object instead.
        tint::Vector<const Attribute*, 8> attributes;
        for (auto* attr : param->Declaration()->attributes) {
            if (IsShaderIOAttribute(attr)) {
                ctx.Remove(param->Declaration()->attributes, attr);
                if ((do_interpolate || !attr->Is<InterpolateAttribute>())) {
                    CloneAttribute(attr, attributes);
                }
            }
        }

        auto name = param->Declaration()->name->symbol.Name();
        auto* input_expr = AddInput(name, param->Type(), param->Location(), std::move(attributes));
        inner_call_parameters.Push(input_expr);
    }

    /// Process a struct parameter.
    /// This creates new objects for each struct member, moving the shader IO
    /// attributes to them. It also creates the structure that will be passed to
    /// the original function.
    /// @param param the original function parameter
    void ProcessStructParameter(const sem::Parameter* param) {
        // Do not add interpolation attributes on vertex input
        bool do_interpolate = func_ast->PipelineStage() != PipelineStage::kVertex;

        auto* str = param->Type()->As<sem::Struct>();

        // Recreate struct members in the outer entry point and build an initializer
        // list to pass them through to the inner function.
        tint::Vector<const Expression*, 8> inner_struct_values;
        for (auto* member : str->Members()) {
            if (TINT_UNLIKELY(member->Type()->Is<core::type::Struct>())) {
                TINT_ICE() << "nested IO struct";
                continue;
            }

            if (auto* wave_intrinsic_call = CallWaveIntrinsic(member->Declaration()->attributes)) {
                inner_struct_values.Push(wave_intrinsic_call);
                continue;
            }

            auto name = member->Name().Name();

            auto attributes =
                CloneShaderIOAttributes(member->Declaration()->attributes, do_interpolate);
            auto* input_expr = AddInput(name, member->Type(), member->Attributes().location,
                                        std::move(attributes));
            inner_struct_values.Push(input_expr);
        }

        // Construct the original structure using the new shader input objects.
        inner_call_parameters.Push(
            b.Call(ctx.Clone(param->Declaration()->type), inner_struct_values));
    }

    /// Process the entry point return type.
    /// This generates a list of output values that are returned by the original
    /// function.
    /// @param inner_ret_type the original function return type
    /// @param original_result the result object produced by the original function
    void ProcessReturnType(const core::type::Type* inner_ret_type, Symbol original_result) {
        // Do not add interpolation attributes on fragment output
        bool do_interpolate = func_ast->PipelineStage() != PipelineStage::kFragment;
        if (auto* str = inner_ret_type->As<sem::Struct>()) {
            for (auto* member : str->Members()) {
                if (TINT_UNLIKELY(member->Type()->Is<core::type::Struct>())) {
                    TINT_ICE() << "nested IO struct";
                    continue;
                }

                auto name = member->Name().Name();
                auto attributes =
                    CloneShaderIOAttributes(member->Declaration()->attributes, do_interpolate);

                // Extract the original structure member.
                AddOutput(name, member->Type(), member->Attributes().location,
                          member->Attributes().index, std::move(attributes),
                          b.MemberAccessor(original_result, name));
            }
        } else if (!inner_ret_type->Is<core::type::Void>()) {
            auto attributes =
                CloneShaderIOAttributes(func_ast->return_type_attributes, do_interpolate);

            // Propagate the non-struct return value as is.
            AddOutput("value", func_sem->ReturnType(), func_sem->ReturnLocation(),
                      func_sem->ReturnIndex(), std::move(attributes), b.Expr(original_result));
        }
    }

    /// Add a fixed sample mask to the wrapper function output.
    /// If there is already a sample mask, bitwise-and it with the fixed mask.
    /// Otherwise, create a new output value from the fixed mask.
    void AddFixedSampleMask() {
        // Check the existing output values for a sample mask builtin.
        for (auto& outval : wrapper_output_values) {
            if (BuiltinOf(outval.attributes) == core::BuiltinValue::kSampleMask) {
                // Combine the authored sample mask with the fixed mask.
                outval.value = b.And(outval.value, u32(cfg.fixed_sample_mask));
                return;
            }
        }

        // No existing sample mask builtin was found, so create a new output value using the fixed
        // sample mask.
        auto* builtin = b.Builtin(core::BuiltinValue::kSampleMask);
        builtin_attrs.Add(builtin, core::BuiltinValue::kSampleMask);
        AddOutput("fixed_sample_mask", b.create<core::type::U32>(), std::nullopt, std::nullopt,
                  {builtin}, b.Expr(u32(cfg.fixed_sample_mask)));
    }

    /// Add a point size builtin to the wrapper function output.
    void AddVertexPointSize() {
        // Create a new output value and assign it a literal 1.0 value.
        auto* builtin = b.Builtin(core::BuiltinValue::kPointSize);
        builtin_attrs.Add(builtin, core::BuiltinValue::kPointSize);
        AddOutput("vertex_point_size", b.create<core::type::F32>(), std::nullopt, std::nullopt,
                  {builtin}, b.Expr(1_f));
    }

    /// Create an expression for gl_Position.[component]
    /// @param component the component of gl_Position to access
    /// @returns the new expression
    const Expression* GLPosition(const char* component) {
        Symbol pos = b.Symbols().Register("gl_Position");
        Symbol c = b.Symbols().Register(component);
        return b.MemberAccessor(b.Expr(pos), c);
    }

    /// Comparison function used to reorder struct members such that all members with
    /// location attributes appear first (ordered by location slot), followed by
    /// those with builtin attributes.
    /// @param x a struct member
    /// @param y another struct member
    /// @returns true if a comes before b
    bool StructMemberComparator(const MemberInfo& x, const MemberInfo& y) {
        auto* x_loc = GetAttribute<LocationAttribute>(x.member->attributes);
        auto* y_loc = GetAttribute<LocationAttribute>(y.member->attributes);
        auto* x_blt = GetAttribute<BuiltinAttribute>(x.member->attributes);
        auto* y_blt = GetAttribute<BuiltinAttribute>(y.member->attributes);
        if (x_loc) {
            if (!y_loc) {
                // `a` has location attribute and `b` does not: `a` goes first.
                return true;
            }
            // Both have location attributes: smallest goes first.
            return x.location < y.location;
        } else {
            if (y_loc) {
                // `b` has location attribute and `a` does not: `b` goes first.
                return false;
            }
            // Both are builtins: order matters for FXC.
            auto builtin_a = BuiltinOf(x_blt);
            auto builtin_b = BuiltinOf(y_blt);
            return BuiltinOrder(builtin_a) < BuiltinOrder(builtin_b);
        }
    }
    /// Create the wrapper function's struct parameter and type objects.
    void CreateInputStruct() {
        // Sort the struct members to satisfy HLSL interfacing matching rules.
        std::sort(wrapper_struct_param_members.begin(), wrapper_struct_param_members.end(),
                  [&](auto& x, auto& y) { return StructMemberComparator(x, y); });

        tint::Vector<const StructMember*, 8> members;
        for (auto& mem : wrapper_struct_param_members) {
            members.Push(mem.member);
        }

        // Create the new struct type.
        auto struct_name = b.Sym();
        auto* in_struct = b.create<Struct>(b.Ident(struct_name), std::move(members), Empty);
        ctx.InsertBefore(ctx.src->AST().GlobalDeclarations(), func_ast, in_struct);

        // Create a new function parameter using this struct type.
        auto* param = b.Param(InputStructSymbol(), b.ty(struct_name));
        wrapper_ep_parameters.Push(param);
    }

    /// Create and return the wrapper function's struct result object.
    /// @returns the struct type
    Struct* CreateOutputStruct() {
        tint::Vector<const Statement*, 8> assignments;

        auto wrapper_result = b.Symbols().New("wrapper_result");

        // Create the struct members and their corresponding assignment statements.
        std::unordered_set<std::string> member_names;
        for (auto& outval : wrapper_output_values) {
            // Use the original output name, unless that is already taken.
            Symbol name;
            if (member_names.count(outval.name)) {
                name = b.Symbols().New(outval.name);
            } else {
                name = b.Symbols().Register(outval.name);
            }
            member_names.insert(name.Name());

            wrapper_struct_output_members.Push(
                {b.Member(name, outval.type, std::move(outval.attributes)), outval.location,
                 std::nullopt});
            assignments.Push(b.Assign(b.MemberAccessor(wrapper_result, name), outval.value));
        }

        // Sort the struct members to satisfy HLSL interfacing matching rules.
        std::sort(wrapper_struct_output_members.begin(), wrapper_struct_output_members.end(),
                  [&](auto& x, auto& y) { return StructMemberComparator(x, y); });

        tint::Vector<const StructMember*, 8> members;
        for (auto& mem : wrapper_struct_output_members) {
            members.Push(mem.member);
        }

        // Create the new struct type.
        auto* out_struct = b.create<Struct>(b.Ident(b.Sym()), std::move(members), Empty);
        ctx.InsertBefore(ctx.src->AST().GlobalDeclarations(), func_ast, out_struct);

        // Create the output struct object, assign its members, and return it.
        auto* result_object = b.Var(wrapper_result, b.ty(out_struct->name->symbol));
        wrapper_body.Push(b.Decl(result_object));
        for (auto* assignment : assignments) {
            wrapper_body.Push(assignment);
        }
        wrapper_body.Push(b.Return(wrapper_result));

        return out_struct;
    }

    /// Create and assign the wrapper function's output variables.
    void CreateGlobalOutputVariables() {
        for (auto& outval : wrapper_output_values) {
            // Disable validation for use of the `output` address space.
            auto attributes = std::move(outval.attributes);
            attributes.Push(b.Disable(DisabledValidation::kIgnoreAddressSpace));

            // Create the global variable and assign it the output value.
            auto name = b.Symbols().New(outval.name);
            Type type = outval.type;
            const Expression* lhs = b.Expr(name);
            if (BuiltinOf(attributes) == core::BuiltinValue::kSampleMask) {
                // Vulkan requires the type of a SampleMask builtin to be an array.
                // Declare it as array<u32, 1> and then store to the first element.
                type = b.ty.array(type, 1_u);
                lhs = b.IndexAccessor(lhs, 0_i);
            }
            b.GlobalVar(name, type, core::AddressSpace::kOut, std::move(attributes));
            wrapper_body.Push(b.Assign(lhs, outval.value));
        }
    }

    // Recreate the original function without entry point attributes and call it.
    /// @returns the inner function call expression
    const CallExpression* CallInnerFunction() {
        Symbol inner_name;
        if (cfg.shader_style == ShaderStyle::kGlsl) {
            // In GLSL, clone the original entry point name, as the wrapper will be
            // called "main".
            inner_name = ctx.Clone(func_ast->name->symbol);
        } else {
            // Add a suffix to the function name, as the wrapper function will take
            // the original entry point name.
            auto ep_name = func_ast->name->symbol.Name();
            inner_name = b.Symbols().New(ep_name + "_inner");
        }

        // Clone everything, dropping the function and return type attributes.
        // The parameter attributes will have already been stripped during
        // processing.
        auto* inner_function = b.create<Function>(b.Ident(inner_name), ctx.Clone(func_ast->params),
                                                  ctx.Clone(func_ast->return_type),
                                                  ctx.Clone(func_ast->body), Empty, Empty);
        ctx.Replace(func_ast, inner_function);

        // Call the function.
        return b.Call(inner_function->name->symbol, inner_call_parameters);
    }

    /// Process the entry point function.
    void Process() {
        bool needs_fixed_sample_mask = false;
        bool needs_vertex_point_size = false;
        if (func_ast->PipelineStage() == PipelineStage::kFragment &&
            cfg.fixed_sample_mask != 0xFFFFFFFF) {
            needs_fixed_sample_mask = true;
        }
        if (func_ast->PipelineStage() == PipelineStage::kVertex && cfg.emit_vertex_point_size) {
            needs_vertex_point_size = true;
        }

        // Exit early if there is no shader IO to handle.
        if (func_sem->Parameters().Length() == 0 &&
            func_sem->ReturnType()->Is<core::type::Void>() && !needs_fixed_sample_mask &&
            !needs_vertex_point_size && cfg.shader_style != ShaderStyle::kGlsl) {
            return;
        }

        // Process the entry point parameters, collecting those that need to be
        // aggregated into a single structure.
        if (!func_sem->Parameters().IsEmpty()) {
            for (auto* param : func_sem->Parameters()) {
                if (param->Type()->Is<core::type::Struct>()) {
                    ProcessStructParameter(param);
                } else {
                    ProcessNonStructParameter(param);
                }
            }

            // Create a structure parameter for the outer entry point if necessary.
            if (!wrapper_struct_param_members.IsEmpty()) {
                CreateInputStruct();
            }
        }

        // Recreate the original function and call it.
        auto* call_inner = CallInnerFunction();

        // Process the return type, and start building the wrapper function body.
        std::function<Type()> wrapper_ret_type = [&] { return b.ty.void_(); };
        if (func_sem->ReturnType()->Is<core::type::Void>()) {
            // The function call is just a statement with no result.
            wrapper_body.Push(b.CallStmt(call_inner));
        } else {
            // Capture the result of calling the original function.
            auto* inner_result = b.Let(b.Symbols().New("inner_result"), call_inner);
            wrapper_body.Push(b.Decl(inner_result));

            // Process the original return type to determine the outputs that the
            // outer function needs to produce.
            ProcessReturnType(func_sem->ReturnType(), inner_result->name->symbol);
        }

        // Add a fixed sample mask, if necessary.
        if (needs_fixed_sample_mask) {
            AddFixedSampleMask();
        }

        // Add the pointsize builtin, if necessary.
        if (needs_vertex_point_size) {
            AddVertexPointSize();
        }

        // Produce the entry point outputs, if necessary.
        if (!wrapper_output_values.IsEmpty()) {
            if (cfg.shader_style == ShaderStyle::kSpirv || cfg.shader_style == ShaderStyle::kGlsl) {
                CreateGlobalOutputVariables();
            } else {
                auto* output_struct = CreateOutputStruct();
                wrapper_ret_type = [&, output_struct] { return b.ty(output_struct->name->symbol); };
            }
        }

        if (cfg.shader_style == ShaderStyle::kGlsl &&
            func_ast->PipelineStage() == PipelineStage::kVertex) {
            auto* pos_y = GLPosition("y");
            auto* negate_pos_y =
                b.create<UnaryOpExpression>(core::UnaryOp::kNegation, GLPosition("y"));
            wrapper_body.Push(b.Assign(pos_y, negate_pos_y));

            auto* two_z = b.Mul(b.Expr(2_f), GLPosition("z"));
            auto* fixed_z = b.Sub(two_z, GLPosition("w"));
            wrapper_body.Push(b.Assign(GLPosition("z"), fixed_z));
        }

        // Create the wrapper entry point function.
        // For GLSL, use "main", otherwise take the name of the original
        // entry point function.
        Symbol name;
        if (cfg.shader_style == ShaderStyle::kGlsl) {
            name = b.Symbols().New("main");
        } else {
            name = ctx.Clone(func_ast->name->symbol);
        }

        auto* wrapper_func =
            b.create<Function>(b.Ident(name), wrapper_ep_parameters, b.ty(wrapper_ret_type()),
                               b.Block(wrapper_body), ctx.Clone(func_ast->attributes), Empty);
        ctx.InsertAfter(ctx.src->AST().GlobalDeclarations(), func_ast, wrapper_func);
    }

    /// Retrieve the gl_ string corresponding to a builtin.
    /// @param builtin the builtin
    /// @param stage the current pipeline stage
    /// @param address_space the address space (input or output)
    /// @returns the gl_ string corresponding to that builtin
    const char* GLSLBuiltinToString(core::BuiltinValue builtin,
                                    PipelineStage stage,
                                    core::AddressSpace address_space) {
        switch (builtin) {
            case core::BuiltinValue::kPosition:
                switch (stage) {
                    case PipelineStage::kVertex:
                        return "gl_Position";
                    case PipelineStage::kFragment:
                        return "gl_FragCoord";
                    default:
                        return "";
                }
            case core::BuiltinValue::kVertexIndex:
                return "gl_VertexID";
            case core::BuiltinValue::kInstanceIndex:
                return "gl_InstanceID";
            case core::BuiltinValue::kFrontFacing:
                return "gl_FrontFacing";
            case core::BuiltinValue::kFragDepth:
                return "gl_FragDepth";
            case core::BuiltinValue::kLocalInvocationId:
                return "gl_LocalInvocationID";
            case core::BuiltinValue::kLocalInvocationIndex:
                return "gl_LocalInvocationIndex";
            case core::BuiltinValue::kGlobalInvocationId:
                return "gl_GlobalInvocationID";
            case core::BuiltinValue::kNumWorkgroups:
                return "gl_NumWorkGroups";
            case core::BuiltinValue::kWorkgroupId:
                return "gl_WorkGroupID";
            case core::BuiltinValue::kSampleIndex:
                return "gl_SampleID";
            case core::BuiltinValue::kSampleMask:
                if (address_space == core::AddressSpace::kIn) {
                    return "gl_SampleMaskIn";
                } else {
                    return "gl_SampleMask";
                }
            default:
                return "";
        }
    }

    /// Convert a given GLSL builtin value to the corresponding WGSL value.
    /// @param builtin the builtin variable
    /// @param value the value to convert
    /// @param ast_type (inout) the incoming WGSL and outgoing GLSL types
    /// @returns an expression representing the GLSL builtin converted to what
    /// WGSL expects
    const Expression* FromGLSLBuiltin(core::BuiltinValue builtin,
                                      const Expression* value,
                                      Type& ast_type) {
        switch (builtin) {
            case core::BuiltinValue::kVertexIndex:
            case core::BuiltinValue::kInstanceIndex:
            case core::BuiltinValue::kSampleIndex:
                // GLSL uses i32 for these, so bitcast to u32.
                value = b.Bitcast(ast_type, value);
                ast_type = b.ty.i32();
                break;
            case core::BuiltinValue::kSampleMask:
                // gl_SampleMask is an array of i32. Retrieve the first element and
                // bitcast it to u32.
                value = b.IndexAccessor(value, 0_i);
                value = b.Bitcast(ast_type, value);
                ast_type = b.ty.array(b.ty.i32(), 1_u);
                break;
            default:
                break;
        }
        return value;
    }

    /// Convert a given WGSL value to the type expected when assigning to a
    /// GLSL builtin.
    /// @param builtin the builtin variable
    /// @param value the value to convert
    /// @param type (out) the type to which the value was converted
    /// @returns the converted value which can be assigned to the GLSL builtin
    const Expression* ToGLSLBuiltin(core::BuiltinValue builtin,
                                    const Expression* value,
                                    const core::type::Type*& type) {
        switch (builtin) {
            case core::BuiltinValue::kVertexIndex:
            case core::BuiltinValue::kInstanceIndex:
            case core::BuiltinValue::kSampleIndex:
            case core::BuiltinValue::kSampleMask:
                type = b.create<core::type::I32>();
                value = b.Bitcast(CreateASTTypeFor(ctx, type), value);
                break;
            default:
                break;
        }
        return value;
    }
};

Transform::ApplyResult CanonicalizeEntryPointIO::Apply(const Program* src,
                                                       const DataMap& inputs,
                                                       DataMap&) const {
    ProgramBuilder b;
    program::CloneContext ctx{&b, src, /* auto_clone_symbols */ true};

    auto* cfg = inputs.Get<Config>();
    if (cfg == nullptr) {
        b.Diagnostics().add_error(diag::System::Transform,
                                  "missing transform data for " + std::string(TypeInfo().name));
        return resolver::Resolve(b);
    }

    // Remove entry point IO attributes from struct declarations.
    // New structures will be created for each entry point, as necessary.
    for (auto* ty : src->AST().TypeDecls()) {
        if (auto* struct_ty = ty->As<Struct>()) {
            for (auto* member : struct_ty->members) {
                for (auto* attr : member->attributes) {
                    if (IsShaderIOAttribute(attr)) {
                        ctx.Remove(member->attributes, attr);
                    }
                }
            }
        }
    }

    for (auto* func_ast : src->AST().Functions()) {
        if (!func_ast->IsEntryPoint()) {
            continue;
        }

        State state(ctx, b, *cfg, func_ast);
        state.Process();
    }

    ctx.Clone();
    return resolver::Resolve(b);
}

CanonicalizeEntryPointIO::Config::Config(ShaderStyle style,
                                         uint32_t sample_mask,
                                         bool emit_point_size)
    : shader_style(style),
      fixed_sample_mask(sample_mask),
      emit_vertex_point_size(emit_point_size) {}

CanonicalizeEntryPointIO::Config::Config(const Config&) = default;
CanonicalizeEntryPointIO::Config::~Config() = default;

CanonicalizeEntryPointIO::HLSLWaveIntrinsic::HLSLWaveIntrinsic(GenerationID pid, NodeID nid, Op o)
    : Base(pid, nid, Empty), op(o) {}
CanonicalizeEntryPointIO::HLSLWaveIntrinsic::~HLSLWaveIntrinsic() = default;
std::string CanonicalizeEntryPointIO::HLSLWaveIntrinsic::InternalName() const {
    StringStream ss;
    switch (op) {
        case Op::kWaveGetLaneCount:
            return "intrinsic_wave_get_lane_count";
        case Op::kWaveGetLaneIndex:
            return "intrinsic_wave_get_lane_index";
    }
    return ss.str();
}

const CanonicalizeEntryPointIO::HLSLWaveIntrinsic*
CanonicalizeEntryPointIO::HLSLWaveIntrinsic::Clone(ast::CloneContext& ctx) const {
    return ctx.dst->ASTNodes().Create<CanonicalizeEntryPointIO::HLSLWaveIntrinsic>(
        ctx.dst->ID(), ctx.dst->AllocateNodeID(), op);
}

}  // namespace tint::ast::transform
