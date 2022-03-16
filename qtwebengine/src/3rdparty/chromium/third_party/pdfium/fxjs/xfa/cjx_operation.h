// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_OPERATION_H_
#define FXJS_XFA_CJX_OPERATION_H_

#include "fxjs/xfa/cjx_textnode.h"

class CXFA_Operation;

class CJX_Operation : public CJX_TextNode {
 public:
  explicit CJX_Operation(CXFA_Operation* node);
  ~CJX_Operation() override;

  JS_PROP(input);
  JS_PROP(output);
  JS_PROP(use);
  JS_PROP(usehref);
};

#endif  // FXJS_XFA_CJX_OPERATION_H_
