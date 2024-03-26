// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFAPI_PAGE_CPDF_FUNCTION_H_
#define CORE_FPDFAPI_PAGE_CPDF_FUNCTION_H_

#include <memory>
#include <set>
#include <vector>

#include "core/fxcrt/retain_ptr.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/base/span.h"

class CPDF_ExpIntFunc;
class CPDF_Object;
class CPDF_SampledFunc;
class CPDF_StitchFunc;

class CPDF_Function {
 public:
  // Valid values are from ISO 32000-1:2008 spec, table 38. DO NOT CHANGE.
  enum class Type {
    kTypeInvalid = -1,
    kType0Sampled = 0,
    kType2ExponentialInterpolation = 2,
    kType3Stitching = 3,
    kType4PostScript = 4,
  };

  static std::unique_ptr<CPDF_Function> Load(
      RetainPtr<const CPDF_Object> pFuncObj);

  virtual ~CPDF_Function();

  absl::optional<uint32_t> Call(pdfium::span<const float> inputs,
                                pdfium::span<float> results) const;
  uint32_t CountInputs() const { return m_nInputs; }
  uint32_t CountOutputs() const { return m_nOutputs; }
  float GetDomain(int i) const { return m_Domains[i]; }
  float GetRange(int i) const { return m_Ranges[i]; }
  float Interpolate(float x,
                    float xmin,
                    float xmax,
                    float ymin,
                    float ymax) const;

#ifdef _SKIA_SUPPORT_
  const CPDF_SampledFunc* ToSampledFunc() const;
  const CPDF_ExpIntFunc* ToExpIntFunc() const;
  const CPDF_StitchFunc* ToStitchFunc() const;
#endif  // _SKIA_SUPPORT_

 protected:
  explicit CPDF_Function(Type type);

  using VisitedSet = std::set<RetainPtr<const CPDF_Object>>;
  static std::unique_ptr<CPDF_Function> Load(
      RetainPtr<const CPDF_Object> pFuncObj,
      VisitedSet* pVisited);
  bool Init(const CPDF_Object* pObj, VisitedSet* pVisited);
  virtual bool v_Init(const CPDF_Object* pObj, VisitedSet* pVisited) = 0;
  virtual bool v_Call(pdfium::span<const float> inputs,
                      pdfium::span<float> results) const = 0;

  const Type m_Type;
  uint32_t m_nInputs = 0;
  uint32_t m_nOutputs = 0;
  std::vector<float> m_Domains;
  std::vector<float> m_Ranges;
};

#endif  // CORE_FPDFAPI_PAGE_CPDF_FUNCTION_H_
