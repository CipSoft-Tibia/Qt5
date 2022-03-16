// Copyright 2015 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCODEC_JBIG2_JBIG2_TRDPROC_H_
#define CORE_FXCODEC_JBIG2_JBIG2_TRDPROC_H_

#include <memory>
#include <vector>

#include "core/fxcodec/jbig2/JBig2_Image.h"
#include "core/fxcrt/fx_system.h"

class CJBig2_ArithDecoder;
class CJBig2_ArithIaidDecoder;
class CJBig2_ArithIntDecoder;
class CJBig2_BitStream;
class CJBig2_HuffmanTable;
struct JBig2ArithCtx;
struct JBig2HuffmanCode;

struct JBig2IntDecoderState {
  CJBig2_ArithIntDecoder* IADT;
  CJBig2_ArithIntDecoder* IAFS;
  CJBig2_ArithIntDecoder* IADS;
  CJBig2_ArithIntDecoder* IAIT;
  CJBig2_ArithIntDecoder* IARI;
  CJBig2_ArithIntDecoder* IARDW;
  CJBig2_ArithIntDecoder* IARDH;
  CJBig2_ArithIntDecoder* IARDX;
  CJBig2_ArithIntDecoder* IARDY;
  CJBig2_ArithIaidDecoder* IAID;
};

enum JBig2Corner {
  JBIG2_CORNER_BOTTOMLEFT = 0,
  JBIG2_CORNER_TOPLEFT = 1,
  JBIG2_CORNER_BOTTOMRIGHT = 2,
  JBIG2_CORNER_TOPRIGHT = 3
};

class CJBig2_TRDProc {
 public:
  CJBig2_TRDProc();
  ~CJBig2_TRDProc();

  std::unique_ptr<CJBig2_Image> DecodeHuffman(CJBig2_BitStream* pStream,
                                              JBig2ArithCtx* grContext);

  std::unique_ptr<CJBig2_Image> DecodeArith(CJBig2_ArithDecoder* pArithDecoder,
                                            JBig2ArithCtx* grContext,
                                            JBig2IntDecoderState* pIDS);

  bool SBHUFF;
  bool SBREFINE;
  uint32_t SBW;
  uint32_t SBH;
  uint32_t SBNUMINSTANCES;
  uint32_t SBSTRIPS;
  uint32_t SBNUMSYMS;

  std::vector<JBig2HuffmanCode> SBSYMCODES;
  uint8_t SBSYMCODELEN;

  CJBig2_Image** SBSYMS;
  bool SBDEFPIXEL;

  JBig2ComposeOp SBCOMBOP;
  bool TRANSPOSED;

  JBig2Corner REFCORNER;
  int8_t SBDSOFFSET;
  const CJBig2_HuffmanTable* SBHUFFFS;
  const CJBig2_HuffmanTable* SBHUFFDS;
  const CJBig2_HuffmanTable* SBHUFFDT;
  const CJBig2_HuffmanTable* SBHUFFRDW;
  const CJBig2_HuffmanTable* SBHUFFRDH;
  const CJBig2_HuffmanTable* SBHUFFRDX;
  const CJBig2_HuffmanTable* SBHUFFRDY;
  const CJBig2_HuffmanTable* SBHUFFRSIZE;
  bool SBRTEMPLATE;
  int8_t SBRAT[4];

 private:
  struct ComposeData {
    int32_t x;
    int32_t y;
    uint32_t increment = 0;
  };
  ComposeData GetComposeData(int32_t SI,
                             int32_t TI,
                             uint32_t WI,
                             uint32_t HI) const;
};

#endif  // CORE_FXCODEC_JBIG2_JBIG2_TRDPROC_H_
