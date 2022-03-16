// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_PAGESET_H_
#define FXJS_XFA_CJX_PAGESET_H_

#include "fxjs/xfa/cjx_container.h"

class CXFA_PageSet;

class CJX_PageSet : public CJX_Container {
 public:
  explicit CJX_PageSet(CXFA_PageSet* node);
  ~CJX_PageSet() override;

  JS_PROP(relation);
  JS_PROP(relevant);
  JS_PROP(use);
  JS_PROP(usehref);
};

#endif  // FXJS_XFA_CJX_PAGESET_H_
