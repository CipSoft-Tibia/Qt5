// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_RUST_SUBSTITUTION_TYPE_H_
#define TOOLS_GN_RUST_SUBSTITUTION_TYPE_H_

#include <set>
#include <vector>

#include "tools/gn/substitution_type.h"

// The set of substitutions available to Rust tools.
extern const SubstitutionTypes RustSubstitutions;

// Valid for Rust tools.
extern const Substitution kRustSubstitutionCrateName;
extern const Substitution kRustSubstitutionCrateType;
extern const Substitution kRustSubstitutionEdition;
extern const Substitution kRustSubstitutionExterns;
extern const Substitution kRustSubstitutionOutputExtension;
extern const Substitution kRustSubstitutionOutputPrefix;
extern const Substitution kRustSubstitutionRustDeps;
extern const Substitution kRustSubstitutionRustEnv;
extern const Substitution kRustSubstitutionRustFlags;

bool IsValidRustSubstitution(const Substitution* type);

#endif  // TOOLS_GN_RUST_SUBSTITUTION_TYPE_H_
