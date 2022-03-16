/*
 * Copyright 2014 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// independent from idl_parser, since this code is not needed for most clients

#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"
#include "flatbuffers/code_generators.h"

namespace flatbuffers {

// Pedantic warning free version of toupper().
inline char ToUpper(char c) {
  return static_cast<char>(::toupper(c));
}

static std::string GeneratedFileName(const std::string &path,
                                     const std::string &file_name) {
  return path + file_name + "_generated.h";
}

namespace cpp {
class CppGenerator : public BaseGenerator {
 public:
  CppGenerator(const Parser &parser, const std::string &path,
               const std::string &file_name)
      : BaseGenerator(parser, path, file_name, "", "::"),
        cur_name_space_(nullptr) {}

  std::string GenIncludeGuard() const {
    // Generate include guard.
    std::string guard = file_name_;
    // Remove any non-alpha-numeric characters that may appear in a filename.
    struct IsAlnum {
      bool operator()(char c) { return !isalnum(c); }
    };
    guard.erase(std::remove_if(guard.begin(), guard.end(), IsAlnum()),
                guard.end());
    guard = "FLATBUFFERS_GENERATED_" + guard;
    guard += "_";
    // For further uniqueness, also add the namespace.
    auto name_space = parser_.namespaces_.back();
    for (auto it = name_space->components.begin();
         it != name_space->components.end(); ++it) {
      guard += *it + "_";
    }
    guard += "H_";
    std::transform(guard.begin(), guard.end(), guard.begin(), ToUpper);
    return guard;
  }

  void GenIncludeDependencies() {
    int num_includes = 0;
    for (auto it = parser_.native_included_files_.begin();
         it != parser_.native_included_files_.end(); ++it) {
      code_ += "#include \"" + *it + "\"";
      num_includes++;
    }
    for (auto it = parser_.included_files_.begin();
         it != parser_.included_files_.end(); ++it) {
      if (it->second.empty())
        continue;
      auto noext = flatbuffers::StripExtension(it->second);
      auto basename = flatbuffers::StripPath(noext);

      code_ += "#include \"" + parser_.opts.include_prefix +
               (parser_.opts.keep_include_path ? noext : basename) +
               "_generated.h\"";
      num_includes++;
    }
    if (num_includes) code_ += "";
  }

  // Iterate through all definitions we haven't generate code for (enums,
  // structs, and tables) and output them to a single file.
  bool generate() {
    code_.Clear();
    code_ += "// " + std::string(FlatBuffersGeneratedWarning()) + "\n\n";

    const auto include_guard = GenIncludeGuard();
    code_ += "#ifndef " + include_guard;
    code_ += "#define " + include_guard;
    code_ += "";

    code_ += "#include \"flatbuffers/flatbuffers.h\"";
    code_ += "";

    if (parser_.opts.include_dependence_headers) {
      GenIncludeDependencies();
    }

    assert(!cur_name_space_);

    // Generate forward declarations for all structs/tables, since they may
    // have circular references.
    for (auto it = parser_.structs_.vec.begin();
         it != parser_.structs_.vec.end(); ++it) {
      const auto &struct_def = **it;
      if (!struct_def.generated) {
        SetNameSpace(struct_def.defined_namespace);
        code_ += "struct " + struct_def.name + ";";
        if (parser_.opts.generate_object_based_api && !struct_def.fixed) {
          code_ += "struct " + NativeName(struct_def.name, &struct_def) + ";";
        }
        code_ += "";
      }
    }

    // Generate code for all the enum declarations.
    for (auto it = parser_.enums_.vec.begin(); it != parser_.enums_.vec.end();
         ++it) {
      const auto &enum_def = **it;
      if (!enum_def.generated) {
        SetNameSpace(enum_def.defined_namespace);
        GenEnum(enum_def);
      }
    }

    // Generate code for all structs, then all tables.
    for (auto it = parser_.structs_.vec.begin();
         it != parser_.structs_.vec.end(); ++it) {
      const auto &struct_def = **it;
      if (struct_def.fixed && !struct_def.generated) {
        SetNameSpace(struct_def.defined_namespace);
        GenStruct(struct_def);
      }
    }
    for (auto it = parser_.structs_.vec.begin();
         it != parser_.structs_.vec.end(); ++it) {
      const auto &struct_def = **it;
      if (!struct_def.fixed && !struct_def.generated) {
        SetNameSpace(struct_def.defined_namespace);
        GenTable(struct_def);
      }
    }
    for (auto it = parser_.structs_.vec.begin();
         it != parser_.structs_.vec.end(); ++it) {
      const auto &struct_def = **it;
      if (!struct_def.fixed && !struct_def.generated) {
        SetNameSpace(struct_def.defined_namespace);
        GenTablePost(struct_def);
      }
    }

    // Generate code for union verifiers.
    for (auto it = parser_.enums_.vec.begin(); it != parser_.enums_.vec.end();
         ++it) {
      const auto &enum_def = **it;
      if (enum_def.is_union && !enum_def.generated) {
        SetNameSpace(enum_def.defined_namespace);
        GenUnionPost(enum_def);
      }
    }

    // Generate convenient global helper functions:
    if (parser_.root_struct_def_) {
      auto &struct_def = *parser_.root_struct_def_;
      SetNameSpace(struct_def.defined_namespace);
      const auto &name = struct_def.name;
      const auto qualified_name =
          parser_.namespaces_.back()->GetFullyQualifiedName(name);
      const auto cpp_name = TranslateNameSpace(qualified_name);

      code_.SetValue("STRUCT_NAME", name);
      code_.SetValue("CPP_NAME", cpp_name);

      // The root datatype accessor:
      code_ += "inline \\";
      code_ += "const {{CPP_NAME}} *Get{{STRUCT_NAME}}(const void *buf) {";
      code_ += "  return flatbuffers::GetRoot<{{CPP_NAME}}>(buf);";
      code_ += "}";
      code_ += "";

      if (parser_.opts.mutable_buffer) {
        code_ += "inline \\";
        code_ += "{{STRUCT_NAME}} *GetMutable{{STRUCT_NAME}}(void *buf) {";
        code_ += "  return flatbuffers::GetMutableRoot<{{STRUCT_NAME}}>(buf);";
        code_ += "}";
        code_ += "";
      }

      if (parser_.file_identifier_.length()) {
        // Return the identifier
        code_ += "inline const char *{{STRUCT_NAME}}Identifier() {";
        code_ += "  return \"" + parser_.file_identifier_ + "\";";
        code_ += "}";
        code_ += "";

        // Check if a buffer has the identifier.
        code_ += "inline \\";
        code_ += "bool {{STRUCT_NAME}}BufferHasIdentifier(const void *buf) {";
        code_ += "  return flatbuffers::BufferHasIdentifier(";
        code_ += "      buf, {{STRUCT_NAME}}Identifier());";
        code_ += "}";
        code_ += "";
      }

      // The root verifier.
      if (parser_.file_identifier_.length()) {
        code_.SetValue("ID", name + "Identifier()");
      } else {
        code_.SetValue("ID", "nullptr");
      }

      code_ += "inline bool Verify{{STRUCT_NAME}}Buffer(";
      code_ += "    flatbuffers::Verifier &verifier) {";
      code_ += "  return verifier.VerifyBuffer<{{CPP_NAME}}>({{ID}});";
      code_ += "}";
      code_ += "";

      if (parser_.file_extension_.length()) {
        // Return the extension
        code_ += "inline const char *{{STRUCT_NAME}}Extension() {";
        code_ += "  return \"" + parser_.file_extension_ + "\";";
        code_ += "}";
        code_ += "";
      }

      // Finish a buffer with a given root object:
      code_ += "inline void Finish{{STRUCT_NAME}}Buffer(";
      code_ += "    flatbuffers::FlatBufferBuilder &fbb,";
      code_ += "    flatbuffers::Offset<{{CPP_NAME}}> root) {";
      if (parser_.file_identifier_.length())
        code_ += "  fbb.Finish(root, {{STRUCT_NAME}}Identifier());";
      else
        code_ += "  fbb.Finish(root);";
      code_ += "}";
      code_ += "";

      if (parser_.opts.generate_object_based_api) {
        // A convenient root unpack function.
        auto native_name =
            NativeName(WrapInNameSpace(struct_def), &struct_def);
        code_.SetValue("UNPACK_RETURN",
                       GenTypeNativePtr(native_name, nullptr, false));
        code_.SetValue("UNPACK_TYPE",
                       GenTypeNativePtr(native_name, nullptr, true));

        code_ += "inline {{UNPACK_RETURN}} UnPack{{STRUCT_NAME}}(";
        code_ += "    const void *buf,";
        code_ += "    const flatbuffers::resolver_function_t *res = nullptr) {";
        code_ += "  return {{UNPACK_TYPE}}\\";
        code_ += "(Get{{STRUCT_NAME}}(buf)->UnPack(res));";
        code_ += "}";
        code_ += "";
      }
    }

    if (cur_name_space_) SetNameSpace(nullptr);

    // Close the include guard.
    code_ += "#endif  // " + include_guard;

    const auto file_path = GeneratedFileName(path_, file_name_);
    const auto final_code = code_.ToString();
    return SaveFile(file_path.c_str(), final_code, false);
  }

 private:
  CodeWriter code_;

  // This tracks the current namespace so we can insert namespace declarations.
  const Namespace *cur_name_space_;

  const Namespace *CurrentNameSpace() const { return cur_name_space_; }

  // Translates a qualified name in flatbuffer text format to the same name in
  // the equivalent C++ namespace.
  static std::string TranslateNameSpace(const std::string &qualified_name) {
    std::string cpp_qualified_name = qualified_name;
    size_t start_pos = 0;
    while ((start_pos = cpp_qualified_name.find(".", start_pos)) !=
           std::string::npos) {
      cpp_qualified_name.replace(start_pos, 1, "::");
    }
    return cpp_qualified_name;
  }

  void GenComment(const std::vector<std::string> &dc, const char *prefix = "") {
    std::string text;
    ::flatbuffers::GenComment(dc, &text, nullptr, prefix);
    code_ += text + "\\";
  }

  // Return a C++ type from the table in idl.h
  std::string GenTypeBasic(const Type &type, bool user_facing_type) const {
    static const char *ctypename[] = {
    #define FLATBUFFERS_TD(ENUM, IDLTYPE, CTYPE, JTYPE, GTYPE, NTYPE, PTYPE) \
            #CTYPE,
        FLATBUFFERS_GEN_TYPES(FLATBUFFERS_TD)
    #undef FLATBUFFERS_TD
    };
    if (user_facing_type) {
      if (type.enum_def) return WrapInNameSpace(*type.enum_def);
      if (type.base_type == BASE_TYPE_BOOL) return "bool";
    }
    return ctypename[type.base_type];
  }

  // Return a C++ pointer type, specialized to the actual struct/table types,
  // and vector element types.
  std::string GenTypePointer(const Type &type) const {
    switch (type.base_type) {
      case BASE_TYPE_STRING: {
        return "flatbuffers::String";
      }
      case BASE_TYPE_VECTOR: {
        const auto type_name = GenTypeWire(type.VectorType(), "", false);
        return "flatbuffers::Vector<" + type_name + ">";
      }
      case BASE_TYPE_STRUCT: {
        return WrapInNameSpace(*type.struct_def);
      }
      case BASE_TYPE_UNION:
      // fall through
      default: {
        return "void";
      }
    }
  }

  // Return a C++ type for any type (scalar/pointer) specifically for
  // building a flatbuffer.
  std::string GenTypeWire(const Type &type, const char *postfix,
                          bool user_facing_type) const {
    if (IsScalar(type.base_type)) {
      return GenTypeBasic(type, user_facing_type) + postfix;
    } else if (IsStruct(type)) {
      return "const " + GenTypePointer(type) + " *";
    } else {
      return "flatbuffers::Offset<" + GenTypePointer(type) + ">" + postfix;
    }
  }

  // Return a C++ type for any type (scalar/pointer) that reflects its
  // serialized size.
  std::string GenTypeSize(const Type &type) const {
    if (IsScalar(type.base_type)) {
      return GenTypeBasic(type, false);
    } else if (IsStruct(type)) {
      return GenTypePointer(type);
    } else {
      return "flatbuffers::uoffset_t";
    }
  }

  // TODO(wvo): make this configurable.
  static std::string NativeName(const std::string &name, const StructDef *sd) {
    return sd && !sd->fixed ? name + "T" : name;
  }

  const std::string &PtrType(const FieldDef *field) {
    auto attr = field ? field->attributes.Lookup("cpp_ptr_type") : nullptr;
    return attr ? attr->constant : parser_.opts.cpp_object_api_pointer_type;
  }

  const std::string NativeString(const FieldDef *field) {
    auto attr = field ? field->attributes.Lookup("cpp_str_type") : nullptr;
    auto &ret = attr ? attr->constant : parser_.opts.cpp_object_api_string_type;
    if (ret.empty()) {
      return "std::string";
    }
    return ret;
  }

  std::string GenTypeNativePtr(const std::string &type, const FieldDef *field,
                               bool is_constructor) {
    auto &ptr_type = PtrType(field);
    if (ptr_type != "naked") {
      return ptr_type + "<" + type + ">";
    } else if (is_constructor) {
      return "";
    } else {
      return type + " *";
    }
  }

  std::string GenPtrGet(const FieldDef &field) {
    auto &ptr_type = PtrType(&field);
    return ptr_type == "naked" ? "" : ".get()";
  }

  std::string GenTypeNative(const Type &type, bool invector,
                            const FieldDef &field) {
    switch (type.base_type) {
      case BASE_TYPE_STRING: {
        return NativeString(&field);
      }
      case BASE_TYPE_VECTOR: {
        const auto type_name = GenTypeNative(type.VectorType(), true, field);
        return "std::vector<" + type_name + ">";
      }
      case BASE_TYPE_STRUCT: {
        auto type_name = WrapInNameSpace(*type.struct_def);
        if (IsStruct(type)) {
          auto native_type = type.struct_def->attributes.Lookup("native_type");
          if (native_type) {
            type_name = native_type->constant;
          }
          if (invector || field.native_inline) {
            return type_name;
          } else {
            return GenTypeNativePtr(type_name, &field, false);
          }
        } else {
          return GenTypeNativePtr(NativeName(type_name, type.struct_def),
                                  &field, false);
        }
      }
      case BASE_TYPE_UNION: {
        return type.enum_def->name + "Union";
      }
      default: {
        return GenTypeBasic(type, true);
      }
    }
  }

  // Return a C++ type for any type (scalar/pointer) specifically for
  // using a flatbuffer.
  std::string GenTypeGet(const Type &type, const char *afterbasic,
                         const char *beforeptr, const char *afterptr,
                         bool user_facing_type) {
    if (IsScalar(type.base_type)) {
      return GenTypeBasic(type, user_facing_type) + afterbasic;
    } else {
      return beforeptr + GenTypePointer(type) + afterptr;
    }
  }

  std::string GenEnumDecl(const EnumDef &enum_def) const {
    const IDLOptions &opts = parser_.opts;
    return (opts.scoped_enums ? "enum class " : "enum ") + enum_def.name;
  }

  std::string GenEnumValDecl(const EnumDef &enum_def,
                             const std::string &enum_val) const {
    const IDLOptions &opts = parser_.opts;
    return opts.prefixed_enums ? enum_def.name + "_" + enum_val : enum_val;
  }

  std::string GetEnumValUse(const EnumDef &enum_def,
                            const EnumVal &enum_val) const {
    const IDLOptions &opts = parser_.opts;
    if (opts.scoped_enums) {
      return enum_def.name + "::" + enum_val.name;
    } else if (opts.prefixed_enums) {
      return enum_def.name + "_" + enum_val.name;
    } else {
      return enum_val.name;
    }
  }

  std::string StripUnionType(const std::string &name) {
    return name.substr(0, name.size() - strlen(UnionTypeFieldSuffix()));
  }

  std::string GetUnionElement(const EnumVal &ev, bool wrap, bool actual_type,
                              bool native_type = false) {
    if (ev.union_type.base_type == BASE_TYPE_STRUCT) {
      auto name = actual_type ? ev.union_type.struct_def->name : ev.name;
      return wrap
          ? WrapInNameSpace(ev.union_type.struct_def->defined_namespace, name)
          : name;
    } else if (ev.union_type.base_type == BASE_TYPE_STRING) {
      return actual_type
          ? (native_type ? "std::string" : "flatbuffers::String")
          : ev.name;
    } else {
      assert(false);
      return ev.name;
    }
  }

  static std::string UnionVerifySignature(const EnumDef &enum_def) {
    return "bool Verify" + enum_def.name +
           "(flatbuffers::Verifier &verifier, const void *obj, " +
           enum_def.name + " type)";
  }

  static std::string UnionVectorVerifySignature(const EnumDef &enum_def) {
    return "bool Verify" + enum_def.name + "Vector" +
           "(flatbuffers::Verifier &verifier, " +
           "const flatbuffers::Vector<flatbuffers::Offset<void>> *values, " +
           "const flatbuffers::Vector<uint8_t> *types)";
  }

  static std::string UnionUnPackSignature(const EnumDef &enum_def,
                                          bool inclass) {
    return (inclass ? "static " : "") +
           std::string("void *") +
           (inclass ? "" : enum_def.name + "Union::") +
           "UnPack(const void *obj, " + enum_def.name +
           " type, const flatbuffers::resolver_function_t *resolver)";
  }

  static std::string UnionPackSignature(const EnumDef &enum_def, bool inclass) {
    return "flatbuffers::Offset<void> " +
           (inclass ? "" : enum_def.name + "Union::") +
           "Pack(flatbuffers::FlatBufferBuilder &_fbb, " +
           "const flatbuffers::rehasher_function_t *_rehasher" +
           (inclass ? " = nullptr" : "") + ") const";
  }

  static std::string TableCreateSignature(const StructDef &struct_def,
                                          bool predecl) {
    return "flatbuffers::Offset<" + struct_def.name + "> Create" +
           struct_def.name  +
           "(flatbuffers::FlatBufferBuilder &_fbb, const " +
           NativeName(struct_def.name, &struct_def) +
           " *_o, const flatbuffers::rehasher_function_t *_rehasher" +
           (predecl ? " = nullptr" : "") + ")";
  }

  static std::string TablePackSignature(const StructDef &struct_def,
                                        bool inclass) {
    return std::string(inclass ? "static " : "") +
           "flatbuffers::Offset<" + struct_def.name + "> " +
           (inclass ? "" : struct_def.name + "::") +
           "Pack(flatbuffers::FlatBufferBuilder &_fbb, " +
           "const " + NativeName(struct_def.name, &struct_def) + "* _o, " +
           "const flatbuffers::rehasher_function_t *_rehasher" +
           (inclass ? " = nullptr" : "") + ")";
  }

  static std::string TableUnPackSignature(const StructDef &struct_def,
                                          bool inclass) {
    return NativeName(struct_def.name, &struct_def) + " *" +
           (inclass ? "" : struct_def.name + "::") +
           "UnPack(const flatbuffers::resolver_function_t *_resolver" +
           (inclass ? " = nullptr" : "") + ") const";
  }

  static std::string TableUnPackToSignature(const StructDef &struct_def,
                                            bool inclass) {
    return "void " + (inclass ? "" : struct_def.name + "::") +
           "UnPackTo(" + NativeName(struct_def.name, &struct_def) + " *" +
           "_o, const flatbuffers::resolver_function_t *_resolver" +
           (inclass ? " = nullptr" : "") + ") const";
  }

  // Generate an enum declaration and an enum string lookup table.
  void GenEnum(const EnumDef &enum_def) {
    code_.SetValue("ENUM_NAME", enum_def.name);
    code_.SetValue("BASE_TYPE", GenTypeBasic(enum_def.underlying_type, false));
    code_.SetValue("SEP", "");

    GenComment(enum_def.doc_comment);
    code_ += GenEnumDecl(enum_def) + "\\";
    if (parser_.opts.scoped_enums)
      code_ += " : {{BASE_TYPE}}\\";
    code_ += " {";

    int64_t anyv = 0;
    const EnumVal *minv = nullptr, *maxv = nullptr;
    for (auto it = enum_def.vals.vec.begin(); it != enum_def.vals.vec.end();
         ++it) {
      const auto &ev = **it;

      GenComment(ev.doc_comment, "  ");
      code_.SetValue("KEY", GenEnumValDecl(enum_def, ev.name));
      code_.SetValue("VALUE", NumToString(ev.value));
      code_ += "{{SEP}}  {{KEY}} = {{VALUE}}\\";
      code_.SetValue("SEP", ",\n");

      minv = !minv || minv->value > ev.value ? &ev : minv;
      maxv = !maxv || maxv->value < ev.value ? &ev : maxv;
      anyv |= ev.value;
    }

    if (parser_.opts.scoped_enums || parser_.opts.prefixed_enums) {
      assert(minv && maxv);

      code_.SetValue("SEP", ",\n");
      if (enum_def.attributes.Lookup("bit_flags")) {
        code_.SetValue("KEY", GenEnumValDecl(enum_def, "NONE"));
        code_.SetValue("VALUE", "0");
        code_ += "{{SEP}}  {{KEY}} = {{VALUE}}\\";

        code_.SetValue("KEY", GenEnumValDecl(enum_def, "ANY"));
        code_.SetValue("VALUE", NumToString(anyv));
        code_ += "{{SEP}}  {{KEY}} = {{VALUE}}\\";
      } else {  // MIN & MAX are useless for bit_flags
        code_.SetValue("KEY",GenEnumValDecl(enum_def, "MIN"));
        code_.SetValue("VALUE", GenEnumValDecl(enum_def, minv->name));
        code_ += "{{SEP}}  {{KEY}} = {{VALUE}}\\";

        code_.SetValue("KEY",GenEnumValDecl(enum_def, "MAX"));
        code_.SetValue("VALUE", GenEnumValDecl(enum_def, maxv->name));
        code_ += "{{SEP}}  {{KEY}} = {{VALUE}}\\";
      }
    }
    code_ += "";
    code_ += "};";

    if (parser_.opts.scoped_enums && enum_def.attributes.Lookup("bit_flags")) {
      code_ += "DEFINE_BITMASK_OPERATORS({{ENUM_NAME}}, {{BASE_TYPE}})";
    }
    code_ += "";

    // Generate a generate string table for enum values.
    // Problem is, if values are very sparse that could generate really big
    // tables. Ideally in that case we generate a map lookup instead, but for
    // the moment we simply don't output a table at all.
    auto range =
        enum_def.vals.vec.back()->value - enum_def.vals.vec.front()->value + 1;
    // Average distance between values above which we consider a table
    // "too sparse". Change at will.
    static const int kMaxSparseness = 5;
    if (range / static_cast<int64_t>(enum_def.vals.vec.size()) <
        kMaxSparseness) {
      code_ += "inline const char **EnumNames{{ENUM_NAME}}() {";
      code_ += "  static const char *names[] = {";

      auto val = enum_def.vals.vec.front()->value;
      for (auto it = enum_def.vals.vec.begin(); it != enum_def.vals.vec.end();
           ++it) {
        const auto &ev = **it;
        while (val++ != ev.value) {
          code_ += "    \"\",";
        }
        code_ += "    \"" + ev.name + "\",";
      }
      code_ += "    nullptr";
      code_ += "  };";

      code_ += "  return names;";
      code_ += "}";
      code_ += "";

      code_ += "inline const char *EnumName{{ENUM_NAME}}({{ENUM_NAME}} e) {";

      code_ += "  const size_t index = static_cast<int>(e)\\";
      if (enum_def.vals.vec.front()->value) {
        auto vals = GetEnumValUse(enum_def, *enum_def.vals.vec.front());
        code_ += " - static_cast<int>(" + vals + ")\\";
      }
      code_ += ";";

      code_ += "  return EnumNames{{ENUM_NAME}}()[index];";
      code_ += "}";
      code_ += "";
    }

    // Generate type traits for unions to map from a type to union enum value.
    if (enum_def.is_union && !enum_def.uses_type_aliases) {
      for (auto it = enum_def.vals.vec.begin(); it != enum_def.vals.vec.end();
        ++it) {
        const auto &ev = **it;

        if (it == enum_def.vals.vec.begin()) {
          code_ += "template<typename T> struct {{ENUM_NAME}}Traits {";
        }
        else {
          auto name = GetUnionElement(ev, true, true);
          code_ += "template<> struct {{ENUM_NAME}}Traits<" + name + "> {";
        }

        auto value = GetEnumValUse(enum_def, ev);
        code_ += "  static const {{ENUM_NAME}} enum_value = " + value + ";";
        code_ += "};";
        code_ += "";
      }
    }

    if (parser_.opts.generate_object_based_api && enum_def.is_union) {
      // Generate a union type
      code_.SetValue("NAME", enum_def.name);
      code_.SetValue("NONE",
          GetEnumValUse(enum_def, *enum_def.vals.Lookup("NONE")));

      code_ += "struct {{NAME}}Union {";
      code_ += "  {{NAME}} type;";
      code_ += "  void *value;";
      code_ += "";
      code_ += "  {{NAME}}Union() : type({{NONE}}), value(nullptr) {}";
      code_ += "  {{NAME}}Union({{NAME}}Union&& u) FLATBUFFERS_NOEXCEPT :";
      code_ += "    type({{NONE}}), value(nullptr)";
      code_ += "    { std::swap(type, u.type); std::swap(value, u.value); }";
      code_ += "  {{NAME}}Union(const {{NAME}}Union &) FLATBUFFERS_NOEXCEPT;";
      code_ += "  {{NAME}}Union &operator=(const {{NAME}}Union &u) FLATBUFFERS_NOEXCEPT";
      code_ += "    { {{NAME}}Union t(u); std::swap(type, t.type); std::swap(value, t.value); return *this; }";
      code_ += "  {{NAME}}Union &operator=({{NAME}}Union &&u) FLATBUFFERS_NOEXCEPT";
      code_ += "    { std::swap(type, u.type); std::swap(value, u.value); return *this; }";
      code_ += "  ~{{NAME}}Union() { Reset(); }";
      code_ += "";
      code_ += "  void Reset();";
      code_ += "";
      if (!enum_def.uses_type_aliases) {
        code_ += "  template <typename T>";
        code_ += "  void Set(T&& val) {";
        code_ += "    Reset();";
        code_ += "    type = {{NAME}}Traits<typename T::TableType>::enum_value;";
        code_ += "    if (type != {{NONE}}) {";
        code_ += "      value = new T(std::forward<T>(val));";
        code_ += "    }";
        code_ += "  }";
        code_ += "";
      }
      code_ += "  " + UnionUnPackSignature(enum_def, true) + ";";
      code_ += "  " + UnionPackSignature(enum_def, true) + ";";
      code_ += "";

      for (auto it = enum_def.vals.vec.begin(); it != enum_def.vals.vec.end();
           ++it) {
        const auto &ev = **it;
        if (!ev.value) {
          continue;
        }

        const auto native_type =
            NativeName(GetUnionElement(ev, true, true, true),
                       ev.union_type.struct_def);
        code_.SetValue("NATIVE_TYPE", native_type);
        code_.SetValue("NATIVE_NAME", ev.name);
        code_.SetValue("NATIVE_ID", GetEnumValUse(enum_def, ev));

        code_ += "  {{NATIVE_TYPE}} *As{{NATIVE_NAME}}() {";
        code_ += "    return type == {{NATIVE_ID}} ?";
        code_ += "      reinterpret_cast<{{NATIVE_TYPE}} *>(value) : nullptr;";
        code_ += "  }";
      }
      code_ += "};";
      code_ += "";
    }

    if (enum_def.is_union) {
      code_ += UnionVerifySignature(enum_def) + ";";
      code_ += UnionVectorVerifySignature(enum_def) + ";";
      code_ += "";
    }
  }

  void GenUnionPost(const EnumDef &enum_def) {
    // Generate a verifier function for this union that can be called by the
    // table verifier functions. It uses a switch case to select a specific
    // verifier function to call, this should be safe even if the union type
    // has been corrupted, since the verifiers will simply fail when called
    // on the wrong type.
    code_.SetValue("ENUM_NAME", enum_def.name);

    code_ += "inline " + UnionVerifySignature(enum_def) + " {";
    code_ += "  switch (type) {";
    for (auto it = enum_def.vals.vec.begin(); it != enum_def.vals.vec.end();
         ++it) {
      const auto &ev = **it;
      code_.SetValue("LABEL", GetEnumValUse(enum_def, ev));

      if (ev.value) {
        code_.SetValue("TYPE", GetUnionElement(ev, true, true));
        code_ += "    case {{LABEL}}: {";
        auto getptr =
            "      auto ptr = reinterpret_cast<const {{TYPE}} *>(obj);";
        if (ev.union_type.base_type == BASE_TYPE_STRUCT) {
          if (ev.union_type.struct_def->fixed) {
            code_ += "      return true;";
          } else {
            code_ += getptr;
            code_ += "      return verifier.VerifyTable(ptr);";
          }
        } else if (ev.union_type.base_type == BASE_TYPE_STRING) {
          code_ += getptr;
          code_ += "      return verifier.Verify(ptr);";
        } else {
          assert(false);
        }
        code_ += "    }";
      } else {
        code_ += "    case {{LABEL}}: {";
        code_ += "      return true;";  // "NONE" enum value.
        code_ += "    }";
      }
    }
    code_ += "    default: return false;";
    code_ += "  }";
    code_ += "}";
    code_ += "";

    code_ += "inline " + UnionVectorVerifySignature(enum_def) + " {";
    code_ += "  if (values->size() != types->size()) return false;";
    code_ += "  for (flatbuffers::uoffset_t i = 0; i < values->size(); ++i) {";
    code_ += "    if (!Verify" + enum_def.name + "(";
    code_ += "        verifier,  values->Get(i), types->GetEnum<" + enum_def.name + ">(i))) {";
    code_ += "      return false;";
    code_ += "    }";
    code_ += "  }";
    code_ += "  return true;";
    code_ += "}";
    code_ += "";

    if (parser_.opts.generate_object_based_api) {
      // Generate union Unpack() and Pack() functions.
      code_ += "inline " + UnionUnPackSignature(enum_def, false) + " {";
      code_ += "  switch (type) {";
      for (auto it = enum_def.vals.vec.begin(); it != enum_def.vals.vec.end();
           ++it) {
        const auto &ev = **it;
        if (!ev.value) {
          continue;
        }

        code_.SetValue("LABEL", GetEnumValUse(enum_def, ev));
        code_.SetValue("TYPE", GetUnionElement(ev, true, true));
        code_ += "    case {{LABEL}}: {";
        code_ += "      auto ptr = reinterpret_cast<const {{TYPE}} *>(obj);";
        if (ev.union_type.base_type == BASE_TYPE_STRUCT) {
          if (ev.union_type.struct_def->fixed) {
            code_ += "      return new " +
                     WrapInNameSpace(*ev.union_type.struct_def) + "(*ptr);";
          } else {
            code_ += "      return ptr->UnPack(resolver);";
          }
        } else if (ev.union_type.base_type == BASE_TYPE_STRING) {
          code_ += "      return new std::string(ptr->c_str(), ptr->size());";
        } else {
          assert(false);
        }
        code_ += "    }";
      }
      code_ += "    default: return nullptr;";
      code_ += "  }";
      code_ += "}";
      code_ += "";

      code_ += "inline " + UnionPackSignature(enum_def, false) + " {";
      code_ += "  switch (type) {";
      for (auto it = enum_def.vals.vec.begin(); it != enum_def.vals.vec.end();
           ++it) {
        auto &ev = **it;
        if (!ev.value) {
          continue;
        }

        code_.SetValue("LABEL", GetEnumValUse(enum_def, ev));
        code_.SetValue("TYPE", NativeName(GetUnionElement(ev, true, true, true),
                                          ev.union_type.struct_def));
        code_.SetValue("NAME", GetUnionElement(ev, false, true));
        code_ += "    case {{LABEL}}: {";
        code_ += "      auto ptr = reinterpret_cast<const {{TYPE}} *>(value);";
        if (ev.union_type.base_type == BASE_TYPE_STRUCT) {
          if (ev.union_type.struct_def->fixed) {
            code_ += "      return _fbb.CreateStruct(*ptr).Union();";
          } else {
            code_ +=
                "      return Create{{NAME}}(_fbb, ptr, _rehasher).Union();";
          }
        } else if (ev.union_type.base_type == BASE_TYPE_STRING) {
          code_ += "      return _fbb.CreateString(*ptr).Union();";
        } else {
          assert(false);
        }
        code_ += "    }";
      }
      code_ += "    default: return 0;";
      code_ += "  }";
      code_ += "}";
      code_ += "";

      // Union copy constructor
      code_ += "inline {{ENUM_NAME}}Union::{{ENUM_NAME}}Union(const "
               "{{ENUM_NAME}}Union &u) FLATBUFFERS_NOEXCEPT : type(u.type), "
               "value(nullptr) {";
      code_ += "  switch (type) {";
      for (auto it = enum_def.vals.vec.begin(); it != enum_def.vals.vec.end();
           ++it) {
        const auto &ev = **it;
        if (!ev.value) {
          continue;
        }
        code_.SetValue("LABEL", GetEnumValUse(enum_def, ev));
        code_.SetValue("TYPE", NativeName(GetUnionElement(ev, true, true, true),
                                          ev.union_type.struct_def));
        code_ += "    case {{LABEL}}: {";
        bool copyable = true;
        if (ev.union_type.base_type == BASE_TYPE_STRUCT) {
          // Don't generate code to copy if table is not copyable.
          // TODO(wvo): make tables copyable instead.
          for (auto fit = ev.union_type.struct_def->fields.vec.begin();
               fit != ev.union_type.struct_def->fields.vec.end(); ++fit) {
            const auto &field = **fit;
            if (!field.deprecated && field.value.type.struct_def) {
              copyable = false;
              break;
            }
          }
        }
        if (copyable) {
          code_ += "      value = new {{TYPE}}(*reinterpret_cast<{{TYPE}} *>"
                   "(u.value));";
        } else {
          code_ += "      assert(false);  // {{TYPE}} not copyable.";
        }
        code_ += "      break;";
        code_ += "    }";
      }
      code_ += "    default:";
      code_ += "      break;";
      code_ += "  }";
      code_ += "}";
      code_ += "";

      // Union Reset() function.
      code_.SetValue("NONE",
          GetEnumValUse(enum_def, *enum_def.vals.Lookup("NONE")));

      code_ += "inline void {{ENUM_NAME}}Union::Reset() {";
      code_ += "  switch (type) {";
      for (auto it = enum_def.vals.vec.begin(); it != enum_def.vals.vec.end();
           ++it) {
        const auto &ev = **it;
        if (!ev.value) {
          continue;
        }
        code_.SetValue("LABEL", GetEnumValUse(enum_def, ev));
        code_.SetValue("TYPE", NativeName(GetUnionElement(ev, true, true, true),
                                          ev.union_type.struct_def));
        code_ += "    case {{LABEL}}: {";
        code_ += "      auto ptr = reinterpret_cast<{{TYPE}} *>(value);";
        code_ += "      delete ptr;";
        code_ += "      break;";
        code_ += "    }";
      }
      code_ += "    default: break;";
      code_ += "  }";
      code_ += "  value = nullptr;";
      code_ += "  type = {{NONE}};";
      code_ += "}";
      code_ += "";
    }
  }

  // Generates a value with optionally a cast applied if the field has a
  // different underlying type from its interface type (currently only the
  // case for enums. "from" specify the direction, true meaning from the
  // underlying type to the interface type.
  std::string GenUnderlyingCast(const FieldDef &field, bool from,
                                const std::string &val) {
    if (from && field.value.type.base_type == BASE_TYPE_BOOL) {
      return val + " != 0";
    } else if ((field.value.type.enum_def &&
                IsScalar(field.value.type.base_type)) ||
               field.value.type.base_type == BASE_TYPE_BOOL) {
      return "static_cast<" + GenTypeBasic(field.value.type, from) + ">(" +
             val + ")";
    } else {
      return val;
    }
  }

  std::string GenFieldOffsetName(const FieldDef &field) {
    std::string uname = field.name;
    std::transform(uname.begin(), uname.end(), uname.begin(), ToUpper);
    return "VT_" + uname;
  }

  void GenFullyQualifiedNameGetter(const std::string &name) {
    if (!parser_.opts.generate_name_strings) {
      return;
    }

    auto fullname = parser_.namespaces_.back()->GetFullyQualifiedName(name);
    code_.SetValue("NAME", fullname);
    code_.SetValue("CONSTEXPR", "FLATBUFFERS_CONSTEXPR");

    code_ += "  static {{CONSTEXPR}} const char *GetFullyQualifiedName() {";
    code_ += "    return \"{{NAME}}\";";
    code_ += "  }";
  }

  std::string GenDefaultConstant(const FieldDef &field) {
    return field.value.type.base_type == BASE_TYPE_FLOAT
               ? field.value.constant + "f"
               : field.value.constant;
  }

  std::string GetDefaultScalarValue(const FieldDef &field) {
    if (field.value.type.enum_def && IsScalar(field.value.type.base_type)) {
      auto ev = field.value.type.enum_def->ReverseLookup(
          static_cast<int>(StringToInt(field.value.constant.c_str())), false);
      if (ev) {
        return WrapInNameSpace(
            field.value.type.enum_def->defined_namespace,
            GetEnumValUse(*field.value.type.enum_def, *ev));
      } else {
        return GenUnderlyingCast(field, true, field.value.constant);
      }
    } else if (field.value.type.base_type == BASE_TYPE_BOOL) {
      return field.value.constant == "0" ? "false" : "true";
    } else {
      return GenDefaultConstant(field);
    }
  }

  void GenParam(const FieldDef &field, bool direct, const char *prefix) {
    code_.SetValue("PRE", prefix);
    code_.SetValue("PARAM_NAME", field.name);
    if (direct && field.value.type.base_type == BASE_TYPE_STRING) {
      code_.SetValue("PARAM_TYPE", "const char *");
      code_.SetValue("PARAM_VALUE", "nullptr");
    } else if (direct && field.value.type.base_type == BASE_TYPE_VECTOR) {
      auto type = GenTypeWire(field.value.type.VectorType(), "", false);
      code_.SetValue("PARAM_TYPE", "const std::vector<" + type + "> *");
      code_.SetValue("PARAM_VALUE", "nullptr");
    } else {
      code_.SetValue("PARAM_TYPE", GenTypeWire(field.value.type, " ", true));
      code_.SetValue("PARAM_VALUE", GetDefaultScalarValue(field));
    }
    code_ += "{{PRE}}{{PARAM_TYPE}}{{PARAM_NAME}} = {{PARAM_VALUE}}\\";
  }

  // Generate a member, including a default value for scalars and raw pointers.
  void GenMember(const FieldDef &field) {
    if (!field.deprecated &&  // Deprecated fields won't be accessible.
        field.value.type.base_type != BASE_TYPE_UTYPE &&
        (field.value.type.base_type != BASE_TYPE_VECTOR ||
         field.value.type.element != BASE_TYPE_UTYPE)) {
      auto type = GenTypeNative(field.value.type, false, field);
      auto cpp_type = field.attributes.Lookup("cpp_type");
      auto full_type = (cpp_type ? cpp_type->constant + " *" : type + " ");
      code_.SetValue("FIELD_TYPE", full_type);
      code_.SetValue("FIELD_NAME", field.name);
      code_ += "  {{FIELD_TYPE}}{{FIELD_NAME}};";
    }
  }

  // Generate the default constructor for this struct. Properly initialize all
  // scalar members with default values.
  void GenDefaultConstructor(const StructDef& struct_def) {
    std::string initializer_list;
    for (auto it = struct_def.fields.vec.begin();
         it != struct_def.fields.vec.end(); ++it) {
      const auto &field = **it;
      if (!field.deprecated &&  // Deprecated fields won't be accessible.
          field.value.type.base_type != BASE_TYPE_UTYPE) {
        auto cpp_type = field.attributes.Lookup("cpp_type");
        // Scalar types get parsed defaults, raw pointers get nullptrs.
        if (IsScalar(field.value.type.base_type)) {
          if (!initializer_list.empty()) {
            initializer_list += ",\n        ";
          }
          initializer_list += field.name;
          initializer_list += "(" + GetDefaultScalarValue(field) + ")";
        } else if (field.value.type.base_type == BASE_TYPE_STRUCT) {
          if (IsStruct(field.value.type)) {
            auto native_default = field.attributes.Lookup("native_default");
            if (native_default) {
              if (!initializer_list.empty()) {
                initializer_list += ",\n        ";
              }
              initializer_list +=
                  field.name + "(" + native_default->constant + ")";
            }
          }
        } else if (cpp_type) {
          if (!initializer_list.empty()) {
            initializer_list += ",\n        ";
          }
          initializer_list += field.name + "(0)";
        }
      }
    }
    if (!initializer_list.empty()) {
      initializer_list = "\n      : " + initializer_list;
    }

    code_.SetValue("NATIVE_NAME", NativeName(struct_def.name, &struct_def));
    code_.SetValue("INIT_LIST", initializer_list);

    code_ += "  {{NATIVE_NAME}}(){{INIT_LIST}} {";
    code_ += "  }";
  }

  void GenNativeTable(const StructDef &struct_def) {
    const auto native_name = NativeName(struct_def.name, &struct_def);
    code_.SetValue("STRUCT_NAME", struct_def.name);
    code_.SetValue("NATIVE_NAME", native_name);

    // Generate a C++ object that can hold an unpacked version of this table.
    code_ += "struct {{NATIVE_NAME}} : public flatbuffers::NativeTable {";
    code_ += "  typedef {{STRUCT_NAME}} TableType;";
    GenFullyQualifiedNameGetter(native_name);
    for (auto it = struct_def.fields.vec.begin();
         it != struct_def.fields.vec.end(); ++it) {
      GenMember(**it);
    }
    GenDefaultConstructor(struct_def);
    code_ += "};";
    code_ += "";
  }

  // Generate the code to call the appropriate Verify function(s) for a field.
  void GenVerifyCall(const FieldDef &field, const char* prefix) {
    code_.SetValue("PRE", prefix);
    code_.SetValue("NAME", field.name);
    code_.SetValue("REQUIRED", field.required ? "Required" : "");
    code_.SetValue("SIZE", GenTypeSize(field.value.type));
    code_.SetValue("OFFSET", GenFieldOffsetName(field));
    if (IsScalar(field.value.type.base_type) || IsStruct(field.value.type)) {
      code_ +=
          "{{PRE}}VerifyField{{REQUIRED}}<{{SIZE}}>(verifier, {{OFFSET}})\\";
    } else {
      code_ += "{{PRE}}VerifyOffset{{REQUIRED}}(verifier, {{OFFSET}})\\";
    }

    switch (field.value.type.base_type) {
      case BASE_TYPE_UNION: {
        code_.SetValue("ENUM_NAME", field.value.type.enum_def->name);
        code_.SetValue("SUFFIX", UnionTypeFieldSuffix());
        code_ += "{{PRE}}Verify{{ENUM_NAME}}(verifier, {{NAME}}(), "
                "{{NAME}}{{SUFFIX}}())\\";
        break;
      }
      case BASE_TYPE_STRUCT: {
        if (!field.value.type.struct_def->fixed) {
          code_ += "{{PRE}}verifier.VerifyTable({{NAME}}())\\";
        }
        break;
      }
      case BASE_TYPE_STRING: {
        code_ += "{{PRE}}verifier.Verify({{NAME}}())\\";
        break;
      }
      case BASE_TYPE_VECTOR: {
        code_ += "{{PRE}}verifier.Verify({{NAME}}())\\";

        switch (field.value.type.element) {
          case BASE_TYPE_STRING: {
            code_ += "{{PRE}}verifier.VerifyVectorOfStrings({{NAME}}())\\";
            break;
          }
          case BASE_TYPE_STRUCT: {
            if (!field.value.type.struct_def->fixed) {
              code_ += "{{PRE}}verifier.VerifyVectorOfTables({{NAME}}())\\";
            }
            break;
          }
          case BASE_TYPE_UNION: {
            code_.SetValue("ENUM_NAME", field.value.type.enum_def->name);
            code_ += "{{PRE}}Verify{{ENUM_NAME}}Vector(verifier, {{NAME}}(), {{NAME}}_type())\\";
            break;
          }
          default:
            break;
        }
        break;
      }
      default: {
        break;
      }
    }
  }

  // Generate an accessor struct, builder structs & function for a table.
  void GenTable(const StructDef &struct_def) {
    if (parser_.opts.generate_object_based_api) {
      GenNativeTable(struct_def);
    }

    // Generate an accessor struct, with methods of the form:
    // type name() const { return GetField<type>(offset, defaultval); }
    GenComment(struct_def.doc_comment);

    code_.SetValue("STRUCT_NAME", struct_def.name);
    code_ += "struct {{STRUCT_NAME}} FLATBUFFERS_FINAL_CLASS"
            " : private flatbuffers::Table {";
    if (parser_.opts.generate_object_based_api) {
      code_ += "  typedef {{NATIVE_NAME}} NativeTableType;";
    }

    GenFullyQualifiedNameGetter(struct_def.name);

    // Generate field id constants.
    if (struct_def.fields.vec.size() > 0) {
      // We need to add a trailing comma to all elements except the last one as
      // older versions of gcc complain about this.
      code_.SetValue("SEP", "");
      code_ += "  enum {";
      for (auto it = struct_def.fields.vec.begin();
           it != struct_def.fields.vec.end(); ++it) {
        const auto &field = **it;
        if (field.deprecated) {
          // Deprecated fields won't be accessible.
          continue;
        }

        code_.SetValue("OFFSET_NAME", GenFieldOffsetName(field));
        code_.SetValue("OFFSET_VALUE", NumToString(field.value.offset));
        code_ += "{{SEP}}    {{OFFSET_NAME}} = {{OFFSET_VALUE}}\\";
        code_.SetValue("SEP", ",\n");
      }
      code_ += "";
      code_ += "  };";
    }

    // Generate the accessors.
    for (auto it = struct_def.fields.vec.begin();
         it != struct_def.fields.vec.end(); ++it) {
      const auto &field = **it;
      if (field.deprecated) {
        // Deprecated fields won't be accessible.
        continue;
      }

      const bool is_struct = IsStruct(field.value.type);
      const bool is_scalar = IsScalar(field.value.type.base_type);
      code_.SetValue("FIELD_NAME", field.name);

      // Call a different accessor for pointers, that indirects.
      std::string accessor = "";
      if (is_scalar) {
        accessor = "GetField<";
      } else if (is_struct) {
        accessor = "GetStruct<";
      } else {
        accessor = "GetPointer<";
      }
      auto offset_str = GenFieldOffsetName(field);
      auto offset_type =
          GenTypeGet(field.value.type, "", "const ", " *", false);

      auto call = accessor + offset_type + ">(" + offset_str;
      // Default value as second arg for non-pointer types.
      if (is_scalar) {
        call += ", " + GenDefaultConstant(field);
      }
      call += ")";

      GenComment(field.doc_comment, "  ");
      code_.SetValue("FIELD_TYPE",
          GenTypeGet(field.value.type, " ", "const ", " *", true));
      code_.SetValue("FIELD_VALUE", GenUnderlyingCast(field, true, call));

      code_ += "  {{FIELD_TYPE}}{{FIELD_NAME}}() const {";
      code_ += "    return {{FIELD_VALUE}};";
      code_ += "  }";

      if (field.value.type.base_type == BASE_TYPE_UNION) {
        auto u = field.value.type.enum_def;

        code_ += "  template<typename T> "
                "const T *{{FIELD_NAME}}_as() const;";

        for (auto u_it = u->vals.vec.begin();
             u_it != u->vals.vec.end(); ++u_it) {
          auto &ev = **u_it;
          if (ev.union_type.base_type == BASE_TYPE_NONE) {
            continue;
          }
          auto full_struct_name = GetUnionElement(ev, true, true);

          // @TODO: Mby make this decisions more universal? How?
          code_.SetValue("U_GET_TYPE", field.name + UnionTypeFieldSuffix());
          code_.SetValue("U_ELEMENT_TYPE", WrapInNameSpace(
                         u->defined_namespace, GetEnumValUse(*u, ev)));
          code_.SetValue("U_FIELD_TYPE", "const " + full_struct_name + " *");
          code_.SetValue("U_FIELD_NAME",
                         field.name + "_as_" + ev.name);

          // `const Type *union_name_asType() const` accessor.
          code_ += "  {{U_FIELD_TYPE}}{{U_FIELD_NAME}}() const {";
          code_ += "    return {{U_GET_TYPE}}() == {{U_ELEMENT_TYPE}} ? "
                  "static_cast<{{U_FIELD_TYPE}}>({{FIELD_NAME}}()) "
                  ": nullptr;";
          code_ += "  }";
        }
      }

      if (parser_.opts.mutable_buffer) {
        if (is_scalar) {
          const auto type = GenTypeWire(field.value.type, "", false);
          code_.SetValue("SET_FN", "SetField<" + type + ">");
          code_.SetValue("OFFSET_NAME", offset_str);
          code_.SetValue("FIELD_TYPE", GenTypeBasic(field.value.type, true));
          code_.SetValue("FIELD_VALUE",
                        GenUnderlyingCast(field, false, "_" + field.name));
          code_.SetValue("DEFAULT_VALUE", GenDefaultConstant(field));

          code_ += "  bool mutate_{{FIELD_NAME}}({{FIELD_TYPE}} "
                  "_{{FIELD_NAME}}) {";
          code_ += "    return {{SET_FN}}({{OFFSET_NAME}}, {{FIELD_VALUE}}, {{DEFAULT_VALUE}});";
          code_ += "  }";
        } else {
          auto type = GenTypeGet(field.value.type, " ", "", " *", true);
          auto underlying = accessor + type + ">(" + offset_str + ")";
          code_.SetValue("FIELD_TYPE", type);
          code_.SetValue("FIELD_VALUE",
                        GenUnderlyingCast(field, true, underlying));

          code_ += "  {{FIELD_TYPE}}mutable_{{FIELD_NAME}}() {";
          code_ += "    return {{FIELD_VALUE}};";
          code_ += "  }";
        }
      }

      auto nested = field.attributes.Lookup("nested_flatbuffer");
      if (nested) {
        std::string qualified_name =
            parser_.namespaces_.back()->GetFullyQualifiedName(
                nested->constant);
        auto nested_root = parser_.structs_.Lookup(qualified_name);
        assert(nested_root);  // Guaranteed to exist by parser.
        (void)nested_root;
        code_.SetValue("CPP_NAME", TranslateNameSpace(qualified_name));

        code_ += "  const {{CPP_NAME}} *{{FIELD_NAME}}_nested_root() const {";
        code_ += "    const uint8_t* data = {{FIELD_NAME}}()->Data();";
        code_ += "    return flatbuffers::GetRoot<{{CPP_NAME}}>(data);";
        code_ += "  }";
      }

      // Generate a comparison function for this field if it is a key.
      if (field.key) {
        const bool is_string = (field.value.type.base_type == BASE_TYPE_STRING);

        code_ += "  bool KeyCompareLessThan(const {{STRUCT_NAME}} *o) const {";
        if (is_string) {
          code_ += "    return *{{FIELD_NAME}}() < *o->{{FIELD_NAME}}();";
        } else {
          code_ += "    return {{FIELD_NAME}}() < o->{{FIELD_NAME}}();";
        }
        code_ += "  }";

        if (is_string) {
          code_ += "  int KeyCompareWithValue(const char *val) const {";
          code_ += "    return strcmp({{FIELD_NAME}}()->c_str(), val);";
          code_ += "  }";
        } else {
          auto type = GenTypeBasic(field.value.type, false);
          if (parser_.opts.scoped_enums && field.value.type.enum_def &&
              IsScalar(field.value.type.base_type)) {
            type = GenTypeGet(field.value.type, " ", "const ", " *", true);
          }

          code_.SetValue("KEY_TYPE", type);
          code_ += "  int KeyCompareWithValue({{KEY_TYPE}} val) const {";
          code_ += "    const auto key = {{FIELD_NAME}}();";
          code_ += "    if (key < val) {";
          code_ += "      return -1;";
          code_ += "    } else if (key > val) {";
          code_ += "      return 1;";
          code_ += "    } else {";
          code_ += "      return 0;";
          code_ += "    }";
          code_ += "  }";
        }
      }
    }

    // Generate a verifier function that can check a buffer from an untrusted
    // source will never cause reads outside the buffer.
    code_ += "  bool Verify(flatbuffers::Verifier &verifier) const {";
    code_ += "    return VerifyTableStart(verifier)\\";
    for (auto it = struct_def.fields.vec.begin();
         it != struct_def.fields.vec.end(); ++it) {
      const auto &field = **it;
      if (field.deprecated) {
        continue;
      }
      GenVerifyCall(field, " &&\n           ");
    }

    code_ += " &&\n           verifier.EndTable();";
    code_ += "  }";

    if (parser_.opts.generate_object_based_api) {
      // Generate the UnPack() pre declaration.
      code_ += "  " + TableUnPackSignature(struct_def, true) + ";";
      code_ += "  " + TableUnPackToSignature(struct_def, true) + ";";
      code_ += "  " + TablePackSignature(struct_def, true) + ";";
    }

    code_ += "};";  // End of table.
    code_ += "";

    // Explicit specializations for union accessors
    for (auto it = struct_def.fields.vec.begin();
         it != struct_def.fields.vec.end(); ++it) {
      const auto &field = **it;
      if (field.deprecated ||
          field.value.type.base_type != BASE_TYPE_UNION) {
        continue;
      }

      auto u = field.value.type.enum_def;
      if (u->uses_type_aliases) continue;

      code_.SetValue("FIELD_NAME", field.name);

      for (auto u_it = u->vals.vec.begin();
           u_it != u->vals.vec.end(); ++u_it) {
        auto &ev = **u_it;
        if (ev.union_type.base_type == BASE_TYPE_NONE) {
          continue;
        }

        auto full_struct_name = GetUnionElement(ev, true, true);

        code_.SetValue("U_ELEMENT_TYPE", WrapInNameSpace(
                       u->defined_namespace, GetEnumValUse(*u, ev)));
        code_.SetValue("U_FIELD_TYPE", "const " + full_struct_name + " *");
        code_.SetValue("U_ELEMENT_NAME", full_struct_name);
        code_.SetValue("U_FIELD_NAME",
                       field.name + "_as_" + ev.name);

        // `template<> const T *union_name_as<T>() const` accessor.
        code_ += "template<> "
                "inline {{U_FIELD_TYPE}}{{STRUCT_NAME}}::{{FIELD_NAME}}_as"
                "<{{U_ELEMENT_NAME}}>() const {";
        code_ += "  return {{U_FIELD_NAME}}();";
        code_ += "}";
        code_ += "";
      }
    }

    GenBuilders(struct_def);

    if (parser_.opts.generate_object_based_api) {
      // Generate a pre-declaration for a CreateX method that works with an
      // unpacked C++ object.
      code_ += TableCreateSignature(struct_def, true) + ";";
      code_ += "";
    }
  }

  void GenBuilders(const StructDef &struct_def) {
    code_.SetValue("STRUCT_NAME", struct_def.name);

    // Generate a builder struct:
    code_ += "struct {{STRUCT_NAME}}Builder {";
    code_ += "  flatbuffers::FlatBufferBuilder &fbb_;";
    code_ += "  flatbuffers::uoffset_t start_;";

    bool has_string_or_vector_fields = false;
    for (auto it = struct_def.fields.vec.begin();
         it != struct_def.fields.vec.end(); ++it) {
      const auto &field = **it;
      if (!field.deprecated) {
        const bool is_scalar = IsScalar(field.value.type.base_type);
        const bool is_string = field.value.type.base_type == BASE_TYPE_STRING;
        const bool is_vector = field.value.type.base_type == BASE_TYPE_VECTOR;
        if (is_string || is_vector) {
          has_string_or_vector_fields = true;
        }

        std::string offset = GenFieldOffsetName(field);
        std::string name = GenUnderlyingCast(field, false, field.name);
        std::string value = is_scalar ? GenDefaultConstant(field) : "";

        // Generate accessor functions of the form:
        // void add_name(type name) {
        //   fbb_.AddElement<type>(offset, name, default);
        // }
        code_.SetValue("FIELD_NAME", field.name);
        code_.SetValue("FIELD_TYPE", GenTypeWire(field.value.type, " ", true));
        code_.SetValue("ADD_OFFSET", struct_def.name + "::" + offset);
        code_.SetValue("ADD_NAME", name);
        code_.SetValue("ADD_VALUE", value);
        if (is_scalar) {
          const auto type = GenTypeWire(field.value.type, "", false);
          code_.SetValue("ADD_FN", "AddElement<" + type + ">");
        } else if (IsStruct(field.value.type)) {
          code_.SetValue("ADD_FN", "AddStruct");
        } else {
          code_.SetValue("ADD_FN", "AddOffset");
        }

        code_ += "  void add_{{FIELD_NAME}}({{FIELD_TYPE}}{{FIELD_NAME}}) {";
          code_ += "    fbb_.{{ADD_FN}}(\\";
        if (is_scalar) {
          code_ += "{{ADD_OFFSET}}, {{ADD_NAME}}, {{ADD_VALUE}});";
        } else {
          code_ += "{{ADD_OFFSET}}, {{ADD_NAME}});";
        }
        code_ += "  }";
      }
    }

    // Builder constructor
    code_ += "  {{STRUCT_NAME}}Builder(flatbuffers::FlatBufferBuilder &_fbb)";
    code_ += "        : fbb_(_fbb) {";
    code_ += "    start_ = fbb_.StartTable();";
    code_ += "  }";

    // Assignment operator;
    code_ += "  {{STRUCT_NAME}}Builder &operator="
             "(const {{STRUCT_NAME}}Builder &);";

    // Finish() function.
    auto num_fields = NumToString(struct_def.fields.vec.size());
    code_ += "  flatbuffers::Offset<{{STRUCT_NAME}}> Finish() {";
    code_ += "    const auto end = fbb_.EndTable(start_, " + num_fields + ");";
    code_ += "    auto o = flatbuffers::Offset<{{STRUCT_NAME}}>(end);";

    for (auto it = struct_def.fields.vec.begin();
         it != struct_def.fields.vec.end(); ++it) {
      const auto &field = **it;
      if (!field.deprecated && field.required) {
        code_.SetValue("FIELD_NAME", field.name);
        code_.SetValue("OFFSET_NAME", GenFieldOffsetName(field));
        code_ += "    fbb_.Required(o, {{STRUCT_NAME}}::{{OFFSET_NAME}});";
      }
    }
    code_ += "    return o;";
    code_ += "  }";
    code_ += "};";
    code_ += "";

    // Generate a convenient CreateX function that uses the above builder
    // to create a table in one go.
    code_ += "inline flatbuffers::Offset<{{STRUCT_NAME}}> "
            "Create{{STRUCT_NAME}}(";
    code_ += "    flatbuffers::FlatBufferBuilder &_fbb\\";
    for (auto it = struct_def.fields.vec.begin();
         it != struct_def.fields.vec.end(); ++it) {
      const auto &field = **it;
      if (!field.deprecated) {
        GenParam(field, false, ",\n    ");
      }
    }
    code_ += ") {";

    code_ += "  {{STRUCT_NAME}}Builder builder_(_fbb);";
    for (size_t size = struct_def.sortbysize ? sizeof(largest_scalar_t) : 1;
         size; size /= 2) {
      for (auto it = struct_def.fields.vec.rbegin();
           it != struct_def.fields.vec.rend(); ++it) {
        const auto &field = **it;
        if (!field.deprecated && (!struct_def.sortbysize ||
                                  size == SizeOf(field.value.type.base_type))) {
          code_.SetValue("FIELD_NAME", field.name);
          code_ += "  builder_.add_{{FIELD_NAME}}({{FIELD_NAME}});";
        }
      }
    }
    code_ += "  return builder_.Finish();";
    code_ += "}";
    code_ += "";

    // Generate a CreateXDirect function with vector types as parameters
    if (has_string_or_vector_fields) {
      code_ += "inline flatbuffers::Offset<{{STRUCT_NAME}}> "
              "Create{{STRUCT_NAME}}Direct(";
      code_ += "    flatbuffers::FlatBufferBuilder &_fbb\\";
      for (auto it = struct_def.fields.vec.begin();
           it != struct_def.fields.vec.end(); ++it) {
        const auto &field = **it;
        if (!field.deprecated) {
          GenParam(field, true, ",\n    ");
        }
      }

      // Need to call "Create" with the struct namespace.
      const auto qualified_create_name = struct_def.defined_namespace->GetFullyQualifiedName("Create");
      code_.SetValue("CREATE_NAME", TranslateNameSpace(qualified_create_name));

      code_ += ") {";
      code_ += "  return {{CREATE_NAME}}{{STRUCT_NAME}}(";
      code_ += "      _fbb\\";
      for (auto it = struct_def.fields.vec.begin();
           it != struct_def.fields.vec.end(); ++it) {
        const auto &field = **it;
        if (!field.deprecated) {
          code_.SetValue("FIELD_NAME", field.name);

          if (field.value.type.base_type == BASE_TYPE_STRING) {
            code_ += ",\n      {{FIELD_NAME}} ? "
                    "_fbb.CreateString({{FIELD_NAME}}) : 0\\";
          } else if (field.value.type.base_type == BASE_TYPE_VECTOR) {
            auto type = GenTypeWire(field.value.type.VectorType(), "", false);
            code_ += ",\n      {{FIELD_NAME}} ? "
                    "_fbb.CreateVector<" + type + ">(*{{FIELD_NAME}}) : 0\\";
          } else {
            code_ += ",\n      {{FIELD_NAME}}\\";
          }
        }
      }
      code_ += ");";
      code_ += "}";
      code_ += "";
    }
  }

  std::string GenUnionUnpackVal(const FieldDef &afield,
                                const char *vec_elem_access,
                                const char *vec_type_access) {
    return afield.value.type.enum_def->name + "Union::UnPack(" + "_e" +
           vec_elem_access + ", " + afield.name + UnionTypeFieldSuffix() +
           "()" + vec_type_access + ", _resolver)";
  }

  std::string GenUnpackVal(const Type &type, const std::string &val,
                           bool invector, const FieldDef &afield) {
    switch (type.base_type) {
      case BASE_TYPE_STRING: {
        return val + "->str()";
      }
      case BASE_TYPE_STRUCT: {
        const auto name = WrapInNameSpace(*type.struct_def);
        if (IsStruct(type)) {
          auto native_type = type.struct_def->attributes.Lookup("native_type");
          if (native_type) {
            return "flatbuffers::UnPack(*" + val + ")";
          } else if (invector || afield.native_inline) {
            return "*" + val;
          } else {
            const auto ptype = GenTypeNativePtr(name, &afield, true);
            return ptype + "(new " + name + "(*" + val + "))";
          }
        } else {
          const auto ptype = GenTypeNativePtr(NativeName(name, type.struct_def),
                                              &afield, true);
          return ptype + "(" + val + "->UnPack(_resolver))";
        }
      }
      case BASE_TYPE_UNION: {
        return GenUnionUnpackVal(afield,
                                 invector ? "->Get(_i)" : "",
                                 invector ? ("->GetEnum<" +
                                             type.enum_def->name +
                                             ">(_i)").c_str() : "");
      }
      default: {
        return val;
        break;
      }
    }
  };

  std::string GenUnpackFieldStatement(const FieldDef &field,
                                      const FieldDef *union_field) {
    std::string code;
    switch (field.value.type.base_type) {
      case BASE_TYPE_VECTOR: {
        std::string indexing;
        if (field.value.type.enum_def) {
          indexing += "(" + field.value.type.enum_def->name + ")";
        }
        indexing += "_e->Get(_i)";
        if (field.value.type.element == BASE_TYPE_BOOL) {
          indexing += " != 0";
        }

        // Generate code that pushes data from _e to _o in the form:
        //   for (uoffset_t i = 0; i < _e->size(); ++i) {
        //     _o->field.push_back(_e->Get(_i));
        //   }
        auto name = field.name;
        if (field.value.type.element == BASE_TYPE_UTYPE) {
          name = StripUnionType(field.name);
        }
        auto access = field.value.type.element == BASE_TYPE_UTYPE
                        ? ".type"
                        : (field.value.type.element == BASE_TYPE_UNION
                          ? ".value"
                          : "");
        code += "{ _o->" + name + ".resize(_e->size()); ";
        code += "for (flatbuffers::uoffset_t _i = 0;";
        code += " _i < _e->size(); _i++) { ";
        code += "_o->" + name + "[_i]" + access + " = ";
        code += GenUnpackVal(field.value.type.VectorType(),
                                  indexing, true, field);
        code += "; } }";
        break;
      }
      case BASE_TYPE_UTYPE: {
        assert(union_field->value.type.base_type == BASE_TYPE_UNION);
        // Generate code that sets the union type, of the form:
        //   _o->field.type = _e;
        code += "_o->" + union_field->name + ".type = _e;";
        break;
      }
      case BASE_TYPE_UNION: {
        // Generate code that sets the union value, of the form:
        //   _o->field.value = Union::Unpack(_e, field_type(), resolver);
        code += "_o->" + field.name + ".value = ";
        code += GenUnionUnpackVal(field, "", "");
        code += ";";
        break;
      }
      default: {
        auto cpp_type = field.attributes.Lookup("cpp_type");
        if (cpp_type) {
          // Generate code that resolves the cpp pointer type, of the form:
          //  if (resolver)
          //    (*resolver)(&_o->field, (hash_value_t)(_e));
          //  else
          //    _o->field = nullptr;
          code += "if (_resolver) ";
          code += "(*_resolver)";
          code += "(reinterpret_cast<void **>(&_o->" + field.name + "), ";
          code += "static_cast<flatbuffers::hash_value_t>(_e));";
          code += " else ";
          code += "_o->" + field.name + " = nullptr;";
        } else {
          // Generate code for assigning the value, of the form:
          //  _o->field = value;
          code += "_o->" + field.name + " = ";
          code += GenUnpackVal(field.value.type, "_e", false, field) + ";";
        }
        break;
      }
    }
    return code;
  }

  std::string GenCreateParam(const FieldDef &field) {
    std::string value = "_o->";
    if (field.value.type.base_type == BASE_TYPE_UTYPE) {
      value += StripUnionType(field.name);
      value += ".type";
    } else {
      value += field.name;
    }
    if (field.attributes.Lookup("cpp_type")) {
      auto type = GenTypeBasic(field.value.type, false);
      value = "_rehasher ? "
              "static_cast<" + type + ">((*_rehasher)(" + value + ")) : 0";
    }

    std::string code;
    switch (field.value.type.base_type) {
      // String fields are of the form:
      //   _fbb.CreateString(_o->field)
      case BASE_TYPE_STRING: {
        code += "_fbb.CreateString(" + value + ")";

        // For optional fields, check to see if there actually is any data
        // in _o->field before attempting to access it.
        if (!field.required) {
          code = value + ".size() ? " + code + " : 0";
        }
        break;
      }
      // Vector fields come in several flavours, of the forms:
      //   _fbb.CreateVector(_o->field);
      //   _fbb.CreateVector((const utype*)_o->field.data(), _o->field.size());
      //   _fbb.CreateVectorOfStrings(_o->field)
      //   _fbb.CreateVectorOfStructs(_o->field)
      //   _fbb.CreateVector<Offset<T>>(_o->field.size() [&](size_t i) {
      //     return CreateT(_fbb, _o->Get(i), rehasher);
      //   });
      case BASE_TYPE_VECTOR: {
        auto vector_type = field.value.type.VectorType();
        switch (vector_type.base_type) {
          case BASE_TYPE_STRING: {
            code += "_fbb.CreateVectorOfStrings(" + value + ")";
            break;
          }
          case BASE_TYPE_STRUCT: {
            if (IsStruct(vector_type)) {
              auto native_type =
                field.value.type.struct_def->attributes.Lookup("native_type");
              if (native_type) {
                code += "_fbb.CreateVectorOfNativeStructs<";
                code += WrapInNameSpace(*vector_type.struct_def) + ">";
              } else {
                code += "_fbb.CreateVectorOfStructs";
              }
              code += "(" + value + ")";
            } else {
              code += "_fbb.CreateVector<flatbuffers::Offset<";
              code += WrapInNameSpace(*vector_type.struct_def) + ">>";
              code += "(" + value + ".size(), [&](size_t i) {";
              code += " return Create" + vector_type.struct_def->name;
              code += "(_fbb, " + value + "[i]" + GenPtrGet(field) + ", ";
              code += "_rehasher); })";
            }
            break;
          }
          case BASE_TYPE_BOOL: {
            code += "_fbb.CreateVector(" + value + ")";
            break;
          }
          case BASE_TYPE_UNION: {
            code += "_fbb.CreateVector<flatbuffers::Offset<void>>(" + value +
                    ".size(), [&](size_t i) { return " + value +
                    "[i].Pack(_fbb, _rehasher); })";
            break;
          }
          case BASE_TYPE_UTYPE: {
            value = StripUnionType(value);
            code += "_fbb.CreateVector<uint8_t>(" + value +
                    ".size(), [&](size_t i) { return static_cast<uint8_t>(" + value +
                    "[i].type); })";
            break;
          }
          default: {
            if (field.value.type.enum_def) {
              // For enumerations, we need to get access to the array data for
              // the underlying storage type (eg. uint8_t).
              const auto basetype = GenTypeBasic(
                  field.value.type.enum_def->underlying_type, false);
              code += "_fbb.CreateVector((const " + basetype + "*)" + value +
                      ".data(), " + value + ".size())";
            } else {
              code += "_fbb.CreateVector(" + value + ")";
            }
            break;
          }
        }

        // For optional fields, check to see if there actually is any data
        // in _o->field before attempting to access it.
        if (!field.required) {
          code = value + ".size() ? " + code + " : 0";
        }
        break;
      }
      case BASE_TYPE_UNION: {
        // _o->field.Pack(_fbb);
        code += value + ".Pack(_fbb)";
        break;
      }
      case BASE_TYPE_STRUCT: {
        if (IsStruct(field.value.type)) {
          auto native_type =
              field.value.type.struct_def->attributes.Lookup("native_type");
          if (native_type) {
            code += "flatbuffers::Pack(" + value + ")";
          } else if (field.native_inline) {
            code += "&" + value;
          } else {
            code += value + " ? " + value + GenPtrGet(field) + " : 0";
          }
        } else {
          // _o->field ? CreateT(_fbb, _o->field.get(), _rehasher);
          const auto type = field.value.type.struct_def->name;
          code += value + " ? Create" + type;
          code += "(_fbb, " + value + GenPtrGet(field) + ", _rehasher)";
          code += " : 0";
        }
        break;
      }
      default: {
        code += value;
        break;
      }
    }
    return code;
  }

  // Generate code for tables that needs to come after the regular definition.
  void GenTablePost(const StructDef &struct_def) {
    code_.SetValue("STRUCT_NAME", struct_def.name);
    code_.SetValue("NATIVE_NAME", NativeName(struct_def.name, &struct_def));

    if (parser_.opts.generate_object_based_api) {
      // Generate the X::UnPack() method.
      code_ += "inline " + TableUnPackSignature(struct_def, false) + " {";
      code_ += "  auto _o = new {{NATIVE_NAME}}();";
      code_ += "  UnPackTo(_o, _resolver);";
      code_ += "  return _o;";
      code_ += "}";
      code_ += "";

      code_ += "inline " + TableUnPackToSignature(struct_def, false) + " {";
      code_ += "  (void)_o;";
      code_ += "  (void)_resolver;";

      for (auto it = struct_def.fields.vec.begin();
           it != struct_def.fields.vec.end(); ++it) {
        const auto &field = **it;
        if (field.deprecated) {
          continue;
        }

        // Assign a value from |this| to |_o|.   Values from |this| are stored
        // in a variable |_e| by calling this->field_type().  The value is then
        // assigned to |_o| using the GenUnpackFieldStatement.
        const bool is_union = field.value.type.base_type == BASE_TYPE_UTYPE;
        const auto statement =
            GenUnpackFieldStatement(field, is_union ? *(it + 1) : nullptr);

        code_.SetValue("FIELD_NAME", field.name);
        auto prefix = "  { auto _e = {{FIELD_NAME}}(); ";
        auto check = IsScalar(field.value.type.base_type) ? "" : "if (_e) ";
        auto postfix = " };";
        code_ += std::string(prefix) + check + statement + postfix;
      }
      code_ += "}";
      code_ += "";

      // Generate the X::Pack member function that simply calls the global
      // CreateX function.
      code_ += "inline " + TablePackSignature(struct_def, false) + " {";
      code_ += "  return Create{{STRUCT_NAME}}(_fbb, _o, _rehasher);";
      code_ += "}";
      code_ += "";

      // Generate a CreateX method that works with an unpacked C++ object.
      code_ += "inline " + TableCreateSignature(struct_def, false) + " {";
      code_ += "  (void)_rehasher;";
      code_ += "  (void)_o;";

      for (auto it = struct_def.fields.vec.begin();
           it != struct_def.fields.vec.end(); ++it) {
        auto &field = **it;
        if (field.deprecated) {
          continue;
        }
        code_ += "  auto _" + field.name + " = " + GenCreateParam(field) + ";";
      }
      // Need to call "Create" with the struct namespace.
      const auto qualified_create_name = struct_def.defined_namespace->GetFullyQualifiedName("Create");
      code_.SetValue("CREATE_NAME", TranslateNameSpace(qualified_create_name));

      code_ += "  return {{CREATE_NAME}}{{STRUCT_NAME}}(";
      code_ += "      _fbb\\";
      for (auto it = struct_def.fields.vec.begin();
           it != struct_def.fields.vec.end(); ++it) {
        auto &field = **it;
        if (field.deprecated) {
          continue;
        }

        bool pass_by_address = false;
        if (field.value.type.base_type == BASE_TYPE_STRUCT) {
          if (IsStruct(field.value.type)) {
            auto native_type =
                field.value.type.struct_def->attributes.Lookup("native_type");
            if (native_type) {
              pass_by_address = true;
            }
          }
        }

        // Call the CreateX function using values from |_o|.
        if (pass_by_address) {
          code_ += ",\n      &_" + field.name + "\\";
        } else {
          code_ += ",\n      _" + field.name + "\\";
        }
      }
      code_ += ");";
      code_ += "}";
      code_ += "";
    }
  }

  static void GenPadding(
      const FieldDef &field, std::string *code_ptr, int *id,
      const std::function<void(int bits, std::string *code_ptr, int *id)> &f) {
    if (field.padding) {
      for (int i = 0; i < 4; i++) {
        if (static_cast<int>(field.padding) & (1 << i)) {
          f((1 << i) * 8, code_ptr, id);
        }
      }
      assert(!(field.padding & ~0xF));
    }
  }

  static void PaddingDefinition(int bits, std::string *code_ptr, int *id) {
    *code_ptr += "  int" + NumToString(bits) + "_t padding" +
        NumToString((*id)++) + "__;";
  }

  static void PaddingInitializer(int bits, std::string *code_ptr, int *id) {
    (void)bits;
    *code_ptr += ",\n        padding" + NumToString((*id)++) + "__(0)";
  }

  static void PaddingNoop(int bits, std::string *code_ptr, int *id) {
    (void)bits;
    *code_ptr += "    (void)padding" + NumToString((*id)++) + "__;";
  }

  // Generate an accessor struct with constructor for a flatbuffers struct.
  void GenStruct(const StructDef &struct_def) {
    // Generate an accessor struct, with private variables of the form:
    // type name_;
    // Generates manual padding and alignment.
    // Variables are private because they contain little endian data on all
    // platforms.
    GenComment(struct_def.doc_comment);
    code_.SetValue("ALIGN", NumToString(struct_def.minalign));
    code_.SetValue("STRUCT_NAME", struct_def.name);

    code_ += "MANUALLY_ALIGNED_STRUCT({{ALIGN}}) "
            "{{STRUCT_NAME}} FLATBUFFERS_FINAL_CLASS {";
    code_ += " private:";

    int padding_id = 0;
    for (auto it = struct_def.fields.vec.begin();
         it != struct_def.fields.vec.end(); ++it) {
      const auto &field = **it;
      code_.SetValue("FIELD_TYPE",
          GenTypeGet(field.value.type, " ", "", " ", false));
      code_.SetValue("FIELD_NAME", field.name);
      code_ += "  {{FIELD_TYPE}}{{FIELD_NAME}}_;";

      if (field.padding) {
        std::string padding;
        GenPadding(field, &padding, &padding_id, PaddingDefinition);
        code_ += padding;
      }
    }

    // Generate GetFullyQualifiedName
    code_ += "";
    code_ += " public:";
    GenFullyQualifiedNameGetter(struct_def.name);

    // Generate a default constructor.
    code_ += "  {{STRUCT_NAME}}() {";
    code_ += "    memset(this, 0, sizeof({{STRUCT_NAME}}));";
    code_ += "  }";

    // Generate a copy constructor.
    code_ += "  {{STRUCT_NAME}}(const {{STRUCT_NAME}} &_o) {";
    code_ += "    memcpy(this, &_o, sizeof({{STRUCT_NAME}}));";
    code_ += "  }";

    // Generate a constructor that takes all fields as arguments.
    std::string arg_list;
    std::string init_list;
    padding_id = 0;
    for (auto it = struct_def.fields.vec.begin();
         it != struct_def.fields.vec.end(); ++it) {
      const auto &field = **it;
      const auto member_name = field.name + "_";
      const auto arg_name = "_" + field.name;
      const auto arg_type =
          GenTypeGet(field.value.type, " ", "const ", " &", true);

      if (it != struct_def.fields.vec.begin()) {
        arg_list += ", ";
        init_list += ",\n        ";
      }
      arg_list += arg_type;
      arg_list += arg_name;
      init_list += member_name;
      if (IsScalar(field.value.type.base_type)) {
        auto type = GenUnderlyingCast(field, false, arg_name);
        init_list += "(flatbuffers::EndianScalar(" + type + "))";
      } else {
        init_list += "(" + arg_name + ")";
      }
      if (field.padding) {
        GenPadding(field, &init_list, &padding_id, PaddingInitializer);
      }
    }

    code_.SetValue("ARG_LIST", arg_list);
    code_.SetValue("INIT_LIST", init_list);
    code_ += "  {{STRUCT_NAME}}({{ARG_LIST}})";
    code_ += "      : {{INIT_LIST}} {";
    padding_id = 0;
    for (auto it = struct_def.fields.vec.begin();
         it != struct_def.fields.vec.end(); ++it) {
      const auto &field = **it;
      if (field.padding) {
        std::string padding;
        GenPadding(field, &padding, &padding_id, PaddingNoop);
        code_ += padding;
      }
    }
    code_ += "  }";

    // Generate accessor methods of the form:
    // type name() const { return flatbuffers::EndianScalar(name_); }
    for (auto it = struct_def.fields.vec.begin();
         it != struct_def.fields.vec.end(); ++it) {
      const auto &field = **it;

      auto field_type = GenTypeGet(field.value.type, " ", "const ", " &", true);
      auto is_scalar = IsScalar(field.value.type.base_type);
      auto member = field.name + "_";
      auto value = is_scalar ? "flatbuffers::EndianScalar(" + member + ")"
                             : member;

      code_.SetValue("FIELD_NAME", field.name);
      code_.SetValue("FIELD_TYPE", field_type);
      code_.SetValue("FIELD_VALUE", GenUnderlyingCast(field, true, value));

      GenComment(field.doc_comment, "  ");
      code_ += "  {{FIELD_TYPE}}{{FIELD_NAME}}() const {";
      code_ += "    return {{FIELD_VALUE}};";
      code_ += "  }";

      if (parser_.opts.mutable_buffer) {
        auto mut_field_type = GenTypeGet(field.value.type, " ", "", " &", true);
        code_.SetValue("FIELD_TYPE", mut_field_type);
        if (is_scalar) {
          code_.SetValue("ARG", GenTypeBasic(field.value.type, true));
          code_.SetValue("FIELD_VALUE",
                        GenUnderlyingCast(field, false, "_" + field.name));

          code_ += "  void mutate_{{FIELD_NAME}}({{ARG}} _{{FIELD_NAME}}) {";
          code_ += "    flatbuffers::WriteScalar(&{{FIELD_NAME}}_, "
                  "{{FIELD_VALUE}});";
          code_ += "  }";
        } else {
          code_ += "  {{FIELD_TYPE}}mutable_{{FIELD_NAME}}() {";
          code_ += "    return {{FIELD_NAME}}_;";
          code_ += "  }";
        }
      }

      // Generate a comparison function for this field if it is a key.
      if (field.key) {
        code_ += "  bool KeyCompareLessThan(const {{STRUCT_NAME}} *o) const {";
        code_ += "    return {{FIELD_NAME}}() < o->{{FIELD_NAME}}();";
        code_ += "  }";
        auto type = GenTypeBasic(field.value.type, false);
        if (parser_.opts.scoped_enums && field.value.type.enum_def &&
            IsScalar(field.value.type.base_type)) {
          type = GenTypeGet(field.value.type, " ", "const ", " *", true);
        }

        code_.SetValue("KEY_TYPE", type);
        code_ += "  int KeyCompareWithValue({{KEY_TYPE}} val) const {";
        code_ += "    const auto key = {{FIELD_NAME}}();";
        code_ += "    return static_cast<int>(key > val) - static_cast<int>(key < val);";
        code_ += "  }";
      }
    }
    code_ += "};";

    code_.SetValue("STRUCT_BYTE_SIZE", NumToString(struct_def.bytesize));
    code_ += "STRUCT_END({{STRUCT_NAME}}, {{STRUCT_BYTE_SIZE}});";
    code_ += "";
  }

  // Set up the correct namespace. Only open a namespace if the existing one is
  // different (closing/opening only what is necessary).
  //
  // The file must start and end with an empty (or null) namespace so that
  // namespaces are properly opened and closed.
  void SetNameSpace(const Namespace *ns) {
    if (cur_name_space_ == ns) {
      return;
    }

    // Compute the size of the longest common namespace prefix.
    // If cur_name_space is A::B::C::D and ns is A::B::E::F::G,
    // the common prefix is A::B:: and we have old_size = 4, new_size = 5
    // and common_prefix_size = 2
    size_t old_size = cur_name_space_ ? cur_name_space_->components.size() : 0;
    size_t new_size = ns ? ns->components.size() : 0;

    size_t common_prefix_size = 0;
    while (common_prefix_size < old_size && common_prefix_size < new_size &&
           ns->components[common_prefix_size] ==
               cur_name_space_->components[common_prefix_size]) {
      common_prefix_size++;
    }

    // Close cur_name_space in reverse order to reach the common prefix.
    // In the previous example, D then C are closed.
    for (size_t j = old_size; j > common_prefix_size; --j) {
      code_ += "}  // namespace " + cur_name_space_->components[j - 1];
    }
    if (old_size != common_prefix_size) {
      code_ += "";
    }

    // open namespace parts to reach the ns namespace
    // in the previous example, E, then F, then G are opened
    for (auto j = common_prefix_size; j != new_size; ++j) {
      code_ += "namespace " + ns->components[j] + " {";
    }
    if (new_size != common_prefix_size) {
      code_ += "";
    }

    cur_name_space_ = ns;
  }
};

}  // namespace cpp

bool GenerateCPP(const Parser &parser, const std::string &path,
                 const std::string &file_name) {
  cpp::CppGenerator generator(parser, path, file_name);
  return generator.generate();
}

std::string CPPMakeRule(const Parser &parser, const std::string &path,
                        const std::string &file_name) {
  const auto filebase =
      flatbuffers::StripPath(flatbuffers::StripExtension(file_name));
  const auto included_files = parser.GetIncludedFilesRecursive(file_name);
  std::string make_rule = GeneratedFileName(path, filebase) + ": ";
  for (auto it = included_files.begin(); it != included_files.end(); ++it) {
    make_rule += " " + *it;
  }
  return make_rule;
}

}  // namespace flatbuffers
